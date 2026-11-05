// selbsttest_f78.inc - rc610 F78: Funde der Codepruefung Bereich 7 (BHED_EDITORTEST=f78)
//
// Eingebunden an genau dieser Stelle von gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
//   B7-2  gpuZeitMerken (karte_gpuzeiten.txt) aus zwei Faeden zugleich - wie das Schnell-Backen und das
//         Ende eines Baus: jede gemerkte Zeit muss danach zu finden sein.
std::vector<Schritt> f78Schritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    // 2 x 200 Karten: unter der Obergrenze des Speichers (GpuZeitenSpeicher::kMax = 500).
    constexpr int kJeFaden = 200;
    const auto name = [](int faden, int i) { return "maps/f78_faden" + std::to_string(faden) + "_" + std::to_string(i) + ".map"; };
    add(tu("B7-2: gpuZeitMerken aus zwei Faeden je 200-mal", [name] {
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(platform::settingsDirectory() + "\\karte_gpuzeiten.txt"), ec);
        std::vector<std::thread> faeden;
        for (int f = 0; f < 2; ++f) {
            faeden.emplace_back([f, name] {
                for (int i = 0; i < kJeFaden; ++i) {
                    karteneditor::gpuZeitMerken(name(f, i), "-light -fast", karte::GpuWeg::Gpu, 1.0 + static_cast<double>(i));
                }
            });
        }
        for (std::thread& t : faeden) { t.join(); }
    }));
    add(pruefe("B7-2: jede der 400 gemerkten Zeiten ist danach zu finden", [name] {
        int fehlt = 0;
        int falsch = 0;
        for (int f = 0; f < 2; ++f) {
            for (int i = 0; i < kJeFaden; ++i) {
                const karte::GpuZeiten z = karteneditor::gpuZeitenHolen(name(f, i), "-light -fast");
                if (z.gpuMs == 0) {
                    ++fehlt;
                } else if (z.gpuMs != 1000LL * (1 + i)) {
                    ++falsch;
                }
            }
        }
        diag::detail("Knopftest: B7-2 - " + std::to_string(fehlt) + " von " + std::to_string(2 * kJeFaden) + " Zeiten fehlen, " +
                     std::to_string(falsch) + " falsch");
        return fehlt == 0 && falsch == 0;
    }));
    return s;
}
