module;

#include <compare>
#include <memory>
#include <string>
#include <utility>

export module notecraft.gui;

export import notecraft.music;

export namespace notecraft::gui {

class MainWindow {
public:
  explicit MainWindow(music::Sequence sequence,
                      std::string title = "NoteCraft",
                      unsigned width = 1280,
                      unsigned height = 720);
  ~MainWindow();

  MainWindow(const MainWindow&) = delete;
  MainWindow& operator=(const MainWindow&) = delete;

  MainWindow(MainWindow&&) noexcept;
  MainWindow& operator=(MainWindow&&) noexcept;

  void run();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace notecraft::gui
