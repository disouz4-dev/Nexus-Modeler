#pragma once

#include <vector>

namespace nexus
{
// Linha de atraso com leitura fracionária (interpolação linear).
class DelayLine
{
public:
  void prepare(int maxSamples);
  void clear();
  void push(double x)
  {
    mWrite = (mWrite + 1) % static_cast<int>(mBuffer.size());
    mBuffer[static_cast<size_t>(mWrite)] = x;
  }
  // "delay" amostras atrás da última escrita (0 = a última).
  double read(double delay) const;

private:
  std::vector<double> mBuffer;
  int mWrite = 0;
};
} // namespace nexus
