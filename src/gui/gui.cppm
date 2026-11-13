module;

#include <memory>
#include <string>

export module notecraft.gui;

export namespace notecraft::gui {

class MainWindow {
public:
  explicit MainWindow(std::string title = "NoteCraft",
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
