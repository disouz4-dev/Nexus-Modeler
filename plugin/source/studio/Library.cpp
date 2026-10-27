#include "Library.h"

#include "../report/Report.h"

namespace nexus::studio
{
namespace
{
const char* const kModelUrl = "https://huggingface.co/datasets/Retrobear/demucs.cpp/resolve/main/ggml-model-htdemucs-6s-f16.bin";
constexpr juce::int64 kModelBytes = 54855129;

juce::String safeName(juce::String s)
{
  s = s.removeCharacters("\\/:*?\"<>|").trim();
  return s.isEmpty() ? juce::String("Música") : s.substring(0, 80);
}

// Assinatura do arquivo de origem: tamanho + começo e fim do conteúdo.
juce::String signature(const juce::File& f)
{
  juce::FileInputStream in(f);
  if (!in.openedOk())
    return {};
  juce::MemoryBlock head;
  in.readIntoMemoryBlock(head, 1 << 20);
  if (in.getTotalLength() > (2 << 20))
  {
    in.setPosition(in.getTotalLength() - (1 << 20));
    in.readIntoMemoryBlock(head, 1 << 20);
  }
  return juce::String(f.getSize()) + "-" + juce::String::toHexString(static_cast<juce::int64>(head.toString().hashCode64()));
}

juce::var mixToVar(const Mix& m)
{
  auto* o = new juce::DynamicObject();
  juce::Array<juce::var> vol, mute, solo;
  for (int s = 0; s < kStems; ++s)
  {
    vol.add(m.volume[static_cast<size_t>(s)]);
    mute.add(m.mute[static_cast<size_t>(s)]);
    solo.add(m.solo[static_cast<size_t>(s)]);
  }
  o->setProperty("volume", vol);
  o->setProperty("mudo", mute);
  o->setProperty("solo", solo);
  o->setProperty("velocidade", m.speed);
  o->setProperty("tom", m.semitones);
  o->setProperty("trecho_inicio", m.loopStart);
  o->setProperty("trecho_fim", m.loopEnd);
  o->setProperty("trecho_ligado", m.loop);
  return o;
}

Mix mixFromVar(const juce::var& v)
{
  Mix m;
  for (int s = 0; s < kStems; ++s)
  {
    if (auto* a = v["volume"].getArray(); a != nullptr && s < a->size())
      m.volume[static_cast<size_t>(s)] = juce::jlimit(0.0f, 2.0f, static_cast<float>((*a)[s]));
    if (auto* a = v["mudo"].getArray(); a != nullptr && s < a->size())
      m.mute[static_cast<size_t>(s)] = (*a)[s];
    if (auto* a = v["solo"].getArray(); a != nullptr && s < a->size())
      m.solo[static_cast<size_t>(s)] = (*a)[s];
  }
  if (v.hasProperty("velocidade"))
    m.speed = juce::jlimit(0.5, 1.0, static_cast<double>(v["velocidade"]));
  m.semitones = juce::jlimit(-6, 6, static_cast<int>(v["tom"]));
  m.loopStart = v["trecho_inicio"];
  m.loopEnd = v["trecho_fim"];
  m.loop = v["trecho_ligado"];
  return m;
}
} // namespace

juce::String stemName(int stem)
{
  static const char* names[] = {"Bateria", "Baixo", "Resto", "Voz", "Guitarra", "Piano"};
  return names[juce::jlimit(0, kStems - 1, stem)];
}

juce::String stemFile(int stem)
{
  static const char* files[] = {"bateria.flac", "baixo.flac", "resto.flac", "voz.flac", "guitarra.flac", "piano.flac"};
  return files[juce::jlimit(0, kStems - 1, stem)];
}

const std::array<int, kStems>& displayOrder()
{
  static const std::array<int, kStems> order{vocals, guitar, bass, drums, piano, other};
  return order;
}

// ---------------------------------------------------------------------------

juce::File Library::root()
{
  return juce::File::getSpecialLocation(juce::File::userDocumentsDirectory)
    .getChildFile("Nexus Modeler")
    .getChildFile(juce::String::fromUTF8("Estúdio"));
}

juce::String Library::importPattern()
{
  return "*.mp3;*.wav;*.flac;*.m4a;*.aac;*.ogg;*.aif;*.aiff";
}

void Library::rescan()
{
  mSongs.clear();
  for (const auto& dir : root().findChildFiles(juce::File::findDirectories, false))
  {
    const auto info = juce::JSON::parse(dir.getChildFile("musica.json"));
    if (!info.isObject())
      continue;
    Song s;
    s.folder = dir;
    s.title = info["titulo"].toString();
    s.seconds = info["segundos"];
    s.mix = mixFromVar(info["mix"]);
    if (auto* shape = info["forma"].getArray())
      for (const auto& v : *shape)
        s.shape.push_back(static_cast<float>(static_cast<int>(v)) / 255.0f);
    bool complete = true;
    for (int i = 0; i < kStems; ++i)
      complete = complete && s.stem(i).existsAsFile();
    if (complete)
      mSongs.push_back(s);
  }
  std::sort(mSongs.begin(), mSongs.end(), [](const Song& a, const Song& b) { return a.title.compareNatural(b.title) < 0; });
  loadPlaylists();
}

const Song* Library::find(const juce::String& id) const
{
  for (const auto& s : mSongs)
    if (s.id() == id)
      return &s;
  return nullptr;
}

void Library::saveMix(const juce::String& id, const Mix& mix)
{
  for (auto& s : mSongs)
    if (s.id() == id)
    {
      s.mix = mix;
      const auto file = s.folder.getChildFile("musica.json");
      auto info = juce::JSON::parse(file);
      if (auto* o = info.getDynamicObject())
      {
        o->setProperty("mix", mixToVar(mix));
        file.replaceWithText(juce::JSON::toString(info));
      }
    }
}

void Library::remove(const juce::String& id)
{
  for (auto& p : mPlaylists)
    p.songs.removeString(id);
  savePlaylists();
  if (const auto* s = find(id))
    s->folder.moveToTrash();
  rescan();
}

void Library::loadPlaylists()
{
  mPlaylists.clear();
  const auto data = juce::JSON::parse(root().getChildFile("playlists.json"));
  if (auto* list = data["playlists"].getArray())
    for (const auto& p : *list)
    {
      Playlist pl;
      pl.name = p["nome"].toString();
      if (auto* songs = p["musicas"].getArray())
        for (const auto& s : *songs)
          if (find(s.toString()) != nullptr)
            pl.songs.add(s.toString());
      mPlaylists.push_back(pl);
    }
}

void Library::savePlaylists() const
{
  juce::Array<juce::var> list;
  for (const auto& p : mPlaylists)
  {
    auto* o = new juce::DynamicObject();
    o->setProperty("nome", p.name);
    juce::Array<juce::var> songs;
    for (const auto& s : p.songs)
      songs.add(s);
    o->setProperty("musicas", songs);
    list.add(o);
  }
  auto* root = new juce::DynamicObject();
  root->setProperty("playlists", list);
  Library::root().createDirectory();
  Library::root().getChildFile("playlists.json").replaceWithText(juce::JSON::toString(juce::var(root)));
}

int Library::addPlaylist(const juce::String& name)
{
  mPlaylists.push_back({name.trim().isEmpty() ? juce::String("Playlist") : name.trim(), {}});
  savePlaylists();
  return static_cast<int>(mPlaylists.size()) - 1;
}

void Library::renamePlaylist(int index, const juce::String& name)
{
  if (index >= 0 && index < static_cast<int>(mPlaylists.size()) && name.trim().isNotEmpty())
  {
    mPlaylists[static_cast<size_t>(index)].name = name.trim();
    savePlaylists();
  }
}

void Library::removePlaylist(int index)
{
  if (index >= 0 && index < static_cast<int>(mPlaylists.size()))
  {
    mPlaylists.erase(mPlaylists.begin() + index);
    savePlaylists();
  }
}

void Library::addToPlaylist(int index, const juce::String& songId)
{
  if (index >= 0 && index < static_cast<int>(mPlaylists.size()) && find(songId) != nullptr)
  {
    mPlaylists[static_cast<size_t>(index)].songs.add(songId);
    savePlaylists();
  }
}

void Library::removeFromPlaylist(int index, int position)
{
  if (index >= 0 && index < static_cast<int>(mPlaylists.size()))
  {
    mPlaylists[static_cast<size_t>(index)].songs.remove(position);
    savePlaylists();
  }
}

void Library::moveInPlaylist(int index, int from, int to)
{
  if (index >= 0 && index < static_cast<int>(mPlaylists.size()))
  {
    auto& songs = mPlaylists[static_cast<size_t>(index)].songs;
    if (from >= 0 && from < songs.size() && to >= 0 && to < songs.size())
    {
      songs.move(from, to);
      savePlaylists();
    }
  }
}

// ---------------------------------------------------------------------------

Separation::Separation() : juce::Thread("Nexus: separar faixas") {}

Separation::~Separation()
{
  mCancel = true;
  stopThread(-1);
}

juce::File Separation::modelFile()
{
  return nexus::report::folder().getChildFile("modelos").getChildFile("ggml-model-htdemucs-6s-f16.bin");
}

bool Separation::start(const juce::File& audio, std::function<void(const juce::String&, const juce::String&)> done)
{
  if (isThreadRunning())
    return false;
  mInput = audio;
  mDone = std::move(done);
  mCancel = false;
  mProgress = 0.0f;
  {
    const std::scoped_lock l(mLock);
    mTitle = audio.getFileNameWithoutExtension();
  }
  mStage = Stage::reading;
  startThread(juce::Thread::Priority::low);
  return true;
}

void Separation::cancel()
{
  mCancel = true;
}

float Separation::overall() const
{
  // Pesos de cada etapa no tempo total (separar é quase tudo).
  const float p = juce::jlimit(0.0f, 1.0f, mProgress.load());
  switch (mStage.load())
  {
    case Stage::reading:
      return 0.04f * p;
    case Stage::separating:
      return 0.04f + 0.93f * p;
    case Stage::saving:
      return 0.97f + 0.03f * p;
    case Stage::done:
      return 1.0f;
    case Stage::downloading:
    case Stage::idle:
    case Stage::failed:
      break;
  }
  return p;
}

double Separation::remainingSeconds() const
{
  if (mStage.load() != Stage::separating)
    return -1.0;
  const double p = mProgress.load();
  const double elapsed = (juce::Time::getMillisecondCounterHiRes() - mStageStart.load()) / 1000.0;
  if (p < 0.03 || elapsed < 5.0)
    return -1.0;
  return elapsed * (1.0 - p) / p + 5.0; // + gravar
}

juce::String Separation::title() const
{
  const std::scoped_lock l(mLock);
  return mTitle;
}

void Separation::finish(const juce::String& songId, const juce::String& error)
{
  mStage = error.isEmpty() ? Stage::done : Stage::failed;
  if (error.isNotEmpty() && error != "cancelado")
    nexus::report::log("estudio", mInput.getFileName() + ": " + error);
  auto done = mDone;
  juce::MessageManager::callAsync([done, songId, error] {
    if (done)
      done(songId, error);
  });
}

bool Separation::ensureModel(juce::String& error)
{
  auto file = modelFile();
  if (file.getSize() == kModelBytes)
    return true;
  mStage = Stage::downloading;
  mProgress = 0.0f;
  file.getParentDirectory().createDirectory();
  const auto part = file.withFileExtension(".parcial");
  int status = 0;
  auto stream = juce::URL(kModelUrl).createInputStream(
    juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress).withConnectionTimeoutMs(20000).withStatusCode(&status).withNumRedirectsToFollow(8));
  if (stream == nullptr || status / 100 != 2)
  {
    error = TRANS(juce::String::fromUTF8("Não foi possível baixar o separador de faixas (sem internet?)."));
    return false;
  }
  {
    part.deleteFile();
    juce::FileOutputStream out(part);
    if (!out.openedOk())
    {
      error = TRANS(juce::String::fromUTF8("Não foi possível gravar o separador de faixas."));
      return false;
    }
    juce::HeapBlock<char> buffer(1 << 16);
    juce::int64 got = 0;
    while (!stream->isExhausted())
    {
      if (mCancel.load() || threadShouldExit())
      {
        error = "cancelado";
        break;
      }
      const int n = stream->read(buffer.getData(), 1 << 16);
      if (n <= 0)
        break;
      out.write(buffer.getData(), static_cast<size_t>(n));
      got += n;
      mProgress = static_cast<float>(static_cast<double>(got) / static_cast<double>(kModelBytes));
    }
  }
  if (error.isNotEmpty())
  {
    part.deleteFile();
    return false;
  }
  if (part.getSize() != kModelBytes)
  {
    part.deleteFile();
    error = TRANS(juce::String::fromUTF8("O download do separador de faixas veio incompleto. Tente de novo."));
    return false;
  }
  file.deleteFile();
  return part.moveFileTo(file);
}

