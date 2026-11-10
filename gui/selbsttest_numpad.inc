// selbsttest_numpad.inc - rc612 NUMPAD: der Ziffernblock setzt im Karten-Editor das Raster (karteneditor, BHED_KARTE_NUR=numpad).
//
// Wunsch eines Mappers (shank 09.11.): "the NUMPAD numbers should change grids in mapping". Vorgabe: Num 0-9 tun, was die
// obere Ziffernreihe tut (NRC: 1..9 = SetGrid1..SetGrid256, 0 = ToggleGrid; grid.cpp:192-200, xywindow.cpp:2216).
// Der Schalter "Numpad sets the grid" (Menue Grid und Preferences > Grid) stellt ausgeschaltet NRCs Belegung her: Num 1
// ViewFront, Num 3 ViewSide, Num 7 ViewTop (xywindow.cpp:2273-2275).

namespace numpad {

namespace ke = karteneditor;

// Die Raster der Ziffern 1..9 der oberen Reihe (SetGrid1 .. SetGrid256)
float rasterDerZiffer(int d) { return static_cast<float>(1 << (d - 1)); }

// Der Zustand vor dem Teil (der volle Lauf geht danach weiter)
struct Vorher {
    int potenz = 0;
    karte::Sicht sicht = karte::Sicht::XY;
    bool raster = true;
};

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto vorher = std::make_shared<Vorher>();
    add(tu("Numpad: Raster 16, Ansicht XY, Raster sichtbar", [vorher] {
        ke::Editor& e = ke::editor();
        *vorher = Vorher{e.raster.potenz(), e.xy[0].ansicht.typ, e.anzeige.raster};
        e.raster.setzePotenz(4);
        e.xy[0].ansicht.typ = karte::Sicht::XY;
        e.anzeige.raster = true;
        ++e.sichtStand;
        e.letzterBefehl.clear();
    }));
    add(pause(0.3));
    // --- Punkt 1: jede Ziffer des Ziffernblocks setzt das Raster der gleichen Ziffer oben (rot bis rc611 fuer 1/3/7: Ansicht)
    for (int d = 1; d <= 9; ++d) {
        add(tasteKombi(static_cast<ImGuiKey>(ImGuiKey_Keypad0 + d), "Num " + std::to_string(d)));
        add(pruefe("Numpad Vorgabe: Num " + std::to_string(d) + " = SetGrid" + std::to_string(1 << (d - 1)) + ", Ansicht bleibt XY", [d] {
            const ke::Editor& e = ke::editor();
            diag::detail("Knopftest: Numpad - Raster " + std::to_string(e.raster.groesse()) + ", Ansicht " +
                         std::to_string(static_cast<int>(e.xy[0].ansicht.typ)) + ", zuletzt " + e.letzterBefehl);
            return e.raster.groesse() == rasterDerZiffer(d) && e.xy[0].ansicht.typ == karte::Sicht::XY;
        }));
    }
    add(tasteKombi(ImGuiKey_Keypad0, "Num 0"));
    add(pruefe("Numpad Vorgabe: Num 0 = ToggleGrid wie die obere 0 (Raster aus)", [] {
        diag::detail("Knopftest: Numpad - zuletzt " + ke::editor().letzterBefehl);
        return !ke::editor().anzeige.raster && ke::editor().letzterBefehl == "ToggleGrid";
    }));
    add(tasteKombi(ImGuiKey_Keypad0, "Num 0"));
    add(pruefe("Numpad Vorgabe: Num 0 nochmal - Raster wieder an", [] { return ke::editor().anzeige.raster; }));
    add(tu("Numpad: Zustand wie vorher, echte Tastatur", [vorher] {
        ke::Editor& e = ke::editor();
        e.raster.setzePotenz(vorher->potenz);
        e.xy[0].ansicht.typ = vorher->sicht;
        e.anzeige.raster = vorher->raster;
        ++e.sichtStand;
        ke::tastaturErsatz = nullptr;
    }));
    return s;
}

}  // namespace numpad
