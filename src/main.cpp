#include <chrono>
#include <thread>

import notecraft.gui;
import notecraft.playback;

namespace {

notecraft::music::Sequence demo_song() {
  using notecraft::music::Note;
  using notecraft::music::ticks_per_beat;

  notecraft::music::Sequence song;
  song.tempo().set_bpm(110);

  auto add = [&song](double beat, double beats, int pitch, int velocity) {
    song.add(Note{
        .start = static_cast<notecraft::music::Tick>(beat * ticks_per_beat),
        .pitch = pitch,
        .duration = static_cast<notecraft::music::Tick>(beats * ticks_per_beat),
        .velocity = velocity,
    });
  };

  constexpr int chords[][3] = {{48, 52, 55}, {45, 48, 52}, {41, 45, 48}, {43, 47, 50}};
  for (int bar = 0; bar < 4; ++bar) {
    for (const int pitch : chords[bar]) {
      add(bar * 4.0, 4.0, pitch, 70);
    }
  }

  constexpr int melody[] = {72, 74, 76, 79, 76, 74, 72, 69,
                            69, 72, 74, 72, 71, 72, 74, 79};
  for (int i = 0; i < 16; ++i) {
    add(i * 1.0, 1.0, melody[i], 110);
  }

  return song;
}

}  // namespace

int main() {
  notecraft::playback::Player player;
  player.set_instrument(notecraft::instruments::piano());
  player.set_sequence(demo_song());
  player.play();

  while (player.state() != notecraft::playback::State::Stopped) {
    std::this_thread::sleep_for(std::chrono::milliseconds(20));
  }
  std::this_thread::sleep_for(std::chrono::milliseconds(500));

  // notecraft::gui::MainWindow window;
  // window.run();
  return 0;
}
