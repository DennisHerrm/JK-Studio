// selbsttest_fix612b.inc - FIX612B (12.11.): die Funde der Code-Pruefung P2 (Gewichte, Malen, Auto-Skin) im laufenden
// Programm nachgestellt (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=fix612b | fix612babsturz)
//
//   fix612b          je Fund ein Abschnitt, jede Zeile "FIX612B P2-<nr> ..." war auf karte1 73c18113f rot:
//     P2-2   Speichern mit offenen Mallisten schreibt, was die Ansicht zeigt (Bytes der Datei gegen die Anzeige, Format des
//            Spiels an den Bytes: hoechstens 4 Gewichte, Rest > 0, hoechstens 32 Knochenverweise); die Pruefung vor dem
//            Speichern misst, was geschrieben wird
//     P2-3   X-Spiegeln beim Malen, das Rad wechselt die automatische Stufe mitten im Strich
//     P2-9   die Auswahl von Ecken und Vielecken gehoert zu ihrer Stufe; ein offenes Kaestchen haelt die Stufe
//     P2-4   ein Modellier-Werkzeug laesst die offenen Mallisten der beruehrten Flaeche stehen
//     P2-1   Auto-Skin "Nur Auswahl" mit "Spiegeln" aendert nur die gewaehlten Flaechen
//     P2-10  Strg+Z mitten im Strich, im Verlauf, im Gizmo-Zug und im Painting
//     P2-5   ein Befehl ohne Aenderung laesst das Wiederholen stehen
//     P2-6   ein Strich, der neben der Figur beginnt, malt mit der gewaehlten Art
//     P2-7   Sperren gelten dem geladenen Modell
//     P2-8   Painting mit Pinselradius 0
//     V1     Auto-Skin auf einem kleinen Teil mit vollem Skelett (Zeit gegen die ganze Figur)
//   fix612babsturz   P2-3, die Richtung mit dem Zugriff hinter das Ende: spiegelMalen mit dem Schweissnetz einer feineren
//                    Stufe (auf einer Kopie des Dokuments). Vor der Behebung stuerzt der Lauf ab oder verdirbt den Speicher.
//   BHED_FIX612B=<p22,p23,p29,p24,p21,p210,p25,p26,p27,p28,v1>   nur diese Abschnitte (Vorgabe: alle)
//   BHED_FIX612B_MODELL=<.glm>   Vorgabe models/players/kyle/model.glm
// Eingebunden in selbsttest.cpp (am Ende der Liste).
namespace fix612b {

namespace w = modell::wof;

ModellTab& mtab() { return g_app->mtab; }
ModellGewichte& gw() { return modellGewichte(); }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }

void melde(const std::string& z) { diag::detail("Knopftest: FIX612B " + z); }

std::string modellPfad() {
    const char* p = std::getenv("BHED_FIX612B_MODELL");
    return p != nullptr && *p != '\0' ? p : "models/players/kyle/model.glm";
}

// Ein Modell mit mehreren Detailstufen fuer P2-3 und P2-9 (kyle hat unter Movie Duels nur eine; stormtrooper aus MD_Models_OT-ST
// hat vier und die Flaechen head, torso, l_arm, r_arm)
std::string stufenPfad() {
    const char* p = std::getenv("BHED_FIX612B_STUFEN");
    return p != nullptr && *p != '\0' ? p : "models/players/stormtrooper/model.glm";
}

bool abschnitt(const char* name) {
    const char* e = std::getenv("BHED_FIX612B");
    if (e == nullptr || *e == '\0') { return true; }
    const std::string liste = std::string(",") + e + ",";
    return liste.find(std::string(",") + name + ",") != std::string::npos;
}

int knochen(const char* name) { return boneIndex(g_app->anim, name); }
int teil(const char* name) { return mtab().dok.inhalt().teilIndex(name); }

const modell::Netz* netz(const modell::Inhalt& in, int t, int stufe) {
    if (t < 0 || uz(t) >= in.teile.size() || stufe < 0 || uz(stufe) >= in.teile[uz(t)].stufen.size()) { return nullptr; }
    return &in.teile[uz(t)].stufen[uz(stufe)];
}

// Was in den Ecken und in den Mallisten steht, als Bytes (ein Teil, eine Stufe; -1: alle).
std::string gewichtStand(const modell::Inhalt& in, int nurTeil = -1, int nurStufe = -1) {
    std::string s;
    for (std::size_t t = 0; t < in.teile.size(); ++t) {
        if (nurTeil >= 0 && static_cast<int>(t) != nurTeil) { continue; }
        for (std::size_t st = 0; st < in.teile[t].stufen.size(); ++st) {
            if (nurStufe >= 0 && static_cast<int>(st) != nurStufe) { continue; }
            const modell::Netz& n = in.teile[t].stufen[st];
            for (const modell::Ecke& e : n.ecken) {
                s.append(reinterpret_cast<const char*>(e.knochen), sizeof e.knochen);
                s.append(reinterpret_cast<const char*>(e.gewicht), sizeof e.gewicht);
            }
            s += "|" + std::to_string(n.mal.size()) + "|";
            for (const modell::GewichtListe& l : n.mal) {
                for (const modell::KnochenGewicht& g : l) {
                    s.append(reinterpret_cast<const char*>(&g.knochen), sizeof g.knochen);
                    s.append(reinterpret_cast<const char*>(&g.gewicht), sizeof g.gewicht);
                }
                s += ";";
            }
        }
    }
    return s;
}

// Knochen und Gewichte einer Ecke, wie die Ansicht sie zeichnet.
std::string gezeigt(const modell::Netz& n, std::size_t e) {
    const GlmVertex v = modell::anzeigeVertex(n, e);
    std::string s;
    // Plaetze ohne Gewicht tragen keinen Knochen (in der Datei steht dort, was beim Packen uebrig war)
    for (int k = 0; k < 4; ++k) {
        const std::uint8_t b = v.weights[k] > 0.0F ? v.bones[k] : static_cast<std::uint8_t>(0);
        s.append(reinterpret_cast<const char*>(&b), 1);
        s.append(reinterpret_cast<const char*>(&v.weights[k]), sizeof v.weights[k]);
    }
    return s;
}

int malend(const modell::Inhalt& in) {
    int z = 0;
    for (const modell::Teil& t : in.teile) {
        for (const modell::Netz& n : t.stufen) { z += modell::malt(n) ? 1 : 0; }
    }
    return z;
}

// --- Die Bytes einer .glm, wie das Spiel sie liest (mdx_format.h: mdxmHeader_t, mdxmLOD_t, mdxmSurface_t, mdxmVertex_t) ----
struct DateiMass {
    bool lesbar = false;
    int flaechen = 0;
    int ecken = 0;
    int gewichteMax = 0;       // die groesste Anzahl (Bit 30-31 + 1)
    int restNichtPositiv = 0;  // Ecken mit mehr als einem Gewicht, deren letztes (1 - Summe der vorigen) nicht > 0 ist
    int verweisFalsch = 0;     // ein Gewicht zeigt hinter die Knochenverweise der Flaeche
    int refsUeber32 = 0;       // Flaechen mit mehr als 32 Knochenverweisen (5 Bit je Verweis)
    int nullGewicht = 0;       // ein Gewicht vor dem letzten mit 10-Bit-Wert 0
};
std::int32_t i32(const std::string& b, std::size_t at) {
    if (at + 4U > b.size()) { return 0; }
    std::int32_t v = 0;
    std::memcpy(&v, b.data() + at, 4);
    return v;
}
DateiMass dateiMass(const std::string& b) {
    DateiMass m;
    if (b.size() < 164U || b.compare(0, 4, "2LGM") != 0) { return m; }
    const std::int32_t stufen = i32(b, 144);
    const std::int32_t flaechen = i32(b, 152);
    std::size_t lod = static_cast<std::size_t>(i32(b, 148));
    for (std::int32_t s = 0; s < stufen; ++s) {
        if (lod + 4U + static_cast<std::size_t>(flaechen) * 4U > b.size()) { return m; }
        for (std::int32_t f = 0; f < flaechen; ++f) {
            const std::size_t base = lod + 4U + static_cast<std::size_t>(i32(b, lod + 4U + static_cast<std::size_t>(f) * 4U));
            if (base + 40U > b.size()) { return m; }
            const std::int32_t nv = i32(b, base + 12U);
            const std::size_t v0 = base + static_cast<std::size_t>(i32(b, base + 16U));
            const std::int32_t refs = i32(b, base + 28U);
            if (nv < 0 || v0 + static_cast<std::size_t>(nv) * 32U > b.size()) { return m; }
            ++m.flaechen;
            m.refsUeber32 += refs > 32 ? 1 : 0;
            for (std::int32_t v = 0; v < nv; ++v) {
                const std::size_t at = v0 + static_cast<std::size_t>(v) * 32U;
                const auto pack = static_cast<std::uint32_t>(i32(b, at + 24U));
                const int n = static_cast<int>((pack >> 30U) & 0x3U) + 1;
                ++m.ecken;
                m.gewichteMax = std::max(m.gewichteMax, n);
                int summe = 0;
                for (int k = 0; k < n; ++k) {
                    const auto lokal = static_cast<std::int32_t>((pack >> (5U * static_cast<unsigned>(k))) & 0x1FU);
                    m.verweisFalsch += lokal >= refs ? 1 : 0;
                    if (k + 1 < n) {
                        const std::uint32_t oben = (pack >> (20U + 2U * static_cast<unsigned>(k))) & 0x3U;
                        const int bits = static_cast<int>(static_cast<unsigned char>(b[at + 28U + static_cast<std::size_t>(k)]) | (oben << 8U));
                        m.nullGewicht += bits == 0 ? 1 : 0;
                        summe += bits;
                    }
                }
                m.restNichtPositiv += n > 1 && summe >= 1023 ? 1 : 0;
            }
        }
        const std::int32_t ende = i32(b, lod);
        if (ende <= 0) { return m; }
        lod += static_cast<std::size_t>(ende);
    }
    m.lesbar = m.ecken > 0;
    return m;
}
std::string massText(const DateiMass& m) {
    return std::to_string(m.flaechen) + " Flaechen, " + std::to_string(m.ecken) + " Ecken, hoechstens " + std::to_string(m.gewichteMax) + " Gewichte je Ecke, Rest nicht > 0: " +
           std::to_string(m.restNichtPositiv) + ", Verweis hinter der Liste: " + std::to_string(m.verweisFalsch) + ", Flaechen ueber 32 Verweise: " + std::to_string(m.refsUeber32) +
           ", Gewicht 0 vor dem letzten: " + std::to_string(m.nullGewicht);
}

