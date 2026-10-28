#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

#include "../studio/Library.h"
#include "../studio/Player.h"
#include "Controls.h"

// Tela do estúdio (no lugar dos equipamentos; o caminho do sinal continua
// embaixo, para tocar a guitarra junto com a música).
//
//   Coluna da esquerda: Músicas ou Playlists, importar música e o andamento
//   da separação.
//   Centro: música aberta, tocar/pausar, a forma da música (clique pula,
//   arrastar marca o trecho A–B), as seis faixas (volume, mudo, solo e
//   medidor) e embaixo velocidade, tom e volume da música.
namespace nexus::ui
{
class StudioView : public juce::Component, public juce::FileDragAndDropTarget, private juce::Timer
{
public:
  StudioView(nexus::studio::Player& player, nexus::studio::Separation& separation);
  ~StudioView() override;

  void retranslate();
  void paint(juce::Graphics&) override;
  void resized() override;
  void mouseDown(const juce::MouseEvent&) override;
  void mouseDrag(const juce::MouseEvent&) override;
  void mouseUp(const juce::MouseEvent&) override;
  void mouseMove(const juce::MouseEvent&) override;
  void mouseExit(const juce::MouseEvent&) override;
  void mouseDoubleClick(const juce::MouseEvent&) override;
  void mouseWheelMove(const juce::MouseEvent&, const juce::MouseWheelDetails&) override;
  bool keyPressed(const juce::KeyPress&) override;
  bool isInterestedInFileDrag(const juce::StringArray& files) override;
  void filesDropped(const juce::StringArray& files, int, int) override;

  // Começa a separar (arquivo escolhido ou arrastado).
  void importFile(const juce::File& file);

private:
  // O que está debaixo do mouse.
  struct Hit
  {
    enum Kind
    {
      none,
      tab,
      importButton,
      cancelButton,
      newPlaylist,
      listRow,
      playlistBack,
      playPlaylist,
      play,
      rewind,
      loopButton,
      wave,
      stemVolume,
      stemMute,
      stemSolo,
      speed,
      pitchDown,
      pitchUp,
      volume,
      emptyImport,
    } kind = none;
    int index = -1;
    bool operator==(const Hit& o) const { return kind == o.kind && index == o.index; }
  };

  void timerCallback() override;
  Hit hitAt(juce::Point<float> p) const;
  void chooseFile();
  void openSong(const juce::String& id, bool play);
  void changeMix(const std::function<void(nexus::studio::Mix&)>& change, bool save);
  void showSongMenu(const juce::String& id);
  void showPlaylistMenu(int index);
  void playNextInPlaylist();
  const nexus::studio::Song* current() const;
  // Itens da lista da esquerda conforme a aba.
  juce::StringArray rowIds() const;
  juce::String formatTime(double seconds) const;
  juce::String stageText() const;

  // Pedaços desenhados.
  void paintSidebar(juce::Graphics&);
  void paintEmpty(juce::Graphics&);
  void paintSong(juce::Graphics&, const nexus::studio::Song&);
  void paintPill(juce::Graphics&, juce::Rectangle<float> r, const juce::String& text, bool on, bool hover,
                 juce::Colour onColour) const;
  void paintSlider(juce::Graphics&, juce::Rectangle<float> r, float value01, bool hover, float level = -1.0f) const;

  nexus::studio::Player& mPlayer;
  nexus::studio::Separation& mSeparation;
  nexus::studio::Library mLibrary;
  Segmented mTabs{{"Músicas", "Playlists"}};
  std::unique_ptr<juce::FileChooser> mChooser;

  int mTab = 0;               // 0 = músicas, 1 = playlists
  int mOpenPlaylist = -1;     // playlist aberta na aba Playlists
  int mPlayingPlaylist = -1;  // tocando em sequência
  int mScroll = 0;            // linhas roladas na lista
  juce::String mPendingSong;  // separada há pouco: abre ao terminar
  Hit mHover, mPressed;
  float mDragStart01 = 0.0f;  // forma da música: onde o arraste começou
  float mDragValue = 0.0f;    // valor do controle quando o arraste começou
  bool mDraggedWave = false;
  std::array<float, nexus::studio::kStems> mLevels{};

  // Áreas calculadas em resized().
  juce::Rectangle<float> mSidebar, mMain, mTabsArea, mImportArea, mList, mJobArea, mCancelArea, mNewPlaylistArea,
    mBackArea, mPlayPlaylistArea, mHeader, mPlayArea, mRewindArea, mLoopArea, mTimeArea, mWaveArea, mStemsArea,
    mBottom, mSpeedArea, mPitchDownArea, mPitchUpArea, mPitchArea, mVolumeArea, mEmptyButton;
  std::array<juce::Rectangle<float>, nexus::studio::kStems> mStemRows;
  float mMusicVolume = 0.8f;
  bool mWasBusy = false;
  // Diagnóstico do play (registro de erros).
  double mPlayStartPos = 0.0;
  juce::uint32 mPlayStartMs = 0;

  static constexpr float kRowHeight = 40.0f;
};
} // namespace nexus::ui
