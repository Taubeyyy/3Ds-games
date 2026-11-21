# FORGE – die Engine der 3DS Sammlung

C++17-Engine für den Nintendo 3DS (citro2d/citro3d) mit einem PC-Backend ohne Fenster: Jedes Spiel läuft auch am PC,
wird per Skript gesteuert und schreibt Screenshots. Dadurch lässt sich alles ohne Konsole testen.

| Modul | Inhalt | Einbinden |
|---|---|---|
| Kern (`forge.hpp`) | App-Schleife (60 Hz fest), Eingabe, 2D-Zeichnen mit Stereo-Tiefe, Sprites, Text, Audio, Speichern, Profiler | immer |
| UI (`ui.hpp`) | Knöpfe, Schalter, Balken, Scrollen, Toasts, Szenen-Stapel mit Übergängen, Synth-Sounds | immer |
| 2D-Welt (`world2d.hpp`) | Kamera 2D, Tilemaps, Partikel | immer |
| `physics2d` | Box2D 2.4: Körper, Sensoren, Kontakte, Raycast, Tilemap-Kollision | `FORGE_MODULES += physics2d` |
| `gfx3d` (`gfx3d.hpp`) | Meshes, Texturen mit Mipmaps, Hardware-Fragmentlicht (8 Lichter), Nebel, Stereo-3D, Modelle (.fmesh) | `FORGE_MODULES += gfx3d` |
| `phys3d` (`phys3d.hpp`) | Bullet 2.87: Körper, Dreiecksnetze, Raycast/Sphere-Cast, Überlappung, Explosion, Figur-Controller | `FORGE_MODULES += gfx3d phys3d` |

## Neues Spiel

```
games/meinspiel/
  Makefile        TARGET, APP_TITLE, APP_DESCRIPTION, APP_AUTHOR, ICON, FORGE_MODULES, dann include …/engine/forge.mk
  game.cfg        NAME, TITLE, DESC, AUTHOR, PRODUCT_CODE, UNIQUE_ID, VERSION (für die CIA)
  source/*.cpp    Spielcode
  gfx/*.t3s       Sprite-Atlanten / Texturen (tex3ds), eingebettet als <name>_t3x.h
  data/*          eingebettete Dateien (.fmesh, .bin, .wav) → <name>_<ext>.h
  meta/           icon.png (48×48), banner.png (256×128), banner.wav
```

Makefile:

```make
TARGET          := meinspiel
APP_TITLE       := Mein Spiel
APP_DESCRIPTION := Beschreibung
APP_AUTHOR      := 3DS Sammlung
ICON            := meta/icon.png
FORGE_MODULES   := gfx3d phys3d
include $(dir $(abspath $(lastword $(MAKEFILE_LIST))))../../engine/forge.mk
```

Kleinstes Programm:

```cpp
#include "forge/forge.hpp"
using namespace forge;

class Spiel : public App {
	void update(float dt, const Input &in) override { if (in.pressed(KEY_START)) quit = true; }
	void draw(Gfx &g) override {
		g.rectGrad(0, 0, g.width, g.height, Color(0x1A2058), Color(0x0A0E22));
		if (g.screen == Screen::Top) g.text(200, 110, 0.8f, Color(0xFFFFFF), "Hallo 3DS", ALIGN_CENTER, 0, true, -4); // Tiefe -4: schwebt vor dem Bild
	}
};
int main() { static Spiel app; Config cfg; cfg.name = "meinspiel"; return run(app, cfg); }
```

Wichtig: große Objekte nie auf den Stack legen (3DS-Stack ist klein). Spielzustand `static` oder als Member der App.

## Bauen und Testen

| Was | Befehl |
|---|---|
| alle Spiele + Demos als CIA/3DSX nach `dist/` | `./build.sh` |
| nur ein Spiel/Demo | `./build.sh physics3d` |
| PC-Lauf mit Skript und Screenshots | `engine/tools/pc.sh engine/demos/physics3d engine/demos/physics3d/test.txt /tmp/shots` |
| Profiler im PC-Lauf | `FORGE_DEBUG=1 engine/tools/pc.sh …` |

Skript-Befehle (eine Zeile je Ereignis, erste Zahl = Bildnummer):
`tap x y` · `hold x y n` · `drag x0 y0 x1 y1 n` · `key A|B|X|Y|L|R|START|SELECT|UP|DOWN|LEFT|RIGHT` · `stick dx dy n` · `shot name` · `time s` · `speed k` · `quit`.
Touch-Koordinaten im Skript gelten für den unteren Bildschirm (320×240). Stick: y nach unten positiv.

## 2D

```cpp
SpriteSheet sheet; sheet.loadMem(FORGE_SHEET(sprites));   // gfx/sprites.t3s
Image held = sheet.get(0);
g.image(held, x, y, sx, sy, winkel, Color(0xFFFFFF), 0, tiefe);
g.rrect(x, y, w, h, radius, farbeOben, farbeUnten, tiefe);
g.text(x, y, groesse, farbe, "Text", ALIGN_CENTER);
```

