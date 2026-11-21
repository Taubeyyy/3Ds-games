# FORGE – 3DS-Engine (Plan)

Ziel: Eine Engine, auf der sich jedes Spiel der Sammlung schnell und in hoher Qualität bauen lässt,
2D und 3D, mit Physik. Später zusätzlich Android (AYN Thor) mit demselben Spielcode.

## Leitideen
1. **Zwei Backends von Anfang an:** 3DS (citro3d) und PC (Software-Renderer, headless).
   Jedes Spiel läuft auch am PC → automatische Tests, Screenshots, Balancing-Bots.
   Später ein drittes Backend: Android/GLES für den Thor.
2. **Spielcode sieht nie libctru/citro3d direkt**, nur die Engine-API.
3. **Kein großer Speicher auf dem Stack** (Absturz-Lehre aus Goo Alert): Arenen und statische Pools.
4. **Stereoskopisches 3D eingebaut:** jede Ebene und jedes Objekt hat eine Tiefe.
5. **Asset-Pipeline in Python:** Bilder → Atlas + Index-Header, Modelle → Mesh-Binär, Schrift, Audio.
6. **Feste Zeitschritte** für Logik und Physik, Rendering davon entkoppelt.

## Module
| Modul | Inhalt |
|---|---|
| core | App-Schleife, feste Zeitschritte, Szenen-Stapel mit Übergängen, Events, Zufall, Arenen, Log, Absturz-Bildschirm |
| math | vec2/3/4, mat3/4, quat, Rechtecke, Kurven, Easing, Rauschen |
| input | Tasten, Circle Pad, Touch-Gesten (Tippen, Halten, Ziehen, Wischen), Tastenbelegung |
| gfx2d | Sprite-Batch, Atlanten, Animationen, 9-Slice-Panels, Tilemaps, Partikel, Text, Kamera 2D (Shake, Zoom), Ebenen mit Tiefe |
| gfx3d | Eigene PICA200-Shader (Licht, Nebel, Vertexfarben, Texturen), Meshes, Materialien, Kamera mit Stereo, Frustum-Culling, Skybox, Billboards, Blob-Schatten, Grundkörper |
| phys2d | Kreis/AABB/Polygon (SAT), Spatial Hash, Ebenen und Masken, Trigger, Raycasts, Tilemap-Kollision, Plattformer-Controller |
| phys3d | Kugel/AABB/Kapsel gegen statische Dreiecksnetze (BVH), Raycasts, Rigid Bodies (Kugel/Box), Charakter-Controller |
| audio | ndsp-Mixer, Effekt-Kanäle, Musik-Streaming, Synth für prozedurale Effekte, Panning |
| ui | Knöpfe mit Icons und Drück-Animation, Listen mit Trägheit, Tabs, Slider, Schalter, Dialoge, Toasts, Themes (beide Bildschirme) |
| save | Versionierte Spielstände mit Prüfsumme, atomares Schreiben, Backup, Migration |
| debug | FPS/CPU/GPU-Anzeige, Debug-Zeichnen (Kollisionen), Einstellungen zur Laufzeit |

## Verzeichnisstruktur
```
engine/
  include/forge/*.h      öffentliche API
  src/core, gfx2d, ...   plattformunabhängig
  platform/3ds/          citro3d, ndsp, hid, Shader (.v.pica)
  platform/pc/           Software-Rasterizer, Skript-Eingabe, PNG-Ausgabe
  tools/                 Python: Atlas, Mesh-Konverter, Schrift, artlib
  demos/                 Testspiele je Phase
games/                   Spiele linken libforge
```

## Phasen
| Phase | Inhalt | Nachweis |
|---|---|---|
| 0 | Gerüst, Build (3DS + PC), CI, Plattform-Schicht, Screenshot-Tests | leeres Fenster mit Text auf 3DS und PC |
| 1 | core, math, input, gfx2d (Sprites, Text, Atlas), audio, save, ui | Demo: Menü mit Knöpfen, Sound, Speichern |
| 2 | phys2d, Tilemaps, Partikel, Kamera 2D | Demo: kleiner Plattformer |
| 3 | gfx3d: Shader, Meshes, Licht, Stereo, Mesh-Konverter | Demo: beleuchtete 3D-Szene mit Kamera |
| 4 | phys3d, Charakter-Controller | Demo: 3D-Figur läuft über Gelände |
| 5 | debug, Profiler, Doku, ein echtes Spiel auf FORGE | Roguelike als erstes Engine-Spiel |
| 6 | Android-Backend (Thor) | Demo läuft auf Android |

## Leistungsziele
- 2D: 60 FPS auf Old 3DS mit 500+ Sprites
- 3D: 30–60 FPS, ca. 5–10k Dreiecke pro Bild auf Old 3DS, mehr auf New 3DS (804 MHz)

## AA-Qualität für das nächste Spiel (Wunsch: „Studio-Vibes“, darf viele MB groß sein)
- **Assets auf RomFS** statt im Programm: beliebig viele Texturen, Musik und Modelle, nur das Nötige im Speicher (Laden pro Level/Bereich).
- **Texturformate:** ETC1/ETC1A4 (komprimiert, 4–8x weniger RAM) für große Flächen, RGBA8 für UI und Effekte, Mipmaps für Verkleinerungen.
- **Art-Pipeline im großen Stil:** mehrere tausend generierte Texturen möglich: Kacheln mit Varianten, Licht und Schatten eingebacken, Normal-Look, Animations-Sheets (Laufen, Angriff, Treffer, Tod), Parallax-Hintergründe in mehreren Ebenen, Umgebungsobjekte in vielen Varianten.
- **Licht und Effekte:** 2D-Lichter (additives Glühen), Tag/Nacht-Tönung, Partikel-Systeme mit Texturen, Bildschirm-Effekte über Render-Targets (Bloom-Imitat, Blitze, Vignette, Übergänge).
- **Präsentation:** animiertes Intro, Kamerafahrten, Bildschirmübergänge, Treffer-Stopps, Schütteln, Zahlen-Popups, dynamische Musik (Streaming) und Soundeffekte mit Varianten.
- **Ziel für das Roguelike:** komplett texturierte Welt, animierte Figuren und Gegner, Bosse mit eigenen Arenen und mehrere Biome.
