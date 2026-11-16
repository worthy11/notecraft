module;

#include <boost/json.hpp>
#include <cstddef>
#include <exception>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

module notecraft.project;

import notecraft.audio;

namespace notecraft::project {

namespace json = boost::json;

namespace {

std::string_view waveform_name(audio::Waveform waveform) {
  switch (waveform) {
    case audio::Waveform::Square:
      return "square";
    case audio::Waveform::Saw:
      return "saw";
    case audio::Waveform::Sine:
      break;
  }
  return "sine";
}

audio::Waveform waveform_from(std::string_view name) {
  if (name == "sine") {
    return audio::Waveform::Sine;
  }
  if (name == "square") {
    return audio::Waveform::Square;
  }
  if (name == "saw") {
    return audio::Waveform::Saw;
  }
  throw std::invalid_argument("unknown waveform");
}

json::object instrument_to_json(const instruments::Instrument& instrument) {
  return {
      {"name", instrument.name},
      {"waveform", waveform_name(instrument.waveform)},
      {"attack", instrument.adsr.attack},
      {"decay", instrument.adsr.decay},
      {"sustain", instrument.adsr.sustain},
      {"release", instrument.adsr.release},
      {"gain", instrument.gain},
  };
}

void instrument_from_json(const json::object& object,
                          instruments::Instrument& instrument) {
  instrument.waveform = waveform_from(object.at("waveform").as_string());
  instrument.adsr.attack = object.at("attack").to_number<double>();
  instrument.adsr.decay = object.at("decay").to_number<double>();
  instrument.adsr.sustain = object.at("sustain").to_number<double>();
  instrument.adsr.release = object.at("release").to_number<double>();
  instrument.gain = object.at("gain").to_number<float>();
}

json::object note_to_json(const music::Note& note) {
  return {
      {"start", note.start},
      {"pitch", note.pitch},
      {"duration", note.duration},
      {"velocity", note.velocity},
  };
}

music::Note note_from_json(const json::object& object) {
  return {
      .start = object.at("start").to_number<music::Tick>(),
      .pitch = object.at("pitch").to_number<int>(),
      .duration = object.at("duration").to_number<music::Tick>(),
      .velocity = object.at("velocity").to_number<int>(),
  };
}

}  // namespace

InstrumentSet default_instruments() {
  return {instruments::piano(), instruments::bass(), instruments::synth()};
}

std::string_view describe(Error error) {
  switch (error) {
    case Error::CannotOpen:
      return "cannot open project file";
    case Error::CannotWrite:
      return "cannot write project file";
    case Error::InvalidJson:
      return "project file is not valid JSON";
    case Error::InvalidData:
      return "project file contains invalid data";
    case Error::UnsupportedVersion:
      return "unsupported project version";
  }
  return "unknown project error";
}

std::string to_json(const Project& project) {
  json::array instrument_list;
  for (const auto& instrument : project.instruments) {
    instrument_list.push_back(instrument_to_json(instrument));
  }

  json::array note_list;
  for (const auto& entry : project.sequence.sorted()) {
    note_list.push_back(note_to_json(entry.note));
  }

  const json::object root{
      {"format", "notecraft"},
      {"version", format_version},
      {"bpm", project.sequence.tempo().bpm()},
      {"instrument", project.instrument_index},
      {"instruments", std::move(instrument_list)},
      {"notes", std::move(note_list)},
  };
  return json::serialize(root);
}

std::expected<Project, Error> from_json(std::string_view text) {
  boost::system::error_code code;
  const json::value value = json::parse(text, code);
  if (code) {
    return std::unexpected(Error::InvalidJson);
  }

  try {
    const auto& root = value.as_object();
    if (root.at("format").as_string() != "notecraft") {
      return std::unexpected(Error::InvalidData);
    }
    if (root.at("version").to_number<int>() != format_version) {
      return std::unexpected(Error::UnsupportedVersion);
    }

    Project project;
    project.sequence.tempo().set_bpm(root.at("bpm").to_number<double>());

    const auto& instrument_list = root.at("instruments").as_array();
    if (instrument_list.size() != instrument_count) {
      return std::unexpected(Error::InvalidData);
    }
    for (std::size_t i = 0; i < instrument_count; ++i) {
      instrument_from_json(instrument_list[i].as_object(), project.instruments[i]);
    }

    project.instrument_index = root.at("instrument").to_number<int>();
    if (project.instrument_index < 0 ||
        project.instrument_index >= static_cast<int>(instrument_count)) {
      return std::unexpected(Error::InvalidData);
    }

    for (const auto& item : root.at("notes").as_array()) {
      if (!project.sequence.add(note_from_json(item.as_object()))) {
        return std::unexpected(Error::InvalidData);
      }
    }
    return project;
  } catch (const std::exception&) {
    return std::unexpected(Error::InvalidData);
  }
}

std::expected<void, Error> save(const Project& project,
                                const std::filesystem::path& path) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    return std::unexpected(Error::CannotWrite);
  }
  file << to_json(project) << '\n';
  if (!file) {
    return std::unexpected(Error::CannotWrite);
  }
  return {};
}

std::expected<Project, Error> load(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::unexpected(Error::CannotOpen);
  }
  const std::string text{std::istreambuf_iterator<char>(file),
                         std::istreambuf_iterator<char>()};
  return from_json(text);
}

}  // namespace notecraft::project
