module;

#include <SFML/Graphics.hpp>
#include <algorithm>
#include <cmath>
#include <numbers>
#include <optional>
#include <vector>

module notecraft.gui:piano_roll;

import notecraft.music;

namespace notecraft::gui {

enum class PianoCursor { Arrow, SizeHorizontal };

class PianoRoll {
public:
  explicit PianoRoll(music::Sequence& sequence) : sequence_(&sequence) {}

  void set_bounds(sf::FloatRect bounds) { bounds_ = bounds; }

  void set_playhead(std::optional<music::Tick> tick) { playhead_ = tick; }

  PianoCursor cursor_at(sf::Vector2f pos) const {
    if (drag_ && drag_->mode == DragMode::Resize) {
      return PianoCursor::SizeHorizontal;
    }
    if (!sequence_ || !bounds_.contains(pos) || erasing_) {
      return PianoCursor::Arrow;
    }
    if (const auto id = hit_test(pos)) {
      if (const auto note = sequence_->find(*id)) {
        if (on_resize_edge(note_rect(*note), pos)) {
          return PianoCursor::SizeHorizontal;
        }
      }
    }
    return PianoCursor::Arrow;
  }

  void handle_mouse_press(sf::Vector2f pos, sf::Mouse::Button button) {
    if (!sequence_ || !bounds_.contains(pos)) {
      return;
    }

    if (button == sf::Mouse::Button::Right) {
      erasing_ = true;
      erase_at(pos);
      return;
    }

    if (button != sf::Mouse::Button::Left) {
      return;
    }

    if (const auto id = hit_test(pos)) {
      const auto note = sequence_->find(*id);
      if (!note) {
        return;
      }
      const auto mode = on_resize_edge(note_rect(*note), pos) ? DragMode::Resize
                                                              : DragMode::Move;
      drag_ = DragState{mode, *id, note->start, note->duration, note->pitch, pos};
      return;
    }

    if (!grid_bounds().contains(pos)) {
      return;
    }

    const music::Note note{
        .start = tick_at(pos.x),
        .pitch = pitch_at(pos.y),
        .duration = default_duration,
        .velocity = 100,
    };
    if (const auto id = sequence_->add(note)) {
      drag_ = DragState{DragMode::Move, *id, note.start, note.duration,
                        note.pitch, pos};
    }
  }

  void handle_mouse_release(sf::Mouse::Button button) {
    if (button == sf::Mouse::Button::Right) {
      erasing_ = false;
      return;
    }
    if (button == sf::Mouse::Button::Left) {
      drag_.reset();
    }
  }

  void handle_mouse_move(sf::Vector2f pos) {
    if (!sequence_) {
      return;
    }

    if (erasing_) {
      erase_at(pos);
      return;
    }

    if (!drag_) {
      return;
    }

    const auto current = sequence_->find(drag_->id);
    if (!current) {
      drag_.reset();
      return;
    }

    music::Note updated = *current;
    const float dx = pos.x - drag_->grab_pos.x;
    const auto tick_delta = static_cast<music::Tick>(
        std::llround(dx / px_per_beat_ * music::ticks_per_beat));

    if (drag_->mode == DragMode::Resize) {
      updated.duration =
          std::max(snap, snap_tick(drag_->grab_duration + tick_delta));
    } else {
      const float dy = pos.y - drag_->grab_pos.y;
      const int pitch_delta = -static_cast<int>(std::lround(dy / row_height()));
      updated.start =
          snap_tick(std::max<music::Tick>(0, drag_->grab_start + tick_delta));
      updated.pitch = std::clamp(drag_->grab_pitch + pitch_delta,
                                 music::min_pitch, music::max_pitch);
    }

    sequence_->update(drag_->id, updated);
  }

  void handle_scroll(float delta, bool shift, bool ctrl) {
    const int steps = delta > 0.f ? 1 : -1;

    if (ctrl) {
      px_per_beat_ =
          std::clamp(px_per_beat_ * (delta > 0.f ? 1.1f : 0.9f), 12.f, 160.f);
      return;
    }

    if (shift) {
      const auto beat_shift =
          static_cast<music::Tick>(steps) * (music::ticks_per_beat / 2);
      view_start_ = std::max<music::Tick>(0, view_start_ - beat_shift);
      return;
    }

    const int min_top = music::min_pitch + visible_rows_ - 1;
    const int max_top = music::max_pitch;
    top_pitch_ = std::clamp(top_pitch_ + steps, min_top, max_top);
  }

  void update(float dt) {
    constexpr float fade_seconds = 0.18f;
    for (auto& fading : fading_) {
      fading.age += dt;
      fading.alpha = std::clamp(1.f - fading.age / fade_seconds, 0.f, 1.f);
    }
    std::erase_if(fading_, [](const FadingNote& note) {
      return note.age >= fade_seconds;
    });
  }

