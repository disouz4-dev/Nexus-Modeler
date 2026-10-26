#pragma once

#include <vector>

namespace nexus
{

// Sinal de guitarra sintético e sempre igual (cordas dedilhadas por
// Karplus-Strong, nível de captador direto, pico perto de -6 dBFS).
// Serve para medir o volume das capturas no carregamento e para os testes.
std::vector<double> makeGuitarLikeSignal(double sampleRate, double seconds, unsigned seed = 1);

double rmsDb(const double* x, int n);

} // namespace nexus
