module;

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <optional>
#include <string>
#include <vector>

module notecraft.gui:control_panel;

import notecraft.instruments;
import notecraft.playback;

namespace notecraft::gui {

enum class ControlAction {
  None,
  Play,
  Pause,
  Stop,
  BpmDown,
  BpmUp,
  Instrument0,
  Instrument1,
  Instrument2,
  ToggleMixer,
  ToggleMenu,
  MenuClose,
};

class ControlPanel {
public:
  static constexpr float toolbar_h = 56.f;
  static constexpr float mixer_w = 300.f;

  void set_window_size(sf::Vector2f size) {
    window_size_ = size;
    rebuild();
  }

  void set_font(const sf::Font* font) { font_ = font; }

  void set_status(playback::State state,
                  double bpm,
                  const std::array<instruments::Instrument, 3>& instruments,
                  int instrument_index,
                  bool mixer_open,
                  bool menu_open) {
    const bool layout_changed =
        mixer_open_ != mixer_open || menu_open_ != menu_open ||
        instrument_index_ != instrument_index ||
        window_size_.x <= 0.f;

    state_ = state;
    bpm_ = bpm;
    instruments_ = instruments;
    instrument_index_ = std::clamp(instrument_index, 0, 2);
    mixer_open_ = mixer_open;
    menu_open_ = menu_open;

    if (layout_changed || buttons_.empty()) {
      rebuild();
      return;
    }

    sync_slider_values();
    rebuild_toolbar_only();
  }

  bool mixer_open() const { return mixer_open_; }

  sf::FloatRect toolbar_bounds() const {
    return {{0.f, 0.f}, {window_size_.x, toolbar_h}};
  }

  sf::FloatRect mixer_bounds() const {
    if (!mixer_open_) {
      return {};
    }
    return {{window_size_.x - mixer_w, toolbar_h},
            {mixer_w, std::max(0.f, window_size_.y - toolbar_h)}};
  }

  sf::FloatRect piano_bounds() const {
    const float right = mixer_open_ ? mixer_w : 0.f;
    return {{0.f, toolbar_h},
            {std::max(0.f, window_size_.x - right),
             std::max(0.f, window_size_.y - toolbar_h)}};
  }

  ControlAction handle_mouse_press(sf::Vector2f pos) {
    for (const auto& button : buttons_) {
      if (button.rect.contains(pos)) {
        return button.action;
      }
    }

    if (mixer_open_ && mixer_bounds().contains(pos)) {
      for (auto& slider : sliders_) {
        if (hit_rect(slider).contains(pos)) {
          dragging_ = slider.id;
          set_slider_from_y(slider, pos.y);
          return ControlAction::None;
        }
      }
    }
    return ControlAction::None;
  }

  bool handle_mouse_drag(sf::Vector2f pos) {
    if (!dragging_) {
      return false;
    }
    if (auto* slider = find_slider(*dragging_)) {
      set_slider_from_y(*slider, pos.y);
      return true;
    }
    dragging_.reset();
    return false;
  }

  void handle_mouse_release() { dragging_.reset(); }

  bool take_instrument_edit(instruments::Instrument& out) {
    if (!instrument_dirty_) {
      return false;
    }
    out = instruments_[static_cast<std::size_t>(instrument_index_)];
    instrument_dirty_ = false;
    return true;
  }

  void draw(sf::RenderTarget& target) const {
    sf::RectangleShape toolbar(toolbar_bounds().size);
    toolbar.setPosition(toolbar_bounds().position);
    toolbar.setFillColor(sf::Color(22, 22, 28));
    target.draw(toolbar);

    for (const auto& button : buttons_) {
      if (button.in_menu && !menu_open_) {
        continue;
      }
      draw_button(target, button);
    }

    if (mixer_open_) {
      draw_mixer(target);
    }
  }

private:
  enum class SliderId { Attack, Decay, Sustain, Release, Gain };

  struct Button {
    sf::FloatRect rect;
    ControlAction action = ControlAction::None;
    std::string label;
    bool active = false;
    bool in_menu = false;
  };

  struct Slider {
    SliderId id = SliderId::Attack;
    sf::FloatRect track;
    float value = 0.f;
    float min = 0.f;
    float max = 1.f;
    const char* label = "";
  };