Stereo: `tiefe > 0` liegt hinter dem Bildschirm, `< 0` davor. Kamera 2D, Tilemap, Partikel: siehe `world2d.hpp` und `demos/platformer`.

## UI

```cpp
ui::beginFrame(g);                         // am Anfang von draw()
if (ui::button(g, 1, 10, 10, 140, 34, "Start", Color(0x5AE07A))) {}
ui::toggle(g, 2, 10, 60, "Musik", musikAn);
// in update():
ui::process(in, dt);
if (ui::clicked(1)) starten();
```

Szenen: `SceneStack` mit `push/pop/replace` und Übergängen (Fade/Slide), siehe `demos/ui`.

## 3D

```cpp
Mesh boden = Mesh::plane(Vec2(20, 20), 16, Color(0xFFFFFF), 6);
Mesh figur; figur.loadMem(held_fmesh, held_fmesh_size);      // data/held.fmesh aus tools/meshtool.py
Texture3D stein; stein.loadMem(FORGE_SHEET(stein));          // gfx/stein.t3s: "-f rgba8 -m lanczos -z auto"

Camera3D cam; cam.eye = Vec3(0, 6, 8); cam.target = Vec3(0, 1, 0); cam.focus = 8; // Fokus = Stereo-Bildschirmebene
Scene3D sz; sz.ambient = Color(0x303040);
sz.lights[sz.nLights++] = Light3D();                          // gerichtetes Licht (pos = Richtung zum Licht)
Light3D fackel; fackel.point = true; fackel.pos = Vec3(2, 1, 0); fackel.color = Color(0xFF9A3A); fackel.range = 6;
sz.lights[sz.nLights++] = fackel;
sz.fog = true; sz.fogNear = 10; sz.fogFar = 40;

// in draw(), nur oben:
gfx3d::begin(g, cam, sz);
Material m; m.tex = &stein; m.specular = 0.2f;
gfx3d::draw(boden, Mat4::identity(), m);
gfx3d::draw(figur, Mat4::translate(pos) * Mat4::rotY(yaw), Material());
gfx3d::end(g);
// danach 2D darüber; Beschriftung an 3D-Punkt mit passender Stereotiefe:
Vec2 s; float t; if (gfx3d::project(g, cam, pos + Vec3(0, 2, 0), &s, &t)) g.text(s.x, s.y, 0.4f, Color(0xFFFFFF), "Held", ALIGN_CENTER, 0, true, t);
```

Grundkörper: `cube`, `sphere`, `plane`, `cylinder`, `grid` (Gelände). Materialien: `color` (mit Alpha), `emission`,
`specular`, `shininess`, `lit = false` für leuchtende Dinge. Modelle: OBJ (+MTL-Farben, `s off` = flach) →
`python3 engine/tools/meshtool.py modell.obj data/modell.fmesh [--scale S] [--center]`.

## 3D-Physik

```cpp
namespace p3 = forge::phys3d;
p3::World w; w.create(Vec3(0, -20, 0));
w.mesh(gelaende, Mat4::identity(), 0);                         // statisches Level
p3::Body *kiste = w.box(Vec3(0, 3, 0), Vec3(0.4f, 0.4f, 0.4f), p3::DYNAMIC, 100);
p3::Character held; held.teleport(Vec3(0, 1, 0));

// update():
held.update(w, wunschGeschwindigkeit, in.pressed(KEY_A), dt);  // gleitet, steigt Stufen, haftet am Boden
w.step(dt);
const p3::Contact *c; int n = w.contacts(&c);                  // Treffer-Ereignisse (begin/ende, Impuls)
p3::Body *getroffen[8]; int k = w.overlapSphere(schwertSpitze, 0.6f, getroffen, 8); // Nahkampf
// draw(): gfx3d::draw(wuerfel, p3::transform(kiste) * Mat4::scale(Vec3(0.8f, 0.8f, 0.8f)), holz);
```

## Profiler

Auf der Konsole **L+R+SELECT** drücken: FPS, CPU-Zeit (Update/Zeichnen), GPU-Zeit, freier Speicher, Bildzeit-Verlauf.
Eigene Messbereiche: `FORGE_PROFILE("KI");` am Anfang eines Blocks. `debug::log("Welle %d", n)` + `debug::drawLog(g, x, y)`.

## Leistung (Richtwerte Old 3DS)

- 2D: einige hundert Sprites bei 60 FPS
- 3D: 5–10k Dreiecke pro Bild bei 30–60 FPS; bei Stereo wird oben doppelt gezeichnet
- Physik: ein paar Dutzend dynamische Körper; Level als ein statisches Dreiecksnetz
- New 3DS: 804 MHz + L2-Cache werden automatisch eingeschaltet
