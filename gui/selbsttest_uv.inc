// selbsttest_uv.inc - Modell-Block 1b: UV erstellen (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=uv | uvmd3 | uvspiel | uvzeit | uvhb)
//
//   uv        Sturmtruppler (JKA-Daten), Bereich UV, wie 3ds Max:
//     Fenster         der UV-Editor neben den Ansichten mit der Textur der Flaeche (Shader und Haut wie die Vorschau),
//                     Dreiecke gezeichnet, die Naehte auch auf dem Modell
//     Erreichbarkeit  jede UV-Aktion im Menue "UV" (Untermenue je Gruppe) mit Tooltip, im Rechtsklick der Ansicht
//                     (UV > Gruppe), in der Suche (X wie 3ds Max, F3 wie Blender); die Tasten je Modus wie die Tabellen
//                     (max_hotkeys_de.txt, blender_keys.txt), sonst wie WhiteoutFlakes (uv.md) nur im Max-Modus
//     Tasten          ueber dem Editor: Max 2/4 -> Kante/Flaeche, Blender 1/4 -> Ecke/Insel
//     je Befehl       auf einer halben Flaeche (Dreiecke mit x > 0): Naht markieren/loeschen, Abwickeln, Projektion,
//                     Entspannen, Pin, Sperren, Begradigen, Ausrichten, Dichte, Packen, Quick unwrap Figur/Requisit/Hart,
//                     Stapeln: je Befehl die Wirkung gemessen (Marken, Inseln, Ueberlappung, Pins, Dichte) und ein
//                     Rueckgaengig-Schritt, der den Inhalt Byte fuer Byte zurueckbringt
//     Speichern       Figur abgewickelt -> .glm im Laufordner -> gelesen: UV je Dreiecksecke gleich, Ecken unter 1000
//   uvmd3     ein .md3 aus den Spieldaten im Model-Tab: geoeffnet, abgewickelt, gespeichert, gelesen: Bilder und Tags gleich
//   uvspiel   Sturmtruppler mit "Requisit" abgewickelt, die Textur ueber die alte UV in die neue umgebacken, als Mod im
//             Laufordner (BHED_UV_MOD) fuer den Vergleich im Spiel; Vorschau-Foto von vorn
//   uvzeit    Bildzeit allein (BHED_D3D_DEBUG=0): Bereich Ansehen gegen Bereich UV mit offenem Editor (ganzes Modell)
//   uvhb      Handbuchbilder
// Eingebunden in selbsttest.cpp (nach selbsttest_rt.inc).
namespace uv1b {

using rig3::imMenue;
using rig3::klickMenue;
using rig3::woIst;

namespace w = modell::wof;

struct Zustand {
    std::string vorher;     // Inhalt als .glm vor einem Befehl
    int marken = 0;
    int inseln = 0;
    int pins = 0;
    std::size_t undo = 0;
};

std::string glmJetzt() {
    std::string b;
    (void)modell::glmSchreiben(g_app->mtab.dok.inhalt(), b);
    return b;
}

int markenZahl() {
    modellUvNetzAktualisieren();
    return static_cast<int>(std::count(g_app->mtab.uv.netz.naht.begin(), g_app->mtab.uv.netz.naht.end(), std::uint8_t{1}));
}

int teilVon(const std::string& name) { return g_app->mtab.dok.inhalt().teilIndex(name); }

std::string inselText(int i) {
    const ModellUv& u = g_app->mtab.uv;
    const w::UvKasten k = w::kastenVon(u.netz, u.inseln, i);
    float fl = 0.0F;
    for (const int d : u.inseln.dreieckeVon(i)) {
        const w::V2 a = u.netz.uv[static_cast<std::size_t>(d * 3)];
        const w::V2 b = u.netz.uv[static_cast<std::size_t>(d * 3 + 1)];
        const w::V2 c = u.netz.uv[static_cast<std::size_t>(d * 3 + 2)];
        fl += std::abs((b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x)) * 0.5F;
    }
    char b[200];
    std::snprintf(b, sizeof b, "Insel %d: %d Dreiecke, UV-Flaeche %.6f, Kasten %.4f %.4f - %.4f %.4f, gepinnt %d, Teil %d", i, static_cast<int>(u.inseln.dreieckeVon(i).size()),
                  static_cast<double>(fl), static_cast<double>(k.tief.x), static_cast<double>(k.tief.y), static_cast<double>(k.hoch.x), static_cast<double>(k.hoch.y),
                  w::inselGepinnt(u.netz, u.inseln, i) ? 1 : 0, u.inseln.dreieckeVon(i).empty() ? -1 : u.netz.teil[static_cast<std::size_t>(u.inseln.dreieckeVon(i).front() * 3)]);
    return b;
}

// Die Dreiecke des Netzes, deren Ecken alle gewaehlt sind
std::vector<int> gewaehlteDreieckeJetzt() {
    const ModellUv& u = g_app->mtab.uv;
    std::set<std::pair<int, int>> wahl;
    for (const modell::EckOrt& o : g_app->mtab.eckenWahl) { wahl.insert({o.teil, o.ecke}); }
    std::vector<int> d;
    for (int t = 0; t < u.netz.dreiecke(); ++t) {
        bool alle = true;
        for (int k = 0; k < 3; ++k) {
            const int h = t * 3 + k;
            alle = alle && wahl.count({u.netz.teile[static_cast<std::size_t>(u.netz.teil[static_cast<std::size_t>(h)])], u.netz.ecke[static_cast<std::size_t>(h)]}) != 0;
        }
        if (alle) { d.push_back(t); }
    }
    return d;
}

// Die Ecken der Dreiecke eines Teils, deren Mitte bei x > 0 liegt (die halbe Flaeche), als Auswahl des Model-Tabs
void halbeFlaecheWaehlen(const std::string& name) {
    ModellTab& t = g_app->mtab;
    t.eckenWahl.clear();
    t.auswahl.clear();
    const int ti = teilVon(name);
    if (ti < 0) {
        meldeFehler("UV: Flaeche " + name + " fehlt");
        return;
    }
    const modell::Netz& n = t.dok.inhalt().teile[static_cast<std::size_t>(ti)].stufen[static_cast<std::size_t>(t.lodJetzt)];
    std::set<int> e;
    for (int d = 0; d < n.dreiecke(); ++d) {
        float mx = 0.0F;
        for (int k = 0; k < 3; ++k) { mx += n.ecken[static_cast<std::size_t>(n.kanten[static_cast<std::size_t>(d * 3 + k)].ecke)].ort[0]; }
        if (mx <= 0.0F) { continue; }
        for (int k = 0; k < 3; ++k) { e.insert(n.kanten[static_cast<std::size_t>(d * 3 + k)].ecke); }
    }
    for (const int x : e) { t.eckenWahl.push_back(modell::EckOrt{ti, x}); }
    g_app->modelDirty = true;
}

void ganzeFlaecheWaehlen(const std::string& name) {
    ModellTab& t = g_app->mtab;
    t.eckenWahl.clear();
    t.auswahl.clear();
    const int ti = teilVon(name);
    if (ti < 0) { return; }
    const modell::Netz& n = t.dok.inhalt().teile[static_cast<std::size_t>(ti)].stufen[static_cast<std::size_t>(t.lodJetzt)];
    for (int e = 0; e < static_cast<int>(n.ecken.size()); ++e) { t.eckenWahl.push_back(modell::EckOrt{ti, e}); }
}

int aktion(Str s) {
    for (int i = 0; i < modellUvAktionZahl(); ++i) {
        if (std::string(modellUvAktionText(i)) == tr(s)) { return i; }
    }
    return -1;
}

Schritt ausloesen(Str s) {
    return tu(std::string("UV: ") + tr(s), [s] {
        const int i = aktion(s);
        if (i < 0) {
            meldeFehler(std::string("UV: Aktion fehlt: ") + tr(s));
            return;
        }
        modellUvAktionAusloesen(i);
    });
}

// Eine Gruppe im Menue: jede Aktion steht da, mit Tooltip
Schritt gruppeVoll(int gruppe, const std::string& wo) {
    return pruefe("UV " + wo + ": jede Aktion der Gruppe " + std::to_string(gruppe) + " steht da, mit Tooltip", [gruppe, wo] {
        int n = 0;
        for (int i = 0; i < modellUvAktionZahl(); ++i) {
            if (modellUvAktionGruppe(i) != gruppe) { continue; }
            const std::string text = modellUvAktionText(i);
            if (imMenue(text) == nullptr || std::string(modellUvAktionHinweis(i)).size() < 10U) {
                meldeFehler("UV: fehlt im Menue oder ohne Tooltip: " + text + " " + woIst(text));
                return false;
            }
            ++n;
        }
        diag::detail("Knopftest: UV " + wo + " Gruppe " + std::to_string(gruppe) + " - " + std::to_string(n) + " Aktionen mit Tooltip");
        return n >= 3;
    });
}

const Str kGruppen[6] = {Str::UvGrAuswahl, Str::UvGrNaehte, Str::UvGrAbwickeln, Str::UvGrAnordnen, Str::UvGrSchnell, Str::UvGrAnzeige};

void menueAlle(std::vector<Schritt>& s, bool rechtsklick) {
    for (int g = 0; g < 6; ++g) {
        s.push_back(taste(ImGuiKey_Escape, "Esc"));
        s.push_back(taste(ImGuiKey_Escape, "Esc"));
        s.push_back(taste(ImGuiKey_Escape, "Esc"));
        s.push_back(pause(0.3));
        if (rechtsklick) {
            s.push_back(klickAn("Rechtsklick in die Ansicht", [] { return kv5::fokusPunkt(); }, 1));
            s.push_back(pause(0.3));
            s.push_back(klickMenue(tr(Str::UvMenue), false));
        } else {
            // die Menueleiste: nur oeffnen, wenn "UV" zu ist (ein Klick auf das offene Menue schliesst es wieder)
            s.push_back(klickAn("\"UV\" (Menueleiste, falls zu)", [] {
                if (imMenue(tr(kGruppen[0])) != nullptr) { return imMenue(tr(kGruppen[0]))->rect.GetCenter(); }
                const Element* e = rig3::menueLeiste(tr(Str::UvMenue));
                if (e == nullptr) {
                    meldeFehler("UV: Menue UV nicht in der Leiste");
                    return ImVec2{};
                }
                return e->rect.GetCenter();
            }));
        }
        s.push_back(pause(0.3));
        s.push_back(klickMenue(tr(kGruppen[g]), false));
        s.push_back(pause(0.3));
        s.push_back(gruppeVoll(g, rechtsklick ? "Rechtsklick > UV" : "Menue UV"));
    }
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
}

void ueberSuche(std::vector<Schritt>& s, Str text) {
    s.push_back(taste(ImGuiKey_Escape, "Esc"));
    s.push_back(tu("UV: Fokus im Model-Tab", [] { g_app->mtab.fokus = true; }));
    s.push_back(ik6::tasteSuche());
    s.push_back(pause(0.2));
    s.push_back(pruefe("UV Suche: Alle Aktionen suchen ist offen (X / F3)", [] { return g_app->mtab.aktionenSichtbar; }));
    const std::string zeile = tr(text);
    s.push_back(tippe(zeile));
    s.push_back(pause(0.2));
    s.push_back(klickAn("\"" + zeile + "\" in der Suche", [zeile] {
        for (const Element& e : g_elemente) {
            if ((e.label == zeile || e.label.rfind(zeile + " (", 0) == 0) && e.fenster.find("##Popup_") != std::string::npos) { return e.rect.GetCenter(); }
        }
        meldeFehler("UV: " + zeile + " nicht in der Suche: " + woIst(zeile));
        return ImVec2{};
    }));
    s.push_back(pause(0.3));
}

ImVec2 leinwandMitte() {
    const ModellUv& u = g_app->mtab.uv;
    return ImVec2{u.leinwand[0] + u.leinwand[2] * 0.5F, u.leinwand[1] + u.leinwand[3] * 0.5F};
}

void figurLaden(std::vector<Schritt>& s, const char* modus) {
    s.push_back(tu("UV: Skript fuer den Test", [] { openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "uv.txt"); }));
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { s.push_back(std::move(x)); }
    const std::string m = modus;
    s.push_back(tu("UV: Bereich UV, " + m, [m] {
        modellBedienungSetzen(m == "blender" ? 1 : 0);
        g_app->mtab.bereich = ModellBereich::Uv;
        g_app->mtab.uv.offen = true;
        g_app->mtab.uv.gruppe = -1;
        g_app->mtab.lodWahl = 1;   // LOD 0 fest (die Ansicht waehlt sonst nach dem Abstand)
        g_app->animPlaying = false;
        g_app->modelDirty = true;
    }));
    s.push_back(pause(0.8));
}

