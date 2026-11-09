# NoteCraft

W ramach projektu zaplanowana została aplikacja desktopowa do tworzenia melodii (uproszczony fragment DAW, w stylu np. FL Studio). Użytkownik układa nuty w piano rollu, wybiera instrument, ustawia tempo i odtwarza muzykę na żywo. Dźwięk generuje własny, prosty syntezator.

## Piano roll

Edytor melodii na siatce czas x wysokość dźwięku. Dodawanie, przesuwanie i usuwanie nut myszą oraz skalowanie widoku.

![Piano roll w FL Studio — nuty na siatce z klawiaturą i velocity](docs/figures/piano_roll.jpg)

_Przykład piano rolla w FL Studio: nuty na siatce, klawiatura po lewej, velocity na dole._

## Syntezator i instrumenty

Własny silnik audio z generatorami fal takich jak sinusoida, prostokąt, piła. Kilka presetów instrumentów (np. Piano, Bass, Synth) z możliwością zmiany parametrów.

![Interfejs syntezatora FLEX — preset, makra, filtry i obwiednie](docs/figures/flex_synth.jpg)

_Przykład panelu syntezatora FLEX w FL Studio: wybór presetu oraz parametry brzmienia (filtry, obwiednie, efekty)._

## Odtwarzanie

Polifonia (kilka nut jednocześnie), miksowanie, Play / Pause / Stop. Synchronizacja z tempem BPM i wskaźnik aktualnej pozycji na piano rollu.

![Kontrola odtwarzania i tempo BPM w FL Studio](docs/figures/play_pause_bpm.png)

_Kontrola odtwarzania i tempa w FL Studio: Play/Pause oraz BPM._

## Pliki MIDI i projekty

Aplikacja będzie umożliwiać import i eksport MIDI oraz zapis/odczyt własnych projektów (nuty, tempo, instrumenty).

![Okno eksportu / renderingu w FL Studio](docs/figures/export.jpg)

_Przykład eksportu projektu w FL Studio: wybór trybu, formatu wyjściowego i jakości._

## Wykorzystanie AI

Wstępny brainstorming i dopracowanie tematu, podział implementacji na kroki i przypisanie do osób w zespole zostały wykonane przy pomocy modelu GPT-6 Luna ([https://chatgpt.com/](https://chatgpt.com/)). Link do konwersacji: [https://chatgpt.com/share/6af2189c-ce94-83ed-8eb1-41969f292c77](https://chatgpt.com/share/6af2189c-ce94-83ed-8eb1-41969f292c77)
