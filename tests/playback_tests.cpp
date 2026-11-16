#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <variant>
#include <vector>

import notecraft.playback;

using namespace notecraft;
using namespace notecraft::playback;
using Catch::Approx;

namespace {

constexpr unsigned rate = 44100;
constexpr std::size_t one_second = rate;

void run(Sequencer& sequencer, std::size_t samples) {
  std::vector<std::int16_t> buffer(samples);
  sequencer.render(buffer);
}

bool silent(Sequencer& sequencer, std::size_t samples) {
  std::vector<std::int16_t> buffer(samples);
  sequencer.render(buffer);
  for (const auto sample : buffer) {
    if (sample != 0) {
      return false;
    }
  }
  return true;
}

music::Sequence chord_then_note() {
  music::Sequence sequence;
  sequence.tempo().set_bpm(120);
  sequence.add({.start = 0, .pitch = 60, .duration = 960});
  sequence.add({.start = 0, .pitch = 64, .duration = 960});
  sequence.add({.start = 0, .pitch = 67, .duration = 960});
  sequence.add({.start = 960, .pitch = 72, .duration = 480});
  return sequence;
}

Sequencer make_sequencer() {
  Sequencer sequencer(rate);
  sequencer.set_instrument(instruments::synth());
  sequencer.set_sequence(chord_then_note());
  return sequencer;
}

}  // namespace

TEST_CASE("make_events orders note offs before note ons", "[playback]") {
  const auto events = make_events(chord_then_note());
  REQUIRE(events.size() == 8);
  CHECK(events.front().tick == 0);
  CHECK(std::holds_alternative<NoteOn>(events.front().data));
  CHECK(events[3].tick == 960);
  CHECK(std::holds_alternative<NoteOff>(events[3].data));
  CHECK(events[6].tick == 960);
  CHECK(std::holds_alternative<NoteOn>(events[6].data));
  CHECK(events.back().tick == 1440);
  for (std::size_t i = 1; i < events.size(); ++i) {
    CHECK(events[i - 1].tick <= events[i].tick);
  }
}

TEST_CASE("Stopped sequencer is silent", "[playback]") {
  auto sequencer = make_sequencer();
  CHECK(sequencer.state() == State::Stopped);
  CHECK(silent(sequencer, 4410));
  CHECK(sequencer.position() == 0);
}

TEST_CASE("Playback follows the tempo", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.play();
  run(sequencer, one_second / 10);
  CHECK(sequencer.state() == State::Playing);
  CHECK(sequencer.active_voices() == 3);
  CHECK(sequencer.position_time().count() == Approx(0.1).margin(1e-3));
  CHECK(std::abs(sequencer.position() - 96) <= 1);

  run(sequencer, one_second - one_second / 10 + 100);
  CHECK(sequencer.position() >= 960);
  CHECK(sequencer.position() < 970);

  run(sequencer, one_second * 3 / 10);
  CHECK(sequencer.active_voices() == 1);
}

TEST_CASE("Playback stops at the end of the song", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.play();
  run(sequencer, one_second * 2);
  CHECK(sequencer.state() == State::Stopped);
  CHECK(sequencer.position() == 0);
  run(sequencer, one_second);
  CHECK(sequencer.active_voices() == 0);
  CHECK(silent(sequencer, 1000));
}

TEST_CASE("Pause holds the position and resume continues", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.play();
  run(sequencer, one_second * 13 / 10);
  sequencer.pause();
  const auto paused_at = sequencer.position();

  run(sequencer, one_second);
  CHECK(sequencer.state() == State::Paused);
  CHECK(sequencer.position() == paused_at);
  CHECK(sequencer.active_voices() == 0);

  sequencer.play();
  run(sequencer, 1000);
  CHECK(sequencer.state() == State::Playing);
  CHECK(sequencer.active_voices() == 1);
  CHECK(sequencer.position() > paused_at);
}

TEST_CASE("Stop rewinds to the beginning", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.play();
  run(sequencer, one_second / 2);
  sequencer.stop();
  CHECK(sequencer.state() == State::Stopped);
  CHECK(sequencer.position() == 0);
  sequencer.pause();
  CHECK(sequencer.state() == State::Stopped);
}

TEST_CASE("Looping restarts the song", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.set_looping(true);
  CHECK(sequencer.looping());
  sequencer.play();
  run(sequencer, one_second * 3 + one_second / 10);
  CHECK(sequencer.state() == State::Playing);
  CHECK(sequencer.position() < 200);
  CHECK(sequencer.active_voices() >= 3);
}

TEST_CASE("Seek jumps and restarts held notes", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.seek(480);
  CHECK(sequencer.position() == 480);
  sequencer.play();
  run(sequencer, 10);
  CHECK(sequencer.active_voices() == 3);

  sequencer.seek(-100);
  CHECK(sequencer.position() == 0);
}

TEST_CASE("BPM changes the playback speed", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.set_bpm(60);
  CHECK(sequencer.bpm() == 60);
  sequencer.play();
  run(sequencer, one_second);
  CHECK(std::abs(sequencer.position() - 480) <= 1);
}

TEST_CASE("Editing the sequence while playing", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.play();
  run(sequencer, one_second / 2);

  music::Sequence edited;
  edited.add({.start = 0, .pitch = 50, .duration = 4 * 960});
  sequencer.set_sequence(edited);
  run(sequencer, 10);
  CHECK(sequencer.state() == State::Playing);
  CHECK(sequencer.active_voices() >= 1);
  CHECK(sequencer.sequence().size() == 1);
}

TEST_CASE("Empty sequence stops immediately", "[playback]") {
  Sequencer sequencer(rate);
  sequencer.play();
  run(sequencer, 10);
  CHECK(sequencer.state() == State::Stopped);
}

TEST_CASE("Instrument can be changed", "[playback]") {
  auto sequencer = make_sequencer();
  sequencer.set_instrument(instruments::bass());
  CHECK(sequencer.instrument().waveform == audio::Waveform::Saw);
  CHECK(sequencer.sample_rate() == rate);
}