// Ein Befehl mit Wirkung und Rueckgaengig: vorher Inhalt merken, Befehl, pruefen, Rueckgaengig -> Byte fuer Byte wie vorher
void befehlRund(std::vector<Schritt>& s, const std::shared_ptr<Zustand>& z, Str befehl, const std::string& was, const std::function<bool(const Zustand&, std::string&)>& wirkung) {
    s.push_back(tu(std::string("UV: vor ") + tr(befehl), [z] {
        z->vorher = glmJetzt();
        z->marken = markenZahl();
        z->inseln = g_app->mtab.uv.inseln.zahl;
        z->pins = static_cast<int>(std::count(g_app->mtab.uv.netz.pin.begin(), g_app->mtab.uv.netz.pin.end(), std::uint8_t{1}));
    }));
    s.push_back(ausloesen(befehl));
    s.push_back(pause(0.3));
    s.push_back(pruefe(std::string("UV ") + tr(befehl) + ": " + was, [z, wirkung, befehl] {
        modellUvNetzAktualisieren();
        std::string info;
        const bool ok = wirkung(*z, info);
        diag::detail(std::string("Knopftest: UV ") + tr(befehl) + " - " + info + " | " + g_app->mtab.uv.meldung);
        return ok;
    }));
    s.push_back(tu(std::string("UV: Rueckgaengig nach ") + tr(befehl), [] { (void)g_app->mtab.dok.rueckgaengig(); }));
    s.push_back(pause(0.2));
    s.push_back(pruefe(std::string("UV ") + tr(befehl) + ": Rueckgaengig bringt das Modell Byte fuer Byte zurueck", [z] { return glmJetzt() == z->vorher; }));
}

