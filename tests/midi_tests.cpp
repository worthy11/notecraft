#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <initializer_list>
#include <vector>

import notecraft.midi;

using namespace notecraft;
using Catch::Approx;
using Bytes = std::vector<std::uint8_t>;

namespace {

Bytes chunk(const char* tag, Bytes body) {
  Bytes out(tag, tag + 4);
  const auto size = body.size();
  out.insert(out.end(), {static_cast<std::uint8_t>(size >> 24),
                         static_cast<std::uint8_t>(size >> 16),
                         static_cast<std::uint8_t>(size >> 8),
                         static_cast<std::uint8_t>(size)});
  out.insert(out.end(), body.begin(), body.end());
  return out;
}

Bytes join(std::initializer_list<Bytes> parts) {
  Bytes out;
  for (const auto& part : parts) {
    out.insert(out.end(), part.begin(), part.end());
  }
  return out;
}

bool same_notes(const music::Sequence& a, const music::Sequence& b) {
  const auto x = a.sorted();
  const auto y = b.sorted();
  if (x.size() != y.size()) {
    return false;
  }
  for (std::size_t i = 0; i < x.size(); ++i) {
    if (x[i].note != y[i].note) {
      return false;
    }
  }
  return true;
}

music::Sequence sample_song() {
  music::Sequence song;
  song.tempo().set_bpm(110);
  song.add({.start = 0, .pitch = 60, .duration = 1920, .velocity = 70});
  song.add({.start = 0, .pitch = 64, .duration = 1920, .velocity = 70});
  song.add({.start = 480, .pitch = 72, .duration = 240, .velocity = 127});
  song.add({.start = 720, .pitch = 72, .duration = 240, .velocity = 1});
  song.add({.start = 5, .pitch = 127, .duration = 3, .velocity = 90});
  song.add({.start = 100000, .pitch = 0, .duration = 1, .velocity = 50});
  return song;
}

}  // namespace

TEST_CASE("MIDI export has a valid header", "[midi]") {
  const auto bytes = midi::encode(sample_song());
  REQUIRE(bytes.size() > 22);
  CHECK(Bytes(bytes.begin(), bytes.begin() + 14) ==
        Bytes{'M', 'T', 'h', 'd', 0, 0, 0, 6, 0, 0, 0, 1, 0x01, 0xE0});
  CHECK(Bytes(bytes.begin() + 14, bytes.begin() + 18) == Bytes{'M', 'T', 'r', 'k'});
  CHECK(Bytes(bytes.end() - 3, bytes.end()) == Bytes{0xFF, 0x2F, 0x00});
  CHECK(Bytes(bytes.begin() + 22, bytes.begin() + 29) ==
        Bytes{0x00, 0xFF, 0x51, 0x03, 0x08, 0x52, 0xAF});
}

TEST_CASE("MIDI export and import round trip", "[midi]") {
  const auto song = sample_song();
  const auto decoded = midi::decode(midi::encode(song));
  REQUIRE(decoded);
  CHECK(same_notes(song, *decoded));
  CHECK(decoded->tempo().bpm() == Approx(110.0).margin(0.001));

  const auto empty = midi::decode(midi::encode(music::Sequence{}));
  REQUIRE(empty);
  CHECK(empty->empty());
}

TEST_CASE("MIDI files are saved and loaded from disk", "[midi]") {
  const auto dir = std::filesystem::temp_directory_path() / "notecraft_midi_tests";
  std::filesystem::create_directories(dir);
  const auto path = dir / "song.mid";

  const auto song = sample_song();
  REQUIRE(midi::save(song, path));
  const auto loaded = midi::load(path);
  REQUIRE(loaded);
  CHECK(same_notes(song, *loaded));

  CHECK(midi::load(dir / "missing.mid").error() == midi::Error::CannotOpen);
  CHECK(midi::save(song, dir / "no_such_dir" / "song.mid").error() ==
        midi::Error::CannotWrite);

  std::filesystem::remove_all(dir);
}

