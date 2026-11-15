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

add_library(notecraft_music STATIC)

target_sources(notecraft_music
  PRIVATE
    src/music/tempo.cpp
    src/music/sequence.cpp
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/music/music.cppm
)

add_library(notecraft_audio STATIC)

target_sources(notecraft_audio
  PRIVATE
    src/audio/oscillator.cpp
    src/audio/envelope.cpp
    src/audio/audio_engine.cpp
    src/audio/voice.cpp
    src/audio/synth.cpp
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/audio/audio.cppm
)

target_link_libraries(notecraft_audio
  PUBLIC
    SFML::Audio
    SFML::System
)

add_library(notecraft_instruments STATIC)

target_sources(notecraft_instruments
  PRIVATE
    src/instruments/instrument.cpp
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/instruments/instruments.cppm
)

target_link_libraries(notecraft_instruments
  PUBLIC
    notecraft_audio
)

add_library(notecraft_playback STATIC)

target_sources(notecraft_playback
  PRIVATE
    src/playback/sequencer.cpp
    src/playback/player.cpp
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/playback/playback.cppm
)

target_link_libraries(notecraft_playback
  PUBLIC
    notecraft_music
    notecraft_audio
    notecraft_instruments
    SFML::Audio
)

add_executable(NoteCraft)

target_sources(NoteCraft
  PRIVATE
    src/main.cpp
    src/gui/main_window.cpp
  PRIVATE
    FILE_SET CXX_MODULES FILES
      src/gui/gui.cppm
      src/gui/piano_roll.cppm
      src/gui/control_panel.cppm
)

target_link_libraries(NoteCraft
  PRIVATE
    notecraft_music
    notecraft_audio
    notecraft_instruments
    notecraft_playback
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)