std::vector<Schritt> modellSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto z = std::make_shared<Zustand>();
    figurLaden(s, "max");
    // --- Fenster ---
    add(pruefe("UV Fenster: der Editor steht neben den Ansichten, mit der Textur der Flaeche, Dreiecke gezeichnet, Naehte auf dem Modell", [] {
        const ModellUv& u = g_app->mtab.uv;
        diag::detail("Knopftest: UV Fenster " + std::to_string(static_cast<int>(u.leinwand[2])) + " x " + std::to_string(static_cast<int>(u.leinwand[3])) + ", Textur " +
                     std::to_string(u.texturDa ? 1 : 0) + ", " + std::to_string(u.dreieckeGezeichnet) + " Dreiecke, " + std::to_string(u.nahtStriche3d) + " Naht-Striche 3D, " +
                     std::to_string(u.inseln.zahl) + " Inseln");
        return u.leinwand[2] > 200.0F && u.texturDa && u.dreieckeGezeichnet > 100 && u.nahtStriche3d > 0 && u.inseln.zahl > 0;
    }));
    // --- Erreichbarkeit ---
    menueAlle(s, false);
    menueAlle(s, true);
    ueberSuche(s, Str::UvPacken);
    add(pruefe("UV Suche: Packen ausgeloest", [] { return g_app->mtab.uv.letzterBefehl == tr(Str::UvPacken); }));
    add(pruefe("UV Tasten je Modus wie die Tabellen (Max 1/2/4/5 Q W E R S G Z, Blender 1/2/3/4 G R S U; WF-Tasten nur wie Max)", [] {
        struct Soll {
            Str s;
            const char* max;
            const char* bl;
        };
        const Soll soll[] = {{Str::UvEbeneEcke, "1", "1"},           {Str::UvEbeneKante, "2", "2"},        {Str::UvEbeneFlaeche, "4", "3"},
                             {Str::UvEbeneInsel, "5", "4"},          {Str::UvWerkzeugAuswahl, "Q", ""},    {Str::UvWerkzeugVerschieben, "W", "G"},
                             {Str::UvWerkzeugDrehen, "E", "R"},      {Str::UvWerkzeugSkalieren, "R", "S"}, {Str::UvPixelFang, "S", "Shift+Tab"},
                             {Str::UvRaster, "G", ""},               {Str::UvEinpassen, "Z", ""},          {Str::UvMenueAbwickeln, "", "U"},
                             {Str::UvNahtMarkieren, "C", ""},        {Str::UvAbwickeln, "U", ""},          {Str::UvPacken, "Shift+P", ""}};
        for (const Soll& x : soll) {
            const int i = aktion(x.s);
            if (i < 0 || std::string(modellUvAktionTaste(i, false)) != x.max || std::string(modellUvAktionTaste(i, true)) != x.bl) {
                meldeFehler(std::string("UV Taste falsch: ") + tr(x.s));
                return false;
            }
        }
        return true;
    }));
    add(tu("UV: Maus ueber den Editor", [] { setzeMaus(leinwandMitte()); }));
    add(pause(0.2));
    add(taste(ImGuiKey_2, "2"));
    add(pruefe("UV Taste wie 3ds Max ueber dem Editor: 2 -> Kante", [] { return g_app->mtab.uv.ebene == 1 && g_app->mtab.auswahlArt == AuswahlArt::Kante; }));
    add(taste(ImGuiKey_4, "4"));
    add(pruefe("UV Taste wie 3ds Max: 4 -> Flaeche (Polygonebene)", [] { return g_app->mtab.uv.ebene == 2 && g_app->mtab.auswahlArt == AuswahlArt::Dreieck; }));
    add(tu("UV: wie Blender", [] { modellBedienungSetzen(1); }));
    add(taste(ImGuiKey_4, "4"));
    add(pruefe("UV Taste wie Blender: 4 -> Insel (use_uv_select_island)", [] { return g_app->mtab.uv.ebene == 3; }));
    add(taste(ImGuiKey_1, "1"));
    add(pruefe("UV Taste wie Blender: 1 -> Ecke", [] { return g_app->mtab.uv.ebene == 0; }));
    add(tu("UV: Maus in die Ansicht, wie Blender U", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.2));
    add(taste(ImGuiKey_U, "U"));
    add(pause(0.3));
    add(pruefe("UV wie Blender: U in der Ansicht oeffnet das Menue Abwickeln (VIEW3D_MT_uv_map)", [] { return imMenue(tr(Str::UvAbwickeln)) != nullptr; }));
    add(taste(ImGuiKey_Escape, "Esc"));
    add(tu("UV: wie 3ds Max, Ebene Flaeche", [] {
        modellBedienungSetzen(0);
        modellUvAktionAusloesen(aktion(Str::UvEbeneFlaeche));
    }));
    // --- je Befehl ---
    add(tu("UV: halber Torso gewaehlt (Flaeche)", [] { halbeFlaecheWaehlen("torso"); }));
    add(pause(0.3));
    befehlRund(s, z, Str::UvNahtMarkieren, "der Rand der Auswahl wird Naht (mehr Marken)", [](const Zustand& v, std::string& i) {
        const int m = markenZahl();
        i = std::to_string(v.marken) + " -> " + std::to_string(m) + " Marken";
        return m > v.marken;
    });
    befehlRund(s, z, Str::UvNahtLoeschen, "die Naehte in der Auswahl weg (weniger Marken)", [](const Zustand& v, std::string& i) {
        const int m = markenZahl();
        i = std::to_string(v.marken) + " -> " + std::to_string(m) + " Marken";
        return m < v.marken;
    });
    befehlRund(s, z, Str::UvAbwickeln, "die halbe Flaeche wird neue Stuecke, ohne Kippen, keines liegt auf einer anderen Insel", [](const Zustand& v, std::string& i) {
        const ModellUv& u = g_app->mtab.uv;
        const std::vector<int> d = gewaehlteDreieckeJetzt();
        const w::Verzerrung m = w::messen(u.netz, u.inseln, 1024, d);
        const std::vector<int> neu = w::inselnVonDreiecken(u.inseln, d);
        int ueber = 0;
        for (const auto& [a, b] : u.befunde.ueberlappPaare) {
            if (std::binary_search(neu.begin(), neu.end(), a) || std::binary_search(neu.begin(), neu.end(), b)) {
                if (ueber < 4) { diag::detail("Knopftest: UV Abwickeln Ueberlappung " + inselText(a) + " | " + inselText(b)); }
                ++ueber;
            }
        }
        i = std::to_string(v.inseln) + " -> " + std::to_string(u.inseln.zahl) + " Inseln, " + std::to_string(neu.size()) + " neue Stuecke aus " + std::to_string(d.size()) +
            " Dreiecken, " + std::to_string(m.gekippt) + " Dreiecke gekippt, Winkel " + std::to_string(m.winkelGrad) + ", " + std::to_string(ueber) + " Ueberlappungen mit neuen";
        return glmJetzt() != v.vorher && !d.empty() && m.gekippt == 0 && ueber == 0;
    });
    befehlRund(s, z, Str::UvProjKasten, "Kasten-Projektion: mehr Inseln", [](const Zustand& v, std::string& i) {
        i = std::to_string(v.inseln) + " -> " + std::to_string(g_app->mtab.uv.inseln.zahl) + " Inseln";
        return g_app->mtab.uv.inseln.zahl > v.inseln;
    });
    befehlRund(s, z, Str::UvEntspannen, "die UV aendert sich", [](const Zustand& v, std::string& i) {
        i = g_app->mtab.uv.meldung;
        return glmJetzt() != v.vorher;
    });
    befehlRund(s, z, Str::UvPin, "die gewaehlten Ecken sind gepinnt", [](const Zustand& v, std::string& i) {
        const int p = static_cast<int>(std::count(g_app->mtab.uv.netz.pin.begin(), g_app->mtab.uv.netz.pin.end(), std::uint8_t{1}));
        i = std::to_string(v.pins) + " -> " + std::to_string(p) + " Pins";
        return p > v.pins;
    });
    befehlRund(s, z, Str::UvSperren, "die Inseln der Auswahl sind ganz gepinnt", [](const Zustand& v, std::string& i) {
        const int p = static_cast<int>(std::count(g_app->mtab.uv.netz.pin.begin(), g_app->mtab.uv.netz.pin.end(), std::uint8_t{1}));
        i = std::to_string(v.pins) + " -> " + std::to_string(p) + " Pins";
        return p > v.pins;
    });
    befehlRund(s, z, Str::UvAusrichten, "die Inseln werden gedreht", [](const Zustand& v, std::string& i) {
        i = g_app->mtab.uv.meldung;
        return glmJetzt() != v.vorher;
    });
    befehlRund(s, z, Str::UvDichte, "die Inseln haben dieselbe Dichte", [](const Zustand& v, std::string& i) {
        const ModellUv& u = g_app->mtab.uv;
        i = "Dichte " + std::to_string(u.mass.dichteMin) + " / " + std::to_string(u.mass.dichteMax);
        return glmJetzt() != v.vorher && u.mass.dichteMax > 0.0F;
    });
    add(tu("UV: Packen - Alles", [] { g_app->mtab.uv.packUmfang = 2; }));
    befehlRund(s, z, Str::UvPacken, "ohne Ueberlappung", [](const Zustand& v, std::string& i) {
        const ModellUv& u = g_app->mtab.uv;
        for (std::size_t k = 0; k < u.befunde.ueberlappPaare.size() && k < 6U; ++k) {
            diag::detail("Knopftest: UV Packen Ueberlappung " + inselText(u.befunde.ueberlappPaare[k].first) + " | " + inselText(u.befunde.ueberlappPaare[k].second));
        }
        i = std::to_string(u.befunde.ueberlappt.size()) + " ueberlappend, " + u.meldung;
        return glmJetzt() != v.vorher && u.befunde.ueberlappt.empty();
    });
    add(tu("UV: Kante, eine Reihe am Rand gewaehlt (Begradigen)", [] {
        modellUvAktionAusloesen(aktion(Str::UvEbeneKante));
        // die Ecken einer Randschleife der groessten Insel
        modellUvNetzAktualisieren();
        const ModellUv& u = g_app->mtab.uv;
        g_app->mtab.eckenWahl.clear();
        int gross = 0;
        for (int k = 1; k < u.inseln.zahl; ++k) {
            if (u.inseln.dreieckeVon(k).size() > u.inseln.dreieckeVon(gross).size()) { gross = k; }
        }
        const std::span<const int> r = u.inseln.randVon(gross);
        std::vector<int> h;
        for (std::size_t k = 0; k < r.size() && k < 6U; ++k) { h.push_back(r[k]); }
        std::set<std::pair<int, int>> e;
        for (const int x : h) {
            for (const int y : {x, w::UvNetz::naechste(x)}) { e.insert({u.netz.teile[static_cast<std::size_t>(u.netz.teil[static_cast<std::size_t>(y)])], u.netz.ecke[static_cast<std::size_t>(y)]}); }
        }
        for (const auto& [t, k] : e) { g_app->mtab.eckenWahl.push_back(modell::EckOrt{t, k}); }
    }));
    befehlRund(s, z, Str::UvBegradigen, "die Reihe liegt auf einer Geraden", [](const Zustand& v, std::string& i) {
        i = g_app->mtab.uv.meldung;
        return glmJetzt() != v.vorher;
    });
    add(tu("UV: nichts gewaehlt, Ebene Flaeche", [] {
        modellUvAktionAusloesen(aktion(Str::UvEbeneFlaeche));
        g_app->mtab.eckenWahl.clear();
        g_app->mtab.auswahl.clear();
    }));
    for (const Str q : {Str::UvSchnellFigur, Str::UvSchnellRequisit, Str::UvSchnellHart}) {
        befehlRund(s, z, q, "ohne Kippen, ohne Ueberlappung, in der Kachel, Ecken unter der Grenze", [q](const Zustand& v, std::string& i) {
            const ModellUv& u = g_app->mtab.uv;
            bool drin = true;
            for (const w::V2 p : u.netz.uv) { drin = drin && p.x >= -1e-4F && p.y >= -1e-4F && p.x <= 1.0001F && p.y <= 1.0001F; }
            int meiste = 0;
            for (const int t : u.netz.teile) {
                meiste = std::max(meiste, static_cast<int>(g_app->mtab.dok.inhalt().teile[static_cast<std::size_t>(t)].stufen[0].ecken.size()));
            }
            i = std::to_string(v.inseln) + " -> " + std::to_string(u.inseln.zahl) + " Inseln, " + std::to_string(u.befunde.stapel) + " Stapel, Winkel " +
                std::to_string(u.mass.winkelGrad) + ", Flaeche " + std::to_string(u.mass.flaeche) + ", Dichte " + std::to_string(u.mass.dichteMedian) + ", meiste Ecken " +
                std::to_string(meiste) + ", " + std::to_string(u.befunde.ueberlappt.size()) + " ueberlappend, " + std::to_string(u.mass.gekippt) + " Dreiecke gekippt, in der Kachel " +
                std::to_string(drin ? 1 : 0);
            const bool stapel = q != Str::UvSchnellFigur || u.befunde.stapel > 0;
            return glmJetzt() != v.vorher && u.befunde.ueberlappt.empty() && u.mass.gekippt == 0 && drin && meiste < 1000 && stapel;
        });
    }
    // --- Speichern ---
    add(ausloesen(Str::UvSchnellFigur));
    add(pause(0.3));
    add(tu("UV: als .glm in den Laufordner", [] {
        g_app->mtab.speicherPfad = mb3::lauf("uv_figur.glm");
        modellSpeichern(false);
    }));
    add(pause(0.3));
    add(pruefe("UV Speichern: gelesen hat jede Dreiecksecke dieselbe UV wie das Dokument", [] {
        modell::Inhalt y;
        if (!modell::ausGlm(mb3::datei(mb3::lauf("uv_figur.glm")), y)) { return false; }
        const modell::Inhalt& x = g_app->mtab.dok.inhalt();
        for (std::size_t t = 0; t < x.teile.size(); ++t) {
            for (std::size_t st = 0; st < x.teile[t].stufen.size(); ++st) {
                const modell::Netz& a = x.teile[t].stufen[st];
                const modell::Netz& b = y.teile[t].stufen[st];
                if (a.kanten.size() != b.kanten.size()) { return false; }
                for (std::size_t h = 0; h < a.kanten.size(); ++h) {
                    if (std::memcmp(a.ecken[static_cast<std::size_t>(a.kanten[h].ecke)].uv, b.ecken[static_cast<std::size_t>(b.kanten[h].ecke)].uv, 8) != 0) { return false; }
                }
            }
        }
        return true;
    }));
    return s;
}