  static sf::FloatRect hit_rect(const Slider& slider) {
    constexpr float pad = 10.f;
    return {{slider.track.position.x - pad, slider.track.position.y - 4.f},
            {slider.track.size.x + 2.f * pad, slider.track.size.y + 8.f}};
  }

  Slider* find_slider(SliderId id) {
    for (auto& slider : sliders_) {
      if (slider.id == id) {
        return &slider;
      }
    }
    return nullptr;
  }

  void add_button(float& x,
                  float y,
                  float w,
                  float h,
                  ControlAction action,
                  std::string label,
                  bool active = false,
                  bool in_menu = false) {
    buttons_.push_back(Button{
        {{x, y}, {w, h}}, action, std::move(label), active, in_menu});
    if (!in_menu) {
      x += w + 6.f;
    }
  }

  void rebuild_toolbar_only() {
    buttons_.clear();
    float x = 10.f;
    const float y = 12.f;
    const float h = 32.f;

    add_button(x, y, 56.f, h, ControlAction::Play, "Play",
               state_ == playback::State::Playing);
    add_button(x, y, 64.f, h, ControlAction::Pause, "Pause",
               state_ == playback::State::Paused);
    add_button(x, y, 56.f, h, ControlAction::Stop, "Stop",
               state_ == playback::State::Stopped);
    x += 8.f;

    add_button(x, y, 28.f, h, ControlAction::BpmDown, "-");
    char bpm_label[32];
    std::snprintf(bpm_label, sizeof(bpm_label), "BPM %d",
                  static_cast<int>(std::lround(bpm_)));
    add_button(x, y, 78.f, h, ControlAction::None, bpm_label);
    add_button(x, y, 28.f, h, ControlAction::BpmUp, "+");
    x += 8.f;

    add_button(x, y, 64.f, h, ControlAction::ToggleMenu, "Menu", menu_open_);
    x += 8.f;

    constexpr std::array names{"Piano", "Bass", "Synth"};
    for (int i = 0; i < 3; ++i) {
      add_button(x, y, 58.f, h,
                 static_cast<ControlAction>(
                     static_cast<int>(ControlAction::Instrument0) + i),
                 names[static_cast<std::size_t>(i)], instrument_index_ == i);
    }
    x += 8.f;

    add_button(x, y, 70.f, h, ControlAction::ToggleMixer, "Mixer", mixer_open_);

    if (menu_open_) {
      float menu_x = 250.f;
      float menu_y = toolbar_h;
      const float item_h = 30.f;
      add_button(menu_x, menu_y, 140.f, item_h, ControlAction::ToggleMixer,
                 mixer_open_ ? "Hide Mixer" : "Show Mixer", false, true);
      menu_y += item_h + 2.f;
      add_button(menu_x, menu_y, 140.f, item_h, ControlAction::MenuClose, "Close",
                 false, true);
    }
  }

  void sync_slider_values() {
    if (sliders_.empty()) {
      return;
    }
    const auto& instrument =
        instruments_[static_cast<std::size_t>(instrument_index_)];
    for (auto& slider : sliders_) {
      if (dragging_ && *dragging_ == slider.id) {
        continue;
      }
      switch (slider.id) {
        case SliderId::Attack:
          slider.value = static_cast<float>(instrument.adsr.attack);
          break;
        case SliderId::Decay:
          slider.value = static_cast<float>(instrument.adsr.decay);
          break;
        case SliderId::Sustain:
          slider.value = static_cast<float>(instrument.adsr.sustain);
          break;
        case SliderId::Release:
          slider.value = static_cast<float>(instrument.adsr.release);
          break;
        case SliderId::Gain:
          slider.value = instrument.gain;
          break;
      }
    }
  }

