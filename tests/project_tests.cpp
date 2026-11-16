#include <catch2/catch_test_macros.hpp>

#include <cstddef>
#include <filesystem>
#include <fstream>
#include <string>

import notecraft.audio;
import notecraft.project;

using namespace notecraft;

namespace {

project::Project sample_project() {
  project::Project result;
  result.sequence.tempo().set_bpm(95);
  result.sequence.add({.start = 0, .pitch = 60, .duration = 480, .velocity = 100});
  result.sequence.add({.start = 480, .pitch = 67, .duration = 960, .velocity = 64});
  result.instruments[1].adsr.attack = 0.42;
  result.instruments[1].gain = 0.5f;
  result.instruments[2].waveform = audio::Waveform::Saw;
  result.instrument_index = 2;
  return result;
}

std::string replaced(std::string text, const std::string& from, const std::string& to) {
  const auto pos = text.find(from);
  REQUIRE(pos != std::string::npos);
  text.replace(pos, from.size(), to);
  return text;
}

}  // namespace

TEST_CASE("Default instruments", "[project]") {
  const auto set = project::default_instruments();
  CHECK(std::string(set[0].name) == "Piano");
  CHECK(std::string(set[1].name) == "Bass");
  CHECK(std::string(set[2].name) == "Synth");
}

TEST_CASE("Project JSON round trip", "[project]") {
  const auto original = sample_project();
  const auto loaded = project::from_json(project::to_json(original));
  REQUIRE(loaded);

  CHECK(loaded->sequence.tempo().bpm() == 95.0);
  CHECK(loaded->instrument_index == 2);
  CHECK(loaded->instruments[1].adsr.attack == 0.42);
  CHECK(loaded->instruments[1].gain == 0.5f);
  CHECK(loaded->instruments[2].waveform == audio::Waveform::Saw);
  CHECK(std::string(loaded->instruments[0].name) == "Piano");

  const auto a = original.sequence.sorted();
  const auto b = loaded->sequence.sorted();
  REQUIRE(a.size() == b.size());
  for (std::size_t i = 0; i < a.size(); ++i) {
    CHECK(a[i].note == b[i].note);
  }
}

TEST_CASE("Project files are saved and loaded from disk", "[project]") {
  const auto dir = std::filesystem::temp_directory_path() / "notecraft_project_tests";
  std::filesystem::create_directories(dir);
  const auto path = dir / "song.json";

  REQUIRE(project::save(sample_project(), path));
  const auto loaded = project::load(path);
  REQUIRE(loaded);
  CHECK(loaded->sequence.size() == 2);

  CHECK(project::load(dir / "missing.json").error() == project::Error::CannotOpen);
  CHECK(project::save(sample_project(), dir / "no_such_dir" / "x.json").error() ==
        project::Error::CannotWrite);

  std::ofstream(dir / "broken.json") << "garbage";
  CHECK(project::load(dir / "broken.json").error() == project::Error::InvalidJson);

  std::filesystem::remove_all(dir);
}

TEST_CASE("Project loading rejects invalid data", "[project]") {
  const auto text = project::to_json(sample_project());

  CHECK(project::from_json("{not json").error() == project::Error::InvalidJson);
  CHECK(project::from_json("").error() == project::Error::InvalidJson);
  CHECK(project::from_json("[]").error() == project::Error::InvalidData);
  CHECK(project::from_json("{}").error() == project::Error::InvalidData);

  CHECK(project::from_json(replaced(text, "\"format\":\"notecraft\"", "\"format\":\"other\"")).error() ==
        project::Error::InvalidData);
  CHECK(project::from_json(replaced(text, "\"version\":1", "\"version\":2")).error() ==
        project::Error::UnsupportedVersion);
  CHECK(project::from_json(replaced(text, "\"pitch\":60", "\"pitch\":200")).error() ==
        project::Error::InvalidData);
  CHECK(project::from_json(replaced(text, "\"pitch\":60", "\"pitch\":\"x\"")).error() ==
        project::Error::InvalidData);
  CHECK(project::from_json(replaced(text, "\"instrument\":2", "\"instrument\":5")).error() ==
        project::Error::InvalidData);
  CHECK(project::from_json(replaced(text, "\"waveform\":\"sine\"", "\"waveform\":\"noise\"")).error() ==
        project::Error::InvalidData);
  CHECK(project::from_json(replaced(text, "\"bpm\":", "\"tempo\":")).error() ==
        project::Error::InvalidData);
}

TEST_CASE("Project errors have descriptions", "[project]") {
  for (const auto error : {project::Error::CannotOpen, project::Error::CannotWrite,
                           project::Error::InvalidJson, project::Error::InvalidData,
                           project::Error::UnsupportedVersion}) {
    CHECK_FALSE(project::describe(error).empty());
  }
}
