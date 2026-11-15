module;

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <tuple>
#include <vector>

module notecraft.audio;

namespace notecraft::audio {

static_assert(SampleSource<Synth>);

Synth::Synth(std::size_t max_voices) : slots_(std::max<std::size_t>(max_voices, 1)) {}

Synth::Slot& Synth::pick_slot() {
  return *std::ranges::min_element(slots_, {}, [](const Slot& slot) {
    return std::tuple{slot.voice.is_active(), !slot.voice.is_released(),
                      slot.order};
  });
}

void Synth::note_on(std::uint64_t key,
                    double frequency,
                    float velocity,
                    const VoiceSettings& settings) {
  note_off(key);
  Slot& slot = pick_slot();
  slot.voice.start(key, frequency, velocity, settings);
  slot.order = ++counter_;
}

void Synth::note_off(std::uint64_t key) {
  for (auto& slot : slots_) {
    if (slot.voice.is_active() && slot.voice.key() == key) {
      slot.voice.release();
    }
  }
}

void Synth::release_all() {
  for (auto& slot : slots_) {
    slot.voice.release();
  }
}

void Synth::reset() {
  for (auto& slot : slots_) {
    slot.voice.kill();
  }
}

std::size_t Synth::max_voices() const { return slots_.size(); }

std::size_t Synth::active_voices() const {
  return static_cast<std::size_t>(std::ranges::count_if(
      slots_, [](const Slot& slot) { return slot.voice.is_active(); }));
}

float Synth::master_gain() const { return master_gain_; }

void Synth::set_master_gain(float gain) {
  if (std::isfinite(gain)) {
    master_gain_ = std::clamp(gain, 0.f, 2.f);
  }
}

float Synth::next_sample(double sample_rate) {
  float mix = 0.f;
  for (auto& slot : slots_) {
    mix += slot.voice.next_sample(sample_rate);
  }
  return std::tanh(mix * master_gain_);
}

void Synth::render(std::span<std::int16_t> output, double sample_rate) {
  for (auto& sample : output) {
    sample = static_cast<std::int16_t>(
        std::clamp(next_sample(sample_rate), -1.f, 1.f) * 32767.f);
  }
}

}  // namespace notecraft::audio
