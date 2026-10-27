// Separa uma música (WAV estéreo 44,1 kHz) nas seis faixas e mede o tempo.
// Uso: nexus_separar <modelo.bin> <musica.wav> <pasta_de_saida> [nucleos]
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>

#include "nexus/Separator.h"

namespace
{
// WAV simples (PCM 16/24 bits ou float 32), estéreo.
bool readWav(const char* path, nexus::studio::Stereo& out, int& rate)
{
  std::ifstream f(path, std::ios::binary);
  if (!f)
    return false;
  char id[4];
  uint32_t size = 0;
  f.read(id, 4).read(reinterpret_cast<char*>(&size), 4).read(id, 4);
  uint16_t format = 0, channels = 0, bits = 0;
  while (f.read(id, 4) && f.read(reinterpret_cast<char*>(&size), 4))
  {
    if (std::strncmp(id, "fmt ", 4) == 0)
    {
      std::vector<char> b(size);
      f.read(b.data(), size);
      std::memcpy(&format, b.data(), 2);
      std::memcpy(&channels, b.data() + 2, 2);
      uint32_t r = 0;
      std::memcpy(&r, b.data() + 4, 4);
      rate = static_cast<int>(r);
      std::memcpy(&bits, b.data() + 14, 2);
      if (format == 0xFFFE)
        std::memcpy(&format, b.data() + 24, 2);
    }
    else if (std::strncmp(id, "data", 4) == 0)
    {
      std::vector<char> b(size);
      f.read(b.data(), size);
      const size_t bytes = bits / 8, frames = size / (bytes * channels);
      out.left.resize(frames);
      out.right.resize(frames);
      for (size_t i = 0; i < frames; ++i)
        for (size_t c = 0; c < 2; ++c)
        {
          const char* p = b.data() + (i * channels + std::min<size_t>(c, channels - 1u)) * bytes;
          float v = 0.0f;
          if (format == 3)
            std::memcpy(&v, p, 4);
          else if (bits == 16)
          {
            int16_t s;
            std::memcpy(&s, p, 2);
            v = s / 32768.0f;
          }
          else if (bits == 24)
          {
            const int32_t s = (static_cast<int32_t>(static_cast<uint8_t>(p[0])) << 8 | static_cast<int32_t>(static_cast<uint8_t>(p[1])) << 16
                               | static_cast<int32_t>(p[2]) << 24) >> 8;
            v = s / 8388608.0f;
          }
          (c == 0 ? out.left : out.right)[i] = v;
        }
      return true;
    }
    else
      f.seekg(size + (size & 1u), std::ios::cur);
  }
  return false;
}

void writeWav(const std::string& path, const nexus::studio::Stereo& s)
{
  std::ofstream f(path, std::ios::binary);
  const uint32_t frames = static_cast<uint32_t>(s.frames()), data = frames * 8, riff = 36 + data, fmt = 16,
                 rate = nexus::studio::kSeparationRate, bps = rate * 8;
  const uint16_t format = 3, channels = 2, align = 8, bits = 32;
  f.write("RIFF", 4).write(reinterpret_cast<const char*>(&riff), 4).write("WAVEfmt ", 8);
  f.write(reinterpret_cast<const char*>(&fmt), 4).write(reinterpret_cast<const char*>(&format), 2);
  f.write(reinterpret_cast<const char*>(&channels), 2).write(reinterpret_cast<const char*>(&rate), 4);
  f.write(reinterpret_cast<const char*>(&bps), 4).write(reinterpret_cast<const char*>(&align), 2);
  f.write(reinterpret_cast<const char*>(&bits), 2).write("data", 4).write(reinterpret_cast<const char*>(&data), 4);
  for (size_t i = 0; i < frames; ++i)
    f.write(reinterpret_cast<const char*>(&s.left[i]), 4).write(reinterpret_cast<const char*>(&s.right[i]), 4);
}
} // namespace

int main(int argc, char** argv)
{
  if (argc < 4)
  {
    std::printf("Uso: nexus_separar <modelo.bin> <musica.wav> <pasta_de_saida> [nucleos]\n");
    return 2;
  }
  nexus::studio::Stereo mix;
  int rate = 0;
  if (!readWav(argv[2], mix, rate) || rate != nexus::studio::kSeparationRate)
  {
    std::printf("Precisa de WAV estéreo em 44100 Hz (veio %d).\n", rate);
    return 1;
  }
  nexus::studio::Separator sep;
  std::string error;
  auto t0 = std::chrono::steady_clock::now();
  if (!sep.load(argv[1], error))
  {
    std::printf("Erro no modelo: %s\n", error.c_str());
    return 1;
  }
  auto t1 = std::chrono::steady_clock::now();
  const int threads = argc > 4 ? std::atoi(argv[4]) : nexus::studio::Separator::defaultThreads();
  std::array<nexus::studio::Stereo, nexus::studio::numStems> out;
  std::atomic<bool> cancel{false};
  int last = -1;
  const bool ok = sep.separate(mix, threads, [&](float p) {
    const int pc = static_cast<int>(p * 10.0f);
    if (pc != last)
    {
      last = pc;
      std::fprintf(stderr, "  %d%%\n", pc * 10);
    }
  }, cancel, out, error);
  auto t2 = std::chrono::steady_clock::now();
  if (!ok)
  {
    std::printf("Erro: %s\n", error.c_str());
    return 1;
  }
  const char* names[] = {"bateria", "baixo", "resto", "voz", "guitarra", "piano"};
  for (int s = 0; s < nexus::studio::numStems; ++s)
    writeWav(std::string(argv[3]) + "/" + names[s] + ".wav", out[static_cast<size_t>(s)]);
  const double secs = static_cast<double>(mix.frames()) / rate;
  std::printf("Música de %.1f s; modelo em %.1f s; separação em %.1f s com %d núcleos (%.2fx o tempo da música)\n", secs,
              std::chrono::duration<double>(t1 - t0).count(), std::chrono::duration<double>(t2 - t1).count(), threads,
              std::chrono::duration<double>(t2 - t1).count() / secs);
  return 0;
}
