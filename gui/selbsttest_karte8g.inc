// selbsttest_karte8g.inc - Stufe 8g des Karten-Editors: Build > "GPU bake". Laeuft nach dem
// Tutorial der Stufe 8c (dessen Karte meinekarte ist gebaut): den Menuepunkt anklicken wie ein
// Mensch, noch einmal Build > [Final] Fast - jetzt mit -bhedgpubake -, danach: der Befehl hatte
// den Schalter, die Konsole zeigt die GPU und den Abweichungsbericht (oder dass keine GPU nutzbar
// war), alle Lumps der .bsp sind Byte fuer Byte die des CPU-Baus davor, die Einstellung ist gespeichert und
// am Ende wieder aus.
//
// Stufe 8d Punkt 1b: "GPU bake" ist ein Untermenue Auto / Ein / Aus, Vorgabe Auto. Erst Ein (der
// Bau oben), dann Auto: der -light-Befehl hat -bhedgpubakeauto mit den gemerkten Zeiten, die Konsole
// nennt Wahl und Grund, die Lumps sind wieder die des CPU-Baus, die Zeit ist gemerkt; am Ende Auto.
//
// Eingebunden in gui/selbsttest.cpp nach selbsttest_karte8c.inc. Modus karteneditor
// (BHED_KARTE_NUR=stufe8g: Tutorial und dieser Teil) und Bildlauf BHED_HB_NUR=gpubacken.
namespace stufe8g {

namespace ke = karteneditor;
using stufe3::istDatei;
using stufe3::konsoleText;

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto cpuBsp = std::make_shared<std::string>();
    add(tu("GPU-Backen: die .bsp des CPU-Baus merken, Konsole leeren", [cpuBsp] {
        *cpuBsp = dateiInhalt(stufe8c::stamm() + ".bsp");
        ke::konsoleLeeren();
        karteDialogeZu();
    }));
    add(pruefe("GPU-Backen: Vorgabe Auto (kGpuBackenVorgabe), die CPU-.bsp ist da", [cpuBsp] {
        return ke::editor().bau.gpuBacken == karte::GpuBacken::Auto && !cpuBsp->empty();
    }));
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeGpuMenue)) + "##baugpubacken"));
    add(pause(0.3));
    add(hb::fotoTeil("h_karte_gpubacken_menue", stufe8c::menueRahmen));
    add(klick(std::string(ke::trK(Str::KeGpuEin)) + "##baugpuein"));
    add(pause(0.3));
    add(pruefe("GPU-Backen: Build > GPU bake > On ist gewaehlt und gespeichert (BhedGpuBakeMode=on)", [] {
        return ke::editor().bau.gpuBacken == karte::GpuBacken::Ein && g_app->settings.karte["BhedGpuBakeMode"] == "on";
    }));
    add(tu("GPU-Backen: Build > [Final] Fast", [] {
        karteDialogeZu();
        const auto& v = ke::editor().bauProjekt.eintraege;
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (v[i].name == "[Final] Fast") {
                ke::bauStarten(i);
                return;
            }
        }
        meldeFehler("GPU-Backen: Eintrag [Final] Fast fehlt");
    }));
    add(warteBis("GPU-Backen: Fortschrittsfenster mit der GPU", [] {
        const ke::BauFortschrittStand st = ke::bauFortschrittStand();
        return !st.laeuft || (st.schritt >= 2 && !ke::editor().bau.gpuAdapter.empty());
    }, 3000));
    add(hb::fotoTeil("h_karte_gpubacken_fortschritt", [] { return fensterRahmen("###kartebaufortschritt"); }));
    add(stufe8::warteSek("GPU-Backen: Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(pause(0.3));
    add(pruefe("GPU-Backen: der -light-Befehl hatte -bhedgpubake; GPU und Abweichungsbericht in der Konsole (100 % gleich) oder keine GPU nutzbar", [] {
        const std::string k = konsoleText();
        const bool schalter = k.find("-bhedgpubake") != std::string::npos;
        const std::string& bericht = ke::editor().bau.gpuBericht;
        const bool gpu = !ke::editor().bau.gpuAdapter.empty() && bericht.find("100.0000") != std::string::npos &&
                         k.find(ke::editor().bau.gpuAdapter) != std::string::npos;
        const bool keine = bericht == ke::trK(Str::KeGpuKeineGpu);
        diag::detail("Knopftest: GPU-Backen - Schalter " + std::to_string(schalter ? 1 : 0) + ", GPU \"" + ke::editor().bau.gpuAdapter + "\", Bericht: " + bericht);
        return schalter && (gpu || keine);
    }));
    add(pruefe("GPU-Backen: alle 18 Lumps der .bsp Byte fuer Byte die des CPU-Baus (kein Leck, kein Fehler)", [cpuBsp] {
        const std::string g = dateiInhalt(stufe8c::stamm() + ".bsp");
        std::string lumps;   // die verschiedenen Lumps (Kopf: 18 Paare Offset/Laenge ab Byte 8)
        for (int i = 0; i < 18 && g.size() >= 152 && cpuBsp->size() >= 152; ++i) {
            std::uint32_t oa = 0, la = 0, ob = 0, lb = 0;
            std::memcpy(&oa, g.data() + 8 + 8 * i, 4);
            std::memcpy(&la, g.data() + 12 + 8 * i, 4);
            std::memcpy(&ob, cpuBsp->data() + 8 + 8 * i, 4);
            std::memcpy(&lb, cpuBsp->data() + 12 + 8 * i, 4);
            if (la != lb || oa + la > g.size() || ob + lb > cpuBsp->size() || std::memcmp(g.data() + oa, cpuBsp->data() + ob, la) != 0) {
                lumps += " " + std::to_string(i);
            }
        }
        diag::detail("Knopftest: GPU-Backen - bsp " + std::to_string(g.size()) + " gegen " + std::to_string(cpuBsp->size()) + " Bytes, andere Lumps:" + lumps);
        for (const ke::KonsolenZeile& z : ke::editor().konsole) {   // Fehlerzeilen ins Protokoll
            for (const auto& t : z.teile) {
                if (t.art == ke::KonsolenZeile::Art::Fehler) { diag::detail("Knopftest: GPU-Backen - Fehlerzeile: " + z.text.substr(t.von, t.laenge)); }
            }
        }
        // alle 18 Lumps gleich (die Datei selbst traegt zwischen Kopf und Lumps die Uhrzeit des Baus)
        return !g.empty() && g.size() == cpuBsp->size() && lumps.empty() && !stufe3::konsoleArtMit(ke::KonsolenZeile::Art::Fehler, "");
    }));
    add(hb::fotoTeil("h_karte_gpubacken_bericht", stufe8c::editorRahmen));
    // Stufe 8d: Auto
    add(klick(ke::trK(Str::KeLBuild), "###main"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeGpuMenue)) + "##baugpubacken"));
    add(pause(0.3));
    add(klick(std::string(ke::trK(Str::KeGpuAuto)) + "##baugpuauto"));
    add(pause(0.3));
    add(pruefe("GPU-Backen: wieder Auto und gespeichert (BhedGpuBakeMode=auto)", [] {
        return ke::editor().bau.gpuBacken == karte::GpuBacken::Auto && g_app->settings.karte["BhedGpuBakeMode"] == "auto";
    }));
    add(tu("GPU-Backen Auto: Konsole leeren, Build > [Final] Fast", [] {
        karteDialogeZu();
        ke::konsoleLeeren();
        ke::editor().bau.gpuWahlText.clear();
        const auto& v = ke::editor().bauProjekt.eintraege;
        for (std::size_t i = 0; i < v.size(); ++i) {
            if (v[i].name == "[Final] Fast") {
                ke::bauStarten(i);
                return;
            }
        }
        meldeFehler("GPU-Backen Auto: Eintrag [Final] Fast fehlt");
    }));
    add(stufe8::warteSek("GPU-Backen Auto: Bau fertig", [] { return !ke::bauLaeuft(); }, 300));
    add(pause(0.3));
    add(pruefe("GPU-Backen Auto: -bhedgpubakeauto mit den gemerkten Zeiten, Wahl und Grund in der Konsole, Lumps gleich dem CPU-Bau, Zeit gemerkt",
               [cpuBsp] {
        const std::string k = konsoleText();
        const std::string& w = ke::editor().bau.gpuWahlText;
        const bool schalter = k.find("-bhedgpubakeauto -bhedgpubakezeiten ") != std::string::npos;
        const bool text = !w.empty() && k.find(w) != std::string::npos;
        const std::string g = dateiInhalt(stufe8c::stamm() + ".bsp");
        bool lumpsGleich = g.size() == cpuBsp->size() && g.size() >= 152;
        for (int i = 0; i < 18 && lumpsGleich; ++i) {
            std::uint32_t oa = 0, la = 0, ob = 0, lb = 0;
            std::memcpy(&oa, g.data() + 8 + 8 * i, 4);
            std::memcpy(&la, g.data() + 12 + 8 * i, 4);
            std::memcpy(&ob, cpuBsp->data() + 8 + 8 * i, 4);
            std::memcpy(&lb, cpuBsp->data() + 12 + 8 * i, 4);
            lumpsGleich = la == lb && oa + la <= g.size() && ob + lb <= cpuBsp->size() && std::memcmp(g.data() + oa, cpuBsp->data() + ob, la) == 0;
        }
        bool gemerkt = false;
        for (std::size_t pos = k.find("-light"); pos != std::string::npos && !gemerkt; pos = k.find("-light", pos + 1)) {
            const std::size_t ende = k.find('\n', pos);
            const std::string zeile = "q " + k.substr(pos, ende == std::string::npos ? std::string::npos : ende - pos);
            const karte::GpuZeiten z = ke::gpuZeitenHolen(ke::editor().name, karte::lichtSchalter(zeile));
            gemerkt = z.cpuMs > 0 || z.gpuMs > 0;
        }
        diag::detail("Knopftest: GPU-Backen Auto - Schalter " + std::to_string(schalter ? 1 : 0) + ", Wahl \"" + w + "\", Lumps gleich " +
                     std::to_string(lumpsGleich ? 1 : 0) + ", gemerkt " + std::to_string(gemerkt ? 1 : 0));
        return schalter && text && lumpsGleich && gemerkt;
    }));
    add(tu("GPU-Backen: Ende - Dialoge zu", [] {
        karteDialogeZu();
        ke::konsoleLeeren();
    }));
    add(pause(0.2));
    return s;
}

}  // namespace stufe8g
