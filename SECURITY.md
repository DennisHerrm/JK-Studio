# Security policy

JK Studio is a desktop tool for Star Wars Jedi Knight: Jedi Academy and Jedi Outcast. It reads
files that other people make (maps, models, scripts, `.pk3` archives) and updates itself from
GitHub, so security bugs matter even for a modding tool.

## Supported versions

Only the newest release (`v1.0.0-rcNNN` on the
[releases page](https://github.com/DennisHerrm/JK-Studio/releases)) gets fixes. The built-in
updater (Help > Check for updates) brings you there.

## Reporting a vulnerability

Please **do not** open a public issue for a security problem.

- Use GitHub's private reporting:
  [Report a vulnerability](https://github.com/DennisHerrm/JK-Studio/security/advisories/new)
  (repository tab *Security* > *Advisories* > *Report a vulnerability*).
- Describe what an attacker can do, the version (Help > About), and how to reproduce it. A
  small sample file that triggers the problem is the most helpful thing you can send.

You can expect a first answer within a week. Once a fix is released, the advisory is published
and you are credited unless you prefer not to be.

## What counts

Examples of what we want to hear about:

- a crafted file (`.pk3`, `.bsp`, `.map`, `.md3`, `.glm`, `.gla`, `.skin`, `.shader`, ICARUS
  `.txt`/`.ibi`, `.menu`, `.roff`, NPC files, ...) that crashes JK Studio, reads or writes out of
  bounds, or makes it write files outside the folder it should write to;
- a way to make the updater install something that is not the published release (it checks
  `SHA256SUMS.txt` and, once enabled, an ECDSA signature before it changes any file);
- anything that runs code or opens files with other programs without you asking for it.

Bugs in the games themselves, in Movie Duels or in third-party tools are out of scope; please
report them to their authors.

## How releases are protected

- Releases are built by GitHub Actions from the tagged source (`.github/workflows/release.yml`)
  and published together with `SHA256SUMS.txt`. See the README section *Verifying a download*.
- The program is **not code-signed**; Windows SmartScreen may warn on first start. The README
  explains why and how to check the download instead.
- The source is analysed with clang-tidy and MSVC `/analyze` (`tools/analyse/`), and every file
  reader is fuzzed with AddressSanitizer (`tools/fuzzziel.cpp`, see `TESTING.md`).
