module;

#include <array>
#include <cstddef>
#include <expected>
#include <filesystem>
#include <string>
#include <string_view>

export module notecraft.project;

export import notecraft.instruments;
export import notecraft.music;

export namespace notecraft::project {

inline constexpr int format_version = 1;
inline constexpr std::size_t instrument_count = 3;

using InstrumentSet = std::array<instruments::Instrument, instrument_count>;

InstrumentSet default_instruments();

struct Project {
  music::Sequence sequence;
  InstrumentSet instruments = default_instruments();
  int instrument_index = 0;
};

enum class Error {
  CannotOpen,
  CannotWrite,
  InvalidJson,
  InvalidData,
  UnsupportedVersion,
};

std::string_view describe(Error error);

std::string to_json(const Project& project);
std::expected<Project, Error> from_json(std::string_view text);

std::expected<void, Error> save(const Project& project,
                                const std::filesystem::path& path);
std::expected<Project, Error> load(const std::filesystem::path& path);

}  // namespace notecraft::project
