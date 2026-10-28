#pragma once

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <set>

#include "PluginProcessor.h"
#include "report/Report.h"
#include "ui/AudioPanel.h"
#include "ui/CapturePanel.h"
#include "ui/GearBrowser.h"
#include "ui/PackMapper.h"
#include "ui/PresetBar.h"
#include "ui/StudioView.h"
#include "ui/EqStage.h"
#include "ui/Tuner.h"
#include "ui/TestBar.h"
#include "ui/UpdateBadge.h"
#include "ui/Controls.h"
#include "ui/Gear.h"

// Tela do Nexus Modeler. De cima para baixo: barra com a marca e o motor;
// o palco, com a peça escolhida renderizada em 3D sob um foco de luz; o nome
// da peça, o pacote e o trilho das capturas; e a cadeia de sinal, montada por
// clique e arrasta: pedais antes, pedal A/B, cabeçote, pedais entre cabeçote
// e caixa, caixa, pedais depois.
class NexusEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer
{
public:
  static constexpr int kWidth = 1400;
  static constexpr int kBrowserWidth = 280;
  static constexpr int kHeight = 800;
  static constexpr int kPedalLooks = NexusProcessor::kPedalLooks;

  // O que o palco mostra: um pedal (0 a 11), um cabeçote ou uma caixa (lado 0 = A, 1 = B).
  enum class Kind
  {
    pedal,
    amp,
    cab,
    eq,   // equalizador do fim da cadeia (fixo)
    tuner // afinador do começo da cadeia (fixo)
  };
  struct Selection
  {
    Kind kind = Kind::amp;
    int index = 0;
    bool operator==(const Selection& o) const { return kind == o.kind && index == o.index; }
  };

  explicit NexusEditor(NexusProcessor&);
  // Testes: uma leitura do afinador fora do relógio.
  void updateTuner() { mTunerStage->update(); }
  ~NexusEditor() override;

  void paint(juce::Graphics&) override;
  void paintOverChildren(juce::Graphics&) override;
  void resized() override;
  void select(Selection s);
  Selection selection() const { return mSel; }
  nexus::ui::AudioPanel* audioPanel() { return mAudioPanel.get(); }
  // Abre a janela das capturas da peça escolhida (o mesmo que "abrir pacote").
  void showCaptures() { choosePack(); }
  // 0 = equipamento, 1 = estúdio.
  void setMode(int mode);


private:
  // Pedal ou cabeçote: imagem, botões renderizados e, no pedal, pisante e LED.
  class GearStage : public juce::Component
  {
  public:
    GearStage(NexusProcessor& p, Kind kind);
    // Monta os botões da peça. Mesma peça: nada muda, a não ser com "rebuild"
    // (troca de canal muda o parâmetro do ganho).
    void setTarget(int index, const nexus::ui::GearArt* art, bool rebuild = false);
    int index() const { return mIndex; }
    void paint(juce::Graphics&) override;
    void paintOverChildren(juce::Graphics&) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    std::function<void(const juce::String&, juce::Component&)> onMidiMenu;
    // Relógio da tela: atualiza o BPM mostrado no TAP.
    void tick() { refreshTempo(); }

  private:
    juce::String paramFor(const juce::String& knobId) const;
    bool pedalOn() const;
    juce::Point<float> toScreen(juce::Point<float> imagePx) const;
    bool overFootswitch(juce::Point<float> p) const;
    // Chaves alavanca do Blueberry Chain (2 | 3 e x | /): centro e raio na tela.
    std::vector<std::pair<juce::Point<float>, float>> chainToggles() const;

    NexusProcessor& mProcessor;
    Kind mKind;
    int mIndex = 0;
    const nexus::ui::GearArt* mArt = nullptr;
    std::vector<std::unique_ptr<nexus::ui::FilmKnob>> mKnobs;
    juce::Rectangle<float> mImage;
    float mScale = 1.0f;
    // Andamento (modulações e delay): sincronizar e tap tempo. Botão direito
    // no TAP liga uma pisada da pedaleira (aprender MIDI).
    struct TapButton : juce::TextButton
    {
      std::function<void()> onRightClick;
      void mouseDown(const juce::MouseEvent& e) override
      {
        if (e.mods.isPopupMenu() && onRightClick)
          onRightClick();
        else
          juce::TextButton::mouseDown(e);
      }
    };
    juce::TextButton mSync;
    TapButton mTap;
    void refreshTempo();
  };

  // Caixa grande, com o seletor Desligada / Captura / IR próprio.
  class CabStage : public juce::Component
  {
  public:
    CabStage(NexusProcessor& p, const nexus::ui::GearArt* arts);
    void setRig(int rig) { mRig = rig; refresh(); }
    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh();

