#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <vector>

import notecraft.audio;

using namespace notecraft::audio;
using Catch::Approx;

namespace {

constexpr double rate = 1000.0;

int peak(const std::vector<std::int16_t>& samples) {
  int result = 0;
  for (const auto sample : samples) {
    result = std::max(result, std::abs(static_cast<int>(sample)));
  }
  return result;
}

float run(Envelope& envelope, int samples) {
  float level = 0.f;
  for (int i = 0; i < samples; ++i) {
    level = envelope.next_level(rate);
  }
  return level;
}

}  // namespace

TEST_CASE("Pitch to frequency", "[audio]") {
  CHECK(pitch_to_frequency(69) == Approx(440.0));
  CHECK(pitch_to_frequency(81) == Approx(880.0));
  CHECK(pitch_to_frequency(57) == Approx(220.0));
  CHECK(pitch_to_frequency(60) == Approx(261.6256).epsilon(1e-6));
}

TEST_CASE("Oscillator waveforms", "[audio][oscillator]") {
  SECTION("sine starts at zero and peaks at a quarter period") {
    Oscillator oscillator(Waveform::Sine, 250.0);
    CHECK(oscillator.next_sample(rate) == Approx(0.f).margin(1e-6));
    CHECK(oscillator.next_sample(rate) == Approx(1.f));
    CHECK(oscillator.next_sample(rate) == Approx(0.f).margin(1e-6));
    CHECK(oscillator.next_sample(rate) == Approx(-1.f));
  }

  SECTION("square switches sign every half period") {
    Oscillator oscillator(Waveform::Square, 250.0);
    CHECK(oscillator.next_sample(rate) == 1.f);
    CHECK(oscillator.next_sample(rate) == 1.f);
    CHECK(oscillator.next_sample(rate) == -1.f);
    CHECK(oscillator.next_sample(rate) == -1.f);
    CHECK(oscillator.next_sample(rate) == 1.f);
  }

  SECTION("saw ramps from -1 upwards") {
    Oscillator oscillator(Waveform::Saw, 250.0);
    CHECK(oscillator.next_sample(rate) == Approx(-1.f));
    CHECK(oscillator.next_sample(rate) == Approx(-0.5f));
    CHECK(oscillator.next_sample(rate) == Approx(0.f).margin(1e-6));
    CHECK(oscillator.next_sample(rate) == Approx(0.5f));
    CHECK(oscillator.next_sample(rate) == Approx(-1.f));
  }

  SECTION("samples stay in range") {
    for (const auto waveform : {Waveform::Sine, Waveform::Square, Waveform::Saw}) {
      Oscillator oscillator(waveform, 123.4);
      for (int i = 0; i < 5000; ++i) {
        const float sample = oscillator.next_sample(44100.0);
        REQUIRE(sample >= -1.f);
        REQUIRE(sample <= 1.f);
      }
    }
  }
}

TEST_CASE("Oscillator ignores invalid input", "[audio][oscillator]") {
  Oscillator oscillator(Waveform::Sine, 440.0);
  oscillator.set_frequency(-5.0);
  oscillator.set_frequency(0.0);
  oscillator.set_frequency(std::nan(""));
  CHECK(oscillator.frequency() == 440.0);
  CHECK(oscillator.next_sample(0.0) == 0.f);
  oscillator.set_waveform(Waveform::Saw);
  CHECK(oscillator.waveform() == Waveform::Saw);
}

TEST_CASE("Envelope goes through ADSR stages", "[audio][envelope]") {
  Envelope envelope({.attack = 0.01, .decay = 0.02, .sustain = 0.5, .release = 0.05});
  CHECK_FALSE(envelope.is_active());
  CHECK(envelope.next_level(rate) == 0.f);

  envelope.note_on();
  CHECK(envelope.is_active());
  CHECK(run(envelope, 5) == Approx(0.5f).margin(0.01));
  CHECK(run(envelope, 6) == Approx(1.f).margin(0.03));
  CHECK(run(envelope, 30) == Approx(0.5f).margin(1e-6));
  CHECK(run(envelope, 1000) == Approx(0.5f).margin(1e-6));

  envelope.note_off();
  CHECK(run(envelope, 25) == Approx(0.25f).margin(0.02));
  run(envelope, 30);
  CHECK_FALSE(envelope.is_active());
  CHECK(envelope.next_level(rate) == 0.f);
}

TEST_CASE("Envelope edge cases", "[audio][envelope]") {
  SECTION("zero times jump straight to the target") {
    Envelope envelope({.attack = 0.0, .decay = 0.0, .sustain = 0.3, .release = 0.0});
    envelope.note_on();
    CHECK(envelope.next_level(rate) == 1.f);
    CHECK(envelope.next_level(rate) == Approx(0.3f));
    envelope.note_off();
    CHECK(envelope.next_level(rate) == 0.f);
    CHECK_FALSE(envelope.is_active());
  }

  SECTION("note_off while idle does nothing") {
    Envelope envelope;
    envelope.note_off();
    CHECK_FALSE(envelope.is_active());
  }

  SECTION("invalid parameters are ignored or clamped") {
    Envelope envelope({.attack = 0.1, .decay = 0.1, .sustain = 0.5, .release = 0.1});
    envelope.set_adsr({.attack = -1.0, .decay = std::nan(""), .sustain = 3.0, .release = -2.0});
    CHECK(envelope.adsr().attack == 0.1);
    CHECK(envelope.adsr().decay == 0.1);
    CHECK(envelope.adsr().sustain == 1.0);
    CHECK(envelope.adsr().release == 0.1);
  }

  SECTION("reset silences immediately") {
    Envelope envelope;
    envelope.note_on();
    run(envelope, 10);
    envelope.reset();
    CHECK_FALSE(envelope.is_active());
  }
}

