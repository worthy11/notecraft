cmake_minimum_required(VERSION 3.28)
project(NoteCraft LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT DEFINED CMAKE_TOOLCHAIN_FILE)
  message(WARNING
    "CMAKE_TOOLCHAIN_FILE nie jest ustawiony. "
    "Ustaw zmienną VCPKG_ROOT i wybierz preset CMake \"default\" "
    "(patrz README → Budowanie)."
  )
endif()

find_package(SFML CONFIG REQUIRED COMPONENTS Graphics Window System Audio)
find_package(boost_json CONFIG REQUIRED)

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

add_library(notecraft_midi STATIC)

target_sources(notecraft_midi
  PRIVATE
    src/midi/midi_writer.cpp
    src/midi/midi_reader.cpp
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/midi/midi.cppm
)

target_link_libraries(notecraft_midi
  PUBLIC
    notecraft_music
)

add_library(notecraft_project STATIC)

target_sources(notecraft_project
  PRIVATE
    src/project/project.cpp
  PUBLIC
    FILE_SET CXX_MODULES FILES
      src/project/project.cppm
)

target_link_libraries(notecraft_project
  PUBLIC
    notecraft_music
    notecraft_instruments
  PRIVATE
    Boost::json
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
    notecraft_midi
    notecraft_project
    SFML::Graphics
    SFML::Window
    SFML::System
    SFML::Audio
)

find_package(Catch2 3 CONFIG REQUIRED)

add_executable(notecraft_tests)

target_sources(notecraft_tests
  PRIVATE
    tests/music_tests.cpp
    tests/audio_tests.cpp
    tests/instruments_tests.cpp
    tests/playback_tests.cpp
    tests/midi_tests.cpp
    tests/project_tests.cpp
    tests/integration_tests.cpp
)

target_link_libraries(notecraft_tests
  PRIVATE
    notecraft_music
    notecraft_audio
    notecraft_instruments
    notecraft_playback
    notecraft_midi
    notecraft_project
    Catch2::Catch2WithMain
)

include(CTest)
include(Catch)
catch_discover_tests(notecraft_tests)
