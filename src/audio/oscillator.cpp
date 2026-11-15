module;

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

module notecraft.audio;

namespace notecraft::audio {

namespace {

constexpr double two_pi = 6.28318530717958647692;

float sample_at(double phase, Waveform waveform) {
  switch (waveform) {
    case Waveform::Square:
      return phase < 0.5 ? 1.f : -1.f;
    case Waveform::Saw:
      return static_cast<float>(2.0 * phase - 1.0);
    case Waveform::Sine:
    default:
      return static_cast<float>(std::sin(two_pi * phase));
  }
}

template <SampleSource G>
float next_from(G& generator, double sample_rate) {
  return generator.next_sample(sample_rate);
}

}  // namespace

double pitch_to_frequency(int pitch) {
  return concert_a_hz * std::pow(2.0, (pitch - concert_a_pitch) / 12.0);
}

Oscillator::Oscillator(Waveform waveform, double frequency)
    : waveform_(waveform) {
  set_frequency(frequency);
}

Waveform Oscillator::waveform() const { return waveform_; }

void Oscillator::set_waveform(Waveform waveform) { waveform_ = waveform; }

double Oscillator::frequency() const { return frequency_; }

void Oscillator::set_frequency(double frequency) {
  if (!std::isfinite(frequency) || frequency <= 0.0) {
    return;
  }
  frequency_ = frequency;
}

void Oscillator::reset() { phase_ = 0.0; }

float Oscillator::next_sample(double sample_rate) {
  if (!std::isfinite(sample_rate) || sample_rate <= 0.0) {
    return 0.f;
  }
  const float sample = sample_at(phase_, waveform_);
  phase_ += frequency_ / sample_rate;
  phase_ -= std::floor(phase_);
  return sample;
}

std::vector<std::int16_t> render_tone(double frequency,
                                      double hold_seconds,
                                      unsigned sample_rate,
                                      Waveform waveform,
                                      const Adsr& adsr,
                                      float amplitude) {
  std::vector<std::int16_t> samples;
  if (!std::isfinite(frequency) || frequency <= 0.0 ||
      !std::isfinite(hold_seconds) || hold_seconds <= 0.0 ||
      sample_rate == 0) {
    return samples;
  }

  amplitude = std::clamp(amplitude, 0.f, 1.f);
  const auto hold_count =
      static_cast<std::size_t>(std::llround(hold_seconds * sample_rate));

  Oscillator oscillator(waveform, frequency);
  Envelope envelope(adsr);
  envelope.note_on();

  samples.reserve(hold_count +
                  static_cast<std::size_t>(
                      std::llround(std::max(0.0, adsr.release) * sample_rate) +
                      1));

  for (std::size_t i = 0; i < hold_count; ++i) {
    const float sample =
        next_from(oscillator, sample_rate) * envelope.next_level(sample_rate) *
        amplitude;
    samples.push_back(static_cast<std::int16_t>(
        std::clamp(sample, -1.f, 1.f) * 32767.f));
  }

  envelope.note_off();
  while (envelope.is_active()) {
    const float sample =
        next_from(oscillator, sample_rate) * envelope.next_level(sample_rate) *
        amplitude;
    samples.push_back(static_cast<std::int16_t>(
        std::clamp(sample, -1.f, 1.f) * 32767.f));
  }

  return samples;
}

}  // namespace notecraft::audio
