#include "nexus/MorphEngine.h"

#include <algorithm>
#include <cmath>

#include "NAM/dsp.h"

namespace nexus
{

void MorphEngine::prepare(std::shared_ptr<Pack> pack, double sampleRate, int maxBlockSize, Settings settings)
{
  mPack = std::move(pack);
  mSettings = settings;
  mSampleRate = sampleRate;
  mSlots.clear();
  mSlots.resize(mPack ? mPack->captures.size() : 0);
  for (auto& s : mSlots)
    s.buffer.assign(maxBlockSize, 0.0);
  mInputCopy.assign(maxBlockSize, 0.0);
  mOrder.reserve(mSlots.size());
  mWeightStep = 1.0 / std::max(1.0, settings.crossfadeMs * 0.001 * sampleRate);
  if (mPack)
    mTarget = std::clamp(mTarget, mPack->minPosition(), mPack->maxPosition());
  jumpToTarget();
}

bool MorphEngine::isRunning(const Slot& s) const
{
  return s.state != State::Cold;
}

int MorphEngine::runningCount() const
{
  int n = 0;
  for (const auto& s : mSlots)
    n += isRunning(s) ? 1 : 0;
  return n;
}

void MorphEngine::computeDesired(double position)
{
  const auto& caps = mPack->captures;
  const int count = static_cast<int>(caps.size());
  for (auto& s : mSlots)
    s.desired = 0.0;
  if (count == 1)
  {
    mSlots[0].desired = 1.0;
    return;
  }
  int hi = 1;
  while (hi < count - 1 && caps[hi].position < position)
    ++hi;
  const int lo = hi - 1;
  const double span = caps[hi].position - caps[lo].position;
  const double t = std::clamp((position - caps[lo].position) / span, 0.0, 1.0);
  if (mStepped)
  {
    mSlots[t < 0.5 ? lo : hi].desired = 1.0;
    return;
  }
  // Pontas exatas não acordam a vizinha à toa.
  if (t <= 1e-9)
    mSlots[lo].desired = 1.0;
  else if (t >= 1.0 - 1e-9)
    mSlots[hi].desired = 1.0;
  else
  {
    mSlots[lo].desired = 1.0 - t;
    mSlots[hi].desired = t;
  }
}

void MorphEngine::jumpToTarget()
{
  if (!mPack || mSlots.empty())
    return;
  mPosition = std::clamp(mTarget, mPack->minPosition(), mPack->maxPosition());
  computeDesired(mPosition);
  for (auto& s : mSlots)
  {
    // O pacote acabou de ser preparado (Reset aquece as capturas), então quem
    // vai tocar já está pronto.
    s.state = s.desired > 0.0 ? State::Hot : State::Cold;
    s.weight = s.target = s.desired;
    s.idleSamples = 0;
    s.warmLeft = 0;
  }
}

void MorphEngine::startWarming()
{
  const auto& caps = mPack->captures;
  const double lo = std::min(mPosition, mTarget);
  const double hi = std::max(mPosition, mTarget);

  // Candidatas: quem tem peso pela posição e quem está no caminho do botão,
  // da mais importante para a menos.
  mOrder.clear();
  for (int i = 0; i < static_cast<int>(mSlots.size()); ++i)
  {
    const bool onPath = caps[i].position >= lo && caps[i].position <= hi;
    if (mSlots[i].state == State::Cold && (mSlots[i].desired > 0.0 || onPath))
      mOrder.push_back(i);
  }
  std::sort(mOrder.begin(), mOrder.end(), [&](int a, int b) {
    if ((mSlots[a].desired > 0.0) != (mSlots[b].desired > 0.0))
      return mSlots[a].desired > 0.0;
    return std::abs(caps[a].position - mPosition) < std::abs(caps[b].position - mPosition);
  });

  int running = runningCount();
  for (int i : mOrder)
  {
    if (running >= mSettings.maxRunning)
    {
      if (mSlots[i].desired <= 0.0)
        break;
      // Libera a captura parada mais distante para dar lugar à necessária.
      int victim = -1;
      double farthest = -1.0;
      for (int j = 0; j < static_cast<int>(mSlots.size()); ++j)
      {
        const auto& s = mSlots[j];
        if (s.state != State::Cold && s.weight == 0.0 && s.target == 0.0 && s.desired == 0.0)
        {
          const double d = std::abs(caps[j].position - mPosition);
          if (d > farthest)
          {
            farthest = d;
            victim = j;
          }
        }
      }
      if (victim < 0)
        break;
      mSlots[victim].state = State::Cold;
      --running;
    }
    mSlots[i].state = State::Warming;
    mSlots[i].warmLeft = mPack->captures[i].warmupSamples;
    mSlots[i].idleSamples = 0;
    ++running;
  }
}

void MorphEngine::computeTargets()
{
  double hotDesired = 0.0;
  for (const auto& s : mSlots)
    if (s.state == State::Hot)
      hotDesired += s.desired;

  if (hotDesired <= 0.0)
  {
    // Nenhuma captura desejada está pronta ainda: segura a mistura atual até
    // o aquecimento terminar.
    for (auto& s : mSlots)
      s.target = s.weight;
    return;
  }
  for (auto& s : mSlots)
    s.target = s.state == State::Hot ? s.desired / hotDesired : 0.0;
}

void MorphEngine::process(const double* input, double* output, int numFrames)
{
  if (!mPack || mSlots.empty())
  {
    std::fill(output, output + numFrames, 0.0);
    return;
  }

  // 1. Posição do botão com inércia.
  double target = std::clamp(mTarget, mPack->minPosition(), mPack->maxPosition());
  if (mStepped)
  {
    double best = mPack->captures.front().position;
    for (const auto& c : mPack->captures)
      if (std::abs(c.position - target) < std::abs(best - target))
        best = c.position;
    target = best;
  }
  const double alpha = 1.0 - std::exp(-numFrames / (mSettings.positionSmoothingMs * 0.001 * mSampleRate));
  mPosition += (target - mPosition) * alpha;
  if (std::abs(target - mPosition) < 1e-4)
    mPosition = target;
  mTarget = target;

  // 2. Pesos, aquecimento e alvos.
  computeDesired(mPosition);
  startWarming();
  computeTargets();

  // 3. Roda as capturas acordadas.
  std::copy(input, input + numFrames, mInputCopy.begin());
  double* in = mInputCopy.data();
  for (auto& s : mSlots)
  {
    if (!isRunning(s))
      continue;
    double* out = s.buffer.data();
    mPack->captures[&s - mSlots.data()].dsp->process(&in, &out, numFrames);
    if (s.state == State::Warming)
    {
      s.warmLeft -= numFrames;
      if (s.warmLeft <= 0)
        s.state = State::Hot;
    }
  }

  // 4. Mistura com rampa de peso amostra a amostra.
  mOrder.clear();
  for (int i = 0; i < static_cast<int>(mSlots.size()); ++i)
    if (mSlots[i].state == State::Hot && (mSlots[i].weight > 0.0 || mSlots[i].target > 0.0))
      mOrder.push_back(i);

  for (int n = 0; n < numFrames; ++n)
  {
    double sum = 0.0, weightSum = 0.0;
    for (int i : mOrder)
    {
      auto& s = mSlots[i];
      const double diff = s.target - s.weight;
      s.weight += std::clamp(diff, -mWeightStep, mWeightStep);
      sum += s.weight * mPack->captures[i].levelGain * s.buffer[n];
      weightSum += s.weight;
    }
    output[n] = weightSum > 0.0 ? sum / weightSum : 0.0;
  }

  // 5. Quem ficou parado tempo demais deixa de rodar.
  const int coolSamples = static_cast<int>(mSettings.coolDownMs * 0.001 * mSampleRate);
  for (auto& s : mSlots)
  {
    if (s.state != State::Hot)
      continue;
    if (s.weight == 0.0 && s.target == 0.0 && s.desired == 0.0)
    {
      s.idleSamples += numFrames;
      if (s.idleSamples > coolSamples)
        s.state = State::Cold;
    }
    else
      s.idleSamples = 0;
  }
}

} // namespace nexus