  void draw(sf::RenderTarget& target) const {
    sf::RectangleShape background(bounds_.size);
    background.setPosition(bounds_.position);
    background.setFillColor(sf::Color(28, 28, 34));
    target.draw(background);

    const float rh = row_height();
    for (int row = 0; row < visible_rows_; ++row) {
      const int pitch = top_pitch_ - row;
      if (pitch < music::min_pitch || pitch > music::max_pitch) {
        continue;
      }

      sf::RectangleShape key{{key_width, rh}};
      key.setPosition({bounds_.position.x, bounds_.position.y + row * rh});
      key.setFillColor(is_black_key(pitch) ? sf::Color(40, 40, 48)
                                           : sf::Color(220, 220, 230));
      key.setOutlineThickness(1.f);
      key.setOutlineColor(sf::Color(20, 20, 24));
      target.draw(key);

      sf::RectangleShape lane{{std::max(0.f, bounds_.size.x - key_width), rh}};
      lane.setPosition(
          {bounds_.position.x + key_width, bounds_.position.y + row * rh});
      lane.setFillColor(is_black_key(pitch) ? sf::Color(30, 30, 38)
                                            : sf::Color(34, 34, 42));
      target.draw(lane);
    }

    const auto grid = grid_bounds();
    const auto end_tick =
        view_start_ + static_cast<music::Tick>(std::llround(
                          (grid.size.x / px_per_beat_) * music::ticks_per_beat));
    const auto first_beat = view_start_ / music::ticks_per_beat;
    const auto last_beat = end_tick / music::ticks_per_beat + 1;

    for (music::Tick beat = first_beat; beat <= last_beat; ++beat) {
      const auto tick = beat * music::ticks_per_beat;
      const float x =
          grid.position.x +
          static_cast<float>(tick - view_start_) * px_per_beat_ /
              static_cast<float>(music::ticks_per_beat);
      if (x < grid.position.x || x > grid.position.x + grid.size.x) {
        continue;
      }
      sf::RectangleShape line{{1.f, grid.size.y}};
      line.setPosition({x, grid.position.y});
      line.setFillColor(beat % 4 == 0 ? sf::Color(80, 80, 100)
                                      : sf::Color(55, 55, 68));
      target.draw(line);
    }

    if (sequence_) {
      for (const auto& entry : sequence_->entries()) {
        const auto rect = note_rect(entry.note);
        if (rect.position.x + rect.size.x < grid.position.x ||
            rect.position.x > grid.position.x + grid.size.x) {
          continue;
        }
        draw_rounded_rect(target, rect, note_radius(rect),
                          sf::Color(80, 160, 220), sf::Color(140, 210, 255));
      }
    }

    for (const auto& fading : fading_) {
      auto fill = sf::Color(80, 160, 220);
      auto outline = sf::Color(140, 210, 255);
      fill.a = static_cast<std::uint8_t>(220.f * fading.alpha);
      outline.a = static_cast<std::uint8_t>(255.f * fading.alpha);
      const float shrink = (1.f - fading.alpha) * 4.f;
      sf::FloatRect rect{
          {fading.rect.position.x + shrink, fading.rect.position.y + shrink * 0.5f},
          {std::max(1.f, fading.rect.size.x - 2.f * shrink),
           std::max(1.f, fading.rect.size.y - shrink)}};
      draw_rounded_rect(target, rect, note_radius(rect), fill, outline);
    }

    if (playhead_) {
      const float x =
          grid.position.x +
          static_cast<float>(*playhead_ - view_start_) * px_per_beat_ /
              static_cast<float>(music::ticks_per_beat);
      if (x >= grid.position.x && x <= grid.position.x + grid.size.x) {
        sf::RectangleShape line{{2.f, grid.size.y}};
        line.setPosition({x, grid.position.y});
        line.setFillColor(sf::Color(255, 90, 90));
        target.draw(line);
      }
    }
  }

private:
  static constexpr float key_width = 28.f;
  static constexpr float resize_handle = 8.f;
  static constexpr float corner_radius = 4.f;
  static constexpr music::Tick snap = music::ticks_per_beat / 4;
  static constexpr music::Tick default_duration = music::ticks_per_beat;

  enum class DragMode { Move, Resize };

  struct DragState {
    DragMode mode = DragMode::Move;
    music::NoteId id = 0;
    music::Tick grab_start = 0;
    music::Tick grab_duration = default_duration;
    int grab_pitch = 60;
    sf::Vector2f grab_pos;
  };

  struct FadingNote {
    sf::FloatRect rect;
    float age = 0.f;
    float alpha = 1.f;
  };

  static float note_radius(sf::FloatRect rect) {
    return std::min({corner_radius, rect.size.x * 0.5f, rect.size.y * 0.5f});
  }

