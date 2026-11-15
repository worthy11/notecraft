module;

#include <SFML/Audio.hpp>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <vector>

module notecraft.playback;

namespace notecraft::playback {

namespace {

constexpr std::size_t chunk_size = 512;

class Stream : public sf::SoundStream {
public:
  Stream(std::mutex& mutex, Sequencer& sequencer)
      : mutex_(mutex), sequencer_(sequencer), buffer_(chunk_size) {
    initialize(1, sequencer.sample_rate(), {sf::SoundChannel::Mono});
  }

  ~Stream() override { stop(); }

private:
  bool onGetData(Chunk& data) override {
    {
      const std::scoped_lock lock(mutex_);
      sequencer_.render(buffer_);
    }
    data.samples = buffer_.data();
    data.sampleCount = buffer_.size();
    return true;
  }

  void onSeek(sf::Time) override {}

  std::mutex& mutex_;
  Sequencer& sequencer_;
  std::vector<std::int16_t> buffer_;
};

}  // namespace

struct Player::Impl {
  explicit Impl(unsigned sample_rate)
      : sequencer(sample_rate), stream(mutex, sequencer) {}

  std::mutex mutex;
  Sequencer sequencer;
  Stream stream;
};

Player::Player(unsigned sample_rate)
    : impl_(std::make_unique<Impl>(sample_rate)) {}

Player::~Player() = default;

void Player::set_sequence(const music::Sequence& sequence) {
  const std::scoped_lock lock(impl_->mutex);
  impl_->sequencer.set_sequence(sequence);
}

void Player::set_instrument(const instruments::Instrument& instrument) {
  const std::scoped_lock lock(impl_->mutex);
  impl_->sequencer.set_instrument(instrument);
}

void Player::set_bpm(double bpm) {
  const std::scoped_lock lock(impl_->mutex);
  impl_->sequencer.set_bpm(bpm);
}

double Player::bpm() const {
  const std::scoped_lock lock(impl_->mutex);
  return impl_->sequencer.bpm();
}

void Player::set_looping(bool looping) {
  const std::scoped_lock lock(impl_->mutex);
  impl_->sequencer.set_looping(looping);
}

bool Player::looping() const {
  const std::scoped_lock lock(impl_->mutex);
  return impl_->sequencer.looping();
}

void Player::play() {
  {
    const std::scoped_lock lock(impl_->mutex);
    impl_->sequencer.play();
  }
  if (impl_->stream.getStatus() != sf::SoundSource::Status::Playing) {
    impl_->stream.play();
  }
}

void Player::pause() {
  {
    const std::scoped_lock lock(impl_->mutex);
    impl_->sequencer.pause();
  }
  impl_->stream.pause();
}

void Player::stop() {
  {
    const std::scoped_lock lock(impl_->mutex);
    impl_->sequencer.stop();
  }
  impl_->stream.stop();
}

void Player::seek(music::Tick tick) {
  const std::scoped_lock lock(impl_->mutex);
  impl_->sequencer.seek(tick);
}

State Player::state() const {
  const std::scoped_lock lock(impl_->mutex);
  return impl_->sequencer.state();
}

music::Tick Player::position() const {
  const std::scoped_lock lock(impl_->mutex);
  return impl_->sequencer.position();
}

music::Seconds Player::position_time() const {
  const std::scoped_lock lock(impl_->mutex);
  return impl_->sequencer.position_time();
}

std::size_t Player::active_voices() const {
  const std::scoped_lock lock(impl_->mutex);
  return impl_->sequencer.active_voices();
}

}  // namespace notecraft::playback
