#include "nexus/Pack.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <regex>
#include <stdexcept>

#include "NAM/get_dsp.h"
#include "NAM/slimmable.h"
#include "json.hpp"
#include "nexus/TestSignal.h"

namespace nexus
{

namespace fs = std::filesystem;

bool positionFromFileName(const std::string& stem, double& position)
{
  std::string lower(stem);
  std::transform(lower.begin(), lower.end(), lower.begin(), [](unsigned char c) { return std::tolower(c); });

  static const std::regex keyword(R"((?:gain|ganho|drive|gn|g)[\s_\-=:]*([0-9]+(?:[.,][0-9]+)?))");
  static const std::regex number(R"(([0-9]+(?:[.,][0-9]+)?))");

  std::smatch match;
  std::string found;
  if (std::regex_search(lower, match, keyword))
    found = match[1];
  else
  {
    for (auto it = std::sregex_iterator(lower.begin(), lower.end(), number); it != std::sregex_iterator(); ++it)
      found = (*it)[1];
  }
  if (found.empty())
    return false;
  std::replace(found.begin(), found.end(), ',', '.');
  position = std::stod(found);
  return true;
}

namespace
{

struct Entry
{
  double position;
  fs::path file;
};

std::vector<Entry> entriesFromManifest(const fs::path& folder, const nlohmann::json& j)
{
  std::vector<Entry> entries;
  if (!j.contains("capturas") || !j["capturas"].is_object())
    throw std::runtime_error("pack.json precisa da lista \"capturas\" (posição -> arquivo).");
  for (auto& [key, value] : j["capturas"].items())
  {
    std::string k = key;
    std::replace(k.begin(), k.end(), ',', '.');
    entries.push_back({std::stod(k), folder / value.get<std::string>()});
  }
  return entries;
}

std::vector<Entry> entriesFromFileNames(const fs::path& folder)
{
  std::vector<Entry> entries;
  for (const auto& item : fs::directory_iterator(folder))
  {
    if (!item.is_regular_file())
      continue;
    std::string ext = item.path().extension().string();
    std::transform(ext.begin(), ext.end(), ext.begin(), [](unsigned char c) { return std::tolower(c); });
    if (ext != ".nam")
      continue;
    double position = 0.0;
    if (!positionFromFileName(item.path().stem().string(), position))
      throw std::runtime_error("Não achei o número da posição no nome \"" + item.path().filename().string()
                               + "\". Renomeie (ex.: \"Ganho 7.nam\") ou crie um pack.json.");
    entries.push_back({position, item.path()});
  }
  return entries;
}

void measureLevel(Capture& c, double sampleRate, int maxBlock)
{
  // Volume medido com o mesmo sinal para todas as capturas: comparável entre
  // pacotes e criadores, ao contrário do volume gravado no arquivo.
  static thread_local std::vector<double> signal;
  if (signal.empty())
    signal = makeGuitarLikeSignal(sampleRate, 2.0, 7);
  std::vector<double> out(signal.size());
  std::vector<double> in(signal);
  for (size_t pos = 0; pos < signal.size(); pos += maxBlock)
  {
    const int n = static_cast<int>(std::min<size_t>(maxBlock, signal.size() - pos));
    double* inPtr = in.data() + pos;
    double* outPtr = out.data() + pos;
    c.dsp->process(&inPtr, &outPtr, n);
  }
  const int skip = static_cast<int>(0.5 * sampleRate);
  c.measuredRmsDb = rmsDb(out.data() + skip, static_cast<int>(out.size()) - skip);
}

} // namespace

std::unique_ptr<Pack> loadPack(const fs::path& folder, const PackOptions& options)
{
  if (!fs::is_directory(folder))
    throw std::runtime_error("A pasta do pacote não existe: " + folder.string());

  auto pack = std::make_unique<Pack>();
  pack->name = folder.filename().string();

  std::vector<Entry> entries;
  const fs::path manifest = folder / "pack.json";
  if (fs::exists(manifest))
  {
    std::ifstream f(manifest);
    nlohmann::json j = nlohmann::json::parse(f);
    pack->name = j.value("nome", pack->name);
    pack->knobName = j.value("botao", pack->knobName);
    pack->toneCircuit = j.value("circuito", pack->toneCircuit);
    if (j.contains("equalizador_da_captura"))
    {
      const auto& eq = j["equalizador_da_captura"];
      pack->captureTone.bass = eq.value("grave", 5.0);
      pack->captureTone.mid = eq.value("medio", 5.0);
      pack->captureTone.treble = eq.value("agudo", 5.0);
      pack->capturePresence = eq.value("presenca", 5.0);
    }
    entries = entriesFromManifest(folder, j);
  }
  else
    entries = entriesFromFileNames(folder);

  if (entries.empty())
    throw std::runtime_error("Nenhuma captura .nam encontrada em " + folder.string());

  std::sort(entries.begin(), entries.end(), [](const Entry& a, const Entry& b) { return a.position < b.position; });
  for (size_t i = 1; i < entries.size(); ++i)
    if (entries[i].position == entries[i - 1].position)
      throw std::runtime_error("Duas capturas na mesma posição (" + std::to_string(entries[i].position) + "): "
                               + entries[i - 1].file.filename().string() + " e "
                               + entries[i].file.filename().string());

  for (const auto& e : entries)
  {
    Capture c;
    c.position = e.position;
    c.file = e.file;
    c.name = e.file.stem().string();
    try
    {
      c.dsp = nam::get_dsp(e.file);
    }
    catch (const std::exception& ex)
    {
      throw std::runtime_error("Não consegui abrir " + e.file.filename().string() + ": " + ex.what());
    }
    const double expected = c.dsp->GetExpectedSampleRate();
    if (expected > 0.0 && std::abs(expected - options.sampleRate) > 0.5 && !c.dsp->SupportsArbitrarySampleRate())
      throw std::runtime_error(e.file.filename().string() + " foi treinada a " + std::to_string((int)expected)
                               + " Hz e o motor está a " + std::to_string((int)options.sampleRate) + " Hz.");
    c.dsp->Reset(options.sampleRate, options.maxBlockSize);
    if (auto* slim = dynamic_cast<nam::SlimmableModel*>(c.dsp.get()))
      slim->SetSlimmableSize(options.slimmableSize);
    c.warmupSamples = std::max(c.dsp->GetPrewarmSamples(), static_cast<int>(0.02 * options.sampleRate));
    measureLevel(c, options.sampleRate, options.maxBlockSize);
    pack->captures.push_back(std::move(c));
  }

  double meanDb = 0.0;
  for (const auto& c : pack->captures)
    meanDb += c.measuredRmsDb;
  meanDb /= pack->captures.size();
  for (auto& c : pack->captures)
  {
    const double refDb = options.levelMode == LevelMode::EqualizeEach ? c.measuredRmsDb : meanDb;
    c.levelGain = std::pow(10.0, (options.targetRmsDb - refDb) / 20.0);
  }
  return pack;
}

} // namespace nexus
