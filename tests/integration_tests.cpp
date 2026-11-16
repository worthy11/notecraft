#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <vector>

import notecraft.midi;
import notecraft.playback;
import notecraft.project;

using namespace notecraft;

namespace {

constexpr unsigned rate = 22050;

music::Sequence melody() {
  music::Sequence song;
  song.tempo().set_bpm(150);
  constexpr int pitches[] = {60, 62, 64, 65, 67, 69, 71, 72};
  for (int i = 0; i < 8; ++i) {
    song.add({.start = i * music::ticks_per_beat / 2, .pitch = pitches[i],
              .duration = music::ticks_per_beat / 2});
  }
  song.add({.start = 0, .pitch = 48, .duration = 4 * music::ticks_per_beat});
  return song;
}

int render_until_stopped(playback::Sequencer& sequencer, std::size_t& samples) {
  std::vector<std::int16_t> buffer(512);
  int peak = 0;
  samples = 0;
  while (sequencer.state() == playback::State::Playing && samples < rate * 10) {
    sequencer.render(buffer);
    samples += buffer.size();
    for (const auto sample : buffer) {
      peak = std::max(peak, std::abs(static_cast<int>(sample)));
    }
  }
  return peak;
}

}  // namespace

TEST_CASE("Project -> MIDI -> playback pipeline", "[integration]") {
  const auto dir = std::filesystem::temp_directory_path() / "notecraft_integration_tests";
  std::filesystem::create_directories(dir);

  project::Project original;
  original.sequence = melody();
  original.instrument_index = 1;
  REQUIRE(project::save(original, dir / "song.json"));

  const auto reopened = project::load(dir / "song.json");
  REQUIRE(reopened);
  REQUIRE(midi::save(reopened->sequence, dir / "song.mid"));

  const auto imported = midi::load(dir / "song.mid");
  REQUIRE(imported);
  CHECK(imported->size() == original.sequence.size());
  CHECK(imported->length() == original.sequence.length());

  playback::Sequencer sequencer(rate);
  sequencer.set_instrument(reopened->instruments[static_cast<std::size_t>(reopened->instrument_index)]);
  sequencer.set_sequence(*imported);
  sequencer.play();

  std::size_t samples = 0;
  const int peak = render_until_stopped(sequencer, samples);

  const auto expected = static_cast<std::size_t>(
      imported->tempo().to_seconds(imported->length()).count() * rate);
  CHECK(sequencer.state() == playback::State::Stopped);
  CHECK(samples >= expected);
  CHECK(samples <= expected + 512);
  CHECK(peak > 1000);

  std::filesystem::remove_all(dir);
}