TEST_CASE("render_tone produces a bounded tone with release tail", "[audio]") {
  const Adsr adsr{.attack = 0.01, .decay = 0.01, .sustain = 0.5, .release = 0.1};
  const auto samples = render_tone(440.0, 0.5, 1000, Waveform::Square, adsr, 0.5f);
  CHECK(samples.size() >= 600);
  CHECK(samples.size() <= 602);
  CHECK(peak(samples) <= 32767 / 2 + 1);
  CHECK(peak(samples) > 10000);
  CHECK(std::abs(samples.back()) < 500);

  CHECK(render_tone(-1.0, 1.0).empty());
  CHECK(render_tone(440.0, 0.0).empty());
  CHECK(render_tone(440.0, 1.0, 0).empty());
  CHECK(peak(render_tone(440.0, 0.1, 1000, Waveform::Square, {}, 5.f)) <= 32767);
}

TEST_CASE("Voice plays, releases and dies", "[audio][voice]") {
  const VoiceSettings settings{Waveform::Square, {.attack = 0.0, .decay = 0.0, .sustain = 1.0, .release = 0.01}, 0.5f};
  Voice voice;
  CHECK_FALSE(voice.is_active());
  CHECK(voice.next_sample(rate) == 0.f);

  voice.start(7, 100.0, 0.5f, settings);
  CHECK(voice.is_active());
  CHECK(voice.key() == 7);
  CHECK_FALSE(voice.is_released());
  CHECK(voice.next_sample(rate) == Approx(0.25f));

  voice.release();
  CHECK(voice.is_released());
  for (int i = 0; i < 20; ++i) {
    voice.next_sample(rate);
  }
  CHECK_FALSE(voice.is_active());

  voice.start(8, 100.0, 1.f, settings);
  voice.kill();
  CHECK_FALSE(voice.is_active());
}

TEST_CASE("Synth mixes voices and steals the oldest", "[audio][synth]") {
  const VoiceSettings settings{Waveform::Square, {.attack = 0.0, .decay = 0.0, .sustain = 1.0, .release = 0.01}, 0.2f};

  SECTION("polyphony and note off") {
    Synth synth(8);
    synth.note_on(1, 100.0, 1.f, settings);
    synth.note_on(2, 150.0, 1.f, settings);
    synth.note_on(3, 200.0, 1.f, settings);
    CHECK(synth.active_voices() == 3);
    CHECK(synth.next_sample(rate) == Approx(std::tanh(0.6f)));

    synth.note_off(2);
    for (int i = 0; i < 20; ++i) {
      synth.next_sample(rate);
    }
    CHECK(synth.active_voices() == 2);

    synth.release_all();
    for (int i = 0; i < 20; ++i) {
      synth.next_sample(rate);
    }
    CHECK(synth.active_voices() == 0);
  }

  SECTION("retriggering the same key reuses one voice") {
    Synth synth(4);
    synth.note_on(1, 100.0, 1.f, settings);
    synth.note_on(1, 100.0, 1.f, settings);
    synth.next_sample(rate);
    synth.next_sample(rate);
    for (int i = 0; i < 20; ++i) {
      synth.next_sample(rate);
    }
    CHECK(synth.active_voices() == 1);
  }

  SECTION("voice limit is respected") {
    Synth synth(4);
    CHECK(synth.max_voices() == 4);
    for (int key = 0; key < 10; ++key) {
      synth.note_on(key, 100.0 + key, 1.f, settings);
    }
    CHECK(synth.active_voices() == 4);
    synth.reset();
    CHECK(synth.active_voices() == 0);
  }

  SECTION("output never clips") {
    Synth synth;
    for (int key = 0; key < 32; ++key) {
      synth.note_on(key, 100.0, 1.f, {Waveform::Square, {}, 1.f});
    }
    synth.set_master_gain(2.f);
    std::vector<std::int16_t> buffer(2000);
    synth.render(buffer, 44100.0);
    CHECK(peak(buffer) <= 32767);
    CHECK(peak(buffer) > 30000);
  }

  SECTION("master gain is clamped") {
    Synth synth;
    synth.set_master_gain(10.f);
    CHECK(synth.master_gain() == 2.f);
    synth.set_master_gain(-1.f);
    CHECK(synth.master_gain() == 0.f);
    synth.set_master_gain(std::nanf(""));
    CHECK(synth.master_gain() == 0.f);
  }
}
