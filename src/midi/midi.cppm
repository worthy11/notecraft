module;

#include <cstdint>
#include <expected>
#include <filesystem>
#include <span>
#include <string_view>
#include <vector>

export module notecraft.midi;

export import notecraft.music;

export namespace notecraft::midi {

enum class Error { CannotOpen, CannotWrite, NotMidi, Unsupported, Corrupted };

std::string_view describe(Error error);

std::vector<std::uint8_t> encode(const music::Sequence& sequence);
std::expected<music::Sequence, Error> decode(std::span<const std::uint8_t> data);

std::expected<void, Error> save(const music::Sequence& sequence,
                                const std::filesystem::path& path);
std::expected<music::Sequence, Error> load(const std::filesystem::path& path);

}  // namespace notecraft::midi
