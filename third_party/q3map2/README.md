# q3map2 from NetRadiant-Custom 20260114 (unchanged)

This directory holds the map compiler q3map2 exactly as it is in the
NetRadiant-Custom release tag **20260114** (commit `68ecbed`,
https://github.com/Garux/netradiant-custom/tree/20260114). BehavEd-Remake builds
it as the separate program `bhed_q3map2.exe` (`src/q3map2/CMakeLists.txt`); the
map editor starts it like NetRadiant-Custom starts its `q3map2.exe`.

**The files are byte-for-byte copies.** Nothing in here is edited; `.gitattributes`
marks these directories as `-text` so Git does not touch line endings either.
Where MSVC needs something different from MinGW/GCC, the change lives outside
this directory: forced-include headers and replacements in `src/q3map2/kompat/`,
and patched copies that CMake writes into the build directory
(`src/q3map2/CMakeLists.txt`, function `bhed_q3_kopie`). The list of these
adaptations, with the reason for each, is in `NOTIZEN_karte3.md` (section
"Punkt 1").

## What was taken

| Path here | From the tag | Licence |
|---|---|---|
| `tools/quake3/q3map2/` | `tools/quake3/q3map2/` (all files) | GPL v2 or later (file headers; text in `/LICENSE`) |
| `tools/quake3/common/` | `tools/quake3/common/` (all files) | GPL v2 or later; `miniz.*` public domain (Unlicense); `md4.cpp` GPL v2 (Samba) |
| `include/` | the three headers q3map2 includes (`itextstream.h`, `stream_version.h`, `version.h`) | GPL v2 or later |
| `libs/` (subset) | only the headers and sources q3map2 includes: `generic/`, `math/`, `memory/`, `os/`, `stream/`, `string/`, `debugging/`, `bytebool.h`, `timer.h`, `filematch.*`, `l_net/`, `ddslib*`, `crnlib/`, `webplib/`, `rapidjson/` | see below |

Licences inside `libs/`:

- `crnlib/crunch/inc/` – crunch by Richard Geldreich / Binomial LLC, zlib licence
  (notice at the end of `crnlib.h`); `crnlib/crnlib.cpp` – GPL v2 (Unvanquished).
- `ddslib/` – Randy Reddig, BSD 3-clause (file header).
- `webplib/tiny_webp.h` – Google Inc. and justus2510, BSD 3-clause (file header);
  `webplib.cpp` – GPL v2 (Unvanquished).
- `rapidjson/` – THL A29 Limited (Tencent) and Milo Yip, MIT (file headers).
- `filematch.*` – LordHavoc (DarkPlaces), part of NetRadiant under GPL v2.
- `l_net/` – Id Software and contributors, GPL v2.
- everything else – NetRadiant / GtkRadiant, GPL v2 or later.

**Not taken:** `libs/etclib.c` and `libs/etclib.h`. They are under the Apache
License 2.0, which is not compatible with GPL v2. `src/q3map2/kompat/etclib.c`
is an own ETC1 block decoder written from the format description (Khronos /
OES_compressed_ETC1_RGB8_texture); it gives the same RGB values as the original
for every block (checked against the original on 4 million random blocks during
development).

## Libraries next to this directory

The same versions NetRadiant-Custom 20260114 ships with its Windows
`q3map2.exe`, taken unchanged from the upstream release archives. Only build
files, tests, docs and sample data that the build does not use were left out.

| Directory | Version | Licence file | Licence |
|---|---|---|---|
| `third_party/zlib/` | 1.3.1 | `LICENSE` | zlib |
| `third_party/libpng/` | 1.6.53 | `LICENSE` | PNG Reference Library License v2 |
| `third_party/libjpeg-turbo/` | 3.1.3 (without `simd/`) | `LICENSE.md` | IJG, BSD 3-clause, zlib |
| `third_party/libxml2/` | 2.15.1 | `Copyright` | MIT |
| `third_party/assimp/` | 6.0.2 | `LICENSE` | BSD 3-clause; `contrib/` parts: clipper (Boost), earcut-hpp (ISC), Open3DGC (MIT), openddlparser (MIT), poly2tri (BSD 3-clause), pugixml (MIT), rapidjson (MIT), stb (MIT / public domain), unzip (zlib), utf8cpp (Boost), zip (Unlicense) |

This software is based in part on the work of the Independent JPEG Group
(libjpeg-turbo, `README.ijg`).

All of these licences are compatible with GPL v2. Building needs no internet
connection: every source is in the repository.
