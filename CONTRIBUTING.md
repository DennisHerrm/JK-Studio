# Contributing to JK Studio

Thank you for helping! JK Studio is a GPL v2 project (see [LICENSE](LICENSE)); by sending a
change you agree that it is published under the same licence.

## Before you start

- Bugs and ideas: open an issue with the version (Help > About), what you did, what you
  expected and what happened. Attach the log `%APPDATA%\jkstudio\jkstudio.log` (or
  `jkstudio-detail.log` for the details) and, if possible, a small sample file.
- Security problems: **not** as an issue - see [SECURITY.md](SECURITY.md).
- Larger changes: please describe them in an issue first, so that nobody works twice.

## Building and testing

Windows 10/11, Visual Studio 2026 (18) with C++, CMake. See [BAUEN.md](BAUEN.md) (German).

```
git clone --recurse-submodules https://github.com/DennisHerrm/JK-Studio.git
cd JK-Studio
build.bat
ctest --test-dir build -C Release
```

The unit tests need no game data. Tests that need Jedi Academy, Movie Duels or Jedi Outcast skip
themselves when the game folders are missing. The full self-test of the program and the map
editor is described in [TESTING.md](TESTING.md). The CI workflow (`.github/workflows/ci.yml`)
builds and runs the tests without game data for every pull request.

## Rules for changes

- **Behave like the game.** The preview reproduces OpenJK (and Movie Duels) formula by formula;
  the map editor follows NetRadiant-Custom and q3map2 1:1. No approximations where the original
  is known - port it and name the source file in a comment.
- **Reproduce first.** For a bug, first add a test that fails, then fix it, then show the same
  test passing.
- **No behaviour change without a reason.** Refactoring must keep the results identical: ctest,
  the self-test, byte comparisons of written files and `.map` hashes.
- **No warnings.** The own code builds with `/W4 /WX`; `third_party/` and `imgui/` are excluded.
  `tools/analyse/statische_analyse.py` (clang-tidy and MSVC `/analyze`) must report nothing new.
- **Line endings:** LF everywhere, except `data/fixtures/*.txt`, `data/base/*.h` and
  `data/jk2/*.h`, which stay CRLF (`.gitattributes`).
- **Languages:** the user interface has English, German, Chinese and Japanese texts
  (`tools/gen_i18n.py` and `tools/i18n_*.py`). Every new text needs all four. The guide
  (`tools/handbuch/`) is updated together with the function it describes.
- **Third-party code:** only licences compatible with GPL v2 (MIT, BSD, zlib, public domain,
  LGPL 2.1, GPL v2). Apache 2.0 and GPL v3-only code cannot be used. Prefer Windows system
  functions over new libraries. Add every new component to [THIRD-PARTY.md](THIRD-PARTY.md) and
  its licence text to the release list in `tools/release_paket.py`.
- Code comments and developer documentation are in German; commit messages, issues and pull
  requests may be English or German.

## Pull requests

Keep them focused: one topic per pull request, with the tests that show it works. Describe what
changed for the user and how you tested it.
