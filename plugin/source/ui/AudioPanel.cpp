#include "AudioPanel.h"

#include "Theme.h"
#include "../report/Report.h"

#include <juce_audio_utils/juce_audio_utils.h>

namespace nexus::ui
{
namespace
{
juce::String ms(double v)
{
  return juce::String(v, 1).replaceCharacter('.', ',') + " ms";
}

juce::String rateText(double sr)
{
  juce::String s = juce::String(sr / 1000.0, sr == std::floor(sr / 1000.0) * 1000.0 ? 0 : 1).replaceCharacter('.', ',') + " kHz";
  if (std::abs(sr - 48000.0) < 1.0)
    s << "  " << TRANS(juce::String::fromUTF8("(recomendada: sem conversão)"));
  return s;
}
} // namespace

AudioPanel::AudioPanel(juce::AudioDeviceManager& devices, std::function<void()> save, std::function<int()> pluginLatency,
                       std::function<float()> inputPeak)
: mDevices(devices)
, mSave(std::move(save))
, mPluginLatency(std::move(pluginLatency))
, mInputPeak(std::move(inputPeak))
{
  retranslate();
  for (auto* row : {&mType, &mInputDevice, &mOutputDevice, &mGuitar, &mOutputs, &mRate, &mBuffer})
    addAndMakeVisible(row->box);

  mType.box.onChange = [this] {
    if (mRefreshing)
      return;
    mDevices.setCurrentAudioDeviceType(mType.box.getText(), true);
    if (mSave)
      mSave();
    refresh();
  };
  mInputDevice.box.onChange = [this] {
    apply([this](auto& s) { s.inputDeviceName = mInputDevice.chosen(); s.useDefaultInputChannels = true; });
  };
  mOutputDevice.box.onChange = [this] {
    apply([this](auto& s) { s.outputDeviceName = mOutputDevice.chosen(); s.useDefaultOutputChannels = true; });
  };
  mGuitar.box.onChange = [this] {
    const int ch = mGuitar.box.getSelectedId() - 1;
    apply([ch](auto& s) {
      s.useDefaultInputChannels = false;
      s.inputChannels.clear();
      s.inputChannels.setBit(ch);
    });
  };
  mOutputs.box.onChange = [this] {
    const int first = (mOutputs.box.getSelectedId() - 1) * 2;
    apply([first](auto& s) {
      s.useDefaultOutputChannels = false;
      s.outputChannels.clear();
      s.outputChannels.setRange(first, 2, true);
    });
  };
  mRate.box.onChange = [this] {
    const double sr = static_cast<double>(mRate.box.getSelectedId());
    apply([sr](auto& s) { s.sampleRate = sr; });
  };
  mBuffer.box.onChange = [this] {
    const int size = mBuffer.box.getSelectedId();
    if (isPipeWireJack())
    {
      // No JACK do PipeWire o bloco é pedido na abertura da conexão: guarda o
      // pedido e reabre a conexão (o JACK não troca o bloco com ela aberta).
      if (mRefreshing)
        return;
      const double sr = mDevices.getCurrentAudioDevice() != nullptr ? mDevices.getCurrentAudioDevice()->getCurrentSampleRate() : 48000.0;
      setPipeWireLatency(size, sr);
      auto setup = mDevices.getAudioDeviceSetup();
      setup.bufferSize = size;
      // Fecha a conexão de vez: só uma conexão nova lê o bloco pedido (reabrir
      // a mesma mantinha o bloco antigo e a lista voltava ao valor anterior).
      mDevices.closeAudioDevice();
      mError = mDevices.setAudioDeviceSetup(setup, true);
      if (mError.isNotEmpty())
        nexus::report::log("audio", "JACK " + juce::String(size) + ": " + mError);
      if (mSave)
        mSave();
      refresh();
      return;
    }
    apply([size](auto& s) { s.bufferSize = size; });
  };

  addAndMakeVisible(mDriverPanel);
  mDriverPanel.onClick = [this] {
    if (auto* device = mDevices.getCurrentAudioDevice())
      if (device->hasControlPanel() && device->showControlPanel())
        mDevices.restartLastAudioDevice();
    refresh();
  };
  // Pareamento de MIDI por Bluetooth: janela do próprio sistema no Mac. No
  // Windows e no Linux o pareamento é nos ajustes de Bluetooth do sistema; o
  // dispositivo aparece aqui depois, como qualquer MIDI.
  if (juce::BluetoothMidiDevicePairingDialogue::isAvailable())
  {
    addAndMakeVisible(mBluetooth);
    mBluetooth.onClick = [] { juce::BluetoothMidiDevicePairingDialogue::open(); };
  }
  addAndMakeVisible(mClose);
  mClose.onClick = [this] {
    if (onClose)
      onClose();
  };

  mDevices.addChangeListener(this);
  refresh();
}

AudioPanel::~AudioPanel()
{
  mDevices.removeChangeListener(this);
}

void AudioPanel::retranslate()
{
  mType.label = TRANS(juce::String::fromUTF8("Sistema de áudio"));
  mOutputDevice.label = TRANS(juce::String::fromUTF8("Interface de saída"));
  mGuitar.label = TRANS("Entrada da guitarra");
  mOutputs.label = TRANS(juce::String::fromUTF8("Saídas"));
  mRate.label = TRANS("Taxa");
  mBuffer.label = TRANS("Buffer");
  mDriverPanel.setButtonText(TRANS("Painel do driver"));
  mBluetooth.setButtonText(TRANS("Parear MIDI Bluetooth"));
  mClose.setButtonText(TRANS("Fechar"));
  refresh();
}

void AudioPanel::visibilityChanged()
{
  if (isVisible())
  {
    refresh();
    startTimerHz(20);
  }
  else
    stopTimer();
}

bool AudioPanel::isPipeWireJack() const
{
 #if JUCE_LINUX || JUCE_BSD
  // O pedido de bloco só vale quando quem atende o JACK é o PipeWire.
  return mDevices.getCurrentAudioDeviceType() == "JACK" && juce::File("/run/user").isDirectory()
         && juce::File("/usr/lib/x86_64-linux-gnu/pipewire-0.3/jack/libjack.so.0").existsAsFile();
 #else
  return false;
 #endif
}

void AudioPanel::setPipeWireLatency(int blockSize, double sampleRate)
{
 #if JUCE_LINUX || JUCE_BSD
  setenv("PIPEWIRE_LATENCY", (juce::String(blockSize) + "/" + juce::String(juce::roundToInt(sampleRate))).toRawUTF8(), 1);
 #else
  juce::ignoreUnused(blockSize, sampleRate);
 #endif
}

void AudioPanel::apply(const std::function<void(juce::AudioDeviceManager::AudioDeviceSetup&)>& change)
{
  if (mRefreshing)
    return;
  auto setup = mDevices.getAudioDeviceSetup();
  change(setup);
  mError = mDevices.setAudioDeviceSetup(setup, true);
  if (mError.isNotEmpty())
    nexus::report::log("audio", setup.outputDeviceName + " / " + setup.inputDeviceName + ": " + mError);
  if (mSave)
    mSave();
  refresh();
}

void AudioPanel::refresh()
{
  const juce::ScopedValueSetter<bool> guard(mRefreshing, true);
  auto* type = mDevices.getCurrentDeviceTypeObject();
  auto* device = mDevices.getCurrentAudioDevice();
  const auto setup = mDevices.getAudioDeviceSetup();

  mType.box.clear(juce::dontSendNotification);
  int id = 1;
  for (auto* t : mDevices.getAvailableDeviceTypes())
    mType.box.addItem(t->getTypeName(), id++);
  mType.box.setText(mDevices.getCurrentAudioDeviceType(), juce::dontSendNotification);

  const bool separate = type != nullptr && type->hasSeparateInputsAndOutputs();
  auto fillDevices = [&](Row& row, bool input, const juce::String& current) {
    row.box.clear(juce::dontSendNotification);
    row.names.clear();
    if (type == nullptr)
      return;
    // No Linux, o dispositivo padrão do ALSA se chama "Default ALSA Output"
    // também na lista de entradas; mostramos "Padrão do sistema".
    auto shown = [](const juce::String& name) {
      return name.startsWith("Default ALSA") ? TRANS(juce::String::fromUTF8("Padrão do sistema")) + name.fromFirstOccurrenceOf(" (", true, false)
                                             : name;
    };
    row.names = type->getDeviceNames(input);
    for (int i = 0; i < row.names.size(); ++i)
      row.box.addItem(shown(row.names[i]), i + 1);
    const int at = row.names.indexOf(current);
    if (at >= 0)
      row.box.setSelectedId(at + 1, juce::dontSendNotification);
    else
      row.box.setText(shown(current), juce::dontSendNotification);
  };
  fillDevices(mInputDevice, true, setup.inputDeviceName);
  fillDevices(mOutputDevice, false, setup.outputDeviceName);
  mInputDevice.label = separate ? TRANS("Interface de entrada") : TRANS("Interface");
  mOutputDevice.box.setVisible(separate);

  mGuitar.box.clear(juce::dontSendNotification);
  mOutputs.box.clear(juce::dontSendNotification);
  mRate.box.clear(juce::dontSendNotification);
  mBuffer.box.clear(juce::dontSendNotification);
  if (device != nullptr)
  {
    const auto ins = device->getInputChannelNames();
    for (int i = 0; i < ins.size(); ++i)
      mGuitar.box.addItem(juce::String(i + 1) + "  " + ins[i], i + 1);
    const int activeIn = device->getActiveInputChannels().findNextSetBit(0);
    if (activeIn >= 0)
      mGuitar.box.setSelectedId(activeIn + 1, juce::dontSendNotification);

    const auto outs = device->getOutputChannelNames();
    for (int i = 0; i + 1 < outs.size(); i += 2)
      mOutputs.box.addItem(juce::String(i + 1) + " + " + juce::String(i + 2), i / 2 + 1);
    if (outs.size() == 1)
      mOutputs.box.addItem("1", 1);
    const int activeOut = device->getActiveOutputChannels().findNextSetBit(0);
    if (activeOut >= 0)
      mOutputs.box.setSelectedId(activeOut / 2 + 1, juce::dontSendNotification);

    for (auto sr : device->getAvailableSampleRates())
      mRate.box.addItem(rateText(sr), juce::roundToInt(sr));
    mRate.box.setSelectedId(juce::roundToInt(device->getCurrentSampleRate()), juce::dontSendNotification);

    const double sr = device->getCurrentSampleRate();
    // O JACK só informa o bloco atual; com o PipeWire dá para pedir outro.
    auto sizes = device->getAvailableBufferSizes();
    if (isPipeWireJack())
    {
      sizes = {64, 128, 256, 512, 1024};
      sizes.addIfNotAlreadyThere(device->getCurrentBufferSizeSamples());
      sizes.sort();
    }
    for (auto size : sizes)
      mBuffer.box.addItem(TRANS("%1 amostras").replace("%1", juce::String(size)) + "  (" + ms(1000.0 * size / sr) + ")", size);
    mBuffer.box.setSelectedId(device->getCurrentBufferSizeSamples(), juce::dontSendNotification);
    mDriverPanel.setVisible(device->hasControlPanel());
  }
  else
    mDriverPanel.setVisible(false);
  resized();
  repaint();
}

double AudioPanel::roundTripMs() const
{
  auto* device = mDevices.getCurrentAudioDevice();
  if (device == nullptr)
    return 0.0;
  const double sr = device->getCurrentSampleRate();
  if (sr <= 0.0)
    return 0.0;
  // Ida e volta: entrada + saída (os drivers já contam o buffer de cada lado)
  // + a conversão de taxa do Nexus, quando a interface não está em 48 kHz.
  const int samples = device->getInputLatencyInSamples() + device->getOutputLatencyInSamples()
                      + (mPluginLatency ? mPluginLatency() : 0);
  return 1000.0 * samples / sr;
}

void AudioPanel::timerCallback()
{
  juce::StringArray names;
  for (const auto& d : juce::MidiInput::getAvailableDevices())
    names.add(d.name);
  const auto midi = names.isEmpty() ? TRANS("nenhum dispositivo conectado") : names.joinIntoString(", ");
  if (midi != mMidiDevices)
  {
    mMidiDevices = midi;
    repaint();
  }
  mMeter = std::max(mInputPeak ? mInputPeak() : 0.0f, mMeter * 0.85f);
  mCpu = mDevices.getCpuUsage();
  repaint(card().removeFromBottom(200));
}

juce::Rectangle<int> AudioPanel::card() const
{
  return getLocalBounds().withSizeKeepingCentre(620, mError.isEmpty() ? 590 : 630);
}

void AudioPanel::resized()
{
  auto r = card().reduced(32, 0);
  r.removeFromTop(76);
  for (auto* row : {&mType, &mInputDevice, &mOutputDevice, &mGuitar, &mOutputs, &mRate, &mBuffer})
  {
    if (!row->box.isVisible())
      continue;
    auto line = r.removeFromTop(44);
    row->box.setBounds(line.removeFromRight(360).reduced(0, 6));
  }
  auto buttons = card().reduced(32, 22).removeFromBottom(36);
  mClose.setBounds(buttons.removeFromRight(110));
  buttons.removeFromRight(10);
  mDriverPanel.setBounds(buttons.removeFromRight(150));
  buttons.removeFromRight(10);
  mBluetooth.setBounds(buttons.removeFromLeft(190));
}

void AudioPanel::paint(juce::Graphics& g)
{
  g.fillAll(juce::Colours::black.withAlpha(0.6f));
  const auto c = card().toFloat();
  g.setColour(juce::Colour(0xff17181b));
  g.fillRoundedRectangle(c, 12.0f);
  g.setColour(colour::boxBorder);
  g.drawRoundedRectangle(c.reduced(0.5f), 12.0f, 1.0f);

  auto r = c.reduced(32.0f, 0.0f);
  auto head = r.removeFromTop(76.0f);
  g.setColour(colour::text);
  g.setFont(Fonts::get().display(20.0f).withExtraKerningFactor(language::allowsLetterSpacing() ? 0.14f : 0.0f));
  g.drawText(upper(TRANS(juce::String::fromUTF8("Interface de áudio"))), head.withTrimmedTop(10.0f), juce::Justification::centredLeft);
  g.setColour(colour::accent);
  g.fillRect(head.getX(), head.getBottom() - 16.0f, 28.0f, 2.0f);

  for (auto* row : {&mType, &mInputDevice, &mOutputDevice, &mGuitar, &mOutputs, &mRate, &mBuffer})
  {
    if (!row->box.isVisible())
      continue;
    auto line = r.removeFromTop(44.0f);
    drawCaps(g, row->label, line.removeFromLeft(r.getWidth() - 370.0f), 10.5f, juce::Justification::centredLeft, colour::dim);
  }

  // Latência de ida e volta, uso do processador e o sinal da guitarra.
  r.removeFromTop(10.0f);
  auto summary = r.removeFromTop(66.0f);
  g.setColour(juce::Colour(0xff111113));
  g.fillRoundedRectangle(summary, 8.0f);
  auto s = summary.reduced(16.0f, 10.0f);
  auto left = s.removeFromLeft(s.getWidth() * 0.5f);
  const double rt = roundTripMs();
  const juce::Colour quality = rt <= 0.0 ? colour::dim : rt <= 12.0 ? juce::Colour(0xff3ddc84) : rt <= 25.0 ? juce::Colour(0xffe6b94a) : colour::danger;
  drawCaps(g, TRANS(juce::String::fromUTF8("Latência ida e volta")), left.removeFromTop(16.0f), 10.0f, juce::Justification::centredLeft, colour::dim);
  g.setColour(quality);
  g.setFont(Fonts::get().display(24.0f));
  g.drawText(rt > 0.0 ? ms(rt) : TRANS("sem interface"), left, juce::Justification::centredLeft);

  drawCaps(g, TRANS("Sinal da guitarra"), s.removeFromTop(16.0f), 10.0f, juce::Justification::centredLeft, colour::dim);
  auto meter = s.removeFromTop(14.0f).withTrimmedTop(5.0f).withHeight(4.0f);
  g.setColour(colour::line);
  g.fillRoundedRectangle(meter, 2.0f);
  const float db = juce::Decibels::gainToDecibels(mMeter, -60.0f);
  g.setColour(db > -1.0f ? colour::danger : colour::accent);
  g.fillRoundedRectangle(meter.withWidth(meter.getWidth() * juce::jlimit(0.0f, 1.0f, (db + 60.0f) / 60.0f)), 2.0f);
  g.setColour(colour::dim);
  g.setFont(Fonts::get().ui(11.5f));
  g.drawText(TRANS("Processador: %1%").replace("%1", juce::String(juce::roundToInt(mCpu * 100.0))), s, juce::Justification::centredLeft);

  // Dispositivos MIDI (USB e Bluetooth): abertos sozinhos.
  r.removeFromTop(10.0f);
  auto midiRow = r.removeFromTop(30.0f);
  drawCaps(g, "MIDI", midiRow.removeFromLeft(r.getWidth() - 370.0f), 10.5f, juce::Justification::centredLeft, colour::dim);
  g.setColour(colour::text);
  g.setFont(Fonts::get().ui(13.0f));
  g.drawFittedText(mMidiDevices, midiRow.toNearestInt(), juce::Justification::centredLeft, 1);

  if (mError.isNotEmpty())
  {
    g.setColour(colour::danger);
    g.setFont(Fonts::get().ui(12.0f));
    g.drawFittedText(mError, r.removeFromTop(34.0f).toNearestInt(), juce::Justification::centredLeft, 2);
  }
}

void AudioPanel::mouseUp(const juce::MouseEvent& e)
{
  // Clicar fora do cartão fecha o painel.
  if (!card().contains(e.getPosition()) && onClose)
    onClose();
}
} // namespace nexus::ui
