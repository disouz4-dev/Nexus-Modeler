#pragma once

#include <juce_audio_formats/juce_audio_formats.h>
#include <juce_events/juce_events.h>

#include <array>
#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <vector>

#include "nexus/Separator.h"

// Estúdio: biblioteca de músicas separadas e playlists, guardadas em
// Documentos/Nexus Modeler/Estúdio.
//
//   Estúdio/<música>/musica.json     título, duração, faixas e a mistura
//   Estúdio/<música>/<faixa>.flac    bateria, baixo, resto, voz, guitarra, piano
//   Estúdio/playlists.json           listas de músicas (pelo nome da pasta)
//
// A separação roda numa linha de execução própria (Separation); a mesma
// música (mesmo arquivo) não é separada duas vezes.
namespace nexus::studio
{
constexpr int kStems = numStems;

// Nome da faixa na tela (português; a tela traduz) e no disco.
juce::String stemName(int stem);
juce::String stemFile(int stem);
// Ordem na tela: voz, guitarra, baixo, bateria, piano, resto.
const std::array<int, kStems>& displayOrder();

struct Mix
{
  std::array<float, kStems> volume{1, 1, 1, 1, 1, 1};
  std::array<bool, kStems> mute{};
  std::array<bool, kStems> solo{};
  double speed = 1.0; // 0,5 a 1,0
  int semitones = 0;  // -6 a +6
  double loopStart = 0.0, loopEnd = 0.0;
  bool loop = false;
};

struct Song
{
  juce::File folder;
  juce::String title;
  double seconds = 0.0;
  Mix mix;
  std::vector<float> shape; // volume (0 a 1) em pedacinhos iguais, para o desenho
  juce::String id() const { return folder.getFileName(); }
  juce::File stem(int s) const { return folder.getChildFile(stemFile(s)); }
};

struct Playlist
{
  juce::String name;
  juce::StringArray songs; // pastas (Song::id)
};

class Library
{
public:
  static juce::File root();
  // Formatos que a importação aceita (para a janela de abrir).
  static juce::String importPattern();

  void rescan();
  const std::vector<Song>& songs() const { return mSongs; }
  const Song* find(const juce::String& id) const;
  void saveMix(const juce::String& id, const Mix& mix);
  void remove(const juce::String& id);

  const std::vector<Playlist>& playlists() const { return mPlaylists; }
  int addPlaylist(const juce::String& name);
  void renamePlaylist(int index, const juce::String& name);
  void removePlaylist(int index);
  void addToPlaylist(int index, const juce::String& songId);
  void removeFromPlaylist(int index, int position);
  void moveInPlaylist(int index, int from, int to);

private:
  void loadPlaylists();
  void savePlaylists() const;
  std::vector<Song> mSongs;
  std::vector<Playlist> mPlaylists;
};

// Separa uma música em segundo plano: baixa o modelo na primeira vez (55 MB),
// lê o arquivo, separa e grava as seis faixas em FLAC.
class Separation : private juce::Thread
{
public:
  enum class Stage
  {
    idle,
    downloading, // modelo
    reading,
    separating,
    saving,
    done,
    failed,
  };

  Separation();
  ~Separation() override;

  // Começa (se nada estiver rodando). "done" é chamado na linha da tela.
  bool start(const juce::File& audio, std::function<void(const juce::String& songId, const juce::String& error)> done);
  void cancel();
  bool busy() const { return isThreadRunning(); }

  Stage stage() const { return mStage.load(); }
  // Andamento da etapa atual (0 a 1).
  float progress() const { return mProgress.load(); }
  // Andamento do trabalho inteiro (ler, separar e gravar), para a barra.
  float overall() const;
  // Estimativa do que falta, em segundos (-1 enquanto não dá para estimar).
  double remainingSeconds() const;
  juce::String title() const;

  static juce::File modelFile();

private:
  void run() override;
  bool ensureModel(juce::String& error);
  bool readAudio(Stereo& mix, juce::String& error);
  void finish(const juce::String& songId, const juce::String& error);

  juce::File mInput;
  std::function<void(const juce::String&, const juce::String&)> mDone;
  std::atomic<Stage> mStage{Stage::idle};
  std::atomic<float> mProgress{0.0f};
  std::atomic<double> mStageStart{0.0}; // hora em que a etapa começou (ms)
  std::atomic<bool> mCancel{false};
  mutable std::mutex mLock;
  juce::String mTitle;
  std::unique_ptr<Separator> mSeparator;
};
} // namespace nexus::studio