  void rebuild() {
    const auto keep_drag = dragging_;
    sliders_.clear();
    rebuild_toolbar_only();

    if (!mixer_open_) {
      dragging_.reset();
      return;
    }

    const auto& instrument =
        instruments_[static_cast<std::size_t>(instrument_index_)];
    const auto panel = mixer_bounds();
    const float left = panel.position.x + 28.f;
    const float top = panel.position.y + 120.f;
    const float track_h = std::max(80.f, panel.size.y - 180.f);
    const float gap = 52.f;

    auto add_slider = [&](SliderId id, const char* label, float value, float min,
                          float max, int column) {
      Slider slider;
      slider.id = id;
      slider.label = label;
      slider.value = value;
      slider.min = min;
      slider.max = max;
      slider.track = {{left + column * gap, top}, {18.f, track_h}};
      sliders_.push_back(slider);
    };

    add_slider(SliderId::Attack, "A", static_cast<float>(instrument.adsr.attack),
               0.f, 2.f, 0);
    add_slider(SliderId::Decay, "D", static_cast<float>(instrument.adsr.decay),
               0.f, 2.f, 1);
    add_slider(SliderId::Sustain, "S",
               static_cast<float>(instrument.adsr.sustain), 0.f, 1.f, 2);
    add_slider(SliderId::Release, "R",
               static_cast<float>(instrument.adsr.release), 0.f, 3.f, 3);
    add_slider(SliderId::Gain, "Vol", instrument.gain, 0.f, 1.f, 4);

    dragging_ = keep_drag;
    if (dragging_ && find_slider(*dragging_) == nullptr) {
      dragging_.reset();
    }
  }

  void set_slider_from_y(Slider& slider, float y) {
    const float t = std::clamp(
        (slider.track.position.y + slider.track.size.y - y) / slider.track.size.y,
        0.f, 1.f);
    slider.value = slider.min + t * (slider.max - slider.min);

    auto& instrument = instruments_[static_cast<std::size_t>(instrument_index_)];
    switch (slider.id) {
      case SliderId::Attack:
        instrument.adsr.attack = slider.value;
        break;
      case SliderId::Decay:
        instrument.adsr.decay = slider.value;
        break;
      case SliderId::Sustain:
        instrument.adsr.sustain = slider.value;
        break;
      case SliderId::Release:
        instrument.adsr.release = slider.value;
        break;
      case SliderId::Gain:
        instrument.gain = slider.value;
        break;
    }
    instrument_dirty_ = true;
  }

  void draw_button(sf::RenderTarget& target, const Button& button) const {
    sf::RectangleShape shape(button.rect.size);
    shape.setPosition(button.rect.position);
    shape.setFillColor(button.active ? sf::Color(70, 120, 180)
                                     : sf::Color(45, 45, 55));
    shape.setOutlineThickness(1.f);
    shape.setOutlineColor(sf::Color(90, 90, 110));
    target.draw(shape);

    if (font_ == nullptr || button.label.empty()) {
      return;
    }
    sf::Text text(*font_, button.label, 13);
    text.setFillColor(sf::Color(230, 230, 235));
    const auto bounds = text.getLocalBounds();
    text.setPosition({button.rect.position.x +
                          (button.rect.size.x - bounds.size.x) * 0.5f -
                          bounds.position.x,
                      button.rect.position.y +
                          (button.rect.size.y - bounds.size.y) * 0.5f -
                          bounds.position.y});
    target.draw(text);
  }

