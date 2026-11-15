module;

#include <algorithm>
#include <cmath>
#include <cstdint>

module notecraft.audio;

namespace notecraft::audio {

static_assert(SampleSource<Voice>);

void Voice::start(std::uint64_t key,
                  double frequency,
                  float velocity,
                  const VoiceSettings& settings) {
  key_ = key;
  amplitude_ = std::clamp(settings.gain, 0.f, 1.f) *
               std::clamp(velocity, 0.f, 1.f);
  released_ = false;
  oscillator_.set_waveform(settings.waveform);
  oscillator_.set_frequency(frequency);
  oscillator_.reset();
  envelope_.set_adsr(settings.adsr);
  envelope_.note_on();
}

void Voice::release() {
  if (!released_) {
    released_ = true;
    envelope_.note_off();
  }
}

void Voice::kill() {
  released_ = false;
  envelope_.reset();
}

bool Voice::is_active() const { return envelope_.is_active(); }

bool Voice::is_released() const { return released_; }

std::uint64_t Voice::key() const { return key_; }

float Voice::next_sample(double sample_rate) {
  if (!envelope_.is_active()) {
    return 0.f;
  }
  return oscillator_.next_sample(sample_rate) *
         envelope_.next_level(sample_rate) * amplitude_;
}

}  // namespace notecraft::audio