  private:
    NexusProcessor& mProcessor;
    const nexus::ui::GearArt* mArts; // uma por visual de caixa (NexusProcessor::kCabLooks)
    const nexus::ui::GearArt& art() const;
    nexus::ui::Segmented mModes{{"Desligada", "Captura", "IR"}};
    juce::Rectangle<float> mImage;
    juce::Rectangle<float> mArea; // espaço para a imagem (sem as abas)
    int mRig = 0;
    bool mDim = false;
  };

  // Trilho das capturas de um pacote: um losango por captura, aceso pelo peso.
  class CaptureRail : public juce::Component
  {
  public:
    explicit CaptureRail(NexusProcessor& p) : mProcessor(p) {}
    void setSlot(int slot) { mSlot = slot; repaint(); }
    void paint(juce::Graphics&) override;

  private:
    NexusProcessor& mProcessor;
    int mSlot = NexusProcessor::ampSlot(0, 0);
  };

  // Cadeia de sinal com clique e arrasta.
  class ChainView : public juce::Component, public juce::DragAndDropTarget
  {
  public:
    explicit ChainView(NexusEditor& editor);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent&) override;
    void mouseDrag(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    void mouseExit(const juce::MouseEvent&) override { mHover = -1; repaint(); }
    void rebuild();
    nexus::ui::LevelFader& input() { return *mInput; }
    nexus::ui::TestBar& testBar() { return *mTest; }
    // Equipamento arrastado da coluna da direita.
    bool isInterestedInDragSource(const SourceDetails&) override;
    void itemDragMove(const SourceDetails&) override;
    void itemDragExit(const SourceDetails&) override;
    void itemDropped(const SourceDetails&) override;
    // Lista da cadeia: 0 = antes, 1 = entre (A), 2 = entre (B), 3 = depois.
    static std::vector<int>& list(ChainLayout& l, int which);
    nexus::ui::LevelFader& output() { return *mOutput; }

  private:
    enum class CardType
    {
      pedal,
      amp,
      cab,
      ab,
      add,
      eq,
      tuner
    };
    struct Card
    {
      CardType type;
      int index; // pedal (0 a 11) ou lado (0 = A, 1 = B)
      juce::Rectangle<float> r;
    };
    // Lugar onde um pedal arrastado pode entrar: lista e posição na lista.
    struct Drop
    {
      int list; // 0 = antes, 1 = entre (A), 2 = entre (B), 3 = depois
      int position;
      juce::Point<float> at;
    };

    int cardAt(juce::Point<float> p) const;
    const Drop* nearestDrop(juce::Point<float> p) const;
    void showCardMenu(int card);
    void dropPedal(int pedal, const Drop& d);

    NexusEditor& mEditor;
    std::unique_ptr<nexus::ui::LevelFader> mInput, mOutput;
    std::unique_ptr<nexus::ui::TestBar> mTest;
    nexus::ui::Segmented mRigs{{"1", "2"}};
    std::vector<Card> mCards;
    std::vector<Drop> mDrops;
    std::vector<juce::Path> mWires;
    int mHover = -1;
    int mPressed = -1;
    bool mDragging = false;
    juce::Point<float> mDragAt;
    bool mExternal = false; // pedal vindo da coluna, ainda no ar
  };

  void timerCallback() override;
  // "Abrir pacote": janela das capturas da TONE3000 para a peça escolhida.
  void choosePack();
  // "Inserir manualmente": pasta de pacote do computador.
  void chooseFolder();
  // Pacote já na pasta do equipamento: mapeia se precisar e carrega.
  void landPack(const juce::File& folder);
  void loadPackInto(Selection target, int channel, const juce::File& folder);
  void loadHeadChannels(int rig);
  void showReportResult(const nexus::report::Sent& sent);
  // Captura padrão: peça vazia (pedal, canal do cabeçote, caixa) recebe a
  // padrão das recomendadas; se ainda não foi baixada e a conta da TONE3000
  // está conectada, baixa sozinha. Quem quiser outra troca pelo pacote.
  void ensureDefaults();
  void ensurePedal(int pedal);
  void ensureCab(int rig, bool swapping = false);
  // searchWord: busca a mais curtida da TONE3000 com essa palavra antes da
  // recomendada (vazia: só a recomendada).
  void fetchDefault(const nexus::t3k::Tone& tone, const char* filter, const juce::String& gearName, int slotKey,
                    std::function<void(const juce::File&)> land, const juce::String& searchWord = {});
  static constexpr int kCabSlotKey = 1000;
  void chooseIr(int rig);
  void chooseIrFile(int rig);
  void openIrCatalog(int rig);
  void applyIr(int rig, const juce::File& file);
  void showAbout();
  void reportProblem();
  // O Nexus fechou sozinho da última vez: oferece mandar o relatório.
  void offerCrashReport();
  void showAudioPanel(bool show);
  void showLanguageMenu();
  void retranslate();
  void showMidiMenu(const juce::String& paramId, juce::Component& target);
  // Coluna de equipamentos: lista, aba e peça marcada.
  void refreshBrowser();
  void pickGear(const nexus::ui::GearItem& item);
  // Põe um pedal novo, com esse visual, na lista e posição dadas.
  void insertPedal(int look, int list, int position);
  int pedalLook(int pedal) const;
  const nexus::ui::GearArt* pedalArt(int pedal) const;
  juce::String packSummary(int slot) const;
  int railSlot() const;
  void refreshChannels();
  void layoutTitle();
  void addPedal();

