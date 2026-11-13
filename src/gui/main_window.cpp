module;

#include <SFML/Graphics.hpp>
#include <utility>

module notecraft.gui;

namespace notecraft::gui {

struct MainWindow::Impl {
  sf::RenderWindow window;
};

MainWindow::MainWindow(std::string title, unsigned width, unsigned height)
    : impl_(std::make_unique<Impl>()) {
  impl_->window.create(sf::VideoMode({width, height}), std::move(title));
}

MainWindow::~MainWindow() = default;

MainWindow::MainWindow(MainWindow&&) noexcept = default;
MainWindow& MainWindow::operator=(MainWindow&&) noexcept = default;

void MainWindow::run() {
  while (impl_->window.isOpen()) {
    while (const std::optional event = impl_->window.pollEvent()) {
      if (event->is<sf::Event::Closed>()) {
        impl_->window.close();
      }
    }

    impl_->window.clear(sf::Color(30, 30, 35));
    impl_->window.display();
  }
}

}  // namespace notecraft::gui