// --- .md3 ------------------------------------------------------------------------------------------------------------
std::vector<Schritt> md3Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto pfad = std::make_shared<std::string>();
    add(tu("UV MD3: eine .md3 mit Bildern aus den Spieldaten", [pfad] {
        g_app->leftMode = 2;
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        std::vector<std::string> kand;
        for (const GamePath& gp : g_app->gamePaths) {
            for (const FoundFile& f : findByExtension(gp, {".md3"})) { kand.push_back(f.name); }
        }
        std::sort(kand.begin(), kand.end());
        kand.erase(std::unique(kand.begin(), kand.end()), kand.end());
        // die erste mit Bildern und Tags, deren Flaechen auch nach dem Teilen unter der Eckengrenze bleiben; sonst eine mit Bildern
        std::string ohneTags;
        for (const std::string& k : kand) {
            std::string b;
            Md3Model m;
            if (!readFromArchives(k, b) || !readMd3(b, m) || m.numFrames < 2 || m.surfaces.empty()) { continue; }
            bool klein = true;
            for (const Md3Surface& f : m.surfaces) { klein = klein && f.numVerts >= 30 && f.numVerts < 400; }
            if (!klein) { continue; }
            if (m.numTags > 0) {
                *pfad = k;
                break;
            }
            if (ohneTags.empty()) { ohneTags = k; }
        }
        if (pfad->empty()) { *pfad = ohneTags; }
        if (pfad->empty()) { meldeFehler("UV MD3: keine .md3 mit Bildern"); }
        diag::detail("Knopftest: UV MD3 - " + *pfad);
    }));
    add(tu("UV MD3: im Model-Tab oeffnen, Bereich UV", [pfad] {
        std::string f;
        if (!oeffneModellAusArchiv(*pfad, &f)) { meldeFehler("UV MD3: " + f); }
        g_app->mtab.bereich = ModellBereich::Uv;
        g_app->mtab.uv.offen = true;
        g_app->mtab.uv.gruppe = -1;
        modellBedienungSetzen(0);
    }));
    add(pause(0.8));
    add(pruefe("UV MD3: im Dokument als .md3, im Editor gezeichnet", [] {
        return g_app->mtab.dok.inhalt().herkunft == modell::Herkunft::Md3 && g_app->mtab.uv.dreieckeGezeichnet > 0 && g_app->mtab.uv.netz.kanten() > 0;
    }));
    add(tu("UV MD3: ohne Aenderung speichern", [] {
        g_app->mtab.speicherPfad = mb3::lauf("uv_roh.md3");
        modellSpeichern(false);
    }));
    add(pause(0.3));
    add(pruefe("UV MD3: ohne Aenderung Byte fuer Byte wie die Datei", [] { return mb3::datei(mb3::lauf("uv_roh.md3")) == *g_app->mtab.dok.inhalt().md3Datei; }));
    add(ausloesen(Str::UvSchnellRequisit));
    add(pause(0.3));
    add(tu("UV MD3: abgewickelt speichern", [] {
        g_app->mtab.speicherPfad = mb3::lauf("uv_neu.md3");
        modellSpeichern(false);
    }));
    add(pause(0.3));
    add(pruefe("UV MD3: gelesen - Bilder, Tags und Flaechen wie vorher, UV je Dreiecksecke wie das Dokument", [] {
        Md3Model alt;
        Md3Model neu;
        if (!readMd3(*g_app->mtab.dok.inhalt().md3Datei, alt) || !readMd3(mb3::datei(mb3::lauf("uv_neu.md3")), neu)) { return false; }
        if (alt.numFrames != neu.numFrames || alt.tags.size() != neu.tags.size() || alt.surfaces.size() != neu.surfaces.size()) { return false; }
        if (mb3::datei(mb3::lauf("uv_neu.md3")) == *g_app->mtab.dok.inhalt().md3Datei) {
            meldeFehler("UV MD3: abgewickelt ist die Datei unveraendert: " + g_app->mtab.uv.meldung);
            return false;
        }
        for (std::size_t b = 0; b < alt.tags.size(); ++b) {
            if (alt.tags[b].name != neu.tags[b].name || std::memcmp(alt.tags[b].origin, neu.tags[b].origin, 12) != 0) { return false; }
        }
        const modell::Inhalt& x = g_app->mtab.dok.inhalt();
        for (std::size_t t = 0; t < neu.surfaces.size(); ++t) {
            const modell::Netz& a = x.teile[t].stufen[0];
            const Md3Surface& b = neu.surfaces[t];
            for (std::size_t h = 0; h < a.kanten.size(); ++h) {
                if (std::memcmp(a.ecken[static_cast<std::size_t>(a.kanten[h].ecke)].uv, b.verts[b.indexes[h]].st, 8) != 0) { return false; }
            }
        }
        diag::detail("Knopftest: UV MD3 - " + std::to_string(neu.numFrames) + " Bilder, " + std::to_string(neu.tags.size()) + " Tags, " + g_app->mtab.uv.meldung);
        return true;
    }));
    return s;
}

