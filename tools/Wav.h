#pragma once

// Leitura e escrita mínima de WAV mono (PCM 16/24/32 e ponto flutuante 32),
// só para a ferramenta de medida.

#include <cstdint>
#include <cstring>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace wav
{

inline std::vector<double> read(const std::string& path, double& sampleRate)
{
  std::ifstream f(path, std::ios::binary);
  if (!f)
    throw std::runtime_error("Não abri " + path);
  std::vector<char> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
  auto u16 = [&](size_t p) { uint16_t v; std::memcpy(&v, &data[p], 2); return v; };
  auto u32 = [&](size_t p) { uint32_t v; std::memcpy(&v, &data[p], 4); return v; };
  if (data.size() < 12 || std::memcmp(data.data(), "RIFF", 4) || std::memcmp(&data[8], "WAVE", 4))
    throw std::runtime_error(path + " não é WAV");
  int format = 0, channels = 0, bits = 0;
  size_t pos = 12;
  std::vector<double> out;
  while (pos + 8 <= data.size())
  {
    const uint32_t size = u32(pos + 4);
    if (!std::memcmp(&data[pos], "fmt ", 4))
    {
      format = u16(pos + 8);
      channels = u16(pos + 10);
      sampleRate = u32(pos + 12);
      bits = u16(pos + 22);
    }
    else if (!std::memcmp(&data[pos], "data", 4))
    {
      const int bytes = bits / 8;
      const size_t frames = size / (bytes * channels);
      out.resize(frames);
      for (size_t i = 0; i < frames; ++i)
      {
        const size_t p = pos + 8 + i * bytes * channels;
        double v = 0.0;
        if (format == 3 && bits == 32)
        {
          float fv;
          std::memcpy(&fv, &data[p], 4);
          v = fv;
        }
        else if (bits == 16)
          v = int16_t(u16(p)) / 32768.0;
        else if (bits == 24)
        {
          int32_t s = (uint8_t(data[p]) << 8) | (uint8_t(data[p + 1]) << 16) | (uint8_t(data[p + 2]) << 24);
          v = (s >> 8) / 8388608.0;
        }
        else if (bits == 32)
          v = int32_t(u32(p)) / 2147483648.0;
        out[i] = v;
      }
      return out;
    }
    pos += 8 + size + (size & 1);
  }
  throw std::runtime_error(path + " sem áudio");
}

inline void write(const std::string& path, const std::vector<double>& x, double sampleRate)
{
  std::ofstream f(path, std::ios::binary);
  auto w32 = [&](uint32_t v) { f.write(reinterpret_cast<const char*>(&v), 4); };
  auto w16 = [&](uint16_t v) { f.write(reinterpret_cast<const char*>(&v), 2); };
  const uint32_t dataBytes = static_cast<uint32_t>(x.size() * 4);
  f.write("RIFF", 4);
  w32(36 + dataBytes);
  f.write("WAVEfmt ", 8);
  w32(16);
  w16(3);
  w16(1);
  w32(static_cast<uint32_t>(sampleRate));
  w32(static_cast<uint32_t>(sampleRate) * 4);
  w16(4);
  w16(32);
  f.write("data", 4);
  w32(dataBytes);
  for (double v : x)
  {
    const float fv = static_cast<float>(v);
    f.write(reinterpret_cast<const char*>(&fv), 4);
  }
}

} // namespace wav