  // Botões de texto com a mesma fonte do resto da tela.
  struct Look : juce::LookAndFeel_V4
  {
    juce::Font getTextButtonFont(juce::TextButton&, int height) override;
    // Avisos: fundo escuro e uma faixa de cor na lateral (vermelha para
    // alerta, ciano para informação), sem o ícone grande por trás do texto.
    void drawAlertBox(juce::Graphics&, juce::AlertWindow&, const juce::Rectangle<int>& textArea, juce::TextLayout&) override;
  };

  NexusProcessor& mProcessor;
  Look mLook;
  juce::TooltipWindow mTooltips{this, 600};
  // Visual de cada cabeçote (Brit 800, Mark Five...), pelo parâmetro "cabecote".
  std::array<nexus::ui::GearArt, NexusProcessor::kAmpLooks> mAmpArts;
  std::array<nexus::ui::GearArt, NexusProcessor::kCabLooks> mCabArts;
  juce::SharedResourcePointer<nexus::t3k::Connection> mT3k;
  std::set<juce::String> mFetching; // tons baixando (número + pacote)
  std::set<int> mFetchingSlots;     // espaços esperando a captura padrão
  const nexus::ui::GearArt& cabArt(int rig) const;
  const nexus::ui::GearArt& ampArt(int rig) const { return mAmpArts[static_cast<size_t>(juce::jlimit(0, NexusProcessor::kAmpLooks - 1, mProcessor.ampLook(rig)))]; }
  int mShownAmpLook = -1;
  std::array<nexus::ui::GearArt, kPedalLooks> mPedalArts;
  Selection mSel;
  int mShownPedalLook = -1;
  int mShownChannel = -1;

  nexus::ui::Segmented mEngine{{"A2 Full", "A2 Lite"}};
  nexus::ui::Segmented mMode{{"Equipamento", juce::String::fromUTF8("Estúdio")}};
  std::unique_ptr<nexus::ui::StudioView> mStudio;
  std::unique_ptr<nexus::ui::EqStage> mEqStage;
  std::unique_ptr<nexus::ui::TunerStage> mTunerStage;

  nexus::ui::IconButton mInfoButton{nexus::ui::Icon::info, "Sobre o Nexus Modeler"};
  nexus::ui::IconButton mLanguageButton{nexus::ui::Icon::globe, "Idioma"};
  nexus::ui::IconButton mAudioButton{nexus::ui::Icon::audio, juce::String::fromUTF8("Interface de áudio")};
  std::unique_ptr<nexus::ui::AudioPanel> mAudioPanel; // só no programa independente
  std::unique_ptr<nexus::ui::CapturePanel> mCapturePanel;
  std::unique_ptr<nexus::ui::PackMapper> mMapper;
  juce::String mPackGearName;
  Selection mPackTarget;
  int mPackChannel = 0;
  std::unique_ptr<GearStage> mPedalStage, mAmpStage;
  std::unique_ptr<CabStage> mCabStage;
  std::unique_ptr<CaptureRail> mRail;
  std::unique_ptr<ChainView> mChain;
  nexus::ui::Segmented mChannels{{}};
  nexus::ui::TextLink mChangeLink{"trocar pacote"};
  std::unique_ptr<nexus::ui::GearBrowser> mBrowser;
  std::unique_ptr<nexus::ui::PresetBar> mPresets;
  nexus::ui::UpdateBadge mUpdate;
  std::unique_ptr<juce::FileChooser> mChooser;
  juce::String mTitle, mSubtitle;
  juce::Rectangle<int> mTitleArea;
  float mInPeak = 0.0f, mOutPeak = 0.0f;
  juce::String mMidiNotice;
  juce::uint32 mMidiNoticeUntil = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusEditor)
};
