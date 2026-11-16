#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

import notecraft.audio;
import notecraft.instruments;

using namespace notecraft;

TEST_CASE("Instrument presets", "[instruments]") {
  const auto piano = instruments::piano();
  const auto bass = instruments::bass();
  const auto synth = instruments::synth();

  CHECK(std::string(piano.name) == "Piano");
  CHECK(std::string(bass.name) == "Bass");
  CHECK(std::string(synth.name) == "Synth");

  CHECK(piano.waveform == audio::Waveform::Sine);
  CHECK(bass.waveform == audio::Waveform::Saw);
  CHECK(synth.waveform == audio::Waveform::Square);

  for (const auto& instrument : {piano, bass, synth}) {
    CHECK(instrument.gain > 0.f);
    CHECK(instrument.gain <= 1.f);
    CHECK(instrument.adsr.sustain >= 0.0);
    CHECK(instrument.adsr.sustain <= 1.0);
  }
}

TEST_CASE("Instrument rendering matches the synthesizer", "[instruments]") {
  const auto bass = instruments::bass();
  const auto by_pitch = instruments::render(bass, 69, 0.1, 8000);
  const auto by_frequency = instruments::render(bass, 440.0, 0.1, 8000);
  const auto direct = audio::render_tone(440.0, 0.1, 8000, bass.waveform, bass.adsr, bass.gain);

  CHECK_FALSE(by_pitch.empty());
  CHECK(by_pitch == by_frequency);
  CHECK(by_pitch == direct);
}

TEST_CASE("Different instruments sound different", "[instruments]") {
  const auto piano = instruments::render(instruments::piano(), 60, 0.2, 8000);
  const auto synth = instruments::render(instruments::synth(), 60, 0.2, 8000);
  CHECK(piano != synth);
}
