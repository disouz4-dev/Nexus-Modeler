#pragma once

#include <optional>

#include <juce_audio_processors/juce_audio_processors.h>

#include <array>
#include <deque>
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
#include "ui/ArteBadge.h"
#include "ui/Controls.h"
#include "ui/Gear.h"
#include "ui/IrMic.h"

// Tela do Nexus Modeler. De cima para baixo: barra com a marca e o motor;
// o palco, com a peça escolhida renderizada em 3D sob um foco de luz (pedal
// escolhido: a pedaleira com todos os pedais da cadeia); o nome
// da peça, o pacote e o trilho das capturas; e a cadeia de sinal, montada por
// clique e arrasta: pedais antes, pedal A/B, cabeçote, pedais entre cabeçote
// e caixa, caixa, pedais depois.
class NexusEditor : public juce::AudioProcessorEditor, public juce::DragAndDropContainer, private juce::Timer, private juce::ChangeListener, public juce::FileDragAndDropTarget
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
  void changeListenerCallback(juce::ChangeBroadcaster*) override { catalogChanged(); }

  void paint(juce::Graphics&) override;
  void paintOverChildren(juce::Graphics&) override;
  void resized() override;
  void select(Selection s);
  // Pedal isolado na pedaleira, grande (-1 = a pedaleira toda).
  void setPedalFocus(int pedal);
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
    void mouseDown(const juce::MouseEvent&) override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    std::function<void(const juce::String&, juce::Component&)> onMidiMenu;
    // Clique no corpo do pedal (quantos cliques).
    std::function<void(int)> onBodyClick;
    // Relógio da tela: atualiza o BPM mostrado no TAP.
    void tick() { refreshTempo(); }
    // Pedaleira: o pedal escolhido é desenhado neste retângulo (a imagem
    // inteira, com a folga) e só ele recebe o mouse; o resto da área deixa
    // passar o clique para a pedaleira por baixo. Vazio = ocupa tudo.
    void setArea(juce::Rectangle<float> area);
    bool hitTest(int x, int y) override;
    // Ganho do cabeçote (canal aberto) e drive do pedal de captura: uma
    // parada por captura; com uma captura só, fica morto.
    void refreshCaptureSteps();

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
    juce::Rectangle<float> mArea;
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

  // Pedaleira (vista dos pedais): todos os pedais da cadeia, na ordem do
  // som, montados na pedaleira 3D e ligados por cabos. O LED mostra quem
  // está ligado; clicar escolhe o pedal e pisar no pisante liga e desliga.
  // O pedal escolhido não é desenhado aqui: o GearStage fica por cima, no
  // lugar dele, com os botões de verdade.
  class BoardView : public juce::Component
  {
  public:
    explicit BoardView(NexusEditor& editor);
    void paint(juce::Graphics&) override;
    void resized() override;
    void mouseUp(const juce::MouseEvent&) override;
    void mouseMove(const juce::MouseEvent&) override;
    bool hitTest(int x, int y) override;
    // Onde fica a imagem do pedal (vazio se não está na cadeia).
    juce::Rectangle<float> imageOf(int pedal) const;
    // Relógio da tela: redesenha só se algo mudou (pedais, LEDs, botões).
    void tick();
    // Pedal isolado (dois cliques): só ele, grande; -1 = a pedaleira toda.
    // O pedal cresce num zoom curto até ocupar o palco, por cima do título.
    void setFocus(int pedal);
    int focus() const { return mFocus; }

  private:
    juce::Rectangle<float> backArea() const;
    // Onde a pedaleira pode aparecer (sem a faixa de cima nem a do título).
    juce::Rectangle<float> stageArea() const;
    // Aproxima a pedaleira até os pedais ocuparem o palco.
    void updateCamera();
    // Quanto do zoom já aconteceu (0 = na pedaleira, 1 = isolado), suavizado.
    float zoom() const;
    bool zooming() const;
    int mFocus = -1;
    int mZoomPedal = -1; // o pedal do zoom (fica até o zoom de volta terminar)
    float mZoomFrom = 0.0f;
    double mZoomStart = 0.0;
    struct Slot
    {
      int pedal;
      juce::Rectangle<float> image; // imagem inteira do pedal, na tela
      juce::Rectangle<float> body;  // o pedal em si (sem a folga)
      float scale;
    };
    std::vector<Slot> layoutSlots() const;
    // O pedal do zoom: entre o lugar dele na pedaleira e o palco inteiro.
    std::optional<Slot> zoomedSlot() const;
    const Slot* slotAt(juce::Point<float> p) const;
    bool overFootswitch(const Slot& s, juce::Point<float> p) const;
    juce::String signature() const;

    NexusEditor& mEditor;
    nexus::ui::GearArt mBoard;
    juce::Rectangle<float> mBoardImage;
    juce::Rectangle<float> mBoardFit; // a pedaleira inteira cabendo no palco, antes da aproximação
    std::vector<Slot> mSlots;
    std::optional<Slot> mZoomSlot;
    juce::String mSignature;
  };

  // Caixa grande, com o seletor Desligada / Captura / IR próprio. Com IR, os
  // microfones que o nome do arquivo diz aparecem na frente do falante, na
  // posição em que o IR foi gravado.
  class CabStage : public juce::Component
  {
  public:
    CabStage(NexusProcessor& p, const std::deque<nexus::ui::GearArt>& arts, const nexus::ui::GearArt* micArts);
    void setRig(int rig) { mRig = rig; refresh(); }
    void paint(juce::Graphics&) override;
    void resized() override;
    void refresh();

  private:
    NexusProcessor& mProcessor;
    const std::deque<nexus::ui::GearArt>& mArts; // uma por caixa do catálogo
    const nexus::ui::GearArt* mMicArts; // uma por tipo de microfone (nexus::ui::kMicKinds)
    void paintMics(juce::Graphics& g);
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
  // Hora em que a pedaleira escolheu um pedal (o clique duplo começa nela e
  // termina no pedal).
  juce::uint32 mPedalChosenAt = 0;
  bool keyPressed(const juce::KeyPress&) override;
  // Coluna de equipamentos: lista, aba e peça marcada.
  void refreshBrowser();
  // O catálogo mudou (equipamento instalado): arte e lista novas.
  void catalogChanged();
  // Peça pedida pelo preset e não instalada na posição escolhida (vazio: nenhuma),
  // o nome dela para o cartão e o texto do link (baixar, se der).
  juce::String missingHere() const;
  juce::String missingName(const juce::String& id) const;
  juce::String missingLink(const juce::String& id) const;
  // Pacote de equipamento (.equipamento) arrastado para a janela: instala.
  bool isInterestedInFileDrag(const juce::StringArray& files) override;
  void filesDropped(const juce::StringArray& files, int, int) override;
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
  // Arte de cada peça, na posição dela no catálogo (gear/GearCatalog). Fila
  // de duas pontas: equipamento novo entra no fim sem mudar o endereço das
  // outras (a lista de equipamentos guarda ponteiros).
  std::deque<nexus::ui::GearArt> mAmpArts;
  std::deque<nexus::ui::GearArt> mCabArts;
  std::array<nexus::ui::GearArt, nexus::ui::kMicKinds> mMicArts;
  juce::SharedResourcePointer<nexus::t3k::Connection> mT3k;
  std::set<juce::String> mFetching; // tons baixando (número + pacote)
  std::set<int> mFetchingSlots;     // espaços esperando a captura padrão
  const nexus::ui::GearArt& cabArt(int rig) const;
  const nexus::ui::GearArt& ampArt(int rig) const { return mAmpArts[static_cast<size_t>(juce::jlimit(0, static_cast<int>(mAmpArts.size()) - 1, mProcessor.ampLook(rig)))]; }
  int mShownAmpLook = -1;
  std::deque<nexus::ui::GearArt> mPedalArts;
  Selection mSel;
  int mShownPedalLook = -1;
  int mShownChannel = -1;
  // Cadeia que a tela está mostrando: mudou por fora (preset, automação),
  // a cadeia de baixo e a pedaleira se refazem juntas.
  juce::String mShownLayout;

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
  std::unique_ptr<BoardView> mBoard;
  std::unique_ptr<GearStage> mPedalStage, mAmpStage;
  std::unique_ptr<CabStage> mCabStage;
  std::unique_ptr<CaptureRail> mRail;
  std::unique_ptr<ChainView> mChain;
  nexus::ui::Segmented mChannels{{}};
  nexus::ui::TextLink mChangeLink{"trocar pacote"};
  std::unique_ptr<nexus::ui::GearBrowser> mBrowser;
  std::unique_ptr<nexus::ui::PresetBar> mPresets;
  nexus::ui::UpdateBadge mUpdate;
  nexus::ui::ArteBadge mArteNova;
  juce::SharedResourcePointer<nexus::update::GearStore> mStore; // o mesmo do aviso
  std::unique_ptr<juce::FileChooser> mChooser;
  juce::String mTitle, mSubtitle;
  juce::Rectangle<int> mTitleArea;
  float mInPeak = 0.0f, mOutPeak = 0.0f;
  juce::String mMidiNotice;
  juce::uint32 mMidiNoticeUntil = 0;

  JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(NexusEditor)
};
