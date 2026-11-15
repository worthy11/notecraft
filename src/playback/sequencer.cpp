module;

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>
#include <utility>
#include <variant>
#include <vector>

module notecraft.playback;

namespace notecraft::playback {

namespace {

template <typename... Fs>
struct overloaded : Fs... {
  using Fs::operator()...;
};

template <typename... Fs>
overloaded(Fs...) -> overloaded<Fs...>;

}  // namespace

std::vector<Event> make_events(const music::Sequence& sequence) {
  std::vector<Event> events;
  events.reserve(sequence.size() * 2);
  for (const auto& [id, note] : sequence.entries()) {
    events.push_back({note.start, NoteOn{id, note.pitch, note.velocity}});
    events.push_back({note.end(), NoteOff{id}});
  }
  std::ranges::stable_sort(events, {}, [](const Event& event) {
    return std::pair{event.tick, std::holds_alternative<NoteOn>(event.data)};
  });
  return events;
}

Sequencer::Sequencer(unsigned sample_rate) {
  if (sample_rate > 0) {
    sample_rate_ = sample_rate;
  }
}

void Sequencer::set_sequence(music::Sequence sequence) {
  sequence_ = std::move(sequence);
  events_ = make_events(sequence_);
  synth_.release_all();
  jump_to(position_);
  if (state_ == State::Playing) {
    start_held_notes();
  }
}

const music::Sequence& Sequencer::sequence() const { return sequence_; }

void Sequencer::set_instrument(const instruments::Instrument& instrument) {
  instrument_ = instrument;
}

const instruments::Instrument& Sequencer::instrument() const {
  return instrument_;
}

void Sequencer::set_bpm(double bpm) { sequence_.tempo().set_bpm(bpm); }

double Sequencer::bpm() const { return sequence_.tempo().bpm(); }

void Sequencer::set_looping(bool looping) { looping_ = looping; }

bool Sequencer::looping() const { return looping_; }

void Sequencer::play() {
  if (state_ == State::Playing) {
    return;
  }
  state_ = State::Playing;
  start_held_notes();
}

void Sequencer::pause() {
  if (state_ != State::Playing) {
    return;
  }
  state_ = State::Paused;
  synth_.release_all();
}

void Sequencer::stop() {
  state_ = State::Stopped;
  synth_.release_all();
  jump_to(0.0);
}

void Sequencer::seek(music::Tick tick) {
  synth_.release_all();
  jump_to(static_cast<double>(std::max<music::Tick>(tick, 0)));
  if (state_ == State::Playing) {
    start_held_notes();
  }
}

State Sequencer::state() const { return state_; }

music::Tick Sequencer::position() const {
  return static_cast<music::Tick>(position_);
}

music::Seconds Sequencer::position_time() const {
  return sequence_.tempo().beat_duration() *
         (position_ / static_cast<double>(music::ticks_per_beat));
}

std::size_t Sequencer::active_voices() const { return synth_.active_voices(); }

unsigned Sequencer::sample_rate() const { return sample_rate_; }

void Sequencer::render(std::span<std::int16_t> output) {
  for (auto& sample : output) {
    if (state_ == State::Playing) {
      advance();
    }
    sample = static_cast<std::int16_t>(
        std::clamp(synth_.next_sample(sample_rate_), -1.f, 1.f) * 32767.f);
  }
}

void Sequencer::advance() {
  const double end = position_ + ticks_per_sample();
  while (next_event_ < events_.size() &&
         static_cast<double>(events_[next_event_].tick) < end) {
    fire(events_[next_event_]);
    ++next_event_;
  }
  position_ = end;

  const auto length = static_cast<double>(sequence_.length());
  if (position_ < length) {
    return;
  }
  synth_.release_all();
  jump_to(0.0);
  if (!looping_ || length <= 0.0) {
    state_ = State::Stopped;
  }
}

void Sequencer::fire(const Event& event) {
  std::visit(overloaded{
                 [this](const NoteOn& on) {
                   synth_.note_on(on.id, audio::pitch_to_frequency(on.pitch),
                                  static_cast<float>(on.velocity) /
                                      music::max_velocity,
                                  voice_settings());
                 },
                 [this](const NoteOff& off) { synth_.note_off(off.id); },
             },
             event.data);
}

void Sequencer::jump_to(double tick) {
  position_ = tick;
  const auto it = std::ranges::lower_bound(
      events_, tick, {},
      [](const Event& event) { return static_cast<double>(event.tick); });
  next_event_ = static_cast<std::size_t>(it - events_.begin());
}

void Sequencer::start_held_notes() {
  for (const auto& [id, note] : sequence_.entries()) {
    const auto start = static_cast<double>(note.start);
    const auto end = static_cast<double>(note.end());
    if (start < position_ && end > position_) {
      fire({note.start, NoteOn{id, note.pitch, note.velocity}});
    }
  }
}

double Sequencer::ticks_per_sample() const {
  const double samples_per_beat =
      sequence_.tempo().beat_duration().count() * sample_rate_;
  return static_cast<double>(music::ticks_per_beat) / samples_per_beat;
}

audio::VoiceSettings Sequencer::voice_settings() const {
  return {instrument_.waveform, instrument_.adsr, instrument_.gain};
}

}  // namespace notecraft::playback
