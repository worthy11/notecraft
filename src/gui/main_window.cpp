module;

#include <SFML/Graphics.hpp>
#include <array>
#include <utility>

module notecraft.gui;

import :control_panel;
import :piano_roll;
import notecraft.playback;

namespace notecraft::gui {

namespace {

bool load_ui_font(sf::Font& font) {
  constexpr std::array paths{
      "C:/Windows/Fonts/segoeui.ttf",
      "C:/Windows/Fonts/arial.ttf",
      "C:/Windows/Fonts/calibri.ttf",
  };
  for (const char* path : paths) {
    if (font.openFromFile(path)) {
      return true;
    }
  }
  return false;
}

}  // namespace

struct MainWindow::Impl {
  sf::RenderWindow window;
  sf::Font font;
  bool has_font = false;
  music::Sequence sequence;
  PianoRoll piano_roll;
  ControlPanel control_panel;
  playback::Player player;
  std::array<instruments::Instrument, 3> instruments{
      instruments::piano(), instruments::bass(), instruments::synth()};
  int instrument_index = 0;
  bool mixer_open = true;
  bool menu_open = false;
  sf::Clock frame_clock;
  std::optional<sf::Cursor> arrow_cursor;
  std::optional<sf::Cursor> resize_cursor;
  PianoCursor current_cursor = PianoCursor::Arrow;

  explicit Impl(music::Sequence seq)
      : sequence(std::move(seq)), piano_roll(sequence) {
    has_font = load_ui_font(font);
    if (has_font) {
      control_panel.set_font(&font);
    }
    player.set_bpm(sequence.tempo().bpm());
    player.set_instrument(instruments[0]);
    arrow_cursor = sf::Cursor::createFromSystem(sf::Cursor::Type::Arrow);
    resize_cursor =
        sf::Cursor::createFromSystem(sf::Cursor::Type::SizeHorizontal);
  }

  instruments::Instrument& current_instrument() {
    return instruments[static_cast<std::size_t>(instrument_index)];
  }

  void layout() {
    const auto size = window.getSize();
    control_panel.set_window_size(
        {static_cast<float>(size.x), static_cast<float>(size.y)});
    refresh_controls();
    piano_roll.set_bounds(control_panel.piano_bounds());
  }

  void refresh_controls() {
    control_panel.set_status(player.state(), sequence.tempo().bpm(), instruments,
                             instrument_index, mixer_open, menu_open);
  }

  void sync_instrument_from_panel() {
    instruments::Instrument edited;
    if (control_panel.take_instrument_edit(edited)) {
      instruments[static_cast<std::size_t>(instrument_index)] = edited;
      player.set_instrument(edited);
    }
  }

  void update_cursor(sf::Vector2f pos) {
    const auto wanted = piano_roll.cursor_at(pos);
    if (wanted == current_cursor) {
      return;
    }
    current_cursor = wanted;
    if (wanted == PianoCursor::SizeHorizontal && resize_cursor) {
      window.setMouseCursor(*resize_cursor);
    } else if (arrow_cursor) {
      window.setMouseCursor(*arrow_cursor);
    }
  }

