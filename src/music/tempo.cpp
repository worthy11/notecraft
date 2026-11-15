module;

#include <algorithm>
#include <chrono>
#include <cmath>

module notecraft.music;

namespace notecraft::music {

Tempo::Tempo(double bpm) { set_bpm(bpm); }

double Tempo::bpm() const { return bpm_; }

void Tempo::set_bpm(double bpm) {
  if (!std::isfinite(bpm)) {
    return;
  }
  bpm_ = std::clamp(bpm, min_bpm, max_bpm);
}

Seconds Tempo::beat_duration() const { return Seconds(60.0 / bpm_); }

Seconds Tempo::to_seconds(Tick ticks) const {
  return beat_duration() * (static_cast<double>(ticks) / ticks_per_beat);
}

Tick Tempo::to_ticks(Seconds time) const {
  return static_cast<Tick>(
      std::llround(time / beat_duration() * ticks_per_beat));
}

}  // namespace notecraft::music
