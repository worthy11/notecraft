module;

#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <variant>
#include <vector>

export module notecraft.playback;

export import notecraft.audio;
export import notecraft.instruments;
export import notecraft.music;

export namespace notecraft::playback {

enum class State { Stopped, Playing, Paused };

struct NoteOn {
  music::NoteId id = 0;
  int pitch = 60;
  int velocity = 100;
};

struct NoteOff {
  music::NoteId id = 0;
};

struct Event {
  music::Tick tick = 0;
  std::variant<NoteOn, NoteOff> data;
};

std::vector<Event> make_events(const music::Sequence& sequence);

class Sequencer {
public:
  explicit Sequencer(unsigned sample_rate = audio::default_sample_rate);

  void set_sequence(music::Sequence sequence);
  const music::Sequence& sequence() const;

  void set_instrument(const instruments::Instrument& instrument);
  const instruments::Instrument& instrument() const;

  void set_bpm(double bpm);
  double bpm() const;

  void set_looping(bool looping);
  bool looping() const;

  void play();
  void pause();
  void stop();
  void seek(music::Tick tick);

  State state() const;
  music::Tick position() const;
  music::Seconds position_time() const;
  std::size_t active_voices() const;
  unsigned sample_rate() const;

  void render(std::span<std::int16_t> output);

private:
  void advance();
  void fire(const Event& event);
  void jump_to(double tick);
  void start_held_notes();
  double ticks_per_sample() const;
  audio::VoiceSettings voice_settings() const;

  music::Sequence sequence_;
  std::vector<Event> events_;
  std::size_t next_event_ = 0;
  instruments::Instrument instrument_ = instruments::piano();
  audio::Synth synth_;
  unsigned sample_rate_ = audio::default_sample_rate;
  double position_ = 0.0;
  State state_ = State::Stopped;
  bool looping_ = false;
};

class Player {
public:
  explicit Player(unsigned sample_rate = audio::default_sample_rate);
  ~Player();

  Player(const Player&) = delete;
  Player& operator=(const Player&) = delete;
  Player(Player&&) = delete;
  Player& operator=(Player&&) = delete;

  void set_sequence(const music::Sequence& sequence);
  void set_instrument(const instruments::Instrument& instrument);
  void set_bpm(double bpm);
  double bpm() const;
  void set_looping(bool looping);
  bool looping() const;

  void play();
  void pause();
  void stop();
  void seek(music::Tick tick);

  State state() const;
  music::Tick position() const;
  music::Seconds position_time() const;
  std::size_t active_voices() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace notecraft::playback