  void apply(ControlAction action) {
    switch (action) {
      case ControlAction::Play:
        player.set_sequence(sequence);
        player.set_instrument(current_instrument());
        player.set_bpm(sequence.tempo().bpm());
        player.play();
        break;
      case ControlAction::Pause:
        player.pause();
        break;
      case ControlAction::Stop:
        player.stop();
        piano_roll.set_playhead(std::nullopt);
        break;
      case ControlAction::BpmDown:
        sequence.tempo().set_bpm(sequence.tempo().bpm() - 5.0);
        player.set_bpm(sequence.tempo().bpm());
        break;
      case ControlAction::BpmUp:
        sequence.tempo().set_bpm(sequence.tempo().bpm() + 5.0);
        player.set_bpm(sequence.tempo().bpm());
        break;
      case ControlAction::Instrument0:
      case ControlAction::Instrument1:
      case ControlAction::Instrument2:
        instrument_index = static_cast<int>(action) -
                           static_cast<int>(ControlAction::Instrument0);
        player.set_instrument(current_instrument());
        break;
      case ControlAction::ToggleMixer:
        mixer_open = !mixer_open;
        menu_open = false;
        layout();
        return;
      case ControlAction::ToggleMenu:
        menu_open = !menu_open;
        break;
      case ControlAction::MenuClose:
        menu_open = false;
        break;
      case ControlAction::None:
        break;
    }
    refresh_controls();
  }
};

MainWindow::MainWindow(music::Sequence sequence,
                       std::string title,
                       unsigned width,
                       unsigned height)
    : impl_(std::make_unique<Impl>(std::move(sequence))) {
  impl_->window.create(sf::VideoMode({width, height}), std::move(title));
  impl_->layout();
}

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(MainWindow&&) noexcept = default;
MainWindow& MainWindow::operator=(MainWindow&&) noexcept = default;

void MainWindow::run() {
  while (impl_->window.isOpen()) {
    while (const std::optional event = impl_->window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        impl_->window.close();
      } else if (event->is<sf::Event::Resized>()) {
        const auto size = event->getIf<sf::Event::Resized>()->size;
        impl_->window.setView(sf::View(sf::FloatRect(
            {0.f, 0.f},
            {static_cast<float>(size.x), static_cast<float>(size.y)})));
        impl_->layout();
      } else if (const auto* pressed =
                     event->getIf<sf::Event::MouseButtonPressed>()) {
        const auto pos = sf::Vector2f(pressed->position);
        if (pressed->button == sf::Mouse::Button::Left) {
          const auto action = impl_->control_panel.handle_mouse_press(pos);
          if (action != ControlAction::None ||
              impl_->control_panel.mixer_bounds().contains(pos) ||
              impl_->control_panel.toolbar_bounds().contains(pos)) {
            impl_->apply(action);
            impl_->sync_instrument_from_panel();
            impl_->refresh_controls();
          } else {
            impl_->piano_roll.handle_mouse_press(pos, pressed->button);
          }
        } else if (!impl_->control_panel.toolbar_bounds().contains(pos) &&
                   !impl_->control_panel.mixer_bounds().contains(pos)) {
          impl_->piano_roll.handle_mouse_press(pos, pressed->button);
        }
        impl_->update_cursor(pos);
      } else if (const auto* released =
                     event->getIf<sf::Event::MouseButtonReleased>()) {
        impl_->control_panel.handle_mouse_release();
        impl_->piano_roll.handle_mouse_release(released->button);
        impl_->sync_instrument_from_panel();
        impl_->refresh_controls();
        impl_->update_cursor(sf::Vector2f(released->position));
      } else if (const auto* moved = event->getIf<sf::Event::MouseMoved>()) {
        const auto pos = sf::Vector2f(moved->position);
        if (impl_->control_panel.handle_mouse_drag(pos)) {
          impl_->sync_instrument_from_panel();
          impl_->refresh_controls();
        } else {
          impl_->piano_roll.handle_mouse_move(pos);
        }
        impl_->update_cursor(pos);
      } else if (const auto* wheel =
                     event->getIf<sf::Event::MouseWheelScrolled>()) {
        if (impl_->control_panel.piano_bounds().contains(
                sf::Vector2f(wheel->position))) {
          const bool shift =
              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LShift) ||
              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RShift);
          const bool ctrl =
              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::LControl) ||
              sf::Keyboard::isKeyPressed(sf::Keyboard::Key::RControl);
          impl_->piano_roll.handle_scroll(wheel->delta, shift, ctrl);
        }
      }
    }

    const float dt = impl_->frame_clock.restart().asSeconds();
    impl_->piano_roll.update(dt);

    if (impl_->player.state() == playback::State::Playing) {
      impl_->piano_roll.set_playhead(impl_->player.position());
    }
    impl_->refresh_controls();

    impl_->window.clear(sf::Color(30, 30, 35));
    impl_->piano_roll.draw(impl_->window);
    impl_->control_panel.draw(impl_->window);
    impl_->window.display();
  }
}

}  // namespace notecraft::gui
