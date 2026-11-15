# Plan implementacji projektu – syntezator muzyczny MIDI

## Cel projektu

Stworzenie niewielkiej aplikacji desktopowej przypominającej uproszczony fragment FL Studio. Użytkownik może tworzyć melodie w piano rollu, wybierać instrument, ustawiać tempo i odtwarzać muzykę bezpośrednio na komputerze.

Projekt zakłada własny, prosty syntezator dźwięku. Dzięki temu aplikacja będzie zawierała własny silnik audio, a nie tylko interfejs do odtwarzania gotowych plików MIDI.

## 1. Plan implementacji

Podział: 2 osoby, 12 kroków, po 6 zadań na osobę. Każdy krok ma jednego właściciela, ale interfejsy między modułami należy uzgodnić wspólnie.

### 🟢 Etap I – fundamenty projektu

#### 1. Architektura projektu i konfiguracja środowiska

**Max Słota** ✅

- Utworzenie repozytorium i struktury katalogów.
- Konfiguracja CMake.
- Integracja bibliotek SFML i Boost.
- Przygotowanie podstawowej aplikacji okienkowej.

**Rezultat:** kompilowalny szkielet aplikacji.

#### 2. Model muzyczny

**Karolina Rozmus** ✅

- Implementacja reprezentacji nuty (pitch, velocity, start, duration).
- Implementacja sekwencji nut.
- Obsługa tempa (BPM).
- Operacje dodawania, usuwania i modyfikowania nut.

**Rezultat:** niezależny od GUI model utworu.

### 🟢 Etap II – silnik audio

#### 3. Generowanie dźwięku

**Max Słota** ✅

- Implementacja generatorów fal: sinusoidalnej, prostokątnej i piłokształtnej.
- Przeliczanie wysokości nuty na częstotliwość.
- Generowanie próbek audio.
- Podłączenie wyjścia audio.

**Rezultat:** możliwość wygenerowania i usłyszenia pojedynczego dźwięku.

#### 4. Obwiednia ADSR i instrumenty

**Max Słota** ✅

- Implementacja obwiedni Attack, Decay, Sustain, Release.
- Definiowanie parametrów instrumentów.
- Przygotowanie kilku presetów, np. Piano-like, Bass, Synth.
- Powiązanie instrumentu z generatorem dźwięku.

**Rezultat:** różne brzmienia generowane przez własny syntezator.

### 🟢 Etap III – odtwarzanie muzyki

#### 5. Silnik odtwarzania i polifonia

**Karolina Rozmus** ✅

- Obsługa wielu jednocześnie odtwarzanych nut.
- Miksowanie sygnałów z wielu generatorów.
- Zarządzanie aktywnymi głosami (voices).
- Obsługa rozpoczęcia i zakończenia nut.

**Rezultat:** możliwość odtwarzania akordów i melodii.

#### 6. Synchronizacja i kontrola odtwarzania

**Karolina Rozmus** ✅

- Synchronizacja nut z tempem BPM.
- Implementacja Play, Pause, Stop.
- Obsługa czasu odtwarzania za pomocą `std::chrono`.
- Odtwarzanie sekwencji w pętli.

**Rezultat:** odtwarzanie całego utworu zgodnie z piano rollem.

### 🟢 Etap IV – interfejs użytkownika

#### 7. Piano roll

**Max Słota** ✅

- Implementacja siatki reprezentującej czas i wysokość nut.
- Wyświetlanie nut na siatce.
- Dodawanie, przesuwanie i usuwanie nut za pomocą myszy.
- Skalowanie widoku.

**Rezultat:** wizualny edytor melodii.

#### 8. Panel instrumentów i sterowanie

**Max Słota** ✅

- Lista dostępnych instrumentów.
- Zmiana parametrów instrumentu.
- Regulacja BPM.
- Przyciski Play, Pause, Stop.
- Wskaźnik aktualnej pozycji odtwarzania.

**Rezultat:** kompletny panel sterowania.

### 🟡 Etap V – zapis, testy i dopracowanie

#### 9. Obsługa plików MIDI

**?**

- Eksport sekwencji do pliku MIDI.
- Import plików MIDI.
- Mapowanie nut i czasu na zdarzenia MIDI.

**Rezultat:** wymiana utworów z innymi programami muzycznymi.

#### 10. Zapisywanie projektów

**?**

- Zapis i odczyt projektu.
- Przechowywanie nut, tempa i instrumentów.
- Obsługa błędów plików i nieprawidłowych danych.

**Rezultat:** możliwość kontynuowania pracy nad utworem.

#### 11. Testy jednostkowe i integracyjne

**?**

- Integracja Catch2.
- Testy przeliczania wysokości nut na częstotliwości.
- Testy modelu muzycznego i obwiedni ADSR.
- Testy importu i eksportu MIDI.

**Rezultat:** automatyczne testy kluczowych funkcjonalności.

#### 12. Integracja, dokumentacja i prezentacja

**?**

- Integracja wszystkich modułów.
- README.md z instrukcją kompilacji i obsługi.
- Pomoc dostępna w aplikacji.
- Informacja o wykorzystaniu AI.
- Przygotowanie demonstracji.

**Rezultat:** kompletna wersja alfa projektu.

## 2. Proponowana struktura projektu

```text
MidiSynth/
├── CMakeLists.txt
├── README.md
├── assets/
│   └── presets/
├── src/
│   ├── audio/
│   │   ├── Oscillator.cpp
│   │   ├── Envelope.cpp
│   │   ├── Voice.cpp
│   │   └── AudioEngine.cpp
│   ├── music/
│   │   ├── Note.cpp
│   │   ├── Sequence.cpp
│   │   └── MidiFile.cpp
│   ├── instruments/
│   │   └── Instrument.cpp
│   ├── gui/
│   │   ├── PianoRoll.cpp
│   │   └── MainWindow.cpp
│   └── main.cpp
└── tests/
    ├── MusicTests.cpp
    ├── AudioTests.cpp
    └── MidiTests.cpp
```
