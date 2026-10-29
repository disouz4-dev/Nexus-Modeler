#include "IrMic.h"

#include <algorithm>
#include <cstring>
#include <regex>

namespace nexus::ui
{
namespace
{
struct KnownMic
{
  const char* name; // em maiúsculas, sem traço nem espaço
  MicKind kind;
};
const KnownMic kKnownMics[] = {
  {"SM57", MicKind::dynamicStick},       {"SM58", MicKind::dynamicStick},       {"BETA57", MicKind::dynamicStick},
  {"BETA57A", MicKind::dynamicStick},    {"M201", MicKind::dynamicStick},       {"M88", MicKind::dynamicStick},
  {"I5", MicKind::dynamicStick},         {"E835", MicKind::dynamicStick},
  {"MD421", MicKind::dynamicSide},       {"MD441", MicKind::dynamicSide},       {"MD409", MicKind::dynamicSide},
  {"E906", MicKind::dynamicSide},        {"E609", MicKind::dynamicSide},
  {"U87", MicKind::largeCondenser},      {"U87AI", MicKind::largeCondenser},    {"U47", MicKind::largeCondenser},
  {"U67", MicKind::largeCondenser},      {"TLM103", MicKind::largeCondenser},   {"TLM102", MicKind::largeCondenser},
  {"TLM49", MicKind::largeCondenser},    {"C414", MicKind::largeCondenser},     {"C214", MicKind::largeCondenser},
  {"C12", MicKind::largeCondenser},      {"AT4050", MicKind::largeCondenser},   {"AT4040", MicKind::largeCondenser},
  {"AT2020", MicKind::largeCondenser},   {"NT1", MicKind::largeCondenser},      {"NT1A", MicKind::largeCondenser},
  {"KSM32", MicKind::largeCondenser},    {"KSM44", MicKind::largeCondenser},
  {"KM184", MicKind::pencilCondenser},   {"KM185", MicKind::pencilCondenser},   {"KM84", MicKind::pencilCondenser},
  {"SM81", MicKind::pencilCondenser},    {"C451", MicKind::pencilCondenser},    {"NT5", MicKind::pencilCondenser},
  {"MK012", MicKind::pencilCondenser},   {"KSM137", MicKind::pencilCondenser},  {"AT4041", MicKind::pencilCondenser},
  {"R121", MicKind::ribbon},             {"R10", MicKind::ribbon},              {"R84", MicKind::ribbon},
  {"R92", MicKind::ribbon},              {"R44", MicKind::ribbon},              {"M160", MicKind::ribbon},
  {"M130", MicKind::ribbon},             {"M260", MicKind::ribbon},             {"4038", MicKind::ribbon},
  {"KSM313", MicKind::ribbon},           {"RIBBON", MicKind::ribbon},
};
} // namespace

std::vector<MicShot> micsOfIr(const juce::String& fileName)
{
  // Sem a extensão (só a de áudio: "1.25in 0.0in" também tem ponto).
  auto name = fileName;
  for (const auto* ext : {".wav", ".wave", ".aif", ".aiff", ".flac"})
    if (name.endsWithIgnoreCase(ext))
      name = name.dropLastCharacters(static_cast<int>(std::strlen(ext)));
  // Palavras: tudo o que não é letra ou número separa ("V30-SM57", "Engl_MD421").
  juce::StringArray words;
  juce::String word;
  for (const auto c : name + " ")
  {
    if (juce::CharacterFunctions::isLetterOrDigit(c))
      word += juce::String::charToString(c);
    else if (word.isNotEmpty())
    {
      words.add(word.toUpperCase());
      word.clear();
    }
  }

  // Falante da caixa (cima/baixo, esquerda/direita).
  int speaker = -1;
  const char* const corners[4][2] = {{"UL", "TL"}, {"UR", "TR"}, {"LL", "BL"}, {"LR", "BR"}};
  for (const auto& w : words)
    for (int i = 0; i < 4 && speaker < 0; ++i)
      if (w == corners[i][0] || w == corners[i][1])
        speaker = i;

  // Distância do centro: a primeira medida em polegadas ("1.25in", 2").
  // (A segunda, quando há, é a distância da tela, que de frente não aparece.)
  float inches = -1.0f;
  std::smatch m;
  const auto lower = name.toLowerCase().toStdString();
  if (std::regex_search(lower, m, std::regex(R"((\d+(?:\.\d+)?)\s*(?:in\b|inch|"))")))
    inches = std::stof(m[1].str());
  // Senão, pela palavra: centro da calota, borda da calota, cone, borda do cone.
  bool cap = false, edge = false, centre = false, cone = false;
  for (const auto& w : words)
  {
    const auto l = w.toLowerCase();
    cap = cap || (l.contains("cap") && !l.contains("capt"));
    edge = edge || l.contains("edge");
    centre = centre || l.contains("center") || l.contains("centre");
    cone = cone || l.contains("cone");
  }
  const float fraction = edge && cap ? 0.35f : (cap || centre) ? 0.0f : cone ? 0.6f : edge ? 0.85f : -1.0f;

  // Microfones: cada palavra, e cada par de palavras juntas ("R-121", "KM 184").
  std::vector<MicShot> out;
  auto take = [&](const juce::String& candidate) {
    for (const auto& known : kKnownMics)
      if (candidate == known.name)
      {
        for (const auto& shot : out)
          if (shot.model.toUpperCase().removeCharacters("- ") == candidate)
            return;
        MicShot shot;
        shot.kind = known.kind;
        shot.model = candidate == "RIBBON" ? juce::String() : candidate;
        shot.speaker = speaker;
        shot.inches = inches;
        shot.fraction = fraction;
        out.push_back(shot);
        return;
      }
  };
  for (int i = 0; i < words.size(); ++i)
  {
    take(words[i]);
    if (i + 1 < words.size())
      take(words[i] + words[i + 1]);
  }
  // "Ribbon" sozinho só vale se nenhum modelo de fita apareceu.
  if (out.size() > 1)
    out.erase(std::remove_if(out.begin(), out.end(), [](const MicShot& s) { return s.kind == MicKind::ribbon && s.model.isEmpty(); }), out.end());
  return out;
}

juce::String micKindName(MicKind kind)
{
  switch (kind)
  {
    case MicKind::dynamicStick: return juce::String::fromUTF8("dinâmico de bastão");
    case MicKind::dynamicSide: return juce::String::fromUTF8("dinâmico lateral");
    case MicKind::largeCondenser: return "condensador grande";
    case MicKind::pencilCondenser: return juce::String::fromUTF8("condensador de lápis");
    case MicKind::ribbon: return "fita";
  }
  return {};
}
} // namespace nexus::ui
