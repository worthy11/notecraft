module;

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <expected>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <map>
#include <span>
#include <string_view>
#include <utility>
#include <vector>

module notecraft.midi;

namespace notecraft::midi {

namespace {

struct Reader {
  std::span<const std::uint8_t> data;
  std::size_t pos = 0;
  bool ok = true;

  bool has(std::size_t count) const { return ok && data.size() - pos >= count; }

  bool tag_is(std::string_view tag) const {
    return has(4) && std::equal(tag.begin(), tag.end(), data.begin() + pos);
  }

  std::uint8_t u8() {
    if (!has(1)) {
      ok = false;
      return 0;
    }
    return data[pos++];
  }

  std::uint32_t u16() {
    const std::uint32_t high = u8();
    return (high << 8) | u8();
  }

  std::uint32_t u24() {
    const std::uint32_t high = u16();
    return (high << 8) | u8();
  }

  std::uint32_t u32() {
    const std::uint32_t high = u16();
    return (high << 16) | u16();
  }

  std::uint32_t varlen() {
    std::uint32_t value = 0;
    for (int i = 0; i < 4; ++i) {
      const auto byte = u8();
      value = (value << 7) | (byte & 0x7F);
      if ((byte & 0x80) == 0) {
        return value;
      }
    }
    ok = false;
    return 0;
  }

  void skip(std::size_t count) {
    if (!has(count)) {
      ok = false;
      return;
    }
    pos += count;
  }
};

struct Pending {
  std::uint64_t start = 0;
  int velocity = 0;
};

bool read_track(std::span<const std::uint8_t> track,
                std::uint32_t division,
                music::Sequence& sequence,
                bool& tempo_set) {
  Reader reader{track};
  std::map<std::pair<int, int>, std::deque<Pending>> open;
  std::uint64_t tick = 0;
  std::uint8_t running = 0;

  auto scale = [division](std::uint64_t value) {
    return static_cast<music::Tick>(
        (value * music::ticks_per_beat + division / 2) / division);
  };

  auto finish = [&](int channel, int pitch, std::uint64_t end) {
    const auto it = open.find({channel, pitch});
    if (it == open.end() || it->second.empty()) {
      return;
    }
    const Pending pending = it->second.front();
    it->second.pop_front();
    const auto start = scale(pending.start);
    sequence.add(music::Note{
        .start = start,
        .pitch = pitch,
        .duration = std::max<music::Tick>(1, scale(end) - start),
        .velocity = pending.velocity,
    });
  };

  while (reader.ok && reader.pos < track.size()) {
    tick += reader.varlen();
    std::uint8_t status = reader.u8();

    if (status < 0x80) {
      if (running == 0) {
        return false;
      }
      status = running;
      --reader.pos;
    }

    if (status == 0xFF) {
      running = 0;
      const auto type = reader.u8();
      const auto length = reader.varlen();
      if (type == 0x51 && length == 3) {
        const auto micros = reader.u24();
        if (!tempo_set && micros > 0) {
          sequence.tempo().set_bpm(60'000'000.0 / micros);
          tempo_set = true;
        }
      } else {
        reader.skip(length);
      }
      continue;
    }

    if (status == 0xF0 || status == 0xF7) {
      running = 0;
      reader.skip(reader.varlen());
      continue;
    }

    if (status > 0xF0) {
      return false;
    }

    running = status;
    const int kind = status & 0xF0;
    const int channel = status & 0x0F;

    if (kind == 0xC0 || kind == 0xD0) {
      reader.u8();
      continue;
    }

    const int pitch = reader.u8() & 0x7F;
    const int velocity = reader.u8() & 0x7F;

    if (kind == 0x90 && velocity > 0) {
      open[{channel, pitch}].push_back({tick, velocity});
    } else if (kind == 0x80 || kind == 0x90) {
      finish(channel, pitch, tick);
    }
  }

  if (!reader.ok) {
    return false;
  }

  for (auto& [key, pending] : open) {
    while (!pending.empty()) {
      finish(key.first, key.second, tick);
    }
  }
  return true;
}

}  // namespace

std::expected<music::Sequence, Error> decode(std::span<const std::uint8_t> data) {
  Reader reader{data};
  if (!reader.tag_is("MThd")) {
    return std::unexpected(Error::NotMidi);
  }
  reader.skip(4);

  const auto header_length = reader.u32();
  if (!reader.ok || header_length < 6) {
    return std::unexpected(Error::Corrupted);
  }
  const auto format = reader.u16();
  const auto track_count = reader.u16();
  const auto division = reader.u16();
  reader.skip(header_length - 6);

  if (!reader.ok) {
    return std::unexpected(Error::Corrupted);
  }
  if (format > 1 || (division & 0x8000) != 0) {
    return std::unexpected(Error::Unsupported);
  }
  if (division == 0) {
    return std::unexpected(Error::Corrupted);
  }

  music::Sequence sequence;
  bool tempo_set = false;
  std::uint32_t tracks_read = 0;

  while (tracks_read < track_count && reader.has(8)) {
    const bool is_track = reader.tag_is("MTrk");
    reader.skip(4);
    const auto length = reader.u32();
    if (!reader.has(length)) {
      return std::unexpected(Error::Corrupted);
    }
    if (is_track) {
      if (!read_track(data.subspan(reader.pos, length), division, sequence,
                      tempo_set)) {
        return std::unexpected(Error::Corrupted);
      }
      ++tracks_read;
    }
    reader.skip(length);
  }

  if (tracks_read != track_count) {
    return std::unexpected(Error::Corrupted);
  }
  return sequence;
}

std::expected<music::Sequence, Error> load(const std::filesystem::path& path) {
  std::ifstream file(path, std::ios::binary);
  if (!file) {
    return std::unexpected(Error::CannotOpen);
  }
  const std::vector<std::uint8_t> data{std::istreambuf_iterator<char>(file),
                                       std::istreambuf_iterator<char>()};
  return decode(data);
}

}  // namespace notecraft::midi
