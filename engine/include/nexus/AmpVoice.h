#pragma once

#include <array>
#include <memory>
#include <vector>

#include "nexus/MorphEngine.h"
#include "nexus/ToneStack.h"

namespace nexus
{

// Um amplificador completo do painel: motor de troca de capturas e, depois
// dele, o equalizador da família (grave, médio, agudo, presença e, se a
// família tiver, o gráfico de barrinhas). Mono, precisão dupla.
class AmpVoice
{
public:
  struct Controls
  {
    double main = 5.0; // botão principal do pacote (ganho, drive...)
    ToneKnobs tone{};
    double presence = 5.0;
    std::array<double, RelativeGraphicEq::kMaxBands> graphicDb{};
    bool stepped = false;
  };

  // Fora do áudio.
  void prepare(std::shared_ptr<Pack> pack, double sampleRate, int maxBlockSize);
  void enableGraphicEq(const double* frequencies, int numBands);

  // Do áudio: valores vindos da tela ou da automação.
  void setControls(const Controls& c) { mTargetControls = c; }
  void jumpToControls();
  void process(const double* input, double* output, int numFrames);

  const MorphEngine& morph() const { return mMorph; }
  const Controls& currentControls() const { return mControls; }

private:
  void applyControls();

  MorphEngine mMorph;
  RelativeToneStack mToneStack;
  RelativeShelf mPresence;
  RelativeGraphicEq mGraphic;
  bool mHasGraphic = false;
  Controls mControls{}, mTargetControls{};
  double mSampleRate = 48000.0;
  double mKnobAlpha = 0.0;
};

} // namespace nexus
