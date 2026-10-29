#include "nexus/Separator.h"

#include <algorithm>
#include <cmath>
#include <mutex>
#include <thread>

#include "model.hpp"

#if defined(__APPLE__)
  #include <pthread.h>
  #include <stdlib.h>
#elif defined(_WIN32)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #include <windows.h>
#else
  #include <sys/resource.h>
  #include <sys/syscall.h>
  #include <unistd.h>
#endif

namespace nexus::studio
{
namespace
{
// Sobreposição entre os pedaços de cada núcleo (como no free-music-demixer,
// do mesmo autor do demucs.cpp): 0,75 s de cada lado, emendados em rampa.
constexpr int kOverlap = static_cast<int>(0.75 * kSeparationRate);

struct Cancelled
{
};

// Separar é trabalho de fundo: o áudio (e a tela) sempre passam na frente.
void lowerThisThread()
{
#if defined(__APPLE__)
  pthread_set_qos_class_self_np(QOS_CLASS_UTILITY, 0);
#elif defined(_WIN32)
  SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_LOWEST);
#else
  setpriority(PRIO_PROCESS, static_cast<id_t>(syscall(SYS_gettid)), 15);
#endif
}
} // namespace

struct Separator::Model
{
  demucscpp::demucs_model model;
};

Separator::Separator() = default;
Separator::~Separator() = default;

bool Separator::load(const std::string& modelFile, std::string& error)
{
#if defined(__APPLE__)
  // Cada linha de separação já é um núcleo: as contas do Accelerate não abrem
  // mais linhas por conta própria (senão o computador inteiro engasga).
  setenv("VECLIB_MAXIMUM_THREADS", "1", 0);
#endif
  auto m = std::make_unique<Model>();
  bool ok = false;
  try
  {
    ok = demucscpp::load_demucs_model(modelFile, &m->model);
  }
  catch (const std::exception& e)
  {
    error = e.what();
    return false;
  }
  if (!ok)
  {
    error = "modelo";
    return false;
  }
  if (m->model.is_4sources)
  {
    error = "modelo de 4 faixas";
    return false;
  }
  mModel = std::move(m);
  return true;
}

bool Separator::loaded() const
{
  return mModel != nullptr;
}

int Separator::defaultThreads()
{
  const int n = static_cast<int>(std::thread::hardware_concurrency());
#if defined(__linux__)
  // No Linux o áudio quase nunca tem prioridade de tempo real (o PipeWire e o
  // JACK sem o grupo "audio" rodam na prioridade comum) e a separação disputa
  // com ele a memória e os núcleos. Medido num Ryzen 7 de 16 núcleos lógicos,
  // bloco de 128, tocando a música e a guitarra enquanto separa: 6 linhas
  // davam cerca de 170 estouros em 29 s; 3 linhas, nenhum. E a separação só
  // ficou 15% mais lenta (1,50 para 1,73 vez a duração da música): o que
  // limita é a memória, não os núcleos. Um quarto dos núcleos lógicos, no
  // máximo 3.
  return std::clamp(n / 4, 1, 3);
#else
  // Metade dos núcleos lógicos, no máximo 6: sobra para o áudio e o resto.
  return std::clamp(n / 2, 1, 6);
#endif
}

bool Separator::separate(const Stereo& mix, int threads, const std::function<void(float)>& progress,
                         const std::atomic<bool>& cancel, std::array<Stereo, numStems>& out, std::string& error) const
{
  if (mModel == nullptr)
  {
    error = "modelo";
    return false;
  }
  const int total = static_cast<int>(mix.frames());
  if (total < kSeparationRate)
  {
    error = "curta";
    return false;
  }
  // Pedaços curtos demais desperdiçam a sobreposição: no mínimo 20 s cada.
  threads = std::clamp(threads, 1, std::max(1, total / (20 * kSeparationRate)));
  const int length = (total + threads - 1) / threads;

  std::vector<float> done(static_cast<size_t>(threads), 0.0f);
  std::mutex progressLock;
  std::vector<Eigen::Tensor3dXf> results(static_cast<size_t>(threads));
  std::vector<std::string> errors(static_cast<size_t>(threads));
  std::vector<std::thread> workers;

  for (int i = 0; i < threads; ++i)
  {
    workers.emplace_back([&, i] {
      lowerThisThread();
      // O pedaço com a sobreposição de cada lado (repetindo a borda nas pontas).
      const int start = i * length;
      const int end = std::min(total, start + length);
      const int n = end - start + 2 * kOverlap;
      Eigen::MatrixXf segment(2, n);
      for (int j = 0; j < n; ++j)
      {
        const int src = std::clamp(start - kOverlap + j, 0, total - 1);
        segment(0, j) = mix.left[static_cast<size_t>(src)];
        segment(1, j) = mix.right[static_cast<size_t>(src)];
      }
      demucscpp::ProgressCallback cb = [&, i](float p, const std::string&) {
        if (cancel.load())
          throw Cancelled{};
        const std::scoped_lock lock(progressLock);
        done[static_cast<size_t>(i)] = std::clamp(p, 0.0f, 1.0f);
        float sum = 0.0f;
        for (auto d : done)
          sum += d;
        if (progress)
          progress(sum / static_cast<float>(done.size()));
      };
      try
      {
        results[static_cast<size_t>(i)] = demucscpp::demucs_inference(mModel->model, segment, cb);
      }
      catch (const Cancelled&)
      {
        errors[static_cast<size_t>(i)] = "cancelado";
      }
      catch (const std::exception& e)
      {
        errors[static_cast<size_t>(i)] = e.what();
      }
    });
  }
  for (auto& w : workers)
    w.join();
  for (const auto& e : errors)
    if (!e.empty())
    {
      error = cancel.load() ? "cancelado" : e;
      return false;
    }

  // Emenda: cada pedaço pesa 1 no meio e cai em rampa na sobreposição.
  for (auto& s : out)
  {
    s.left.assign(static_cast<size_t>(total), 0.0f);
    s.right.assign(static_cast<size_t>(total), 0.0f);
  }
  std::vector<float> weightSum(static_cast<size_t>(total), 0.0f);
  for (int i = 0; i < threads; ++i)
  {
    const auto& r = results[static_cast<size_t>(i)];
    const int start = i * length;
    const int n = static_cast<int>(r.dimension(2));
    for (int j = 0; j < n; ++j)
    {
      const int g = start - kOverlap + j;
      if (g < 0 || g >= total)
        continue;
      const float w = std::min({1.0f, static_cast<float>(j + 1) / (2.0f * kOverlap),
                                static_cast<float>(n - j) / (2.0f * kOverlap)});
      weightSum[static_cast<size_t>(g)] += w;
      for (int t = 0; t < numStems; ++t)
      {
        out[static_cast<size_t>(t)].left[static_cast<size_t>(g)] += r(t, 0, j) * w;
        out[static_cast<size_t>(t)].right[static_cast<size_t>(g)] += r(t, 1, j) * w;
      }
    }
  }
  for (int g = 0; g < total; ++g)
  {
    const float w = weightSum[static_cast<size_t>(g)];
    if (w <= 0.0f)
      continue;
    for (auto& s : out)
    {
      s.left[static_cast<size_t>(g)] /= w;
      s.right[static_cast<size_t>(g)] /= w;
    }
  }
  if (progress)
    progress(1.0f);
  return true;
}
} // namespace nexus::studio
