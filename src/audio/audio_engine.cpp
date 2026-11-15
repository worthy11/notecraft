module;

#include <SFML/Audio.hpp>
#include <SFML/System.hpp>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

module notecraft.audio;

namespace notecraft::audio {

struct AudioEngine::Impl {
  unsigned sample_rate = default_sample_rate;
  sf::SoundBuffer buffer;
  std::optional<sf::Sound> sound;
};

AudioEngine::AudioEngine(unsigned sample_rate)
    : impl_(std::make_unique<Impl>()) {
  if (sample_rate > 0) {
    impl_->sample_rate = sample_rate;
  }
}

AudioEngine::~AudioEngine() = default;

AudioEngine::AudioEngine(AudioEngine&&) noexcept = default;
AudioEngine& AudioEngine::operator=(AudioEngine&&) noexcept = default;

unsigned AudioEngine::sample_rate() const { return impl_->sample_rate; }

bool AudioEngine::play_samples(const std::vector<std::int16_t>& samples) {
  if (samples.empty()) {
    return false;
  }

  stop();

  const std::vector<sf::SoundChannel> channels{sf::SoundChannel::Mono};
  if (!impl_->buffer.loadFromSamples(samples.data(), samples.size(), 1,
                                     impl_->sample_rate, channels)) {
    return false;
  }

  impl_->sound.emplace(impl_->buffer);
  impl_->sound->play();

  while (impl_->sound->getStatus() == sf::SoundSource::Status::Playing) {
    sf::sleep(sf::milliseconds(10));
  }
  return true;
}

bool AudioEngine::play_tone(double frequency,
                            double hold_seconds,
                            Waveform waveform,
                            const Adsr& adsr,
                            float amplitude) {
  return play_samples(render_tone(frequency, hold_seconds, impl_->sample_rate,
                                  waveform, adsr, amplitude));
}

bool AudioEngine::play_tone(int pitch,
                            double hold_seconds,
                            Waveform waveform,
                            const Adsr& adsr,
                            float amplitude) {
  return play_tone(pitch_to_frequency(pitch), hold_seconds, waveform, adsr, amplitude);
}

void AudioEngine::stop() {
  if (impl_->sound) {
    impl_->sound->stop();
    impl_->sound.reset();
  }
}

bool AudioEngine::is_playing() const {
  return impl_->sound &&
         impl_->sound->getStatus() == sf::SoundSource::Status::Playing;
}

}  // namespace notecraft::audio
