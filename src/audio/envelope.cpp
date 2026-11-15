module;

#include <algorithm>
#include <cmath>

module notecraft.audio;

namespace notecraft::audio {

Envelope::Envelope(Adsr adsr) { set_adsr(adsr); }

const Adsr& Envelope::adsr() const { return adsr_; }

void Envelope::set_adsr(Adsr adsr) {
  if (std::isfinite(adsr.attack) && adsr.attack >= 0.0) {
    adsr_.attack = adsr.attack;
  }
  if (std::isfinite(adsr.decay) && adsr.decay >= 0.0) {
    adsr_.decay = adsr.decay;
  }
  if (std::isfinite(adsr.sustain)) {
    adsr_.sustain = std::clamp(adsr.sustain, 0.0, 1.0);
  }
  if (std::isfinite(adsr.release) && adsr.release >= 0.0) {
    adsr_.release = adsr.release;
  }
}

void Envelope::note_on() {
  stage_ = Stage::Attack;
  level_ = 0.f;
  release_from_ = 0.f;
  release_elapsed_ = 0.0;
}

void Envelope::note_off() {
  if (stage_ == Stage::Idle) {
    return;
  }
  release_from_ = level_;
  release_elapsed_ = 0.0;
  stage_ = Stage::Release;
}

void Envelope::reset() {
  stage_ = Stage::Idle;
  level_ = 0.f;
  release_from_ = 0.f;
  release_elapsed_ = 0.0;
}

bool Envelope::is_active() const { return stage_ != Stage::Idle; }

float Envelope::next_level(double sample_rate) {
  if (!std::isfinite(sample_rate) || sample_rate <= 0.0 || stage_ == Stage::Idle) {
    return 0.f;
  }

  const double dt = 1.0 / sample_rate;

  switch (stage_) {
    case Stage::Attack: {
      if (adsr_.attack <= 0.0) {
        level_ = 1.f;
        stage_ = Stage::Decay;
      } else {
        level_ += static_cast<float>(dt / adsr_.attack);
        if (level_ >= 1.f) {
          level_ = 1.f;
          stage_ = Stage::Decay;
        }
      }
      break;
    }

    case Stage::Decay: {
      const auto sustain = static_cast<float>(adsr_.sustain);
      if (adsr_.decay <= 0.0) {
        level_ = sustain;
        stage_ = Stage::Sustain;
      } else {
        level_ -= static_cast<float>((1.0 - adsr_.sustain) * dt / adsr_.decay);
        if (level_ <= sustain) {
          level_ = sustain;
          stage_ = Stage::Sustain;
        }
      }
      break;
    }

    case Stage::Sustain: {
      break;
    }

    case Stage::Release: {
      if (adsr_.release <= 0.0) {
        level_ = 0.f;
        stage_ = Stage::Idle;
      } else {
        release_elapsed_ += dt;
        if (release_elapsed_ >= adsr_.release) {
          level_ = 0.f;
          stage_ = Stage::Idle;
        } else {
          level_ = release_from_ * static_cast<float>(
                                       1.0 - release_elapsed_ / adsr_.release);
        }
      }
      break;
    }

    case Stage::Idle: {
      break;
    }
  }

  return level_;
}

}  // namespace notecraft::audio