// --- Bildzeit ------------------------------------------------------------------------------------------------------
std::vector<Schritt> zeitSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "max");
    auto messung = [&](const std::string& was, bool uv) {
        add(m1Messblock("Sturmtruppler", "UV Zeit: " + was, [uv] {
            g_app->mtab.bereich = uv ? ModellBereich::Uv : ModellBereich::Ansehen;
            g_app->mtab.uv.gruppe = -1;
            g_app->animPlaying = true;   // die Figur bewegt sich: jedes Bild neu
        }));
        add(tu("UV Zeit: Editor " + was, [] { diag::detail("Knopftest: UV Zeit - Editor im letzten Bild " + std::to_string(g_app->mtab.uv.bildMs) + " ms CPU"); }));
    };
    messung("Bereich Ansehen", false);
    messung("Bereich UV mit Editor (ganze Gruppe Torso)", true);
    add(tu("UV Zeit: Quick unwrap Figur, dann wieder messen", [] { modellUvAktionAusloesen(aktion(Str::UvSchnellFigur)); }));
    messung("Bereich UV nach Quick unwrap", true);
    return s;
}

// --- Handbuch (BHED_HB_NUR=uv) ---------------------------------------------------------------------------------------
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    figurLaden(s, "max");
    add(tu("UV Handbuch: Ansicht von vorn, eingepasst; der halbe Torso gewaehlt", [] {
        ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        a.art = AnsichtArt::Vorne;
        modellAnsichtEinpassenAktiv();
        halbeFlaecheWaehlen("torso");
        modellUvAktionAusloesen(aktion(Str::UvEbeneFlaeche));
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(tu("UV Handbuch: Maus in die Ansicht (kein Tooltip im Bild)", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_uv_editor"));
    add(tu("UV Handbuch: nichts gewaehlt, Quick unwrap Figur", [] {
        g_app->mtab.eckenWahl.clear();
        g_app->mtab.auswahl.clear();
        modellUvAktionAusloesen(aktion(Str::UvSchnellFigur));
    }));
    add(pause(0.6));
    add(rig3::rollenZu(tr(Str::UvGrPruefen), "mtrollouts"));
    add(tu("UV Handbuch: Maus in die Ansicht", [] { setzeMaus(kv5::fokusPunkt()); }));
    add(pause(0.8));
    add(hilfe2::schrittFoto("h_uv_schnell"));
    return s;
}

// --- Rundlauf (BHED_MODELL_NUR=uvrundlauf, BHED_MB_SPIEL=jka|jk2|md) ------------------------------------------------
// Jede .glm des Spiels: jede LOD-Stufe ueber das Arbeitsnetz (netzAus -> zurueckschreiben, erster Besuch: Datei-Naehte werden
// Marken) - danach schreibt der GLM-Schreiber dieselben Bytes wie ohne den Umweg; und jede Ecke behaelt ihre UV.
std::vector<Schritt> rundlaufSchritte() {
    std::vector<Schritt> s;
    const char* sp = std::getenv("BHED_MB_SPIEL");
    const std::string spiel = sp != nullptr ? sp : "md";
    s.push_back(tu("UV Rundlauf: Spiel " + spiel, [spiel] {
        g_app->leftMode = 2;
        if (g_app->gamePaths.empty()) { rescanGamePaths(); }
        if (spiel != "md") { mb1::spielSetzen(spiel); }
    }));
    s.push_back(pause(1.0));
    struct Zustand {
        std::vector<std::string> liste;
        std::size_t i = 0;
        int glm = 0;
        int ohneUmweg = 0;   // der Schreiber allein gibt dieselben Bytes (Stufe 2)
        int gleich = 0;      // mit dem Umweg ueber das Arbeitsnetz ebenso
        int anders = 0;
        std::string bericht;
    };
    auto z = std::make_shared<Zustand>();
    s.push_back({"UV Rundlauf: jede .glm (" + spiel + ") ueber das Arbeitsnetz zurueck, Byte fuer Byte", [=](int b) {
                     if (b == 0) {
                         for (const GamePath& gp : g_app->gamePaths) {
                             for (const FoundFile& f : findByExtension(gp, {".glm"})) { z->liste.push_back(f.name); }
                         }
                         std::sort(z->liste.begin(), z->liste.end());
                         z->liste.erase(std::unique(z->liste.begin(), z->liste.end()), z->liste.end());
                         return false;
                     }
                     for (int n = 0; n < 4 && z->i < z->liste.size(); ++n, ++z->i) {
                         std::string bytes;
                         modell::Inhalt in;
                         std::string ohne;
                         if (!readFromArchives(z->liste[z->i], bytes) || !modell::ausGlm(bytes, in) || !modell::glmSchreiben(in, ohne)) { continue; }
                         ++z->glm;
                         if (ohne != bytes) { continue; }   // Stufe 2 nennt diese Dateien (Rest hinter dem Namen u. a.)
                         ++z->ohneUmweg;
                         bool ok = true;
                         for (int st = 0; st < in.stufenZahl && ok; ++st) {
                             std::vector<int> teile;
                             for (std::size_t t = 0; t < in.teile.size(); ++t) { teile.push_back(static_cast<int>(t)); }
                             const w::UvNetz netz = w::netzAus(in, teile, st);
                             w::ZurueckBericht br;
                             ok = w::zurueckschreiben(netz, in, 0, &br) && br.geaendert == 0 && br.neueEcken == 0;
                         }
                         std::string mit;
                         if (ok && modell::glmSchreiben(in, mit) && mit == bytes) {
                             ++z->gleich;
                         } else {
                             ++z->anders;
                             if (z->anders <= 8) { z->bericht += " " + z->liste[z->i]; }
                         }
                     }
                     if (z->i < z->liste.size()) { return false; }
                     diag::detail("Knopftest: UV Rundlauf " + spiel + " - " + std::to_string(z->glm) + " .glm gelesen, " + std::to_string(z->ohneUmweg) +
                                  " ohne Umweg gleich, davon ueber das Arbeitsnetz gleich " + std::to_string(z->gleich) + ", anders " + std::to_string(z->anders) + z->bericht);
                     return true;
                 }});
    // REST Punkt 5: das Ergebnis als Pruefung (bis hier nur FEHLER bei Abweichung - die Zusammenfassung zaehlte "0 OK, 0 FEHLER",
    // auch wenn keine einzige Datei verglichen wurde)
    s.push_back(pruefe("UV Rundlauf " + spiel + ": jede .glm des Spiels ueber das Arbeitsnetz Byte fuer Byte gleich (mindestens eine verglichen)", [z] {
        return z->gleich > 0 && z->anders == 0;
    }));
    return s;
}

std::vector<Schritt> spielSchritte();   // unten

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "uvrundlauf") { return rundlaufSchritte(); }
    if (n == "uv") { return modellSchritte(); }
    if (n == "uvmd3") { return md3Schritte(); }
    if (n == "uvzeit") { return zeitSchritte(); }
    if (n == "uvspiel") { return spielSchritte(); }
    return {};
}

// --- Im Spiel (BHED_MODELL_NUR=uvspiel) --------------------------------------------------------------------------
//
// Der Sturmtruppler wird mit "Requisit" neu abgewickelt (jede Textur), jede Textur ueber die alte UV in die neue umgebacken
// (je Texel der neuen Karte die Stelle der alten, bilinear, 4 Texel ausgeblutet), und als Mod in BHED_UV_MOD geschrieben:
// models/players/stormtrooper/model.glm und je Textur ein .tga unter dem Namen der Haut (model_default.skin nennt .tga; die
// Engine laedt die .tga des fs_homepath vor dem .jpg der Archive). Im Spiel muss er aussehen wie vorher.
namespace {

struct Bildchen {
    int b = 0;
    int h = 0;
    std::vector<std::uint8_t> rgba;
};

void bilinear(const Bildchen& q, float u, float v, std::uint8_t out[4]) {
    // wie GL_REPEAT + GL_LINEAR: Texelmitten bei (i + 0,5) / b
    const float x = u * static_cast<float>(q.b) - 0.5F;
    const float y = v * static_cast<float>(q.h) - 0.5F;
    const int x0 = static_cast<int>(std::floor(x));
    const int y0 = static_cast<int>(std::floor(y));
    const float fx = x - static_cast<float>(x0);
    const float fy = y - static_cast<float>(y0);
    const auto px = [&](int xx, int yy, int k) {
        xx = ((xx % q.b) + q.b) % q.b;
        yy = ((yy % q.h) + q.h) % q.h;
        return static_cast<float>(q.rgba[static_cast<std::size_t>((yy * q.b + xx) * 4 + k)]);
    };
    for (int k = 0; k < 4; ++k) {
        const float a = px(x0, y0, k) * (1.0F - fx) + px(x0 + 1, y0, k) * fx;
        const float c = px(x0, y0 + 1, k) * (1.0F - fx) + px(x0 + 1, y0 + 1, k) * fx;
        out[k] = static_cast<std::uint8_t>(std::clamp(a * (1.0F - fy) + c * fy + 0.5F, 0.0F, 255.0F));
    }
}

// Die Textur einer Gruppe aus der alten UV (vorher) in die neue (nachher) umbacken; dieselben Halbkanten
Bildchen umbacken(const Bildchen& alt, const w::UvNetz& vorher, const w::UvNetz& nachher) {
    Bildchen neu;
    neu.b = alt.b;
    neu.h = alt.h;
    neu.rgba.assign(alt.rgba.size(), 0);
    std::vector<std::uint8_t> belegt(static_cast<std::size_t>(neu.b * neu.h), 0);
    for (int d = 0; d < nachher.dreiecke(); ++d) {
        const w::V2 p[3] = {nachher.uv[static_cast<std::size_t>(d * 3)], nachher.uv[static_cast<std::size_t>(d * 3 + 1)], nachher.uv[static_cast<std::size_t>(d * 3 + 2)]};
        const w::V2 q[3] = {vorher.uv[static_cast<std::size_t>(d * 3)], vorher.uv[static_cast<std::size_t>(d * 3 + 1)], vorher.uv[static_cast<std::size_t>(d * 3 + 2)]};
        const float x0 = std::min({p[0].x, p[1].x, p[2].x}) * static_cast<float>(neu.b);
        const float x1 = std::max({p[0].x, p[1].x, p[2].x}) * static_cast<float>(neu.b);
        const float y0 = std::min({p[0].y, p[1].y, p[2].y}) * static_cast<float>(neu.h);
        const float y1 = std::max({p[0].y, p[1].y, p[2].y}) * static_cast<float>(neu.h);
        const float f = (p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[1].y - p[0].y) * (p[2].x - p[0].x);
        if (f == 0.0F) { continue; }
        for (int y = std::max(0, static_cast<int>(std::floor(y0))); y <= std::min(neu.h - 1, static_cast<int>(std::ceil(y1))); ++y) {
            for (int x = std::max(0, static_cast<int>(std::floor(x0))); x <= std::min(neu.b - 1, static_cast<int>(std::ceil(x1))); ++x) {
                const float u = (static_cast<float>(x) + 0.5F) / static_cast<float>(neu.b);
                const float v = (static_cast<float>(y) + 0.5F) / static_cast<float>(neu.h);
                const float l1 = ((p[1].x - u) * (p[2].y - v) - (p[1].y - v) * (p[2].x - u)) / f;
                const float l2 = ((p[2].x - u) * (p[0].y - v) - (p[2].y - v) * (p[0].x - u)) / f;
                const float l3 = 1.0F - l1 - l2;
                if (l1 < -1e-4F || l2 < -1e-4F || l3 < -1e-4F) { continue; }
                std::uint8_t c[4];
                bilinear(alt, q[0].x * l1 + q[1].x * l2 + q[2].x * l3, q[0].y * l1 + q[1].y * l2 + q[2].y * l3, c);
                std::memcpy(&neu.rgba[static_cast<std::size_t>((y * neu.b + x) * 4)], c, 4);
                belegt[static_cast<std::size_t>(y * neu.b + x)] = 1;
            }
        }
    }
    // 4 Texel ausbluten (der Abstand des Packens): die Mipmaps ziehen sonst den schwarzen Grund in die Raender
    for (int r = 0; r < 4; ++r) {
        std::vector<std::uint8_t> naechst = belegt;
        for (int y = 0; y < neu.h; ++y) {
            for (int x = 0; x < neu.b; ++x) {
                if (belegt[static_cast<std::size_t>(y * neu.b + x)] != 0) { continue; }
                for (const auto& [dx, dy] : {std::pair{1, 0}, std::pair{-1, 0}, std::pair{0, 1}, std::pair{0, -1}}) {
                    const int xx = x + dx;
                    const int yy = y + dy;
                    if (xx < 0 || yy < 0 || xx >= neu.b || yy >= neu.h || belegt[static_cast<std::size_t>(yy * neu.b + xx)] == 0) { continue; }
                    std::memcpy(&neu.rgba[static_cast<std::size_t>((y * neu.b + x) * 4)], &neu.rgba[static_cast<std::size_t>((yy * neu.b + xx) * 4)], 4);
                    naechst[static_cast<std::size_t>(y * neu.b + x)] = 1;
                    break;
                }
            }
        }
        belegt = std::move(naechst);
    }
    return neu;
}

std::string modOrdner() {
    const char* d = std::getenv("BHED_UV_MOD");
    return d != nullptr ? d : mb3::lauf("uv_mod");
}

}  // namespace

std::vector<Schritt> spielSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // BHED_MB_SPIEL=jka: der Sturmtruppler des Original-JKA (fuer den Vergleich in jasp.exe); sonst Movie Duels
    if (const char* sp = std::getenv("BHED_MB_SPIEL"); sp != nullptr && std::string(sp) != "md") {
        const std::string spiel = sp;
        add(tu("UV Spiel: Daten " + spiel, [spiel] {
            if (g_app->gamePaths.empty()) { rescanGamePaths(); }
            mb1::spielSetzen(spiel);
        }));
        add(pause(1.0));
    }
    figurLaden(s, "max");
    add(tu("UV Spiel: Ansicht von vorn, Foto der Vorschau vorher", [] {
        ModellAnsicht& a = g_app->mtab.ansichten[g_app->mtab.aktiv];
        a.art = AnsichtArt::Vorne;
        modellAnsichtEinpassenAktiv();
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(mission1::probeFoto("uv_vorschau_vorher"));
    add(tu("UV Spiel: jede Textur mit Requisit neu abwickeln, umbacken, als Mod schreiben", [] {
        ModellTab& t = g_app->mtab;
        t.bereich = ModellBereich::Uv;
        t.eckenWahl.clear();
        t.auswahl.clear();
        namespace fs = std::filesystem;
        const std::string mod = modOrdner();
        std::error_code ec;
        fs::create_directories(fs::path(mod) / "models/players/stormtrooper", ec);
        int n = 0;
        std::string bericht;
        for (int g = 0; g < modellUvGruppenZahl(); ++g) {
            t.uv.gruppe = g;
            t.uv.fuer.clear();
            modellUvNetzAktualisieren();
            if (t.uv.netz.teile.empty()) { continue; }
            const int erste = t.uv.netz.teile.front();
            // der Shader nach der Haut (model_default.skin nennt .tga) und ein Bild dazu; "*off" zeichnet das Spiel nicht
            const std::string sh = g_app->model.surfaces[static_cast<std::size_t>(erste)].texture;
            if (sh.empty() || sh == "*off" || static_cast<std::size_t>(erste) >= g_app->modelTextures.bySurface.size() ||
                g_app->modelTextures.bySurface[static_cast<std::size_t>(erste)].rgba.empty()) {
                continue;
            }
            const w::UvNetz vorher = t.uv.netz;
            // Stufe 0 (die LOD-Stufen teilen die Textur; im Spiel mit r_lodbias -5 nur Stufe 0, siehe uv1b/spiel.py)
            modellUvAktionAusloesen(aktion(Str::UvSchnellRequisit));
            t.uv.fuer.clear();
            modellUvNetzAktualisieren();
            const TextureSet::Tex& tex = g_app->modelTextures.bySurface[static_cast<std::size_t>(erste)];
            Bildchen alt{tex.width, tex.height, tex.rgba};
            const Bildchen neu = umbacken(alt, vorher, t.uv.netz);
            // Gegenprobe des Umbackens: in der Mitte jedes Dreiecks die alte Textur an der alten UV gegen die neue an der neuen
            double abw = 0.0;
            int proben = 0;
            for (int d = 0; d < t.uv.netz.dreiecke(); ++d) {
                const auto mitte = [d](const w::UvNetz& n) {
                    w::V2 m{};
                    for (int k = 0; k < 3; ++k) { m = m + n.uv[static_cast<std::size_t>(d * 3 + k)] * (1.0F / 3.0F); }
                    return m;
                };
                std::uint8_t ca[4];
                std::uint8_t cb[4];
                const w::V2 ma = mitte(vorher);
                const w::V2 mb = mitte(t.uv.netz);
                bilinear(alt, ma.x, ma.y, ca);
                bilinear(neu, mb.x, mb.y, cb);
                for (int k = 0; k < 3; ++k) { abw += std::abs(static_cast<int>(ca[k]) - static_cast<int>(cb[k])); }
                ++proben;
            }
            bericht += " [Mitten " + std::to_string(proben) + ", mittlere Abweichung " + std::to_string(proben > 0 ? abw / (3.0 * proben) : 0.0) + "]";
            std::string ziel = sh;
            if (ziel.size() > 4 && ziel[ziel.size() - 4] == '.') { ziel = ziel.substr(0, ziel.size() - 4); }
            ziel += ".tga";
            const std::vector<unsigned char> tga = image::encodeTga(neu.rgba.data(), neu.b, neu.h);
            const fs::path pf = fs::path(mod) / ziel;
            fs::create_directories(pf.parent_path(), ec);
            std::ofstream f(pf, std::ios::binary);
            f.write(reinterpret_cast<const char*>(tga.data()), static_cast<std::streamsize>(tga.size()));
            bericht += " " + ziel + " (" + std::to_string(t.uv.inseln.zahl) + " Inseln)";
            // die Glanzkarte (zweite Stufe des Shaders, <name>_spec) liest dieselbe UV: ebenso umbacken
            const std::string stamm = ziel.substr(0, ziel.size() - 4);
            for (const char* endung : {".jpg", ".png", ".tga"}) {
                std::string roh;
                if (!readFromArchives(stamm + "_spec" + endung, roh)) { continue; }
                const image::Image bild = image::decode(reinterpret_cast<const unsigned char*>(roh.data()), roh.size());
                if (!bild.ok) { break; }
                const Bildchen glanz = umbacken(Bildchen{bild.width, bild.height, bild.rgba}, vorher, t.uv.netz);
                const std::vector<unsigned char> gt = image::encodeTga(glanz.rgba.data(), glanz.b, glanz.h);
                std::ofstream gf(fs::path(mod) / (stamm + "_spec.tga"), std::ios::binary);
                gf.write(reinterpret_cast<const char*>(gt.data()), static_cast<std::streamsize>(gt.size()));
                bericht += " + " + stamm + "_spec" + endung;
                break;
            }
            ++n;
        }
        t.uv.gruppe = -1;
        t.speicherPfad = (fs::path(mod) / "models/players/stormtrooper/model.glm").string();
        modellSpeichern(false);
        diag::detail("Knopftest: UV Spiel - " + std::to_string(n) + " Texturen umgebacken nach " + mod + ":" + bericht + " | " + t.meldung);
    }));
    add(pause(0.5));
    add(tu("UV Spiel: Vorschau nachher (neue UV, die Textur der Datei - wie das Spiel mit nur der neuen model.glm), von vorn", [] {
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(mission1::probeFoto("uv_vorschau_nachher"));
    add(pruefe("UV Spiel: Mod geschrieben (model.glm gelesen, jede Textur da)", [] {
        modell::Inhalt y;
        return modell::ausGlm(mb3::datei((std::filesystem::path(modOrdner()) / "models/players/stormtrooper/model.glm").string()), y) && !y.teile.empty();
    }));
    return s;
}

}  // namespace uv1b
