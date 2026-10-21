# BehavEd-Remake

A remake of **BehavEd**, the ICARUS script editor from Raven's Jedi Academy
SDK – with a modern interface and a 3D preview that plays cutscenes the way the
game does (Jedi Academy / Movie Duels).

- Open, edit and compile scripts (`.txt` / `.ibi`) – commands, macros and
  dialogs as in the original.
- Map view: BSP maps straight from the game archives, characters with
  animations, effects (`.efx`), sound, camera moves and an editable timeline.
- ICARUS re-implementation (`src/ablauf.cpp`): scripts run on the engine's
  50 ms tick, including `affect`, `run`, `use`, spawners and signals.
- Undo history with selective undo, rolling backups of the last 10 saves,
  split views, bookmarks, change markers with revert-to-original.
- Built-in updater: new releases are offered at startup.
- English, German, Chinese and Japanese.
- A full guide with pictures, tutorials and search, in all four languages
  (Help → *Guide and tutorials*).

## Download

Open the [latest release](https://github.com/DennisHerrm/BehavEd-Remake/releases/latest)
and, under **Assets**, download **`BehavEd-Remake-rcNNN-windows-x64.zip`**
(Windows 10/11, 64-bit). *Source code (zip)* and *Source code (tar.gz)* are
for developers – they contain no program. Unzip the file (right-click →
*Extract All…*) and start `BehavEd-Remake/behaved.exe`. Nothing is installed;
keep `behaved.exe` next to its `data` folder.

Just want the program? **`BehavEd-Remake-rcNNN-windows-x64.exe`** from the same
release runs on its own: it carries its data and unpacks it to
`%APPDATA%\behaved\data\base` the first time. The offline guide is only in the
zip.

The program is not code-signed, so Windows SmartScreen may warn the first time
("Windows protected your PC"): click **More info → Run anyway**.
`SHA256SUMS.txt` in the same release lists the checksums of both downloads.

Add your Jedi Academy / Movie Duels game folders under **Edit → Game folders**.
The program updates itself from here (Help → *Check for updates…*).

The guide (`handbuch/` next to `behaved.exe`) explains every function and walks
through four tutorials: a first cutscene, a walking figure, a ship on a ROFF
path and a mission of your own. It works offline; open it from
Help → *Guide and tutorials*. Its source is in `tools/handbuch/`.

## Building

Windows, Visual Studio 2026 (18) with C++, CMake.

```
git clone --recurse-submodules https://github.com/DennisHerrm/BehavEd-Remake.git
cd BehavEd-Remake
build.bat
```

Details and known pitfalls are in [BAUEN.md](BAUEN.md), the tests in
[TESTING.md](TESTING.md), the change log in [AENDERUNGEN.md](AENDERUNGEN.md)
(the developer documentation and code comments are in German).

## Third-party content

- `imgui/` – [Dear ImGui](https://github.com/ocornut/imgui) (MIT), as a submodule.
- `third_party/minimp3.h` – minimp3 (CC0).
- `data/` contains files from the Jedi Academy SDK and from the original
  `BehavEd.exe` (images, the command description `behaved.bhc`, headers such as
  `Q3_Interface.h` and `anims.h`). These are the property of Raven Software /
  Activision.
- Code ported from [NetRadiant-Custom](https://github.com/Garux/netradiant-custom),
  GtkRadiant and q3map2 (GPL v2): the map editor core in `src/karte/` and
  `include/bhed/karte/`, and the map editor workspace in `gui/app_karte*.cpp`
  (menus, toolbars, shortcuts, layouts as NetRadiant-Custom). Each ported file
  names its NetRadiant-Custom source files and keeps their copyright lines (Id
  Software, Inc. and the GtkRadiant / NetRadiant contributors). The toolbar
  icons are our own drawings (`gui/karte_vektoricons.cpp`) that keep the
  meaning of NetRadiant-Custom's icons at the same places. The
  shortcut matching follows Qt 5 (QKeySequence, QWindowsKeyMapper::possibleKeys,
  LGPL), as NetRadiant-Custom uses it; the pane, toolbar and status bar sizes
  follow Qt 5.15's layout code (qGeomCalc, QSplitter, QToolBarLayout,
  QStatusBar; GPL v2/v3 or LGPL v3) in `src/karte/teiler.cpp`.
- Code ported from [OpenJK](https://github.com/JACoders/OpenJK) (GPL v2): the
  engine behaviour reproduced in the preview (movement, collision, ICARUS,
  rendering rules) and the entity definitions generated from its QUAKED comments.

## License

The code of BehavEd-Remake is licensed under the GNU General Public License,
version 2 – see [LICENSE](LICENSE). It contains code ported from
NetRadiant-Custom / GtkRadiant / q3map2 and from OpenJK, which are GPL v2 as well.

The game data from Raven Software / Activision in `data/` (see above) remains
their property and is not covered by this license.