std::string dateiLesen(const std::string& pfad) {
    std::ifstream f(::bhed::utf8Pfad(pfad), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

// Ecken, deren Gewichte in der geschriebenen Datei anders sind, als die Ansicht sie zeichnet (alle Stufen).
int dateiGegenAnzeige(const std::string& bytes, const modell::Inhalt& dok, int* verglichen = nullptr) {
    modell::Inhalt datei;
    if (!modell::ausGlm(bytes, datei) || datei.teile.size() != dok.teile.size()) { return -1; }
    int anders = 0;
    int n = 0;
    for (std::size_t t = 0; t < dok.teile.size(); ++t) {
        if (datei.teile[t].stufen.size() != dok.teile[t].stufen.size()) { return -1; }
        for (std::size_t st = 0; st < dok.teile[t].stufen.size(); ++st) {
            const modell::Netz& a = datei.teile[t].stufen[st];
            const modell::Netz& b = dok.teile[t].stufen[st];
            if (a.ecken.size() != b.ecken.size()) { return -1; }
            for (std::size_t e = 0; e < b.ecken.size(); ++e) {
                anders += gezeigt(a, e) != gezeigt(b, e) ? 1 : 0;
                ++n;
            }
        }
    }
    if (verglichen != nullptr) { *verglichen = n; }
    return anders;
}

// --- Grundstand: Modell frisch geladen, Bereich Weights, wie 3ds Max, Stufe 0 fest ------------------------------------------
void grund(std::vector<Schritt>& s, const std::string& was, const std::string& pfad = std::string()) {
    for (Schritt& x : mb1::laden(pfad.empty() ? modellPfad() : pfad)) { s.push_back(std::move(x)); }
    s.push_back(tu("FIX612B " + was + ": Bereich Weights, wie 3ds Max, Stufe 0", [] {
        ModellTab& t = mtab();
        modellBedienungSetzen(0);
        t.bereich = ModellBereich::Ansehen;
        modellGewichteBereichSchritt();
        ModellGewichte neu;
        neu.gesperrt.assign(g_app->anim.bones.size(), 0);
        gw() = neu;
        modellAutoSkinZustand() = ModellAutoSkin{};
        t.bereich = ModellBereich::Weights;
        t.lodWahl = 1;
        t.auswahl.clear();
        t.eckenWahl.clear();
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        t.pinselArt = static_cast<int>(modell::PinselArt::Hinzufuegen);
        t.pinselGewicht = 1.0F;
        t.pinselStaerke = 1.0F;
        t.pinselRadius = 40.0F;
        t.pinselSpiegeln = false;
        t.gizmo = 0;
        t.vierAnsichten = false;
        t.fokus = true;
        g_app->animPlaying = false;
        g_app->modelDirty = true;
    }));
    s.push_back(pause(0.6));
}

// Die Ecke einer Flaeche, die zur Kamera zeigt und der Mitte der Flaeche im Bild am naechsten liegt.
int vordereEcke(int t, ImVec2& ort) {
    ImVec2 mitte;
    if (!modellFlaecheAufSchirm(t, mitte)) { return -1; }
    ModellEckenBild bild;
    modellEckenBild(mtab().ansichten[mtab().aktiv], bild);
    if (t < 0 || uz(t) >= bild.ort.size()) { return -1; }
    int best = -1;
    float bd = 1e30F;
    for (std::size_t k = 0; k < bild.ort[uz(t)].size(); ++k) {
        if (bild.imBild[uz(t)][k] == 0 || bild.vorn[uz(t)][k] == 0) { continue; }
        const float d = std::hypot(bild.ort[uz(t)][k][0] - mitte.x, bild.ort[uz(t)][k][1] - mitte.y);
        if (d < bd) {
            bd = d;
            best = static_cast<int>(k);
            ort = ImVec2{std::floor(bild.ort[uz(t)][k][0]), std::floor(bild.ort[uz(t)][k][1])};
        }
    }
    return best;
}

// Ein Zug mit der linken Taste: ab `start()` je Bild `dx` nach rechts, `bilder` Bilder lang; im Bild `tasteBei` nach dem
// Druecken Strg+Z (-1: keine Taste). `jeBild(i)` laeuft in jedem Bild des Zugs.
Schritt zug(const std::string& was, std::function<ImVec2()> start, float dx, int bilder, int tasteBei, std::function<void(int)> jeBild = nullptr) {
    auto z = std::make_shared<ImVec2>();
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *z = start();
                    setzeMaus(*z);
                    return false;
                }
                if (b == 1) { return false; }
                if (b == 2) {
                    io.AddMouseButtonEvent(0, true);
                    return false;
                }
                const int i = b - 3;
                if (i < bilder) {
                    setzeMaus(ImVec2{z->x + dx * static_cast<float>(i + 1), z->y});
                    if (tasteBei >= 0) {
                        if (i == tasteBei) { io.AddKeyEvent(ImGuiMod_Ctrl, true); }
                        if (i == tasteBei + 1) { io.AddKeyEvent(ImGuiKey_Z, true); }
                        if (i == tasteBei + 2) { io.AddKeyEvent(ImGuiKey_Z, false); }
                        if (i == tasteBei + 3) { io.AddKeyEvent(ImGuiMod_Ctrl, false); }
                    }
                    if (jeBild) { jeBild(i); }
                    return false;
                }
                if (i == bilder) {
                    io.AddMouseButtonEvent(0, false);
                    return false;
                }
                return i >= bilder + 5;
            }};
}

