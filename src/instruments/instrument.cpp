module;

#include <cstdint>
#include <vector>

module notecraft.instruments;

namespace notecraft::instruments {

Instrument piano() {
  return Instrument{
      .name = "Piano",
      .waveform = audio::Waveform::Sine,
      .adsr = {.attack = 0.005, .decay = 0.25, .sustain = 0.2, .release = 0.35},
      .gain = 0.35f,
  };
}

Instrument bass() {
  return Instrument{
      .name = "Bass",
      .waveform = audio::Waveform::Saw,
      .adsr = {.attack = 0.01, .decay = 0.15, .sustain = 0.7, .release = 0.12},
      .gain = 0.3f,
  };
}

Instrument synth() {
  return Instrument{
      .name = "Synth",
      .waveform = audio::Waveform::Square,
      .adsr = {.attack = 0.05, .decay = 0.1, .sustain = 0.65, .release = 0.25},
      .gain = 0.25f,
  };
}

std::vector<std::int16_t> render(const Instrument& instrument,
                                 double frequency,
                                 double hold_seconds,
                                 unsigned sample_rate) {
  return audio::render_tone(frequency, hold_seconds, sample_rate,
                            instrument.waveform, instrument.adsr,
                            instrument.gain);
}

std::vector<std::int16_t> render(const Instrument& instrument,
                                 int pitch,
                                 double hold_seconds,
                                 unsigned sample_rate) {
  return render(instrument, audio::pitch_to_frequency(pitch), hold_seconds,
                sample_rate);
}

bool play(audio::AudioEngine& engine,
          const Instrument& instrument,
          double frequency,
          double hold_seconds) {
  return engine.play_samples(
      render(instrument, frequency, hold_seconds, engine.sample_rate()));
}

bool play(audio::AudioEngine& engine,
          const Instrument& instrument,
          int pitch,
          double hold_seconds) {
  return play(engine, instrument, audio::pitch_to_frequency(pitch),
              hold_seconds);
}

}  // namespace notecraft::instruments