TEST_CASE("MIDI import handles files from other programs", "[midi]") {
  const auto file = join({
      chunk("MThd", {0, 1, 0, 2, 0, 96}),
      chunk("MTrk", {0x00, 0xFF, 0x51, 0x03, 0x07, 0xA1, 0x20, 0x00, 0xFF, 0x2F, 0x00}),
      chunk("XYZW", {1, 2}),
      chunk("MTrk", {0x00, 0x91, 60, 100,
                     0x00, 64, 80,
                     0x18, 60, 0,
                     0x00, 0xF0, 0x02, 0x7E, 0xF7,
                     0x00, 0xFF, 0x01, 0x02, 'h', 'i',
                     0x00, 0x91, 60, 90,
                     0x00, 0xC1, 5,
                     0x00, 0xB1, 7, 100,
                     0x60, 0x81, 60, 0,
                     0x00, 0x81, 64, 0,
                     0x30, 0xFF, 0x2F, 0x00}),
  });

  const auto decoded = midi::decode(file);
  REQUIRE(decoded);
  CHECK(decoded->tempo().bpm() == Approx(120.0));

  const auto notes = decoded->sorted();
  REQUIRE(notes.size() == 3);
  CHECK(notes[0].note == music::Note{.start = 0, .pitch = 60, .duration = 120, .velocity = 100});
  CHECK(notes[1].note == music::Note{.start = 0, .pitch = 64, .duration = 600, .velocity = 80});
  CHECK(notes[2].note == music::Note{.start = 120, .pitch = 60, .duration = 480, .velocity = 90});
}

TEST_CASE("MIDI import closes notes left open", "[midi]") {
  const auto file = join({
      chunk("MThd", {0, 0, 0, 1, 0x01, 0xE0}),
      chunk("MTrk", {0x00, 0x90, 50, 60, 0x83, 0x60, 0xFF, 0x2F, 0x00}),
  });
  const auto decoded = midi::decode(file);
  REQUIRE(decoded);
  REQUIRE(decoded->size() == 1);
  CHECK(decoded->sorted()[0].note.duration == 480);
}

TEST_CASE("MIDI import reports errors", "[midi]") {
  const auto good = midi::encode(sample_song());

  CHECK(midi::decode(Bytes{}).error() == midi::Error::NotMidi);
  CHECK(midi::decode(Bytes{'R', 'I', 'F', 'F', 0, 0, 0, 0}).error() == midi::Error::NotMidi);

  Bytes truncated(good.begin(), good.end() - 5);
  CHECK(midi::decode(truncated).error() == midi::Error::Corrupted);

  auto format2 = good;
  format2[9] = 2;
  CHECK(midi::decode(format2).error() == midi::Error::Unsupported);

  auto smpte = good;
  smpte[12] = 0xE7;
  CHECK(midi::decode(smpte).error() == midi::Error::Unsupported);

  auto no_division = good;
  no_division[12] = 0;
  no_division[13] = 0;
  CHECK(midi::decode(no_division).error() == midi::Error::Corrupted);

  const auto data_without_status = join({
      chunk("MThd", {0, 0, 0, 1, 0x01, 0xE0}),
      chunk("MTrk", {0x00, 60, 100}),
  });
  CHECK(midi::decode(data_without_status).error() == midi::Error::Corrupted);
}

TEST_CASE("MIDI import never crashes on cut files", "[midi]") {
  const auto good = midi::encode(sample_song());
  for (std::size_t cut = 0; cut < good.size(); ++cut) {
    const Bytes part(good.begin(), good.begin() + static_cast<std::ptrdiff_t>(cut));
    CHECK_FALSE(midi::decode(part));
  }
}

TEST_CASE("MIDI errors have descriptions", "[midi]") {
  for (const auto error : {midi::Error::CannotOpen, midi::Error::CannotWrite,
                           midi::Error::NotMidi, midi::Error::Unsupported,
                           midi::Error::Corrupted}) {
    CHECK_FALSE(midi::describe(error).empty());
  }
}