  void draw_mixer(sf::RenderTarget& target) const {
    const auto panel = mixer_bounds();
    sf::RectangleShape background(panel.size);
    background.setPosition(panel.position);
    background.setFillColor(sf::Color(18, 18, 24));
    background.setOutlineThickness(1.f);
    background.setOutlineColor(sf::Color(60, 60, 80));
    target.draw(background);

    if (font_ != nullptr) {
      const auto& instrument =
          instruments_[static_cast<std::size_t>(instrument_index_)];
      sf::Text title(*font_, std::string(instrument.name) + " Mixer", 18);
      title.setFillColor(sf::Color(240, 240, 245));
      title.setPosition({panel.position.x + 16.f, panel.position.y + 16.f});
      target.draw(title);

      sf::Text hint(*font_, "ADSR / Volume", 13);
      hint.setFillColor(sf::Color(160, 160, 175));
      hint.setPosition({panel.position.x + 16.f, panel.position.y + 44.f});
      target.draw(hint);
    }

    draw_envelope_preview(target, panel);

    for (const auto& slider : sliders_) {
      sf::RectangleShape track(slider.track.size);
      track.setPosition(slider.track.position);
      track.setFillColor(sf::Color(35, 35, 45));
      track.setOutlineThickness(1.f);
      track.setOutlineColor(sf::Color(70, 70, 90));
      target.draw(track);

      const float t =
          (slider.value - slider.min) / std::max(0.0001f, slider.max - slider.min);
      const float fill_h = slider.track.size.y * t;
      sf::RectangleShape fill{{slider.track.size.x, fill_h}};
      fill.setPosition({slider.track.position.x,
                        slider.track.position.y + slider.track.size.y - fill_h});
      fill.setFillColor(sf::Color(255, 140, 60));
      target.draw(fill);

      sf::CircleShape knob(7.f);
      knob.setOrigin({7.f, 7.f});
      knob.setPosition({slider.track.position.x + slider.track.size.x * 0.5f,
                        slider.track.position.y + slider.track.size.y - fill_h});
      knob.setFillColor(sf::Color(255, 200, 140));
      knob.setOutlineThickness(1.f);
      knob.setOutlineColor(sf::Color(255, 160, 80));
      target.draw(knob);

      if (font_ != nullptr) {
        sf::Text label(*font_, slider.label, 14);
        label.setFillColor(sf::Color(220, 220, 230));
        label.setPosition(
            {slider.track.position.x + 3.f,
             slider.track.position.y + slider.track.size.y + 8.f});
        target.draw(label);

        char value[16];
        std::snprintf(value, sizeof(value), "%.2f", slider.value);
        sf::Text value_text(*font_, value, 11);
        value_text.setFillColor(sf::Color(180, 180, 190));
        value_text.setPosition(
            {slider.track.position.x - 2.f, slider.track.position.y - 18.f});
        target.draw(value_text);
      }
    }
  }

  void draw_envelope_preview(sf::RenderTarget& target,
                             sf::FloatRect panel) const {
    const auto& adsr =
        instruments_[static_cast<std::size_t>(instrument_index_)].adsr;
    const sf::FloatRect box{{panel.position.x + 16.f, panel.position.y + 68.f},
                            {panel.size.x - 32.f, 40.f}};
    sf::RectangleShape frame(box.size);
    frame.setPosition(box.position);
    frame.setFillColor(sf::Color(28, 28, 36));
    frame.setOutlineThickness(1.f);
    frame.setOutlineColor(sf::Color(70, 70, 90));
    target.draw(frame);

    const float total = static_cast<float>(
        std::max(0.05, adsr.attack + adsr.decay + 0.35 + adsr.release));
    auto x_at = [&](double seconds) {
      return box.position.x +
             box.size.x * static_cast<float>(seconds / total);
    };
    auto y_at = [&](double level) {
      return box.position.y + box.size.y * (1.f - static_cast<float>(level));
    };

    sf::VertexArray line(sf::PrimitiveType::LineStrip, 5);
    line[0].position = {x_at(0.0), y_at(0.0)};
    line[1].position = {x_at(adsr.attack), y_at(1.0)};
    line[2].position = {x_at(adsr.attack + adsr.decay), y_at(adsr.sustain)};
    line[3].position = {
        x_at(adsr.attack + adsr.decay + 0.35), y_at(adsr.sustain)};
    line[4].position = {
        x_at(adsr.attack + adsr.decay + 0.35 + adsr.release), y_at(0.0)};
    for (std::size_t i = 0; i < line.getVertexCount(); ++i) {
      line[i].color = sf::Color(255, 150, 70);
    }
    target.draw(line);
  }

  sf::Vector2f window_size_{1280.f, 720.f};
  const sf::Font* font_ = nullptr;
  playback::State state_ = playback::State::Stopped;
  double bpm_ = 120.0;
  std::array<instruments::Instrument, 3> instruments_{
      instruments::piano(), instruments::bass(), instruments::synth()};
  int instrument_index_ = 0;
  bool mixer_open_ = true;
  bool menu_open_ = false;
  bool instrument_dirty_ = false;
  std::vector<Button> buttons_;
  std::vector<Slider> sliders_;
  std::optional<SliderId> dragging_;
};

}  // namespace notecraft::gui
