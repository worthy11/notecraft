import notecraft.audio;
import notecraft.gui;
import notecraft.instruments;

int main() {
  notecraft::audio::AudioEngine audio;
  const auto piano = notecraft::instruments::piano();
  constexpr int c_major[] = {60, 62, 64, 65, 67, 69, 71, 72};

  for (const int pitch : c_major) {
    notecraft::instruments::play(audio, piano, pitch, 0.1);
  }

  // notecraft::gui::MainWindow window;
  // window.run();
  return 0;
}
