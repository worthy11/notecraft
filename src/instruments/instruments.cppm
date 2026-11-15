module;

#include <cstdint>
#include <vector>

export module notecraft.instruments;

import notecraft.audio;

export namespace notecraft::instruments {

struct Instrument {
  const char* name = "Synth";
  audio::Waveform waveform = audio::Waveform::Sine;
  audio::Adsr adsr{};
  float gain = 0.3f;
};

Instrument piano();
Instrument bass();
Instrument synth();

std::vector<std::int16_t> render(const Instrument& instrument,
                                 double frequency,
                                 double hold_seconds,
                                 unsigned sample_rate = audio::default_sample_rate);

std::vector<std::int16_t> render(const Instrument& instrument,
                                 int pitch,
                                 double hold_seconds,
                                 unsigned sample_rate = audio::default_sample_rate);

bool play(audio::AudioEngine& engine,
          const Instrument& instrument,
          double frequency,
          double hold_seconds);

bool play(audio::AudioEngine& engine,
          const Instrument& instrument,
          int pitch,
          double hold_seconds);

}  // namespace notecraft::instruments
