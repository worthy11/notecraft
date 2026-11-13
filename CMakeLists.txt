cmake_minimum_required(VERSION 3.28)
project(NoteCraft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(WARNING
    "CMAKE_TOOLCHAIN_FILE nie jest ustawiony. "
    "Ustaw zmienną VCPKG_ROOT i wybierz preset CMake \"default\" "
    "(patrz README → Budowanie)."
  )
endif()

find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System Audio)

add_executable(NoteCraft)

target_sources(NoteCraft
  PRIVATE
    src/main.cpp
)

target_link_libraries(NoteCraft
  PRIVATE
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)
