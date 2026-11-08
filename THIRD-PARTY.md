# Third-party components

JK Studio is GPL v2 (see [LICENSE](LICENSE)). Everything below is compatible with
GPL v2; nothing is licensed under Apache 2.0 or GPL v3 only. The licence texts ship in
`JK-Studio/licenses/` of every release zip (`tools/release_paket.py`, list `LIZENZEN`).

## Animation data shipped in `data/presets` (preset library)

| Work | Licence | Where | Notes |
|---|---|---|---|
| [Quaternius - Universal Animation Library](https://quaternius.itch.io/universal-animation-library) (Standard) | CC0 1.0 | `data/presets/quaternius_ual/*.bvh` | selected loops/actions, converted to BVH on their own skeleton by `tools/presetbau` |
| [The 100STYLE Dataset - Ian Mason](https://www.ianxmason.com/100style/) (Zenodo 8127870) | CC BY 4.0 | `data/presets/100style/*.bvh` | one walking cycle per style cut from the FW take, 60 -> 30 fps, in place, seam spread; attribution in `data/presets/CREDITS.txt` and the About window |

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
| WhiteoutFlakes MCP HTTP guard (Fernando Sahmkow) | 2026 | BSD 3-Clause (with LICENSE-AI.md: AI-derived code keeps the copyright and the BSD conditions) | check list followed in `src/ki.cpp` (`pruefeHttp`, `gleichesGeheimnis`), notice in `include/bhed/ki.h` | rewritten, no code copied; loopback Host/Origin, bearer token in constant time. Copyright (c) 2026, Fernando Sahmkow. Redistribution and use in source and binary forms, with or without modification, are permitted provided that the BSD 3-Clause conditions are met (retain this notice; reproduce it in documentation; no endorsement without permission). Provided "as is", without warranty. |
| WhiteoutLib / WhiteoutFlakes UV tools (Fernando Sahmkow) | WhiteoutLib 35d068d, WhiteoutFlakes 2026 | BSD 3-Clause with LICENSE-AI.md (AI-derived code is a derivative work and keeps the copyright, conditions and disclaimer) | `src/modell/wof/`, `include/bhed/modell/wof/`, `gui/wof/` (each file carries the full notice; `fase_blender.*` and the marked Blender parts of `mesh_werkzeuge.cpp` are GPL-2.0-or-later, see the Blender row) | islands, LSCM/minimum stretch, relax, straighten, projections, packing, stacks, seams, checks and the UV commands, and (model block 1d) the modelling tools - polygon working mesh, extrude, inset, bevel, chamfer, connect, loop cut, cut, weld, target weld, merge, bridge, remove, delete, detach, symmetrize, to quads, primitives, tool caddy - rewritten for .glm/.md3 surfaces; text `licenses/WhiteoutFlakes-BSD-3.txt`. Copyright (c) 2026, Fernando Sahmkow. No endorsement implied. |
| [Blender](https://projects.blender.org/blender/blender) `bmesh_bevel.cc` | 5.0.0 | GPL v2 or later (used as GPL v2) | ported into `src/modell/wof/fase_blender.cpp`, `src/modell/wof/fase_blender.h` and the marked Blender parts of `src/modell/wof/mesh_werkzeuge.cpp` (segmentieren with the corner grids, the segment data in chamferKern: selcount, tri_corner_test, pipe_test; that file is BSD 3-Clause AND GPL-2.0-or-later, as a whole GPL v2) | chamfer with segments: edge profiles, equal chords of the superellipse, corner grids (adj_vmesh, cube corner, pipe); Copyright 2023 Blender Authors |
| Blender (`weight_to_rgb`, `bone_octahedral_verts`, `paint_vertex_weight_utils`, brush falloff) | – | GPL v2 or later (used as GPL v2) | ported into `gui/app_modelltab_ansicht.cpp`, `src/modell/gewichte.cpp`, `include/bhed/modell/gewichte.h` | weight colours, octahedral bones, weight paint falloff of the model tab; Copyright (C) Blender Authors |
| [xatlas](https://github.com/jpcy/xatlas) | f700c779 | MIT | `third_party/xatlas/` | charts by shape for the UV tools (Copyright (c) 2018-2020 Jonathan Young); text `third_party/xatlas/LICENSE` |
| Fonts Cinzel and Michroma | – | SIL OFL 1.1 | lettering of `res/jkstudio_logo.*`, `res/jkstudio.ico` | `licenses/OFL-*.txt` |
| Windows system libraries | – | part of Windows | `bcrypt`, `winhttp`, `ws2_32`, `crypt32` (DPAPI), `d3d11` (loaded at run time) | no redistribution |

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
