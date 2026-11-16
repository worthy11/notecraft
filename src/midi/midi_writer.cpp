module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <expected>
#include <filesystem>
#include <fstream>
#include <string_view>
#include <tuple>
#include <vector>

module notecraft.midi;

namespace notecraft::midi {

namespace {

constexpr std::uint8_t note_on_status = 0x90;
constexpr std::uint8_t note_off_status = 0x80;

struct TrackEvent {
  music::Tick tick = 0;
  std::uint8_t status = 0;
  std::uint8_t pitch = 0;
  std::uint8_t velocity = 0;
};

void put_u16(std::vector<std::uint8_t>& out, std::uint16_t value) {
  out.push_back(static_cast<std::uint8_t>(value >> 8));
  out.push_back(static_cast<std::uint8_t>(value));
}

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t value) {
  for (int shift = 24; shift >= 0; shift -= 8) {
    out.push_back(static_cast<std::uint8_t>(value >> shift));
  }
}

void put_varlen(std::vector<std::uint8_t>& out, std::uint32_t value) {
  std::uint8_t bytes[5];
  int count = 0;
  do {
    bytes[count++] = static_cast<std::uint8_t>(value & 0x7F);
    value >>= 7;
  } while (value > 0);
  while (count > 0) {
    --count;
    out.push_back(static_cast<std::uint8_t>(bytes[count] | (count > 0 ? 0x80 : 0)));
  }
}

std::vector<std::uint8_t> track_data(const music::Sequence& sequence) {
  std::vector<TrackEvent> events;
  for (const auto& [id, note] : sequence.entries()) {
    const auto pitch = static_cast<std::uint8_t>(note.pitch);
    events.push_back({note.start, note_on_status, pitch,
                      static_cast<std::uint8_t>(note.velocity)});
    events.push_back({note.end(), note_off_status, pitch, 0});
  }
  std::ranges::stable_sort(events, {}, [](const TrackEvent& event) {
    return std::tuple{event.tick, event.status == note_on_status};
  });

  std::vector<std::uint8_t> out;
  const auto micros_per_beat =
      static_cast<std::uint32_t>(std::lround(60'000'000.0 / sequence.tempo().bpm()));
  put_varlen(out, 0);
  out.insert(out.end(), {0xFF, 0x51, 0x03});
  out.push_back(static_cast<std::uint8_t>(micros_per_beat >> 16));
  out.push_back(static_cast<std::uint8_t>(micros_per_beat >> 8));
  out.push_back(static_cast<std::uint8_t>(micros_per_beat));

  music::Tick previous = 0;
  for (const auto& event : events) {
    put_varlen(out, static_cast<std::uint32_t>(event.tick - previous));
    previous = event.tick;
    out.insert(out.end(), {event.status, event.pitch, event.velocity});
  }

  put_varlen(out, 0);
  out.insert(out.end(), {0xFF, 0x2F, 0x00});
  return out;
}

}  // namespace

std::string_view describe(Error error) {
  switch (error) {
    case Error::CannotOpen:
      return "cannot open MIDI file";
    case Error::CannotWrite:
      return "cannot write MIDI file";
    case Error::NotMidi:
      return "file is not a MIDI file";
    case Error::Unsupported:
      return "unsupported MIDI format";
    case Error::Corrupted:
      return "MIDI file is corrupted";
  }
  return "unknown MIDI error";
}

std::vector<std::uint8_t> encode(const music::Sequence& sequence) {
  const auto track = track_data(sequence);

  std::vector<std::uint8_t> out;
  out.insert(out.end(), {'M', 'T', 'h', 'd'});
  put_u32(out, 6);
  put_u16(out, 0);
  put_u16(out, 1);
  put_u16(out, static_cast<std::uint16_t>(music::ticks_per_beat));

  out.insert(out.end(), {'M', 'T', 'r', 'k'});
  put_u32(out, static_cast<std::uint32_t>(track.size()));
  out.insert(out.end(), track.begin(), track.end());
  return out;
}

std::expected<void, Error> save(const music::Sequence& sequence,
                                const std::filesystem::path& path) {
  std::ofstream file(path, std::ios::binary);
  if (!file) {
    return std::unexpected(Error::CannotWrite);
  }
  const auto data = encode(sequence);
  file.write(reinterpret_cast<const char*>(data.data()),
             static_cast<std::streamsize>(data.size()));
  if (!file) {
    return std::unexpected(Error::CannotWrite);
  }
  return {};
}

}  // namespace notecraft::midi