  static void draw_rounded_rect(sf::RenderTarget& target,
                                sf::FloatRect rect,
                                float radius,
                                sf::Color fill,
                                sf::Color outline) {
    if (radius < 0.5f) {
      sf::RectangleShape shape(rect.size);
      shape.setPosition(rect.position);
      shape.setFillColor(fill);
      shape.setOutlineThickness(1.f);
      shape.setOutlineColor(outline);
      target.draw(shape);
      return;
    }

    constexpr unsigned points_per_corner = 8;
    const unsigned count = points_per_corner * 4;
    sf::ConvexShape shape(count);

    const float x = rect.position.x;
    const float y = rect.position.y;
    const float w = rect.size.x;
    const float h = rect.size.y;
    const float left = x + radius;
    const float right = x + w - radius;
    const float top = y + radius;
    const float bottom = y + h - radius;
    constexpr float pi = std::numbers::pi_v<float>;

    auto set_corner = [&](unsigned index_base, float cx, float cy,
                          float start_angle) {
      for (unsigned i = 0; i < points_per_corner; ++i) {
        const float t =
            static_cast<float>(i) / static_cast<float>(points_per_corner - 1);
        const float angle = start_angle + t * (pi * 0.5f);
        shape.setPoint(index_base + i,
                       {cx + std::cos(angle) * radius,
                        cy + std::sin(angle) * radius});
      }
    };

    // Clockwise from top-right, equal arcs on every corner.
    set_corner(0, right, top, -pi * 0.5f);
    set_corner(points_per_corner, right, bottom, 0.f);
    set_corner(points_per_corner * 2, left, bottom, pi * 0.5f);
    set_corner(points_per_corner * 3, left, top, pi);

    shape.setFillColor(fill);
    shape.setOutlineThickness(1.f);
    shape.setOutlineColor(outline);
    target.draw(shape);
  }

  void erase_at(sf::Vector2f pos) {
    if (!sequence_) {
      return;
    }
    if (const auto id = hit_test(pos)) {
      if (const auto note = sequence_->find(*id)) {
        fading_.push_back(FadingNote{note_rect(*note)});
        sequence_->remove(*id);
      }
    }
  }

  bool on_resize_edge(sf::FloatRect rect, sf::Vector2f pos) const {
    const float edge = rect.position.x + rect.size.x;
    return pos.x >= edge - resize_handle && pos.x <= edge + 2.f &&
           pos.y >= rect.position.y && pos.y <= rect.position.y + rect.size.y;
  }

  sf::FloatRect grid_bounds() const {
    return {{bounds_.position.x + key_width, bounds_.position.y},
            {std::max(0.f, bounds_.size.x - key_width), bounds_.size.y}};
  }

  float row_height() const {
    return bounds_.size.y / static_cast<float>(std::max(1, visible_rows_));
  }

  music::Tick snap_tick(music::Tick tick) const {
    if (tick < 0) {
      return 0;
    }
    return (tick / snap) * snap;
  }

  int pitch_at(float y) const {
    const float local = y - bounds_.position.y;
    const int row = static_cast<int>(std::floor(local / row_height()));
    return std::clamp(top_pitch_ - row, music::min_pitch, music::max_pitch);
  }

  music::Tick tick_at(float x) const {
    const auto grid = grid_bounds();
    const float local = x - grid.position.x;
    const double beats = local / px_per_beat_;
    const auto ticks = static_cast<music::Tick>(
                           std::llround(beats * music::ticks_per_beat)) +
                       view_start_;
    return snap_tick(std::max<music::Tick>(0, ticks));
  }

  sf::FloatRect note_rect(const music::Note& note) const {
    const auto grid = grid_bounds();
    const float x =
        grid.position.x +
        static_cast<float>(note.start - view_start_) * px_per_beat_ /
            static_cast<float>(music::ticks_per_beat);
    const float w = static_cast<float>(note.duration) * px_per_beat_ /
                    static_cast<float>(music::ticks_per_beat);
    const float y = bounds_.position.y +
                    static_cast<float>(top_pitch_ - note.pitch) * row_height();
    return {{x, y}, {std::max(4.f, w), std::max(4.f, row_height() - 1.f)}};
  }

  std::optional<music::NoteId> hit_test(sf::Vector2f pos) const {
    if (!sequence_) {
      return std::nullopt;
    }
    for (const auto& entry : sequence_->entries()) {
      if (note_rect(entry.note).contains(pos)) {
        return entry.id;
      }
    }
    return std::nullopt;
  }

  bool is_black_key(int pitch) const {
    switch (pitch % 12) {
      case 1:
      case 3:
      case 6:
      case 8:
      case 10:
        return true;
      default:
        return false;
    }
  }

  music::Sequence* sequence_ = nullptr;
  sf::FloatRect bounds_{};
  float px_per_beat_ = 48.f;
  int top_pitch_ = 84;
  int visible_rows_ = 36;
  music::Tick view_start_ = 0;
  std::optional<DragState> drag_;
  std::optional<music::Tick> playhead_;
  bool erasing_ = false;
  std::vector<FadingNote> fading_;
};

}  // namespace notecraft::gui
