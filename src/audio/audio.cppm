module;

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <span>
#include <vector>

export module notecraft.audio;

export namespace notecraft::audio {

enum class Waveform { Sine, Square, Saw };

inline constexpr unsigned default_sample_rate = 44100;
inline constexpr int concert_a_pitch = 69;
inline constexpr double concert_a_hz = 440.0;

struct Adsr {
  double attack = 0.01;
  double decay = 0.1;
  double sustain = 0.7;
  double release = 0.2;
};

template <typename G>
concept SampleSource = requires(G& generator, double sample_rate) {
  { generator.next_sample(sample_rate) } -> std::convertible_to<float>;
};

double pitch_to_frequency(int pitch);

class Oscillator {
public:
  explicit Oscillator(Waveform waveform = Waveform::Sine,
                      double frequency = concert_a_hz);

  Waveform waveform() const;
  void set_waveform(Waveform waveform);

  double frequency() const;
  void set_frequency(double frequency);

  void reset();
  float next_sample(double sample_rate);

private:
  Waveform waveform_ = Waveform::Sine;
  double frequency_ = concert_a_hz;
  double phase_ = 0.0;
};

class Envelope {
public:
  explicit Envelope(Adsr adsr = {});

  const Adsr& adsr() const;
  void set_adsr(Adsr adsr);

  void note_on();
  void note_off();
  void reset();

  float next_level(double sample_rate);
  bool is_active() const;

private:
  enum class Stage { Idle, Attack, Decay, Sustain, Release };

  Adsr adsr_{};
  Stage stage_ = Stage::Idle;
  float level_ = 0.f;
  float release_from_ = 0.f;
  double release_elapsed_ = 0.0;
};

struct VoiceSettings {
  Waveform waveform = Waveform::Sine;
  Adsr adsr{};
  float gain = 0.3f;
};

class Voice {
public:
  void start(std::uint64_t key,
             double frequency,
             float velocity,
             const VoiceSettings& settings);
  void release();
  void kill();

  bool is_active() const;
  bool is_released() const;
  std::uint64_t key() const;

  float next_sample(double sample_rate);

private:
  Oscillator oscillator_;
  Envelope envelope_;
  std::uint64_t key_ = 0;
  float amplitude_ = 0.f;
  bool released_ = false;
};

class Synth {
public:
  static constexpr std::size_t default_max_voices = 32;

  explicit Synth(std::size_t max_voices = default_max_voices);

  void note_on(std::uint64_t key,
               double frequency,
               float velocity,
               const VoiceSettings& settings);
  void note_off(std::uint64_t key);
  void release_all();
  void reset();

  std::size_t max_voices() const;
  std::size_t active_voices() const;

  float master_gain() const;
  void set_master_gain(float gain);

  float next_sample(double sample_rate);
  void render(std::span<std::int16_t> output, double sample_rate);

private:
  struct Slot {
    Voice voice;
    std::uint64_t order = 0;
  };

  Slot& pick_slot();

  std::vector<Slot> slots_;
  std::uint64_t counter_ = 0;
  float master_gain_ = 1.f;
};

std::vector<std::int16_t> render_tone(double frequency,
                                      double hold_seconds,
                                      unsigned sample_rate = default_sample_rate,
                                      Waveform waveform = Waveform::Sine,
                                      const Adsr& adsr = {},
                                      float amplitude = 0.3f);

class AudioEngine {
public:
  explicit AudioEngine(unsigned sample_rate = default_sample_rate);
  ~AudioEngine();

  AudioEngine(const AudioEngine&) = delete;
  AudioEngine& operator=(const AudioEngine&) = delete;

  AudioEngine(AudioEngine&&) noexcept;
  AudioEngine& operator=(AudioEngine&&) noexcept;

  unsigned sample_rate() const;

  bool play_samples(const std::vector<std::int16_t>& samples);

  bool play_tone(double frequency,
                 double hold_seconds,
                 Waveform waveform = Waveform::Sine,
                 const Adsr& adsr = {},
                 float amplitude = 0.3f);
  bool play_tone(int pitch,
                 double hold_seconds,
                 Waveform waveform = Waveform::Sine,
                 const Adsr& adsr = {},
                 float amplitude = 0.3f);

  void stop();
  bool is_playing() const;

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace notecraft::audio
