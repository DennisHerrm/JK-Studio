# Notizen Menü-Editor, Stufe M5 (Zweig menue5 ab menue3)

Auftrag: SP\auftrag_m5.md mit SP\auftrag_gemeinsam.md. Vorwissen NOTIZEN_menue1/2/3.md (M4 steht in
NOTIZEN_menue3.md).

## Punkt 0 - Original-JKA (Steam-Fassung) gegen den Editor

### Spielstarts

- Steam-Ausnahme (shank, 29.10., siehe auftrag_gemeinsam.md und Gedächtnis „kein-steam-start“): vor
  jedem Start geprüft, dass steam.exe läuft und `HKCU\Software\Valve\Steam\ActiveProcess\ActiveUser`
  != 0 ist (SP\m5\orig.py bricht sonst ab). Gestartet per Python Popen (Idle, `SW_SHOWNOACTIVATE`),
  Fenster 960x720, `s_initsound 0`, `in_mouse 0`, `in_joystick 0`, `SteamAppId/SteamGameId=6020`
  in der Umgebung des eigenen Prozesses (wie Steam es seinen Spielen setzt). Steam hat nie neu
  gestartet und keinen Dialog gezeigt; nach jedem Lauf keine eigenen jasp/jamp-Prozesse mehr.
- Ordner (shank: „der normale game data folder ist leer“): Steam startet aus `Jedi Academy\GameData`.
  Vorher enthielt GameData nur `base\saves` (Steam-Cloud: auto.sav, auto_yavin1.sav,
  auto_yavin1b.sav, steam_autocloud.vdf). Für die Läufe kopiert: aus „GameData original“ EaxMan.dll,
  IFC22.dll, OpenAL32.dll, jagamex86.dll, jamp.exe, jasp.exe, version.inf und base\assets0-3.pk3.
  Nicht kopiert (nicht original bzw. Anwenderdaten): base\jaconfig.cfg, base\jampconfig.cfg
  (Einstellungen von shank, 2024), base\saves (Spielstände), base\shaders\shaderlist.txt (21.10.2026,
  nicht von JKA), leerer Ordner base\maps.
- Geschrieben hat das Spiel nur in eigene Ordner im Scratchpad: `+set fs_basepath <SP\m5\home_*>`
  `+set fs_cdpath <GameData>` (jamp zusätzlich fs_homepath; jamp lädt uix86.dll nur aus
  fs_basepath/fs_cdpath - mit fs_homepath allein blieb es mit „Failed to load dll“ in einer
  Fehlermeldung stehen, eigene PID sofort beendet). Fotos: PrintWindow liefert beim alten
  GL-Fenster nur Schwarz; deshalb `screenshot_tga` des Spiels (landet im eigenen fs_basepath).
- Danach alle kopierten Dateien wieder entfernt (vorher mit dem Original verglichen), Ordnerzeiten
  zurückgesetzt: `find` über GameData und „GameData original“ (Typ, Größe, Zeit, Pfad) gleich der
  Liste vor dem ersten Start, Hashes der Spielstände und der Configs in „GameData original“ gleich
  (SP\m5\spielordner_vorher.txt / _nachher.txt, hash_vorher.txt).

### Rangliste (Editor gegen Original)

Werkzeuge SP\m5: orig_rang.sh (Spielfotos, SP `+uimenu <menü>`, MP Testpaket mit
`close all ; open <menü>` am Ende von onOpen des MP-Hauptmenüs, Foto ~213 Bilder nach dem Öffnen),
rang_editor.sh <lauf> (Editorfotos mit den Daten aus „GameData original“, Bildpaare
SP\m3\vergleich\neben_orig_{sp,mp}_*.png, Ergebnis rang_ergebnis_{sp,mp}_<lauf>.txt). Mittlere
Abweichung wie in M4 (Grauwerte 0-255, 8x6-Raster).