// =====================================================================================================================
// P2-2 [H]: Speichern mit offenen Mallisten
struct P22 {
    modell::EckOrt ecke{-1, -1};
    ImVec2 ort{};
    float vorher = 0.0F;
    float gemalt = 0.0F;
    std::string original;
    std::string geschrieben;
    std::string pfad;
};
void p22(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto p = std::make_shared<P22>();
    grund(s, "P2-2");
    add(tu("FIX612B P2-2: Knochen rhumerus, eine Ecke von torso suchen", [p] {
        ModellTab& t = mtab();
        t.knochenWahl = knochen("rhumerus");
        gw().malen = true;
        const int torso = teil("torso");
        p->ecke = modell::EckOrt{torso, vordereEcke(torso, p->ort)};
        const modell::Netz* n = netz(t.dok.inhalt(), torso, 0);
        p->vorher = n != nullptr ? modell::gewichtVon(*n, p->ecke.ecke, t.knochenWahl) : -1.0F;
        (void)readFromArchives(g_app->modelPath, p->original);
    }));
    add(klickAn("FIX612B P2-2: ein Tupfer auf torso", [p] { return p->ort; }));
    add(pause(0.3));
    add(pruefe("FIX612B P2-2 Vorbereitung: der Tupfer hat gemalt, die Mallisten sind offen, die Ansicht zeigt das neue Gewicht", [p] {
        const ModellTab& t = mtab();
        const modell::Netz* n = netz(t.dok.inhalt(), p->ecke.teil, 0);
        if (n == nullptr || p->ecke.ecke < 0) { return false; }
        p->gemalt = modell::gewichtVon(*n, p->ecke.ecke, t.knochenWahl);
        const GlmVertex v = modell::anzeigeVertex(*n, uz(p->ecke.ecke));
        float anzeige = 0.0F;
        for (int k = 0; k < 4; ++k) { anzeige += v.bones[k] == t.knochenWahl ? v.weights[k] : 0.0F; }
        char z[240];
        std::snprintf(z, sizeof z, "P2-2 - torso Ecke %d, rhumerus vorher %.4f, Malliste %.4f, Anzeige %.4f, Flaechen mit Malliste %d", p->ecke.ecke, static_cast<double>(p->vorher),
                      static_cast<double>(p->gemalt), static_cast<double>(anzeige), malend(t.dok.inhalt()));
        melde(z);
        return modell::malt(*n) && p->gemalt > p->vorher + 0.05F && anzeige > p->vorher + 0.05F && modellUngespeichert();
    }));
    add(tu("FIX612B P2-2: Speichern unter (Laufordner), ohne Uebernehmen", [p] {
        p->pfad = mb3::lauf("fix612b_p22.glm");
        mtab().speicherPfad = p->pfad;
        modellSpeichern(false);
    }));
    add(pause(0.3));
    add(pruefe("FIX612B P2-2: die geschriebene Datei traegt an jeder Ecke die Gewichte, die die Ansicht zeigt", [p] {
        p->geschrieben = dateiLesen(p->pfad);
        int verglichen = 0;
        const int anders = dateiGegenAnzeige(p->geschrieben, mtab().dok.inhalt(), &verglichen);
        melde("P2-2 - Datei " + std::to_string(p->geschrieben.size()) + " Bytes, " + std::to_string(verglichen) + " Ecken verglichen, anders als die Anzeige: " + std::to_string(anders) +
              "; gegen das Original: " + (p->geschrieben == p->original ? std::string("bytegleich") : modell::glmUnterschied(p->original, p->geschrieben)) + "; Meldung: " + mtab().meldung);
        return mtab().frageArt == 0 && verglichen > 1000 && anders == 0;
    }));
    add(pruefe("FIX612B P2-2: die Datei ist nicht mehr das Original, an der gemalten Ecke steht das gemalte Gewicht", [p] {
        modell::Inhalt datei;
        if (p->geschrieben.empty() || !modell::ausGlm(p->geschrieben, datei)) { return false; }
        const modell::Netz* a = netz(datei, p->ecke.teil, 0);
        const modell::Netz* b = netz(mtab().dok.inhalt(), p->ecke.teil, 0);
        if (a == nullptr || b == nullptr || uz(p->ecke.ecke) >= a->ecken.size()) { return false; }
        const float inDatei = modell::gewichtVon(*a, p->ecke.ecke, mtab().knochenWahl);
        char z[160];
        std::snprintf(z, sizeof z, "P2-2 - rhumerus an der gemalten Ecke: Datei %.6f, vor dem Malen %.6f", static_cast<double>(inDatei), static_cast<double>(p->vorher));
        melde(z);
        return p->geschrieben != p->original && inDatei > p->vorher + 0.05F && gezeigt(*a, uz(p->ecke.ecke)) == gezeigt(*b, uz(p->ecke.ecke));
    }));
    add(pruefe("FIX612B P2-2 Format des Spiels an den Bytes: hoechstens 4 Gewichte je Ecke, das letzte (Rest) > 0, hoechstens 32 Knochenverweise je Flaeche", [p] {
        const DateiMass neu = dateiMass(p->geschrieben);
        const DateiMass alt = dateiMass(p->original);
        melde("P2-2 Bytes geschrieben - " + massText(neu));
        melde("P2-2 Bytes Original    - " + massText(alt));
        return neu.lesbar && neu.ecken == alt.ecken && neu.gewichteMax <= 4 && neu.restNichtPositiv <= alt.restNichtPositiv && neu.verweisFalsch == 0 && neu.refsUeber32 == 0 &&
               neu.nullGewicht <= alt.nullGewicht;
    }));
    add(pruefe("FIX612B P2-2: nach dem Speichern gilt das Dokument als gespeichert - und die Datei enthaelt, was man sieht (die Mallisten bleiben offen)", [p] {
        return !modellUngespeichert() && malend(mtab().dok.inhalt()) > 0 && dateiGegenAnzeige(p->geschrieben, mtab().dok.inhalt()) == 0;
    }));
    // die Pruefung vor dem Speichern: mehr als 32 Knochen in einer Flaeche nur in den Mallisten
    add(tu("FIX612B P2-2: Mallisten von head mit 40 verschiedenen Knochen (in den Ecken steht das nicht)", [] {
        const int head = teil("head");
        if (head < 0) { return; }
        mtab().dok.aendern("FIX612B viele Knochen", [&](modell::Inhalt& x) {
            modell::Netz& n = x.teile[uz(head)].stufen[0];
            modell::malAnlegen(n);
            for (std::size_t e = 0; e < n.mal.size(); ++e) { n.mal[e] = {{static_cast<int>(e % 40U), 1.0F}}; }
        });
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(pruefe("FIX612B P2-2: die Pruefung vor dem Speichern misst, was geschrieben wuerde (Knochen je Flaeche aus den Mallisten)", [] {
        int knochenBefunde = 0;
        for (const modell::Befund& b : modellBefunde()) { knochenBefunde += b.art == modell::BefundArt::Knochen ? 1 : 0; }
        melde("P2-2 - Befunde Knochen je Flaeche: " + std::to_string(knochenBefunde));
        return knochenBefunde >= 1;
    }));
    add(tu("FIX612B P2-2: zurueck", [] {
        (void)modellRueckgaengig();
        mtab().speicherPfad.clear();
        mtab().frageArt = 0;
    }));
}

// =====================================================================================================================
// P2-3 [M]: X-Spiegeln beim Malen, die automatische Stufe wechselt mitten im Strich
struct P23 {
    int arm = -1;
    int k = -1;
    int sk = -1;
    int stufeNah = -1;
    int stufeStart = -1;
    float richtung = 0.0F;   // Rad, das herauszoomt
    int rasten = 0;
    bool strichLief = false;
    bool imStrichGewechselt = false;
    int stufeNachher = -1;
    int tupfer = 0;
    int endeBild = -1;
    bool monoton = true;
    modell::Inhalt vorher;
};
ImVec2 ansichtMitte() {
    const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
    return ImVec2{std::floor(a.lo[0] + a.groesse[0] * 0.5F), std::floor(a.lo[1] + a.groesse[1] * 0.5F)};
}
// Geaenderte Ecken einer Stufe gegen `vorher` (Ecken und Mallisten).
int geaenderteEcken(const modell::Inhalt& vorher, const modell::Inhalt& jetzt, int stufe) {
    int z = 0;
    for (std::size_t t = 0; t < jetzt.teile.size() && t < vorher.teile.size(); ++t) {
        const modell::Netz* a = netz(vorher, static_cast<int>(t), stufe);
        const modell::Netz* b = netz(jetzt, static_cast<int>(t), stufe);
        if (a == nullptr || b == nullptr || a->ecken.size() != b->ecken.size()) { continue; }
        for (std::size_t e = 0; e < a->ecken.size(); ++e) { z += gezeigt(*a, e) != gezeigt(*b, e) ? 1 : 0; }
    }
    return z;
}
// Spiegeln beim Malen (ohne Auto-Normalisieren): jeder geaenderte Punkt der Spiegelseite traegt am Spiegelknochen genau das
// Gewicht eines Punkts der Pinselseite, dessen Partner er ist. Rueckgabe: Punkte, fuer die das nicht gilt.
int spiegelVerstoesse(const modell::Inhalt& vorher, const modell::Inhalt& jetzt, int stufe, int k, int sk, float pinselSeite, int* geaendert) {
    const modell::Schweissnetz s = modell::schweissen(jetzt, stufe);
    const std::vector<int> partner = modell::spiegelPartner(jetzt, stufe, s, gw().toleranz);
    std::vector<std::vector<int>> quellen(uz(s.zahl()));
    for (std::size_t p = 0; p < partner.size(); ++p) {
        if (partner[p] >= 0) { quellen[uz(partner[p])].push_back(static_cast<int>(p)); }
    }
    int verstoesse = 0;
    int zahl = 0;
    for (int p = 0; p < s.zahl(); ++p) {
        const modell::EckOrt o = s.ecken[uz(p)].front();
        const modell::Netz* a = netz(vorher, o.teil, stufe);
        const modell::Netz* b = netz(jetzt, o.teil, stufe);
        if (a == nullptr || b == nullptr || uz(o.ecke) >= a->ecken.size() || uz(o.ecke) >= b->ecken.size()) { continue; }
        // geaendert am Spiegelknochen: das schreibt das Spiegeln (der Pinsel selbst malt den gewaehlten Knochen, auch wenn er in der
        // Pose ueber die Mitte reicht - stormtrooper haelt das Gewehr vor dem Bauch)
        bool anders = false;
        for (const modell::EckOrt& q : s.ecken[uz(p)]) {
            const modell::Netz* qa = netz(vorher, q.teil, stufe);
            const modell::Netz* qb = netz(jetzt, q.teil, stufe);
            if (qa != nullptr && qb != nullptr && uz(q.ecke) < qa->ecken.size() && uz(q.ecke) < qb->ecken.size()) {
                anders = anders || modell::gewichtVon(*qa, q.ecke, sk) != modell::gewichtVon(*qb, q.ecke, sk);
            }
        }
        if (!anders) { continue; }
        ++zahl;
        const float x = b->ecken[uz(o.ecke)].ort[0];
        if (x * pinselSeite > 0.0F) { continue; }   // die Pinselseite: gemalt
        bool gut = false;
        std::string q1;
        for (const int q : quellen[uz(p)]) {
            const modell::EckOrt qo = s.ecken[uz(q)].front();
            const modell::Netz* qn = netz(jetzt, qo.teil, stufe);
            gut = gut || (qn != nullptr && modell::gewichtVon(*qn, qo.ecke, k) == modell::gewichtVon(*b, o.ecke, sk));
            if (qn != nullptr && q1.size() < 120U) {
                char z[96];
                std::snprintf(z, sizeof z, " q%d x%.2f k%.4f", q, static_cast<double>(qn->ecken[uz(qo.ecke)].ort[0]), static_cast<double>(modell::gewichtVon(*qn, qo.ecke, k)));
                q1 += z;
            }
        }
        if (!gut && verstoesse < 6) {
            char z[200];
            std::snprintf(z, sizeof z, "P2-3 - Verstoss Stufe %d Punkt %d (%s) x %.2f: k %.4f -> %.4f, sk %.4f -> %.4f, Quellen %zu:", stufe, p, jetzt.teile[uz(o.teil)].name.c_str(),
                          static_cast<double>(x), static_cast<double>(modell::gewichtVon(*a, o.ecke, k)), static_cast<double>(modell::gewichtVon(*b, o.ecke, k)),
                          static_cast<double>(modell::gewichtVon(*a, o.ecke, sk)), static_cast<double>(modell::gewichtVon(*b, o.ecke, sk)), quellen[uz(p)].size());
            melde(z + q1);
        }
        verstoesse += gut ? 0 : 1;
    }
    if (geaendert != nullptr) { *geaendert = zahl; }
    return verstoesse;
}
void p23(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto p = std::make_shared<P23>();
    grund(s, "P2-3", stufenPfad());
    add(tu("FIX612B P2-3: Stufe automatisch, X-Spiegeln, Knochen lhumerus, ohne Auto-Normalisieren", [p] {
        ModellTab& t = mtab();
        t.lodWahl = 0;
        t.pinselSpiegeln = true;
        gw().malen = true;
        gw().autoNormalisieren = false;
        p->k = knochen("lhumerus");
        p->sk = knochen("rhumerus");
        t.knochenWahl = p->k;
        // ein kleiner Pinsel: herausgezoomt deckten 40 Bildpunkte die Mitte der Figur ab - dann malt der Pinsel selbst auf der
        // Spiegelseite, und die Pruefung "nur Gegenstuecke" gilt nicht mehr
        t.pinselRadius = 10.0F;
        p->arm = teil("l_arm");
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    // herauszoomen, bis die naechste Stufe gezeichnet wird
    add({"FIX612B P2-3: mit dem Rad herauszoomen, bis die Stufe wechselt", [p](int b) {
             ModellTab& t = mtab();
             ImGuiIO& io = ImGui::GetIO();
             if (b == 0) {
                 p->stufeNah = t.lodJetzt;
                 setzeMaus(ansichtMitte());
                 return false;
             }
             if (b < 3) { return false; }
             if (t.lodJetzt > p->stufeNah) {
                 char z[160];
                 std::snprintf(z, sizeof z, "P2-3 - herausgezoomt: Kamera %.1f %.1f %.1f", static_cast<double>(g_app->modelCam.pos[0]), static_cast<double>(g_app->modelCam.pos[1]),
                               static_cast<double>(g_app->modelCam.pos[2]));
                 melde(z);
                 p->stufeStart = t.lodJetzt;
                 p->richtung = b - 3 <= 60 ? -1.0F : 1.0F;
                 p->rasten = b - 3 <= 60 ? b - 3 : b - 3 - 120;
                 return true;
             }
             if (b - 3 >= 260) {
                 meldeFehler("FIX612B P2-3: die Stufe wechselt beim Zoomen nicht (Stufen " + std::to_string(t.dok.inhalt().stufenZahl) + ")");
                 return true;
             }
             io.AddMouseWheelEvent(0.0F, b - 3 < 60 ? -1.0F : 1.0F);
             return false;
         }});
    add(pause(0.3));
    add(tu("FIX612B P2-3: Stand vor dem Strich", [p] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        p->vorher = in;
        // der Strich fuehrt von der groben zur feinen Stufe: die Nummern der groben gibt es in der feinen (kein Zugriff hinter das Ende) -
        // verglichen werden die beiden Stufen des Strichs an den Armen (gemalt und gespiegelt); andere Flaechen nur im Protokoll
        // (stormtrooper: sand_pack1 hat in Stufe 1 mehr Ecken als in Stufe 0, liegt aber auf dem Ruecken, abseits des Pinsels)
        std::string andere;
        for (std::size_t i = 0; i < in.teile.size(); ++i) {
            const modell::Teil& t = in.teile[i];
            if (p->stufeStart <= 0 || p->stufeNah < 0 || uz(p->stufeStart) >= t.stufen.size()) { continue; }
            const bool steigt = t.stufen[uz(p->stufeStart)].ecken.size() > t.stufen[uz(p->stufeNah)].ecken.size();
            if (!steigt) { continue; }
            if (static_cast<int>(i) == p->arm || static_cast<int>(i) == teil("r_arm")) { p->monoton = false; }
            andere += " " + t.name;
        }
        if (!andere.empty()) { melde("P2-3 - Flaechen mit mehr Ecken in der groben Stufe:" + andere); }
        char z[200];
        std::snprintf(z, sizeof z, "P2-3 - Stufe nah %d, nach %d Rasten (Rad %+.0f) Stufe %d; Ecken je Stufe fallend: %d", p->stufeNah, p->rasten, static_cast<double>(p->richtung), p->stufeStart,
                      p->monoton ? 1 : 0);
        melde(z);
    }));
    add({"FIX612B P2-3: Strich auf l_arm, dabei mit dem Rad wieder heranzoomen", [p](int b) {
             ModellTab& t = mtab();
             ImGuiIO& io = ImGui::GetIO();
             if (p->stufeStart < 0 || !p->monoton || p->arm < 0) { return true; }
             ImVec2 m;
             if (!modellFlaecheAufSchirm(p->arm, m)) { m = ansichtMitte(); }
             m = ImVec2{std::floor(m.x), std::floor(m.y)};
             if (b == 0) {
                 setzeMaus(m);
                 p->tupfer = t.tupferZahl;
                 return false;
             }
             if (b == 1) { return false; }
             if (b == 2) {
                 io.AddMouseButtonEvent(0, true);
                 return false;
             }
             const int i = b - 3;
             const int zoomBilder = p->rasten + 15;   // mehr heran als vorher heraus: das Rad zoomt zur Maus, der Weg zurueck ist nicht derselbe
             if (t.malStrich) {
                 p->strichLief = true;
                 p->imStrichGewechselt = p->imStrichGewechselt || t.lodJetzt != p->stufeStart;
             }
             if (i < zoomBilder + 16) {
                 if (i >= 2 && i - 2 < zoomBilder) { io.AddMouseWheelEvent(0.0F, -p->richtung); }
                 setzeMaus(ImVec2{m.x + (i % 2 == 0 ? 6.0F : -6.0F), m.y});
                 return false;
             }
             if (i == zoomBilder + 16) {
                 io.AddMouseButtonEvent(0, false);
                 return false;
             }
             // die Ereignisse des Rads und der Maus kommen nacheinander an (ImGui verteilt sie auf Bilder): warten, bis der Strich
             // wirklich zu Ende ist, dann noch einige Bilder
             if (t.malStrich && i < zoomBilder + 300) { return false; }
             if (p->endeBild < 0) { p->endeBild = i; }
             if (i < p->endeBild + 8) { return false; }
             p->stufeNachher = t.lodJetzt;
             char z[240];
             std::snprintf(z, sizeof z, "P2-3 - nach dem Loslassen: Kamera %.1f %.1f %.1f, Strich offen %d, Stufe gehalten %d, Stufe %d", static_cast<double>(g_app->modelCam.pos[0]),
                           static_cast<double>(g_app->modelCam.pos[1]), static_cast<double>(g_app->modelCam.pos[2]), t.malStrich ? 1 : 0, modellStufeGehalten() ? 1 : 0, t.lodJetzt);
             melde(z);
             p->tupfer = t.tupferZahl - p->tupfer;
             return true;
         }});
    add(pruefe("FIX612B P2-3 Vorbereitung: der Strich lief (Tupfer vor und nach dem Zoomen), nach dem Loslassen zeichnet die Ansicht die nahe Stufe", [p] {
        melde("P2-3 - Tupfer " + std::to_string(p->tupfer) + ", Stufe nach dem Strich " + std::to_string(p->stufeNachher));
        return p->strichLief && p->tupfer >= 4 && p->stufeNachher == p->stufeNah && p->stufeStart > p->stufeNah;
    }));
    add(pruefe("FIX612B P2-3: die gezeichnete Stufe wechselt nicht mitten im Strich", [p] { return p->strichLief && !p->imStrichGewechselt; }));
    add(pruefe("FIX612B P2-3: der Strich hat nur die Stufe geaendert, in der er begann", [p] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        std::string z;
        int andere = 0;
        for (int st = 0; st < in.stufenZahl; ++st) {
            const int n = geaenderteEcken(p->vorher, in, st);
            z += " Stufe " + std::to_string(st) + ": " + std::to_string(n);
            andere += st != p->stufeStart ? n : 0;
        }
        melde("P2-3 - geaenderte Ecken je Stufe:" + z);
        return p->strichLief && andere == 0 && geaenderteEcken(p->vorher, in, p->stufeStart) > 0;
    }));
    add(pruefe("FIX612B P2-3: gespiegelt wird nur auf die Gegenstuecke der gemalten Punkte (jede Stufe)", [p] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const modell::Netz* arm = netz(in, p->arm, 0);
        if (arm == nullptr || arm->ecken.empty() || !p->strichLief) { return false; }
        double x = 0.0;
        for (const modell::Ecke& e : arm->ecken) { x += static_cast<double>(e.ort[0]); }
        const float seite = x > 0.0 ? 1.0F : -1.0F;
        int verstoesse = 0;
        std::string z;
        for (int st = 0; st < in.stufenZahl; ++st) {
            int geaendert = 0;
            const int v = spiegelVerstoesse(p->vorher, in, st, p->k, p->sk, seite, &geaendert);
            z += " Stufe " + std::to_string(st) + ": " + std::to_string(geaendert) + " Punkte geaendert, " + std::to_string(v) + " ohne gemaltes Gegenstueck;";
            verstoesse += v;
        }
        melde("P2-3 - Spiegeln:" + z);
        return verstoesse == 0;
    }));
    add(tu("FIX612B P2-3: zurueck", [] {
        (void)modellRueckgaengig();
        mtab().lodWahl = 1;
        mtab().pinselSpiegeln = false;
        gw().autoNormalisieren = true;
    }));
    add(pause(0.3));
}

// P2-3, die Richtung mit dem Zugriff hinter das Ende (eigene Gruppe fix612babsturz): spiegelMalen bekommt das Schweissnetz der
// feinen Stufe und schreibt in die grobe - an Punkten, deren Gegenstueck in der groben Stufe keine Ecke mit dieser Nummer hat.
void p23absturz(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    grund(s, "P2-3 Absturz", stufenPfad());
    add(pruefe("FIX612B P2-3 Absturz: spiegelMalen mit dem Schweissnetz einer anderen Stufe fasst nichts an (vorher: Zugriff hinter das Ende der Ecken)", [] {
        modell::Inhalt kopie = mtab().dok.inhalt();
        const int grob = kopie.stufenZahl - 1;
        const int arm = kopie.teilIndex("l_arm");
        if (grob < 1 || arm < 0) { return false; }
        const modell::Schweissnetz fein = modell::schweissen(kopie, 0);
        const std::vector<int> partner = modell::spiegelPartner(kopie, 0, fein, 0.5F);
        const std::size_t grobEcken = kopie.teile[uz(arm)].stufen[uz(grob)].ecken.size();
        std::vector<std::vector<modell::Tupfer>> je(kopie.teile.size());
        int hinten = 0;
        for (std::size_t e = 0; e < grobEcken && e < fein.punktVon[uz(arm)].size() && hinten < 8; ++e) {
            const int q = partner[uz(fein.punktVon[uz(arm)][e])];
            if (q < 0 || q == fein.punktVon[uz(arm)][e]) { continue; }
            for (const modell::EckOrt& o : fein.ecken[uz(q)]) {
                if (uz(o.ecke) >= kopie.teile[uz(o.teil)].stufen[uz(grob)].ecken.size()) {
                    je[uz(arm)].push_back(modell::Tupfer{static_cast<int>(e), 1.0F});
                    ++hinten;
                    break;
                }
            }
        }
        melde("P2-3 Absturz - Stufe " + std::to_string(grob) + ", " + std::to_string(hinten) + " Tupfer, deren Gegenstueck in der groben Stufe hinter dem Ende laege");
        if (hinten == 0) { return false; }
        const std::string vorher = gewichtStand(kopie);
        // lhumerus mit Gewicht an den getupften Ecken, damit das Spiegeln etwas zu schreiben hat
        const int k = knochen("lhumerus");
        const int sk = knochen("rhumerus");
        const int n = modell::spiegelMalen(kopie, grob, fein, partner, je, k, sk);
        melde("P2-3 Absturz - spiegelMalen gibt " + std::to_string(n) + " zurueck");
        return n == 0 && gewichtStand(kopie) == vorher;
    }));
}

// =====================================================================================================================
// P2-9 [M]: die Auswahl gehoert zu ihrer Stufe
struct P29 {
    int head = -1;
    int k = -1;
    std::string stufe1Vorher;
    int gesetzt = -1;
    int wahlNachWechsel = -1;
    int meshNachWechsel = -1;
    int stufeImKaestchen = -1;
    int stufeNachKaestchen = -1;
};
void p29(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto p = std::make_shared<P29>();
    grund(s, "P2-9", stufenPfad());
    add(tu("FIX612B P2-9: Stufe 0, Ebene Ecke, fuenf Ecken von head gewaehlt, Knochen thoracic, Gewicht .5", [p] {
        ModellTab& t = mtab();
        gw().malen = false;
        p->head = teil("head");
        p->k = knochen("thoracic");
        t.knochenWahl = p->k;
        t.pinselGewicht = 0.5F;
        modellAuswahlArtSetzen(AuswahlArt::Ecke);
        t.eckenWahl.clear();
        for (int e = 0; e < 5; ++e) { t.eckenWahl.push_back(modell::EckOrt{p->head, e}); }
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("FIX612B P2-9: Stufe 1 waehlen", [] {
        mtab().lodWahl = 2;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(tu("FIX612B P2-9: Festwert auf die Auswahl", [p] {
        ModellTab& t = mtab();
        p->wahlNachWechsel = static_cast<int>(t.eckenWahl.size());
        p->stufe1Vorher = gewichtStand(t.dok.inhalt(), -1, 1);
        p->gesetzt = modellAufAuswahlSetzen();
    }));
    add(pruefe("FIX612B P2-9: nach dem Wechsel der Stufe zeigt die Auswahl nicht auf Ecken der anderen Stufe", [p] {
        melde("P2-9 - gezeichnete Stufe " + std::to_string(mtab().lodJetzt) + ", gewaehlte Ecken nach dem Wechsel " + std::to_string(p->wahlNachWechsel));
        return mtab().lodJetzt == 1 && p->wahlNachWechsel == 0;
    }));
    add(pruefe("FIX612B P2-9: 'Setzen' auf die Auswahl aendert in der anderen Stufe nichts", [p] {
        melde("P2-9 - Setzen meldet " + std::to_string(p->gesetzt) + " Ecken (Knochen thoracic " + std::to_string(p->k) + ")");
        return p->k >= 0 && p->head >= 0 && p->gesetzt == 0 && gewichtStand(mtab().dok.inhalt(), -1, 1) == p->stufe1Vorher;
    }));
    add(tu("FIX612B P2-9: zurueck auf Stufe 0", [p] {
        if (p->gesetzt > 0) { (void)modellRueckgaengig(); }
        mtab().lodWahl = 1;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefe("FIX612B P2-9: zurueck in der Stufe steht ihre Auswahl wieder da (fuenf Ecken von head)", [p] {
        const ModellTab& t = mtab();
        bool gleich = t.eckenWahl.size() == 5U;
        for (std::size_t e = 0; gleich && e < 5U; ++e) { gleich = t.eckenWahl[e].teil == p->head && t.eckenWahl[e].ecke == static_cast<int>(e); }
        return t.lodJetzt == 0 && gleich;
    }));
    // die Auswahl des Arbeitsnetzes (Modellieren)
    add(tu("FIX612B P2-9: Modellieren, drei Vielecke gewaehlt", [] {
        ModellTab& t = mtab();
        t.eckenWahl.clear();
        t.bereich = ModellBereich::Modellieren;
        modellAuswahlArtSetzen(AuswahlArt::Dreieck);
        modellMeshNetzAktualisieren();
        t.mesh.auswahl = w::PAuswahl{{}, {}, {0, 1, 2}};
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("FIX612B P2-9: Stufe 1 waehlen", [] {
        mtab().lodWahl = 2;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefe("FIX612B P2-9: die Auswahl des Arbeitsnetzes zeigt nach dem Wechsel nicht auf Vielecke der anderen Stufe", [p] {
        modellMeshNetzAktualisieren();
        p->meshNachWechsel = static_cast<int>(mtab().mesh.auswahl.flaechen.size());
        melde("P2-9 - Stufe " + std::to_string(mtab().lodJetzt) + ", gewaehlte Vielecke " + std::to_string(p->meshNachWechsel));
        return mtab().lodJetzt == 1 && p->meshNachWechsel == 0;
    }));
    // ein offenes Kaestchen haelt die Stufe
    add(tu("FIX612B P2-9: Stufe 0, ein Vieleck von torso, Extrudieren im Kaestchen offen", [] {
        ModellTab& t = mtab();
        t.lodWahl = 1;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(tu("FIX612B P2-9: Kaestchen auf, dann Stufe 1 waehlen", [] {
        ModellTab& t = mtab();
        modellMeshNetzAktualisieren();
        const int torso = teil("torso");
        int f = -1;
        for (std::size_t i = 0; i < t.mesh.netz.flaechen.size() && f < 0; ++i) {
            if (t.mesh.netz.flaechen[i].teil == torso && !t.mesh.netz.flaechen[i].weg) { f = static_cast<int>(i); }
        }
        t.mesh.auswahl = w::PAuswahl{{}, {}, {f}};
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        t.mesh.betrag = 1.0F;
        modellMeshVorschau();
        t.lodWahl = 2;
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(tu("FIX612B P2-9: Stufe im offenen Kaestchen merken, abbrechen", [p] {
        p->stufeImKaestchen = mtab().mesh.kaestchenOffen ? mtab().lodJetzt : -2;
        modellMeshAbbrechen();
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(pruefe("FIX612B P2-9: solange das Kaestchen offen ist, bleibt die gezeichnete Stufe; danach wechselt sie", [p] {
        p->stufeNachKaestchen = mtab().lodJetzt;
        melde("P2-9 - Stufe im Kaestchen " + std::to_string(p->stufeImKaestchen) + ", danach " + std::to_string(p->stufeNachKaestchen));
        return p->stufeImKaestchen == 0 && p->stufeNachKaestchen == 1;
    }));
    add(tu("FIX612B P2-9: zurueck", [] {
        ModellTab& t = mtab();
        t.mesh.auswahl = w::PAuswahl{};
        t.lodWahl = 1;
        t.bereich = ModellBereich::Weights;
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
}

// =====================================================================================================================
// P2-4 [M]: ein Modellier-Werkzeug und die offenen Mallisten
struct P24 {
    modell::EckOrt a{-1, -1};   // torso
    modell::EckOrt b{-1, -1};   // hips, am selben Ort
    float ort[3]{};
    int k = -1;
    float vorher = 0.0F;
    std::string gezeigtA;
    std::string gezeigtB;
    modell::GewichtListe liste;
    std::string pfad;
};
// Die Ecke eines Netzes am Ort (bitgleich), -1 keine.
int eckeAm(const modell::Netz& n, const float ort[3]) {
    for (std::size_t e = 0; e < n.ecken.size(); ++e) {
        if (std::memcmp(n.ecken[e].ort, ort, sizeof n.ecken[e].ort) == 0) { return static_cast<int>(e); }
    }
    return -1;
}
void p24(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto p = std::make_shared<P24>();
    grund(s, "P2-4");
    add(tu("FIX612B P2-4: ein Punkt an der Grenze torso/hips, Festwert .5 fuer rhumerus (beide Flaechen bekommen Mallisten)", [p] {
        ModellTab& t = mtab();
        const modell::Inhalt& in = t.dok.inhalt();
        const int torso = teil("torso");
        const int hips = teil("hips");
        const modell::Schweissnetz netzAlle = modell::schweissen(in, 0);
        for (int q = 0; q < netzAlle.zahl() && p->a.ecke < 0; ++q) {
            modell::EckOrt a{-1, -1};
            modell::EckOrt b{-1, -1};
            for (const modell::EckOrt& o : netzAlle.ecken[uz(q)]) {
                if (o.teil == torso && a.ecke < 0) { a = o; }
                if (o.teil == hips && b.ecke < 0) { b = o; }
            }
            if (a.ecke >= 0 && b.ecke >= 0) {
                p->a = a;
                p->b = b;
            }
        }
        if (p->a.ecke < 0) { return; }
        std::memcpy(p->ort, in.teile[uz(torso)].stufen[0].ecken[uz(p->a.ecke)].ort, sizeof p->ort);
        p->k = knochen("rhumerus");
        t.knochenWahl = p->k;
        t.pinselGewicht = 0.5F;
        gw().malen = false;
        p->vorher = modell::gewichtVon(in.teile[uz(torso)].stufen[0], p->a.ecke, p->k);
        modellAuswahlArtSetzen(AuswahlArt::Ecke);
        t.eckenWahl = {p->a};
        (void)modellAufAuswahlSetzen();
    }));
    add(pause(0.2));
    add(pruefe("FIX612B P2-4 Vorbereitung: torso und hips tragen am selben Ort dieselbe Malliste mit dem neuen Gewicht", [p] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const modell::Netz* a = netz(in, p->a.teil, 0);
        const modell::Netz* b = netz(in, p->b.teil, 0);
        if (a == nullptr || b == nullptr || p->a.ecke < 0 || !modell::malt(*a) || !modell::malt(*b)) { return false; }
        p->gezeigtA = gezeigt(*a, uz(p->a.ecke));
        p->gezeigtB = gezeigt(*b, uz(p->b.ecke));
        p->liste = a->mal[uz(p->a.ecke)];
        char z[200];
        std::snprintf(z, sizeof z, "P2-4 - torso Ecke %d / hips Ecke %d, rhumerus vorher %.4f, jetzt %.4f / %.4f", p->a.ecke, p->b.ecke, static_cast<double>(p->vorher),
                      static_cast<double>(modell::gewichtVon(*a, p->a.ecke, p->k)), static_cast<double>(modell::gewichtVon(*b, p->b.ecke, p->k)));
        melde(z);
        return p->gezeigtA == p->gezeigtB && modell::gewichtVon(*a, p->a.ecke, p->k) > p->vorher + 0.05F;
    }));
    add(tu("FIX612B P2-4: Modellieren, ein Vieleck von torso abseits der Ecke extrudieren und uebernehmen", [p] {
        ModellTab& t = mtab();
        t.eckenWahl.clear();
        t.bereich = ModellBereich::Modellieren;
        modellAuswahlArtSetzen(AuswahlArt::Dreieck);
        modellMeshNetzAktualisieren();
        const w::PolyNetz& n = t.mesh.netz;
        int f = -1;
        for (std::size_t i = 0; i < n.flaechen.size() && f < 0; ++i) {
            if (n.flaechen[i].teil != p->a.teil || n.flaechen[i].weg) { continue; }
            bool fern = true;
            for (const w::PEcke& c : n.flaechen[i].ecken) {
                const float* o = n.punkte[uz(c.punkt)].ort;
                fern = fern && std::hypot(o[0] - p->ort[0], o[1] - p->ort[1], o[2] - p->ort[2]) > 6.0F;
            }
            if (fern) { f = static_cast<int>(i); }
        }
        t.mesh.auswahl = w::PAuswahl{{}, {}, {f}};
        (void)modellMeshWerkzeugStarten(MeshWerkzeug::Extrude);
        t.mesh.betrag = 1.0F;
        modellMeshVorschau();
        modellMeshUebernehmen(false);
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("FIX612B P2-4: die Ecke von torso zeigt nach dem Werkzeug dieselben Gewichte wie davor - und wie die Ecke gleichen Orts in hips", [p] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const modell::Netz* a = netz(in, p->a.teil, 0);
        const modell::Netz* b = netz(in, p->b.teil, 0);
        if (a == nullptr || b == nullptr) { return false; }
        const int e = eckeAm(*a, p->ort);
        if (e < 0) { return false; }
        char z[240];
        std::snprintf(z, sizeof z, "P2-4 - Rueckgaengig-Schritt '%s', torso %zu Ecken, Ecke am Ort %d: rhumerus %.4f (vor dem Setzen %.4f), hips %.4f", mtab().dok.rueckgaengigName().c_str(),
                      a->ecken.size(), e, static_cast<double>(modell::gewichtVon(*a, e, p->k)), static_cast<double>(p->vorher), static_cast<double>(modell::gewichtVon(*b, p->b.ecke, p->k)));
        melde(z);
        return gezeigt(*a, uz(e)) == p->gezeigtA && gezeigt(*a, uz(e)) == gezeigt(*b, uz(p->b.ecke));
    }));
    add(pruefe("FIX612B P2-4: die Malliste der unberuehrten Ecke ist mitgekommen (torso malt weiter, Liste gleich der von hips)", [p] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const modell::Netz* a = netz(in, p->a.teil, 0);
        const modell::Netz* b = netz(in, p->b.teil, 0);
        if (a == nullptr || b == nullptr || !modell::malt(*a) || !modell::malt(*b)) { return false; }
        const int e = eckeAm(*a, p->ort);
        if (e < 0 || a->mal[uz(e)].size() != p->liste.size()) { return false; }
        bool gleich = a->mal.size() == a->ecken.size();
        for (std::size_t i = 0; gleich && i < p->liste.size(); ++i) {
            gleich = a->mal[uz(e)][i].knochen == p->liste[i].knochen && a->mal[uz(e)][i].gewicht == p->liste[i].gewicht;
        }
        return gleich;
    }));
    add(tu("FIX612B P2-4: Speichern unter (Laufordner)", [p] {
        p->pfad = mb3::lauf("fix612b_p24.glm");
        mtab().speicherPfad = p->pfad;
        modellSpeichern(false);
    }));
    add(pause(0.3));
    add(pruefe("FIX612B P2-4: die Datei nach Modellieren und Malen traegt die gezeigten Gewichte im Format des Spiels", [p] {
        const std::string bytes = dateiLesen(p->pfad);
        const DateiMass m = dateiMass(bytes);
        int verglichen = 0;
        const int anders = dateiGegenAnzeige(bytes, mtab().dok.inhalt(), &verglichen);
        melde("P2-4 Bytes - " + massText(m) + "; anders als die Anzeige: " + std::to_string(anders) + " von " + std::to_string(verglichen) + "; " + mtab().meldung);
        return m.lesbar && m.gewichteMax <= 4 && m.restNichtPositiv == 0 && m.verweisFalsch == 0 && m.refsUeber32 == 0 && anders == 0 && verglichen > 1000;
    }));
    add(tu("FIX612B P2-4: zurueck", [] {
        ModellTab& t = mtab();
        t.speicherPfad.clear();
        t.frageArt = 0;
        t.mesh.auswahl = w::PAuswahl{};
        t.bereich = ModellBereich::Weights;
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
    }));
    add(pause(0.2));
}

// =====================================================================================================================
// P2-1 [M]: Auto-Skin "Nur Auswahl" mit "Spiegeln"
struct P21 {
    int arm = -1;
    int kopf = -1;
    std::vector<std::string> vorher;   // je Teil
    modell::GewichtListe kopfListe;
    int probeTeil = -1;
    int probeEcke = -1;
};
void p21(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto p = std::make_shared<P21>();
    grund(s, "P2-1");
    add(tu("FIX612B P2-1: rechte Seite von Hand anders (eine Ecke je nicht gewaehlter Flaeche bei x < 0), offene Malliste auf head", [p] {
        ModellTab& t = mtab();
        p->arm = teil("l_arm");
        p->kopf = teil("head");
        const int becken = knochen("pelvis");
        t.dok.aendern("FIX612B rechte Seite", [&](modell::Inhalt& x) {
            for (std::size_t i = 0; i < x.teile.size(); ++i) {
                if (static_cast<int>(i) == p->arm || x.teile[i].istTag() || x.teile[i].stufen.empty()) { continue; }
                for (modell::Netz& n : x.teile[i].stufen) {
                    for (std::size_t e = 0; e < n.ecken.size(); ++e) {
                        if (n.ecken[e].ort[0] < -2.0F) {   // eine Ecke der Seite, auf die gespiegelt wird
                            modell::Ecke& ecke = n.ecken[e];
                            for (int k = 0; k < 4; ++k) {
                                ecke.knochen[k] = 0;
                                ecke.gewicht[k] = 0.0F;
                            }
                            ecke.knochen[0] = static_cast<std::uint8_t>(becken);
                            ecke.gewicht[0] = 1.0F;
                            if (p->probeTeil < 0 && &n == &x.teile[i].stufen[0]) {
                                p->probeTeil = static_cast<int>(i);
                                p->probeEcke = static_cast<int>(e);
                            }
                            break;
                        }
                    }
                }
            }
            // eine offene Malliste mit fuenf Knochen auf einer nicht gewaehlten Flaeche
            if (p->kopf >= 0) {
                modell::Netz& n = x.teile[uz(p->kopf)].stufen[0];
                modell::malAnlegen(n);
                n.mal[0] = {{1, 0.3F}, {2, 0.25F}, {3, 0.2F}, {4, 0.15F}, {5, 0.1F}};
                p->kopfListe = n.mal[0];
            }
        });
        const modell::Inhalt& in = t.dok.inhalt();
        p->vorher.clear();
        for (std::size_t i = 0; i < in.teile.size(); ++i) { p->vorher.push_back(gewichtStand(in, static_cast<int>(i))); }
        ModellAutoSkin& a = modellAutoSkinZustand();
        a = ModellAutoSkin{};
        a.nurAuswahl = true;
        a.o.spiegeln = true;
        a.o.aufloesung = 96;
        t.auswahl = {p->arm};
        (void)modellAutoSkin();
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("FIX612B P2-1: Auto-Skin 'Nur Auswahl' (l_arm) mit 'Spiegeln' - jede nicht gewaehlte Flaeche behaelt ihre Gewichte", [p] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const ModellAutoSkin& a = modellAutoSkinZustand();
        std::string anders;
        int zahl = 0;
        for (std::size_t i = 0; i < in.teile.size() && i < p->vorher.size(); ++i) {
            if (static_cast<int>(i) == p->arm) { continue; }
            if (gewichtStand(in, static_cast<int>(i)) != p->vorher[i]) {
                ++zahl;
                if (zahl <= 8) { anders += " " + in.teile[i].name; }
            }
        }
        melde("P2-1 - gelaufen " + std::to_string(a.gelaufen ? 1 : 0) + ", gespiegelt " + std::to_string(a.bericht.gespiegelt) + " Ecken; nicht gewaehlte Flaechen mit anderen Gewichten: " +
              std::to_string(zahl) + anders);
        return a.gelaufen && p->arm >= 0 && gewichtStand(in, p->arm) != p->vorher[uz(p->arm)] && zahl == 0;
    }));
    add(pruefe("FIX612B P2-1: die offene Malliste einer nicht gewaehlten Flaeche (head) bleibt offen", [p] {
        const modell::Netz* n = netz(mtab().dok.inhalt(), p->kopf, 0);
        return n != nullptr && modell::malt(*n) && n->mal[0].size() == p->kopfListe.size();
    }));
    add(tu("FIX612B P2-1: zurueck, beide Arme gewaehlt, noch einmal", [p] {
        ModellTab& t = mtab();
        (void)modellRueckgaengig();
        t.auswahl = {p->arm, teil("r_arm")};
        (void)modellAutoSkin();
    }));
    add(pause(0.3));
    add(pruefe("FIX612B P2-1: mit beiden Armen in der Auswahl traegt der Arm bei x < 0 die gespiegelten Gewichte des anderen", [p] {
        const modell::Inhalt& in = mtab().dok.inhalt();
        const ModellAutoSkin& a = modellAutoSkinZustand();
        const int rechts = teil("r_arm");
        // der Arm, auf den gespiegelt wird, liegt bei x < 0
        const modell::Netz* la = netz(in, p->arm, 0);
        const modell::Netz* ra = netz(in, rechts, 0);
        if (la == nullptr || ra == nullptr || la->ecken.empty() || ra->ecken.empty()) { return false; }
        const modell::Netz* ziel = la->ecken[0].ort[0] < 0.0F ? la : ra;
        const int lh = knochen("lhumerus");
        const int rh = knochen("rhumerus");
        const int zielKnochen = ziel == la ? lh : rh;
        const int fremd = ziel == la ? rh : lh;
        double eigen = 0.0;
        double anderer = 0.0;
        for (std::size_t e = 0; e < ziel->ecken.size(); ++e) {
            eigen += static_cast<double>(modell::gewichtVon(*ziel, static_cast<int>(e), zielKnochen));
            anderer += static_cast<double>(modell::gewichtVon(*ziel, static_cast<int>(e), fremd));
        }
        char z[200];
        std::snprintf(z, sizeof z, "P2-1 - gespiegelt %d Ecken; Arm bei x < 0: Summe eigener Oberarm %.2f, fremder Oberarm %.2f", a.bericht.gespiegelt, eigen, anderer);
        melde(z);
        return a.gelaufen && a.bericht.gespiegelt > 0 && eigen > 1.0 && anderer < 0.01;
    }));
    add(tu("FIX612B P2-1: zurueck", [] {
        (void)modellRueckgaengig();
        (void)modellRueckgaengig();
        modellAutoSkinZustand() = ModellAutoSkin{};
        mtab().auswahl.clear();
    }));
}

// =====================================================================================================================
// P2-10 [M]: Strg+Z mitten im Strich, im Verlauf, im Gizmo-Zug, im Painting
struct P210 {
    std::string vorher;
    std::string undoVorher;
    ImVec2 ort{};
    bool strichNachTaste = false;
    int tupferNachTaste = 0;
    std::vector<std::uint8_t> bild;
    int stufeImZug = -1;
};
// Gewichte, Mallisten und die Orte aller Ecken des Dokuments.
std::string dokStand() {
    const modell::Inhalt& in = mtab().dok.inhalt();
    std::string s = gewichtStand(in);
    for (const modell::Teil& t : in.teile) {
        for (const modell::Netz& n : t.stufen) {
            for (const modell::Ecke& e : n.ecken) { s.append(reinterpret_cast<const char*>(e.ort), sizeof e.ort); }
        }
    }
    return s;
}
// Nach einem Zug mit Strg+Z mittendrin (Entscheidung A17: abbrechen und zuruecknehmen, Strg+Y bringt ihn wieder): der Stand ist
// der vor dem Zug, oben liegt der Schritt von davor, auf Wiederholen der Schritt `name`; Strg+Y bringt die Aenderung, Strg+Z nimmt
// sie wieder ganz zurueck.
bool zugSauber(const std::string& was, const std::string& vorher, const std::string& undoVorher, const std::string& name) {
    ModellTab& t = mtab();
    const bool geaendert = dokStand() != vorher;
    const std::string oben = t.dok.rueckgaengigName();
    const std::string vor = t.dok.wiederholenName();
    melde("P2-10 " + was + " - nach dem Zug " + (geaendert ? "geaendert" : "unveraendert") + ", oberster Schritt '" + oben + "' (davor '" + undoVorher + "'), Wiederholen '" + vor +
          "'");
    if (geaendert || oben != undoVorher || vor != name) { return false; }
    (void)modellWiederholen();
    const bool wieder = t.dok.rueckgaengigName() == name && dokStand() != vorher;
    (void)modellRueckgaengig();
    melde("P2-10 " + was + " - Strg+Y bringt den Zug wieder: " + (wieder ? "ja" : "nein"));
    return wieder && dokStand() == vorher;
}
void p210(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto p = std::make_shared<P210>();
    grund(s, "P2-10");
    // (a) Pinselstrich
    add(tu("FIX612B P2-10: Pinsel, Knochen rhumerus, Stand merken", [p] {
        ModellTab& t = mtab();
        t.knochenWahl = knochen("rhumerus");
        gw().malen = true;
        gw().werkzeug = 0;
        t.pinselRadius = 30.0F;
        t.pinselGewicht = 0.3F;
        (void)vordereEcke(teil("torso"), p->ort);
        p->vorher = dokStand();
        p->undoVorher = t.dok.rueckgaengigName();
        p->strichNachTaste = false;
        p->tupferNachTaste = 0;
    }));
    add(zug("FIX612B P2-10: Pinselstrich ueber torso, mittendrin Strg+Z", [p] { return ImVec2{p->ort.x - 40.0F, p->ort.y}; }, 4.0F, 24, 8, [p](int i) {
        if (i == 14) {
            p->strichNachTaste = mtab().malStrich;
            p->tupferNachTaste = mtab().tupferZahl;
        }
        if (i == 23) { p->tupferNachTaste = mtab().tupferZahl - p->tupferNachTaste; }
    }));
    add(pruefe("FIX612B P2-10 Pinsel: Strg+Z im Strich laesst keine Aenderung ohne eigenen Rueckgaengig-Schritt zurueck", [p] {
        melde("P2-10 Pinsel - nach der Taste: Strich laeuft " + std::to_string(p->strichNachTaste ? 1 : 0) + ", Tupfer danach " + std::to_string(p->tupferNachTaste));
        return zugSauber("Pinsel", p->vorher, p->undoVorher, tr(Str::MbUndoMalen));
    }));
    add(pruefe("FIX612B P2-10 Pinsel: Strg+Z beendet den Strich (wie Esc) - die gehaltene Taste malt nicht weiter", [p] { return !p->strichNachTaste && p->tupferNachTaste == 0; }));
    // (b) Verlauf
    add(tu("FIX612B P2-10: Verlauf, Stand merken", [p] {
        ModellTab& t = mtab();
        while (dokStand() != p->vorher && modellRueckgaengig()) {}
        modellGewichteWerkzeug(2);
        t.pinselGewicht = 0.8F;
        p->vorher = dokStand();
        p->undoVorher = t.dok.rueckgaengigName();
    }));
    add(zug("FIX612B P2-10: Verlauf ueber torso, mittendrin Strg+Z", [p] { return ImVec2{p->ort.x - 40.0F, p->ort.y}; }, 4.0F, 24, 8, [p](int i) {
        if (i == 14) { p->strichNachTaste = modellGewichte().verlaufZieht; }
    }));
    add(pruefe("FIX612B P2-10 Verlauf: Strg+Z im Ziehen laesst keine Aenderung ohne eigenen Rueckgaengig-Schritt zurueck", [p] {
        melde("P2-10 Verlauf - nach der Taste: Verlauf zieht " + std::to_string(p->strichNachTaste ? 1 : 0));
        return zugSauber("Verlauf", p->vorher, p->undoVorher, tr(Str::MgUndoVerlauf)) && !p->strichNachTaste;
    }));
    // (c) Gizmo
    add(tu("FIX612B P2-10: Gizmo Verschieben, torso gewaehlt, Ansicht von der Seite", [p] {
        ModellTab& t = mtab();
        while (dokStand() != p->vorher && modellRueckgaengig()) {}
        modellGewichteWerkzeug(0);
        gw().malen = false;
        t.bereich = ModellBereich::Ansehen;
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        t.auswahl = {teil("torso")};
        t.gizmo = 1;
        g_app->modelOrbit.turn(g_app->modelCam, 90.0F, 0.0F);
        g_app->modelDirty = true;
        p->vorher = dokStand();
        p->undoVorher = t.dok.rueckgaengigName();
    }));
    add(pause(0.4));
    add(zug("FIX612B P2-10: am Y-Griff des Gizmos ziehen, mittendrin Strg+Z", [] {
        ImVec2 g{0.0F, 0.0F};
        (void)modellGizmoGriffOrt(2, g);
        return g;
    }, 3.0F, 24, 8, [p](int i) {
        if (i == 14) { p->strichNachTaste = mtab().griff != 0; }
    }));
    add(pruefe("FIX612B P2-10 Gizmo: Strg+Z im Zug laesst keine Aenderung ohne eigenen Rueckgaengig-Schritt zurueck", [p] {
        melde("P2-10 Gizmo - nach der Taste: Griff gehalten " + std::to_string(p->strichNachTaste ? 1 : 0));
        return zugSauber("Gizmo", p->vorher, p->undoVorher, tr(Str::MbUndoBewegen)) && !p->strichNachTaste;
    }));
    add(tu("FIX612B P2-10: Ansicht wieder von vorne, Gizmo aus", [p] {
        ModellTab& t = mtab();
        while (dokStand() != p->vorher && modellRueckgaengig()) {}
        t.gizmo = 0;
        t.auswahl.clear();
        g_app->modelOrbit.turn(g_app->modelCam, -90.0F, 0.0F);
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    // (d) Painting
    add(tu("FIX612B P2-10: Bereich Painting, Bild von torso", [] {
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Painting;
        t.auswahl = {teil("torso")};
        t.malen.gruppe = -1;
        t.malen.radiusPx = 16.0F;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(tu("FIX612B P2-10: Bild vor dem Strich merken", [p] {
        const modell::texmalen::Leinwand* l = modellMalenLeinwand();
        p->bild = l != nullptr ? l->bild : std::vector<std::uint8_t>{};
        (void)modellFlaecheAufSchirm(teil("torso"), p->ort);
        p->ort = ImVec2{std::floor(p->ort.x), std::floor(p->ort.y)};
    }));
    add(zug("FIX612B P2-10: Painting-Strich ueber torso, mittendrin Strg+Z", [p] { return ImVec2{p->ort.x - 30.0F, p->ort.y}; }, 3.0F, 24, 8, [p](int i) {
        if (i == 14) { p->strichNachTaste = mtab().malen.strichLaeuft; }
    }));
    add(pause(0.3));
    add(tu("FIX612B P2-10: Strg+Z, bis nichts mehr geht", [] {
        int n = 0;
        while (modellRueckgaengig() && n < 40) { ++n; }
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(pruefe("FIX612B P2-10 Painting: nach Strg+Z im Strich bringt Rueckgaengig jedes gemalte Texel zurueck", [p] {
        const modell::texmalen::Leinwand* l = modellMalenLeinwand();
        if (l == nullptr || p->bild.empty()) {
            meldeFehler("FIX612B P2-10: keine Leinwand fuer torso - " + mtab().malen.meldung);
            return false;
        }
        int anders = 0;
        for (std::size_t i = 0; i + 3U < l->bild.size() && i + 3U < p->bild.size(); i += 4U) { anders += std::memcmp(&l->bild[i], &p->bild[i], 4) != 0 ? 1 : 0; }
        melde("P2-10 Painting - nach der Taste: Strich laeuft " + std::to_string(p->strichNachTaste ? 1 : 0) + "; Texel anders als vor dem Strich nach allem Rueckgaengig: " +
              std::to_string(anders));
        return anders == 0 && l->bild.size() == p->bild.size();
    }));
    add(pruefe("FIX612B P2-10 Painting: Strg+Y bringt den abgebrochenen Strich wieder", [p] {
        (void)modellWiederholen();
        const modell::texmalen::Leinwand* l = modellMalenLeinwand();
        int anders = 0;
        for (std::size_t i = 0; l != nullptr && i + 3U < l->bild.size() && i + 3U < p->bild.size(); i += 4U) { anders += std::memcmp(&l->bild[i], &p->bild[i], 4) != 0 ? 1 : 0; }
        melde("P2-10 Painting - nach Strg+Y anders als vor dem Strich: " + std::to_string(anders) + " Texel");
        (void)modellRueckgaengig();
        return anders > 0;
    }));
    add(tu("FIX612B P2-10: zurueck", [] {
        ModellTab& t = mtab();
        t.auswahl.clear();
        t.bereich = ModellBereich::Weights;
    }));
    add(pause(0.3));
}

// =====================================================================================================================
// P2-5 [N]: ein Befehl ohne Aenderung und das Wiederholen
void p25(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto n = std::make_shared<int>(-99);
    auto vorher = std::make_shared<bool>(false);
    grund(s, "P2-5");
    add(tu("FIX612B P2-5: Alles normalisieren, eine Aenderung, Rueckgaengig (Wiederholen ist moeglich), dann noch einmal 'Alles normalisieren' (ohne Wirkung)", [=] {
        ModellTab& t = mtab();
        // die Datei hat Ecken, die das Normalisieren aendert - so oft, bis es nichts mehr aendert (mit dem Knochen, der danach aktiv
        // ist: "Alles normalisieren" haelt den aktiven Knochen fest wie Blender lock_active)
        t.knochenWahl = knochen("rhumerus");
        for (int i = 0; i < 8 && modellGewichteBereinigen(0) > 0; ++i) {}
        t.pinselGewicht = 0.5F;
        t.auswahl = {teil("l_hand")};
        (void)modellAufAuswahlSetzen();
        (void)modellRueckgaengig();
        t.auswahl.clear();
        *vorher = t.dok.kannWiederholen();
        *n = modellGewichteBereinigen(0);
    }));
    add(pruefe("FIX612B P2-5: ein Befehl, der nichts aendert, laesst das Wiederholen stehen (Alles normalisieren)", [=] {
        melde("P2-5 - Wiederholen vorher " + std::to_string(*vorher ? 1 : 0) + ", Alles normalisieren aendert " + std::to_string(*n) + " Ecken, Wiederholen danach " +
              std::to_string(mtab().dok.kannWiederholen() ? 1 : 0));
        return *vorher && *n == 0 && mtab().dok.kannWiederholen();
    }));
    add(tu("FIX612B P2-5: Verlauf, ein Klick ohne Strecke", [] {
        ModellTab& t = mtab();
        ImVec2 ort;
        (void)vordereEcke(teil("torso"), ort);
        modellGewichteWerkzeug(2);
        (void)modellTupfer(t.aktiv, ort, true);
        modellStrichEnde();
        modellGewichteWerkzeug(0);
    }));
    add(pruefe("FIX612B P2-5: ein Klick mit dem Verlauf ohne Strecke laesst das Wiederholen stehen", [] { return mtab().dok.kannWiederholen(); }));
    add(tu("FIX612B P2-5: Wiederholen", [] { (void)modellWiederholen(); }));
    add(pruefe("FIX612B P2-5: Wiederholen bringt die Aenderung zurueck", [] {
        const modell::Netz* h = netz(mtab().dok.inhalt(), teil("l_hand"), 0);
        return h != nullptr && modell::malt(*h);
    }));
    add(tu("FIX612B P2-5: zurueck", [] { (void)modellRueckgaengig(); }));
}

// =====================================================================================================================
// P2-6 [N]: die Pinselart eines Strichs, der neben der Figur beginnt
void p26(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto ort = std::make_shared<ImVec2>();
    auto ecke = std::make_shared<int>(-1);
    auto vorher = std::make_shared<float>(0.0F);
    grund(s, "P2-6");
    add(tu("FIX612B P2-6: ein Strich mit 'Abziehen' auf torso, dann 'Hinzufuegen' und ein Strich, der neben der Figur beginnt", [=] {
        ModellTab& t = mtab();
        const int torso = teil("torso");
        t.knochenWahl = knochen("rhumerus");
        gw().malen = true;
        gw().autoNormalisieren = false;
        t.pinselGewicht = 0.25F;
        *ecke = vordereEcke(torso, *ort);
        t.pinselArt = static_cast<int>(modell::PinselArt::Abziehen);
        (void)modellTupfer(t.aktiv, *ort, true);
        modellStrichEnde();
        const modell::Netz* n = netz(t.dok.inhalt(), torso, 0);
        *vorher = n != nullptr ? modell::gewichtVon(*n, *ecke, t.knochenWahl) : -1.0F;
        t.pinselArt = static_cast<int>(modell::PinselArt::Hinzufuegen);
        const ModellAnsicht& a = t.ansichten[t.aktiv];
        (void)modellTupfer(t.aktiv, ImVec2{a.lo[0] + 6.0F, a.lo[1] + a.groesse[1] * 0.5F}, true);   // am linken Rand: keine Ecke im Pinsel
        (void)modellTupfer(t.aktiv, *ort, false);
        modellStrichEnde();
    }));
    add(pruefe("FIX612B P2-6: der Strich, der neben der Figur beginnt, malt mit der gewaehlten Art (Hinzufuegen)", [=] {
        const ModellTab& t = mtab();
        const modell::Netz* n = netz(t.dok.inhalt(), teil("torso"), 0);
        const float jetzt = n != nullptr ? modell::gewichtVon(*n, *ecke, t.knochenWahl) : -1.0F;
        char z[200];
        std::snprintf(z, sizeof z, "P2-6 - Art des Strichs %d, gewaehlt %d; rhumerus an der Ecke vorher %.4f, danach %.4f", t.strichArt, t.pinselArt, static_cast<double>(*vorher),
                      static_cast<double>(jetzt));
        melde(z);
        return t.strichArt == t.pinselArt && jetzt > *vorher + 0.1F;
    }));
    add(tu("FIX612B P2-6: zurueck", [] {
        (void)modellRueckgaengig();
        (void)modellRueckgaengig();
        gw().autoNormalisieren = true;
    }));
}

// =====================================================================================================================
// P2-7 [N]: gesperrte Knochen und ein anderes Modell
void p27(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto zahl = std::make_shared<std::size_t>(0);
    auto anderes = std::make_shared<std::string>();
    grund(s, "P2-7");
    add(tu("FIX612B P2-7: lhumerus sperren", [=] {
        mtab().knochenWahl = knochen("lhumerus");
        modellGewichteSperren(1, 1);
        *zahl = g_app->anim.bones.size();
        // ein anderes Modell mit demselben Skelett
        for (const char* k : {"models/players/luke/model.glm", "models/players/jan/model.glm", "models/players/stormtrooper/model.glm"}) {
            std::string b;
            if (anderes->empty() && readFromArchives(k, b)) { *anderes = k; }
        }
        if (anderes->empty()) { *anderes = modellPfad(); }
    }));
    add(pruefe("FIX612B P2-7 Vorbereitung: lhumerus ist gesperrt", [] { return modellGewichtGesperrt(knochen("lhumerus")); }));
    add(tu("FIX612B P2-7: ein anderes Modell laden", [=] {
        std::string fehler;
        if (!oeffneModellAusArchiv(*anderes, &fehler)) { meldeFehler("FIX612B P2-7: " + *anderes + ": " + fehler); }
        mtab().bereich = ModellBereich::Weights;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(pruefe("FIX612B P2-7: im neu geladenen Modell (gleiche Knochenzahl) ist kein Knochen gesperrt", [=] {
        (void)modellGewichteRegeln();
        int gesperrt = 0;
        for (const char c : gw().gesperrt) { gesperrt += c != 0 ? 1 : 0; }
        melde("P2-7 - " + *anderes + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen (vorher " + std::to_string(*zahl) + "), gesperrt " + std::to_string(gesperrt));
        return g_app->anim.bones.size() == *zahl && gesperrt == 0 && !modellGewichtGesperrt(knochen("lhumerus"));
    }));
}

// =====================================================================================================================
// P2-8 [N]: Painting mit Pinselradius 0
void p28(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto tupfer = std::make_shared<int>(0);
    auto ms = std::make_shared<double>(0.0);
    auto ort = std::make_shared<ImVec2>();
    grund(s, "P2-8");
    add(tu("FIX612B P2-8: Bereich Painting, Bild von torso", [] {
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Painting;
        t.auswahl = {teil("torso")};
        t.malen.gruppe = -1;
        g_app->modelDirty = true;
    }));
    add(pause(0.8));
    add(tu("FIX612B P2-8: Radius 0 (wie mit Strg+Klick in den Regler getippt), ein Strich von 6 Bildpunkten", [=] {
        ModellTab& t = mtab();
        (void)modellFlaecheAufSchirm(teil("torso"), *ort);
        t.malen.radiusPx = 0.0F;
        const ModellAnsicht& a = t.ansichten[t.aktiv];
        const int vorher = t.malen.tupfer;
        const auto start = std::chrono::steady_clock::now();
        (void)modellMalenAnsicht(a, *ort, true);
        (void)modellMalenAnsicht(a, ImVec2{ort->x + 6.0F, ort->y}, false);
        modellMalenEnde();
        *ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
        *tupfer = t.malen.tupfer - vorher;
    }));
    add(pruefe("FIX612B P2-8: ein Pinselradius von 0 setzt nicht tausende Tupfer auf 6 Bildpunkte Weg", [=] {
        char z[200];
        std::snprintf(z, sizeof z, "P2-8 - Radius im Zustand %.2f, %d Tupfer auf 6 Bildpunkten, %.1f ms", static_cast<double>(mtab().malen.radiusPx), *tupfer, *ms);
        melde(z);
        // ab Radius 1 und dem kleinsten Abstand des Reglers (1 %) liegen auf 6 Bildpunkten hoechstens 600 Tupferstellen
        return *tupfer <= 601;
    }));
    add(tu("FIX612B P2-8: zurueck", [] {
        ModellTab& t = mtab();
        int n = 0;
        while (modellRueckgaengig() && n < 10) { ++n; }
        t.malen.radiusPx = 24.0F;
        t.auswahl.clear();
        t.bereich = ModellBereich::Weights;
    }));
    add(pause(0.3));
}

// =====================================================================================================================
// V1: Auto-Skin auf einem kleinen Teil mit vollem Skelett
void v1(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    grund(s, "V1");
    for (const int aufl : {256, 512}) {
        add(pruefe("FIX612B V1: Auto-Skin auf dem Kopf allein (volles Skelett, die meisten Knochen weit ausserhalb des Gitters), Aufloesung " + std::to_string(aufl) +
                       " - nicht Sekunden bis Minuten laenger als die ganze Figur (hoechstens doppelt so lang)",
                   [aufl] {
            const modell::Inhalt& in = mtab().dok.inhalt();
            modell::AutoSkinOptionen o;
            o.aufloesung = aufl;
            o.ziel = static_cast<modell::Zielspiel>(mtab().zielspiel);
            const auto lauf = [&](modell::Inhalt x, double& ms, std::string& stand) {
                modell::AutoSkinBericht b;
                const auto start = std::chrono::steady_clock::now();
                const bool ok = modell::autoSkin(x, g_app->anim, o, b, nullptr);
                ms = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - start).count();
                stand = gewichtStand(x);
                return ok;
            };
            double msGanz = 0.0;
            double msKopf = 0.0;
            std::string standGanz;
            std::string standKopf;
            const bool okGanz = lauf(in, msGanz, standGanz);
            // der Kopf allein: alle anderen Flaechen ohne Netz (sie zaehlen dann nicht zum Gitter)
            modell::Inhalt kopf = in;
            const int head = kopf.teilIndex("head");
            for (std::size_t t = 0; t < kopf.teile.size(); ++t) {
                if (static_cast<int>(t) != head && !kopf.teile[t].istTag()) { kopf.teile[t].stufen.clear(); }
            }
            const bool okKopf = lauf(kopf, msKopf, standKopf);
            std::uint64_t h = 1469598103934665603ULL;
            for (const char c : standKopf) { h = (h ^ static_cast<unsigned char>(c)) * 1099511628211ULL; }
            char z[240];
            std::snprintf(z, sizeof z, "V1 - Aufloesung %d: ganze Figur %.0f ms (ok %d), Kopf allein %.0f ms (ok %d), Ergebnis Kopf FNV %016llx", aufl, msGanz, okGanz ? 1 : 0, msKopf,
                          okKopf ? 1 : 0, static_cast<unsigned long long>(h));
            melde(z);
            return okGanz && head >= 0 && msKopf <= 2.0 * msGanz;
        }));
    }
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "fix612b" && n != "fix612babsturz") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto bedienung = std::make_shared<int>(0);
    add(tu("FIX612B: Fenster, Skript fuer den Test", [=] {
        *bedienung = g_app->settings.modellBedienung;
        (void)platform::hauptfensterGroesse(2560, 1440);
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "fix612b.txt");
    }));
    add(pause(0.8));
    if (n == "fix612babsturz") {
        p23absturz(s);
    } else {
        if (abschnitt("p22")) { p22(s); }
        if (abschnitt("p23")) { p23(s); }
        if (abschnitt("p29")) { p29(s); }
        if (abschnitt("p24")) { p24(s); }
        if (abschnitt("p21")) { p21(s); }
        if (abschnitt("p210")) { p210(s); }
        if (abschnitt("p25")) { p25(s); }
        if (abschnitt("p26")) { p26(s); }
        if (abschnitt("p27")) { p27(s); }
        if (abschnitt("p28")) { p28(s); }
        if (abschnitt("v1")) { v1(s); }
    }
    add(tu("FIX612B: Modell-Tab zurueck", [=] {
        ModellTab& t = mtab();
        int z = 0;
        while (modellRueckgaengig() && z < 100) { ++z; }
        gw() = ModellGewichte{};
        modellAutoSkinZustand() = ModellAutoSkin{};
        modellBedienungSetzen(*bedienung);
        t.eckenWahl.clear();
        t.auswahl.clear();
        t.lodWahl = 0;
        t.speicherPfad.clear();
        t.bereich = ModellBereich::Ansehen;
        modellGewichteBereichSchritt();
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace fix612b
