# Third-party components

JK Studio is GPL v2 (see [LICENSE](LICENSE)). Everything below is compatible with
GPL v2; nothing is licensed under Apache 2.0 or GPL v3 only. The licence texts ship in
`JK-Studio/licenses/` of every release zip (`tools/release_paket.py`, list `LIZENZEN`).

## Compiled into `jkstudio.exe`

| Component | Version | Licence | Where | Notes |
|---|---|---|---|---|
| [Dear ImGui](https://github.com/ocornut/imgui) | v1.92.9b-docking | MIT | `imgui/` (submodule) | user interface; text `licenses/imgui-MIT.txt` |
| [minimp3](https://github.com/lieff/minimp3) | header | CC0 1.0 | `third_party/minimp3.h` | MP3 sound of the preview |
| QuickHull (Antti Kuukka) | from NetRadiant-Custom 20260114 | public domain | `third_party/quickhull/`, `src/karte/quickhull.cpp` | Snap to Grid of brushes |
| NetRadiant-Custom / GtkRadiant / q3map2 | 20260114 | GPL v2 | ported into `src/karte/`, `include/bhed/karte/`, `gui/app_karte*.cpp` | each file names its sources and keeps their copyright lines |
| NetRadiant-Custom prtview plugin | 20260114 | LGPL 2.1+ (used as GPL v2, LGPL 2.1 section 3) | `src/karte/portale.cpp`, `gui/app_karte_portale.cpp` | Portal Viewer |
| Qt 5.15 layout and key code | 5.15 | LGPL v3 / GPL v2 | ported into `src/karte/teiler.cpp`, `src/karte/befehle.cpp` | QSplitter, QToolBarLayout, QKeySequence rules |
| [OpenJK](https://github.com/JACoders/OpenJK) | master | GPL v2 | ported into `src/` (preview, ICARUS, movement) | engine behaviour of the preview |
| Fonts Cinzel and Michroma | – | SIL OFL 1.1 | lettering of `res/jkstudio_logo.*`, `res/jkstudio.ico` | `licenses/OFL-*.txt` |
| Windows system libraries | – | part of Windows | `bcrypt`, `winhttp`, `d3d11` (loaded at run time) | no redistribution |

## Compiled into `bhed_q3map2.exe` (map compiler)

| Component | Version | Licence | Where |
|---|---|---|---|
| q3map2 and its `libs/` (crnlib, ddslib, webplib, rapidjson, ...) | NetRadiant-Custom 20260114 | GPL v2; parts zlib, BSD, MIT | `third_party/q3map2/` (provenance in its README.md) |
| zlib | 1.3.1 | zlib | `third_party/zlib/` |
| libpng | 1.6.53 | PNG Reference Library License v2 | `third_party/libpng/` |
| libjpeg-turbo | 3.1.3 | IJG, BSD 3-clause, zlib | `third_party/libjpeg-turbo/` |
| libxml2 | 2.15.1 | MIT | `third_party/libxml2/` |
| assimp (with clipper, earcut, openddlparser, poly2tri, pugixml, rapidjson, zip, Open3DGC, stb) | 6.0.2 | BSD 3-clause; contrib MIT / Boost / BSL / Unlicense / public domain | `third_party/assimp/` (q3map2; since stage 9 also the editor: geometry import) |
| fdlibm `acos` (in the GPU bake shader) | – | Sun Microsystems permission notice | `src/q3map2/bhed_gpubacken.hlsl` (notice kept in the source) |

## Only for development (not shipped)

| Component | Licence | Use |
|---|---|---|
| [Tracy](https://github.com/wolfpld/tracy) | BSD 3-clause | optional profiler, `-DBEHAVED_TRACY=ON` downloads it |
| LLVM clang-cl / clang-tidy (from Visual Studio) | Apache 2.0 with LLVM exception | compiler and static analysis only; no LLVM code is linked into the programs |
| Python standard library | PSF | build and test scripts in `tools/` |

## Game data

`data/` contains files from the Jedi Academy SDK, Jedi Outcast and the original `BehavEd.exe`
(images, `behaved.bhc`, headers such as `Q3_Interface.h` and `anims.h`). They are the property
of Raven Software / Activision and are not covered by the GPL.
