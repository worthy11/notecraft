module;

#include <chrono>
#include <compare>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

export module notecraft.music;

export namespace notecraft::music {

using Tick = std::int64_t;
using NoteId = std::uint64_t;
using Seconds = std::chrono::duration<double>;

inline constexpr Tick ticks_per_beat = 480;
inline constexpr int min_pitch = 0;
inline constexpr int max_pitch = 127;
inline constexpr int min_velocity = 1;
inline constexpr int max_velocity = 127;

struct Note {
  Tick start = 0;
  int pitch = 60;
  Tick duration = ticks_per_beat;
  int velocity = 100;

  constexpr Tick end() const { return start + duration; }

  constexpr bool is_valid() const {
    return start >= 0 && duration > 0 && pitch >= min_pitch &&
           pitch <= max_pitch && velocity >= min_velocity &&
           velocity <= max_velocity;
  }

  auto operator<=>(const Note&) const = default;
};

struct NoteEntry {
  NoteId id = 0;
  Note note;

  bool operator==(const NoteEntry&) const = default;
};

class Tempo {
public:
  static constexpr double min_bpm = 20.0;
  static constexpr double max_bpm = 300.0;
  static constexpr double default_bpm = 120.0;

  explicit Tempo(double bpm = default_bpm);

  double bpm() const;
  void set_bpm(double bpm);

  Seconds beat_duration() const;
  Seconds to_seconds(Tick ticks) const;
  Tick to_ticks(Seconds time) const;

private:
  double bpm_ = default_bpm;
};

class Sequence {
public:
  std::optional<NoteId> add(const Note& note);
  bool remove(NoteId id);
  bool update(NoteId id, const Note& note);
  void clear();

  std::optional<Note> find(NoteId id) const;
  std::span<const NoteEntry> entries() const;
  std::vector<NoteEntry> sorted() const;
  std::vector<NoteEntry> notes_between(Tick from, Tick to) const;
  std::vector<NoteEntry> notes_at(Tick tick) const;

  std::size_t size() const;
  bool empty() const;
  Tick length() const;

  Tempo& tempo();
  const Tempo& tempo() const;

private:
  std::vector<NoteEntry> entries_;
  NoteId next_id_ = 1;
  Tempo tempo_;
};

}  // namespace notecraft::music