bool Separation::readAudio(Stereo& mix, juce::String& error)
{
  mStage = Stage::reading;
  juce::AudioFormatManager formats;
  formats.registerBasicFormats();
  std::unique_ptr<juce::AudioFormatReader> reader(formats.createReaderFor(mInput));
  if (reader == nullptr)
  {
    error = TRANS(juce::String::fromUTF8("O Nexus não conseguiu ler este arquivo de áudio."));
    return false;
  }
  const auto frames = static_cast<int>(std::min<juce::int64>(reader->lengthInSamples, static_cast<juce::int64>(reader->sampleRate * 60 * 20)));
  if (frames < reader->sampleRate)
  {
    error = TRANS(juce::String::fromUTF8("A música é curta demais."));
    return false;
  }
  juce::AudioBuffer<float> raw(2, frames);
  // Em pedaços de 5 s, para a barra andar (um MP3 longo demora a abrir).
  const int chunk = static_cast<int>(reader->sampleRate * 5);
  for (int at = 0; at < frames; at += chunk)
  {
    if (mCancel.load())
    {
      error = "cancelado";
      return false;
    }
    reader->read(&raw, at, std::min(chunk, frames - at), at, true, true);
    mProgress = 0.8f * static_cast<float>(at) / static_cast<float>(frames);
  }
  if (reader->numChannels == 1)
    raw.copyFrom(1, 0, raw, 0, 0, frames);
  // Para 44,1 kHz (a taxa do modelo).
  const double ratio = reader->sampleRate / kSeparationRate;
  const int outFrames = static_cast<int>(frames / ratio);
  mix.left.assign(static_cast<size_t>(outFrames), 0.0f);
  mix.right.assign(static_cast<size_t>(outFrames), 0.0f);
  if (std::abs(ratio - 1.0) < 1e-6)
  {
    std::copy(raw.getReadPointer(0), raw.getReadPointer(0) + outFrames, mix.left.begin());
    std::copy(raw.getReadPointer(1), raw.getReadPointer(1) + outFrames, mix.right.begin());
  }
  else
  {
    juce::WindowedSincInterpolator a, b;
    int usedA = 0, usedB = 0;
    const int step = kSeparationRate * 5;
    for (int at = 0; at < outFrames; at += step)
    {
      const int n = std::min(step, outFrames - at);
      usedA += a.process(ratio, raw.getReadPointer(0) + usedA, mix.left.data() + at, n, frames - usedA, 0);
      usedB += b.process(ratio, raw.getReadPointer(1) + usedB, mix.right.data() + at, n, frames - usedB, 0);
      mProgress = 0.8f + 0.2f * static_cast<float>(at) / static_cast<float>(outFrames);
    }
  }
  // Título: o das etiquetas do arquivo, se tiver.
  auto tag = [&](const char* k) { return reader->metadataValues.getValue(k, {}).trim(); };
  const auto t = tag("title").isNotEmpty() ? tag("title") : tag("TITLE");
  const auto a = tag("artist").isNotEmpty() ? tag("artist") : tag("ARTIST");
  if (t.isNotEmpty())
  {
    const std::scoped_lock l(mLock);
    mTitle = a.isNotEmpty() ? a + " - " + t : t;
  }
  return true;
}

