#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <chrono>
#include <cmath>
#include <compare>
#include <optional>

import notecraft.music;

using namespace notecraft::music;
using Catch::Approx;

TEST_CASE("Note validity and end", "[music][note]") {
  const Note note{.start = 480, .pitch = 60, .duration = 240, .velocity = 100};
  CHECK(note.end() == 720);
  CHECK(note.is_valid());

  CHECK_FALSE((Note{.start = -1}).is_valid());
  CHECK_FALSE((Note{.duration = 0}).is_valid());
  CHECK_FALSE((Note{.pitch = -1}).is_valid());
  CHECK_FALSE((Note{.pitch = 128}).is_valid());
  CHECK_FALSE((Note{.velocity = 0}).is_valid());
  CHECK_FALSE((Note{.velocity = 128}).is_valid());
  CHECK((Note{.pitch = 0, .velocity = 1}).is_valid());
  CHECK((Note{.pitch = 127, .velocity = 127}).is_valid());
}

TEST_CASE("Notes are ordered by start, then pitch", "[music][note]") {
  CHECK(Note{.start = 0, .pitch = 70} < Note{.start = 10, .pitch = 50});
  CHECK(Note{.start = 0, .pitch = 50} < Note{.start = 0, .pitch = 70});
  CHECK(Note{.start = 0, .pitch = 60} == Note{.start = 0, .pitch = 60});
}

TEST_CASE("Sequence add, find, update and remove", "[music][sequence]") {
  Sequence sequence;
  REQUIRE(sequence.empty());
  CHECK(sequence.length() == 0);

  const auto first = sequence.add({.start = 0, .pitch = 60, .duration = 480});
  const auto second = sequence.add({.start = 480, .pitch = 64, .duration = 480});
  REQUIRE(first);
  REQUIRE(second);
  CHECK(*first != *second);
  CHECK(sequence.size() == 2);
  CHECK(sequence.length() == 960);

  SECTION("invalid notes are rejected") {
    CHECK_FALSE(sequence.add({.duration = 0}));
    CHECK_FALSE(sequence.add({.pitch = 200}));
    CHECK(sequence.size() == 2);
  }

  SECTION("find returns the stored note") {
    REQUIRE(sequence.find(*second));
    CHECK(sequence.find(*second)->pitch == 64);
    CHECK_FALSE(sequence.find(999));
  }

  SECTION("update changes a note and rejects invalid data") {
    CHECK(sequence.update(*first, {.start = 960, .pitch = 67, .duration = 480}));
    CHECK(sequence.find(*first)->pitch == 67);
    CHECK(sequence.length() == 1440);
    CHECK_FALSE(sequence.update(*first, {.pitch = -5}));
    CHECK(sequence.find(*first)->pitch == 67);
    CHECK_FALSE(sequence.update(999, {}));
  }

  SECTION("remove deletes exactly once") {
    CHECK(sequence.remove(*first));
    CHECK_FALSE(sequence.remove(*first));
    CHECK_FALSE(sequence.find(*first));
    CHECK(sequence.size() == 1);
  }

  SECTION("ids are never reused") {
    sequence.remove(*second);
    const auto third = sequence.add({});
    REQUIRE(third);
    CHECK(*third > *second);
  }

  SECTION("clear removes everything") {
    sequence.clear();
    CHECK(sequence.empty());
    CHECK(sequence.length() == 0);
  }
}

TEST_CASE("Sequence time queries", "[music][sequence]") {
  Sequence sequence;
  const auto late = *sequence.add({.start = 960, .pitch = 64, .duration = 480});
  const auto low = *sequence.add({.start = 0, .pitch = 60, .duration = 960});
  const auto high = *sequence.add({.start = 0, .pitch = 67, .duration = 480});

  const auto sorted = sequence.sorted();
  REQUIRE(sorted.size() == 3);
  CHECK(sorted[0].id == low);
  CHECK(sorted[1].id == high);
  CHECK(sorted[2].id == late);

  CHECK(sequence.notes_at(0).size() == 2);
  REQUIRE(sequence.notes_at(500).size() == 1);
  CHECK(sequence.notes_at(500)[0].id == low);
  REQUIRE(sequence.notes_at(960).size() == 1);
  CHECK(sequence.notes_at(960)[0].id == late);
  CHECK(sequence.notes_at(5000).empty());
  CHECK(sequence.notes_between(400, 1000).size() == 3);
  CHECK(sequence.notes_between(480, 960).size() == 1);
}

TEST_CASE("Tempo converts between ticks and time", "[music][tempo]") {
  Tempo tempo;
  CHECK(tempo.bpm() == Tempo::default_bpm);
  CHECK(tempo.beat_duration().count() == Approx(0.5));
  CHECK(tempo.to_seconds(ticks_per_beat).count() == Approx(0.5));
  CHECK(tempo.to_ticks(Seconds(1.0)) == 2 * ticks_per_beat);

  tempo.set_bpm(60);
  CHECK(tempo.to_ticks(std::chrono::milliseconds(1500)) == 720);
  CHECK(tempo.to_seconds(720).count() == Approx(1.5));
}

TEST_CASE("Tempo clamps and ignores invalid values", "[music][tempo]") {
  Tempo tempo(1000);
  CHECK(tempo.bpm() == Tempo::max_bpm);
  tempo.set_bpm(1);
  CHECK(tempo.bpm() == Tempo::min_bpm);
  tempo.set_bpm(100);
  tempo.set_bpm(std::nan(""));
  CHECK(tempo.bpm() == 100);
  tempo.set_bpm(INFINITY);
  CHECK(tempo.bpm() == 100);
}
