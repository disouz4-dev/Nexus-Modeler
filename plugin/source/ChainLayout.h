#pragma once

#include <juce_data_structures/juce_data_structures.h>

#include <algorithm>
#include <array>
#include <vector>

// Ordem da cadeia de sinal. A tela monta (clique e arrasta) e entrega ao
// áudio por troca de ponteiro; o áudio só lê.
//
//   entrada → pedais "antes" → [pedal A/B] → lado A ou B:
//             cabeçote → pedais "entre cabeçote e caixa" → caixa
//           → pedais "depois" → saída
//
// Cada número é a posição do pedal (0 a 11). Um pedal aparece em no máximo
// uma lista; o que não está em nenhuma não existe na cadeia.
struct ChainLayout
{
  static constexpr int kPedals = 12;
  static constexpr int kRigs = 2;

  std::vector<int> pre;
  std::array<std::vector<int>, kRigs> loop;
  std::vector<int> post;
  bool dual = false; // dois cabeçotes com pedal A/B

  bool uses(int pedal) const
  {
    auto in = [pedal](const std::vector<int>& v) { return std::find(v.begin(), v.end(), pedal) != v.end(); };
    return in(pre) || in(loop[0]) || in(loop[1]) || in(post);
  }

  int count() const { return static_cast<int>(pre.size() + loop[0].size() + loop[1].size() + post.size()); }

  // Primeira posição livre (-1 se as 12 estão em uso).
  int freePedal() const
  {
    for (int i = 0; i < kPedals; ++i)
      if (!uses(i))
        return i;
    return -1;
  }

  void remove(int pedal)
  {
    for (auto* v : {&pre, &loop[0], &loop[1], &post})
      v->erase(std::remove(v->begin(), v->end(), pedal), v->end());
  }

  juce::ValueTree toValueTree() const
  {
    juce::ValueTree t("cadeia");
    auto join = [](const std::vector<int>& v) {
      juce::StringArray s;
      for (int i : v)
        s.add(juce::String(i));
      return s.joinIntoString(",");
    };
    t.setProperty("antes", join(pre), nullptr);
    t.setProperty("entre_a", join(loop[0]), nullptr);
    t.setProperty("entre_b", join(loop[1]), nullptr);
    t.setProperty("depois", join(post), nullptr);
    t.setProperty("dois_cabecotes", dual, nullptr);

    return t;
  }

  static ChainLayout fromValueTree(const juce::ValueTree& t)
  {
    ChainLayout c;
    if (!t.isValid())
    {
      c.pre = {0}; // padrão: um pedal antes do cabeçote
      return c;
    }
    auto split = [](const juce::var& v) {
      std::vector<int> out;
      for (const auto& s : juce::StringArray::fromTokens(v.toString(), ",", ""))
        if (s.isNotEmpty())
          out.push_back(juce::jlimit(0, kPedals - 1, s.getIntValue()));
      return out;
    };
    c.pre = split(t["antes"]);
    c.loop[0] = split(t["entre_a"]);
    c.loop[1] = split(t["entre_b"]);
    c.post = split(t["depois"]);
    c.dual = static_cast<bool>(t["dois_cabecotes"]);

    return c;
  }
};