void Separation::run()
{
  juce::String error;
  const auto sig = signature(mInput);
  // Já separada antes: só aponta para ela.
  {
    Library lib;
    lib.rescan();
    for (const auto& s : lib.songs())
    {
      const auto info = juce::JSON::parse(s.folder.getChildFile("musica.json"));
      if (sig.isNotEmpty() && info["assinatura"].toString() == sig)
      {
        mProgress = 1.0f;
        finish(s.id(), {});
        return;
      }
    }
  }
  if (!ensureModel(error))
    return finish({}, error);
  if (mSeparator == nullptr)
  {
    auto sep = std::make_unique<Separator>();
    std::string e;
    if (!sep->load(modelFile().getFullPathName().toStdString(), e))
    {
      modelFile().deleteFile(); // estragado: baixa de novo na próxima
      return finish({}, TRANS(juce::String::fromUTF8("O separador de faixas não abriu. Tente de novo.")));
    }
    mSeparator = std::move(sep);
  }
  Stereo mix;
  if (!readAudio(mix, error))
    return finish({}, error);
  if (mCancel.load())
    return finish({}, "cancelado");

  mProgress = 0.0f;
  mStageStart = juce::Time::getMillisecondCounterHiRes();
  mStage = Stage::separating;
  std::array<Stereo, numStems> stems;
  std::string e;
  if (!mSeparator->separate(mix, Separator::defaultThreads(), [this](float p) { mProgress = p; }, mCancel, stems, e))
    return finish({}, e == "cancelado" ? juce::String("cancelado") : juce::String::fromUTF8(e.c_str()));

  // Grava numa pasta temporária e só então dá o nome final.
  mStage = Stage::saving;
  const auto title = this->title();
  auto folder = Library::root().getNonexistentChildFile(safeName(title), {}, true);
  const auto temp = Library::root().getChildFile("." + folder.getFileName() + ".parcial");
  temp.deleteRecursively();
  temp.createDirectory();
  juce::FlacAudioFormat flac;
  for (int s = 0; s < kStems; ++s)
  {
    auto out = std::make_unique<juce::FileOutputStream>(temp.getChildFile(stemFile(s)));
    std::unique_ptr<juce::AudioFormatWriter> writer(
      flac.createWriterFor(out.get(), kSeparationRate, 2, 24, {}, 0));
    if (writer == nullptr)
      return finish({}, TRANS(juce::String::fromUTF8("Não foi possível gravar as faixas.")));
    out.release();
    const float* ch[2] = {stems[static_cast<size_t>(s)].left.data(), stems[static_cast<size_t>(s)].right.data()};
    writer->writeFromFloatArrays(ch, 2, static_cast<int>(stems[static_cast<size_t>(s)].frames()));
    mProgress = static_cast<float>(s + 1) / kStems;
  }
  auto* info = new juce::DynamicObject();
  info->setProperty("titulo", title);
  info->setProperty("origem", mInput.getFileName());
  info->setProperty("assinatura", sig);
  info->setProperty("segundos", static_cast<double>(mix.frames()) / kSeparationRate);
  info->setProperty("modelo", "htdemucs_6s");
  info->setProperty("mix", mixToVar({}));
  // Forma da música (volume médio em 800 pedacinhos, 0 a 255), para o desenho.
  {
    juce::Array<juce::var> shape;
    const size_t n = mix.frames(), parts = 800;
    std::vector<double> sum(parts, 0.0);
    std::vector<size_t> count(parts, 0);
    for (size_t i = 0; i < n; ++i)
    {
      const size_t k = std::min(parts - 1, i * parts / n);
      sum[k] += 0.5 * (mix.left[i] * mix.left[i] + mix.right[i] * mix.right[i]);
      ++count[k];
    }
    double top = 1e-12;
    for (size_t k = 0; k < parts; ++k)
      top = std::max(top, sum[k] / std::max<size_t>(1, count[k]));
    for (size_t k = 0; k < parts; ++k)
      shape.add(juce::roundToInt(255.0 * std::sqrt(sum[k] / std::max<size_t>(1, count[k]) / top)));
    info->setProperty("forma", shape);
  }
  temp.getChildFile("musica.json").replaceWithText(juce::JSON::toString(juce::var(info)));
  if (!temp.moveFileTo(folder))
    return finish({}, TRANS(juce::String::fromUTF8("Não foi possível gravar as faixas.")));
  finish(folder.getFileName(), {});
}
} // namespace nexus::studio
