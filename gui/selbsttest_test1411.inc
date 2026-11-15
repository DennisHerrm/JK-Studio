// selbsttest_test1411.inc - TEST1411 (14.11.): die Testrueckmeldungen von shank zu rc612, je Punkt erst rot im laufenden Programm,
// dann gruen (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=test1411; BHED_T1411=<d,i,f,...> nur diese Punkte, Vorgabe alle)
//
//   D  Modell doppelt beim Zoomen: das Bild der Ansicht gegen ein frisch gezeichnetes Bild derselben Kamera (Bildpunkte des
//      Umrisses) und gegen die Ueberlagerung (Ecken der Haut auf dem Schirm), in drei Bereichen, fuenf Ansichtsarten, nach
//      Rad-Rasten heraus und herein; dazu das Drahtgitter ab Werk im Bereich Modellieren (Objektebene ohne Kanten) und die
//      Geister (Ghosts) nur im Bereich Animation
//   I  Bereich Animation ohne Control Rig: ein Klick auf die Figur waehlt einen Knochen, keine Meldung "eingefroren"
//   H  Knochen als saubere Kette je Glied: je Knochen eine Form, kein Kasten schneidet einen anderen ausser an den Gelenken, die
//      Kaesten reichen bis zum Gelenk der Kette, Drehhilfen als Punkt, Klick trifft, GLA unberuehrt (_humanoid von Movie Duels, rancor)
//   G  Zeitleiste und Statuszeile wie 3ds Max in allen Bereichen (kein Text unter der Leiste, Zeitschieber oben, Knoepfe in der einen
//      Statuszeile), keine Fusszeile im Explorer; Scrubben, Klick in die Spurleiste, Bildfeld, , und ., Schluessel waehlen/ziehen/
//      kopieren/Menue, Mini Curve Editor; wie Blender ohne Text unter der Leiste
//   J  Rechtsklick in die Ansicht: wie 3ds Max das Quad-Menue (Rotate setzt das Werkzeug), wie Blender Transform im Kontextmenue
//   L  ViewCube: ein Klick an der Stelle eines Kompassbuchstabens hinter dem Wuerfel trifft die Flaeche, Uebergang ohne Nachschwingen
//   M  Kontrast der Beschriftung ueber der Ansicht (WCAG >= 4,5:1) auf dunklem, mittlerem, weissem Grund und in der Silhouette
//   K  Fussplattform wie CAT (Stil Standard): Klick auf die Platte waehlt den IK-Fuss, heben hebt den Fuss und beugt das Knie, Key;
//      Abrollen um die Zehe hebt die Ferse
//   C  Load .gla... oeffnet die Liste der .gla aus den pk3 (wie Other GLA...), Klick laedt; Open file... fuer eine lose .gla
//   fotos  Fotos fuer D, I, H (BHED_FOTOS)
//   N  Motion-Feld mit Setup-/Animationsmodus wie CAT: Gruppen je Modus, Setup-Pose, kein Schluessel im Setup-Modus, Tasten, Blender
//   O  Painting: ein Strich ueber Rumpf und Gesicht malt in beide Bilder, Strg+Z nimmt beide, Ziel folgt (Explorer, Alt+Klick, Vorschaubilder), Nur Ziel, Bauen
//   P  Weights wie Skin: Ecke klicken, Liste des Weight Tools, Wert tippen, Mirror Paste, Copy/Paste, Weight Table, Backface Cull, Format des Spiels, Blender
//   A  Load mission ohne pk3-Dialog: Liste des Spiels nach Archiv, Filter auf Archiv, Laden -> zuletzt geoeffnet, Ordner/Archiv oeffnen, nichts eingerichtet
//   E  Sequenzen als eigener Reiter: Baum ohne Ast, Strg+F + Suche, Gruppe, Sortieren, Klick/Rechtsklick, Zeigen
//   B  Model-Tab ohne Modell wie mit Modell: je Bereich und Bedienmodus dieselben Rahmen, Reiter und Gruppen (zu, gesperrt), Ansicht mit
//      ViewCube/Kopfzeile und Ladehinweis (NPC-Browser, .glm oeffnen), Explorer mit Reitern/Suche/Filter und Hinweis im leeren Baum
//   F  Zeitleiste: die Knoepfe rechts der Bildanzeige stehen still, wenn die Bildzahl waechst (0, 9, 10, 99, 100, letztes Bild der
//      laengsten Sequenz - unter Movie Duels BOTH_MD_CIN_51 mit 660 Bildern)
//   s  TEST1411e: Load mission - waehrend die Suche im Hintergrund Karten bringt, bleibt die Zeile unter der Maus dieselbe (auch am Ende der
//      Suche, solange die Maus ueber der Liste steht); verlaesst sie die Liste, ordnet sie sich (Namen, Gruppen)
//   w  TEST1411e: Weights-Mirror - ein Knochenpaar von Hand ueber die Klappliste der Tabelle (beide verlassen ihre alten Paare, Befund), Mirror
//      Paste folgt dem Paar von Hand, ein neues Paar aus zwei beliebigen Knochen, "Paare nach den Namen" stellt die Namen wieder her
//   x  TEST1411e: ohne Auswahl nennt die Statuszeile (wie 3ds Max) X/Y/Z des Mauspunkts auf dem Raster der Ansicht darunter (eine Ansicht
//      oben/vorne/links/Perspektive, vier Ansichten), ausserhalb der Ansichten bleibt der letzte Punkt, mit Auswahl deren Mitte
// Eingebunden in selbsttest.cpp (am Ende der Modell-Block-Gruppen).
namespace t1411 {

ModellTab& mtab() { return g_app->mtab; }
std::size_t uz(int i) { return static_cast<std::size_t>(i); }

void melde(const std::string& z) { diag::detail("Knopftest: T1411 " + z); }

bool abschnitt(const char* name) {
    const char* e = std::getenv("BHED_T1411");
    if (e == nullptr || *e == '\0') { return true; }
    const std::string liste = std::string(",") + e + ",";
    return liste.find(std::string(",") + name + ",") != std::string::npos;
}

int teil(const char* name) { return mtab().dok.inhalt().teilIndex(name); }

int sequenz(const char* name) {
    for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
        if (g_app->animList[i].name == name) { return static_cast<int>(i); }
    }
    return -1;
}

// die laengste Sequenz der animation.cfg (fuer F: Bildzahlen bis 1000)
int laengsteSequenz() {
    int best = -1;
    for (std::size_t i = 0; i < g_app->animList.size(); ++i) {
        if (best < 0 || g_app->animList[i].numFrames > g_app->animList[uz(best)].numFrames) { best = static_cast<int>(i); }
    }
    return best;
}

const char* bereichName(ModellBereich b) {
    switch (b) {
        case ModellBereich::Ansehen: return "View/Check";
        case ModellBereich::Modellieren: return "Modeling";
        case ModellBereich::Uv: return "UV";
        case ModellBereich::Weights: return "Weights";
        case ModellBereich::Painting: return "Painting";
        case ModellBereich::Animation: return "Animation";
    }
    return "?";
}

const char* artName(AnsichtArt a) {
    switch (a) {
        case AnsichtArt::Oben: return "Top";
        case AnsichtArt::Vorne: return "Front";
        case AnsichtArt::Links: return "Left";
        case AnsichtArt::Perspektive: return "Perspective";
        case AnsichtArt::Orthographisch: return "Orthographic";
        default: break;
    }
    return "other";
}

// Ein Schirmpunkt AUF der Flaeche: die Mitte ihres groessten Dreiecks auf dem Schirm (die Mitte aller Ecken kann in einem Loch liegen -
// hips ist ein Ring um die Huefte)
ImVec2 flaechenPunkt(const char* name) {
    const int i = teil(name);
    const auto& h = modellHaut();
    const GlmModel& m = mtab().anzeige;
    ImVec2 best{0.0F, 0.0F};
    float bestF = -1.0F;
    if (i < 0 || uz(i) >= h.size() || uz(i) >= m.surfaces.size()) { return best; }
    const GlmSurface& sf = m.surfaces[uz(i)];
    const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
    for (std::size_t k = 0; k + 2 < sf.indexes.size(); k += 3) {
        ImVec2 p[3];
        bool ok = true;
        for (std::size_t e = 0; e < 3 && ok; ++e) { ok = modellAufSchirm(a, h[uz(i)][sf.indexes[k + e]].data(), p[e]); }
        if (!ok) { continue; }
        const float f = std::fabs((p[1].x - p[0].x) * (p[2].y - p[0].y) - (p[2].x - p[0].x) * (p[1].y - p[0].y));
        if (f > bestF) {
            bestF = f;
            best = ImVec2{std::floor((p[0].x + p[1].x + p[2].x) / 3.0F), std::floor((p[0].y + p[1].y + p[2].y) / 3.0F)};
        }
    }
    return best;
}

ImVec2 ansichtMitte() {
    const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
    return ImVec2{std::floor(a.lo[0] + a.groesse[0] * 0.5F), std::floor(a.lo[1] + a.groesse[1] * 0.5F)};
}

// --- D: Umriss in Bildpunkten -------------------------------------------------------------------------------------------

struct Rahmen {
    float x0 = 1e9F;
    float y0 = 1e9F;
    float x1 = -1e9F;
    float y1 = -1e9F;
    [[nodiscard]] bool da() const { return x1 >= x0 && y1 >= y0; }
    void dazu(float x, float y) {
        x0 = std::min(x0, x);
        y0 = std::min(y0, y);
        x1 = std::max(x1, x);
        y1 = std::max(y1, y);
    }
};

// Der Umriss der Figur im Bild: alle Bildpunkte, die sich vom Grund (Bildpunkt links oben; ohne Raster und Boden einfarbig)
// unterscheiden. Die Kante eines Bildpunkts zaehlt mit (x1 = letzter Bildpunkt + 1), wie die Ecken der Ueberlagerung.
Rahmen bildRahmen(const std::vector<unsigned char>& rgba, int w, int h) {
    Rahmen r;
    if (w <= 0 || h <= 0 || rgba.size() < static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4U) { return r; }
    const unsigned char* g = rgba.data();
    for (int y = 0; y < h; ++y) {
        const unsigned char* z = rgba.data() + static_cast<std::size_t>(y) * static_cast<std::size_t>(w) * 4U;
        for (int x = 0; x < w; ++x) {
            const int d = std::abs(z[x * 4] - g[0]) + std::abs(z[x * 4 + 1] - g[1]) + std::abs(z[x * 4 + 2] - g[2]);
            if (d > 24) {
                r.dazu(static_cast<float>(x), static_cast<float>(y));
                r.dazu(static_cast<float>(x + 1), static_cast<float>(y + 1));
            }
        }
    }
    return r;
}

// Der Umriss der Haut, wie die Ueberlagerung (Drahtgitter, Auswahl, Kanten) sie auf den Schirm rechnet - in Bildpunkten der Ansicht.
Rahmen hautRahmen() {
    Rahmen r;
    const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
    const auto& h = modellHaut();
    const GlmModel& m = mtab().anzeige;
    for (std::size_t i = 0; i < h.size() && i < m.surfaces.size(); ++i) {
        if (m.surfaces[i].isTag() || !modellTeilGezeichnet(static_cast<int>(i))) { continue; }
        for (const auto& p : h[i]) {
            ImVec2 s;
            if (modellAufSchirm(a, p.data(), s)) { r.dazu(s.x - a.lo[0], s.y - a.lo[1]); }
        }
    }
    return r;
}

float rahmenAbstand(const Rahmen& a, const Rahmen& b) {
    if (!a.da() || !b.da()) { return 1e9F; }
    return std::max({std::fabs(a.x0 - b.x0), std::fabs(a.y0 - b.y0), std::fabs(a.x1 - b.x1), std::fabs(a.y1 - b.y1)});
}

std::string rahmenText(const Rahmen& r) {
    char z[96];
    std::snprintf(z, sizeof z, "%.0f,%.0f-%.0f,%.0f", static_cast<double>(r.x0), static_cast<double>(r.y0), static_cast<double>(r.x1),
                  static_cast<double>(r.y1));
    return z;
}

struct DFall {
    ModellBereich bereich = ModellBereich::Ansehen;
    AnsichtArt art = AnsichtArt::Perspektive;
    float bildFrisch = 0.0F;   // groesster Abstand Bild der Ansicht <-> frisch gezeichnet (px)
    float gitterBild = 0.0F;   // groesster Abstand Ueberlagerung <-> gezeigtes Bild (px)
    int messungen = 0;
};

// Eine Messung: das Bild, das die Ansicht zeigt, gegen ein frisch gezeichnetes Bild derselben Kamera und gegen die Ecken der Haut.
void dMessen(DFall& f, const char* wann) {
    ModellTab& t = mtab();
    const ModellAnsicht& a = t.ansichten[t.aktiv];
    std::vector<unsigned char> gezeigt;
    int gw = 0;
    int gh = 0;
    std::vector<unsigned char> frisch;
    int fw = 0;
    int fh = 0;
    const bool ok1 = modellAnsichtPixel(t.aktiv, gezeigt, gw, gh);
    const Rahmen haut = hautRahmen();
    const bool ok2 = modellFotoMitUeberlagerung(static_cast<int>(a.groesse[0]), frisch, fw, fh);
    const Rahmen rg = ok1 ? bildRahmen(gezeigt, gw, gh) : Rahmen{};
    const Rahmen rf = ok2 ? bildRahmen(frisch, fw, fh) : Rahmen{};
    // die Haut nur, soweit sie in der Ansicht liegt (das Bild endet am Rand: die Fuesse unter dem Rand zaehlen nicht)
    Rahmen hautIn = haut;
    hautIn.x0 = std::clamp(hautIn.x0, 0.0F, static_cast<float>(gw));
    hautIn.x1 = std::clamp(hautIn.x1, 0.0F, static_cast<float>(gw));
    hautIn.y0 = std::clamp(hautIn.y0, 0.0F, static_cast<float>(gh));
    hautIn.y1 = std::clamp(hautIn.y1, 0.0F, static_cast<float>(gh));
    const float d1 = rahmenAbstand(rg, rf);
    const float d2 = rahmenAbstand(hautIn, rg);   // Ueberlagerung gegen das Bild, das die Ansicht ZEIGT
    f.bildFrisch = std::max(f.bildFrisch, d1);
    f.gitterBild = std::max(f.gitterBild, d2);
    ++f.messungen;
    char z[400];
    std::snprintf(z, sizeof z, "D %s/%s %s: Ansicht %dx%d, gezeigt %s, frisch %s (%dx%d), Haut %s - Bild<->frisch %.1f px, Gitter<->Flaeche %.1f px",
                  bereichName(f.bereich), artName(f.art), wann, gw, gh, rahmenText(rg).c_str(), rahmenText(rf).c_str(), fw, fh, rahmenText(haut).c_str(),
                  static_cast<double>(d1), static_cast<double>(d2));
    melde(z);
}

// Rad-Rasten in der Mitte der Ansicht (je Bild eine), dann einige Bilder Ruhe, dann messen
Schritt dZoom(std::shared_ptr<DFall> f, int rasten, const char* wann) {
    return {std::string("T1411 D: ") + std::to_string(rasten) + " Rad-Rasten, dann messen", [=](int b) {
                const int n = std::abs(rasten);
                if (b == 0) {
                    setzeMaus(ansichtMitte());
                    return false;
                }
                if (b <= n) {
                    ImGui::GetIO().AddMouseWheelEvent(0.0F, rasten < 0 ? -1.0F : 1.0F);
                    return false;
                }
                if (b < n + 6) { return false; }
                dMessen(*f, wann);
                return true;
            }};
}

void dSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto faelle = std::make_shared<std::vector<std::shared_ptr<DFall>>>();
    // einfarbiger Grund (ab Werk -1: der Verlauf, dessen untere Zeilen sich vom Bildpunkt links oben unterscheiden)
    auto hgVorher = std::make_shared<int>(-1);
    add(tu("T1411 D: einfarbiger Hintergrund 0x303030", [hgVorher] {
        *hgVorher = g_app->settings.modellHintergrund;
        g_app->settings.modellHintergrund = 0x303030;
    }));
    const ModellBereich bereiche[3] = {ModellBereich::Ansehen, ModellBereich::Modellieren, ModellBereich::Animation};
    const AnsichtArt arten[5] = {AnsichtArt::Perspektive, AnsichtArt::Vorne, AnsichtArt::Oben, AnsichtArt::Links, AnsichtArt::Orthographisch};
    for (const ModellBereich ber : bereiche) {
        for (const AnsichtArt art : arten) {
            auto f = std::make_shared<DFall>();
            f->bereich = ber;
            f->art = art;
            faelle->push_back(f);
            add(tu(std::string("T1411 D: ") + bereichName(ber) + ", " + artName(art) + " - weisse Texturen, ohne Raster und Boden", [f] {
                ModellTab& t = mtab();
                t.bereich = f->bereich;
                t.vierAnsichten = false;
                t.aktiv = 3;
                t.auswahlArt = AuswahlArt::Objekt;
                t.an.boden = false;
                modellPr().overlaysAus[3] = true;
                ModellAnsicht& a = t.ansichten[3];
                a.sch.weiss = true;
                a.art = f->art;
                if (f->bereich == ModellBereich::Animation) {
                    const int q = sequenz("SBD_WALK_NORMAL");
                    if (q >= 0) {
                        animWaehlen(q, false);
                        g_app->animFrame = g_app->animList[uz(q)].firstFrame + std::min(7, std::max(g_app->animList[uz(q)].numFrames - 1, 0));
                    }
                }
                modellAnsichtEinpassenAktiv();
                g_app->modelDirty = true;
            }));
            add(pause(0.5));
            add(tu("T1411 D: Messung vor dem Zoomen", [f] { dMessen(*f, "eingepasst"); }));
            add(dZoom(f, -3, "3 Rasten heraus"));
            add(dZoom(f, -3, "6 Rasten heraus"));
            add(dZoom(f, 5, "5 Rasten herein"));
        }
    }
    add(pruefe("T1411 D: in allen Bereichen und Ansichten zeigt die Ansicht nach dem Zoomen das Bild ihrer Kamera (Bild <-> frisch hoechstens 1 px)", [faelle] {
        bool ok = true;
        for (const auto& f : *faelle) {
            if (f->messungen < 4 || f->bildFrisch > 1.0F) {
                ok = false;
                char z[200];
                std::snprintf(z, sizeof z, "D %s/%s: Bild <-> frisch bis %.1f px (%d Messungen)", bereichName(f->bereich), artName(f->art), static_cast<double>(f->bildFrisch),
                              f->messungen);
                melde(z);
            }
        }
        return ok;
    }));
    add(pruefe("T1411 D: Drahtgitter, Auswahl und Kanten decken die Flaeche auf jeder Zoomstufe (Ueberlagerung <-> Bild hoechstens 2 px)", [faelle] {
        bool ok = true;
        for (const auto& f : *faelle) {
            if (f->messungen < 4 || f->gitterBild > 2.0F) {
                ok = false;
                char z[200];
                std::snprintf(z, sizeof z, "D %s/%s: Ueberlagerung <-> Bild bis %.1f px", bereichName(f->bereich), artName(f->art), static_cast<double>(f->gitterBild));
                melde(z);
            }
        }
        return ok;
    }));
    add(tu("T1411 D: Ansicht zurueck (Texturen, Overlays, Hintergrund)", [hgVorher] {
        g_app->settings.modellHintergrund = *hgVorher;
        ModellTab& t = mtab();
        t.ansichten[3].sch.weiss = false;
        t.ansichten[3].art = AnsichtArt::Perspektive;
        modellPr().overlaysAus[3] = false;
        t.bereich = ModellBereich::Ansehen;
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    // D2: das Drahtgitter ab Werk im Bereich Modellieren
    auto kanten = std::make_shared<std::array<int, 4>>();
    const int arten2[4] = {0, 1, 0, 1};   // Bedienmodus: wie 3ds Max, wie Blender
    const AuswahlArt ebenen[4] = {AuswahlArt::Objekt, AuswahlArt::Objekt, AuswahlArt::Kante, AuswahlArt::Kante};
    for (int k = 0; k < 4; ++k) {
        add(tu(std::string("T1411 D2: Bereich Modellieren, ") + (arten2[k] == 0 ? "wie 3ds Max" : "wie Blender") + (k < 2 ? ", Objektebene" : ", Kanten"),
               [k, arten2, ebenen] {
                   modellBedienungSetzen(arten2[k]);
                   mtab().bereich = ModellBereich::Modellieren;
                   mtab().auswahlArt = ebenen[k];
                   g_app->modelDirty = true;
               }));
        add(pause(0.3));
        add(tu("T1411 D2: Kanten der Ueberlagerung zaehlen", [kanten, k] {
            (*kanten)[uz(k)] = mtab().mesh.kantenGezeichnet;
            melde("D2 Fall " + std::to_string(k) + ": " + std::to_string((*kanten)[uz(k)]) + " Kanten gezeichnet");
        }));
    }
    add(pruefe("T1411 D2: wie 3ds Max und wie Blender zeigt die Objektebene das Modell schattiert ohne Gitter (0 Kanten)",
               [kanten] { return (*kanten)[0] == 0 && (*kanten)[1] == 0; }));
    add(pruefe("T1411 D2: auf der Kanten-Ebene stehen die Kanten da (beide Bedienmodi)", [kanten] { return (*kanten)[2] > 0 && (*kanten)[3] > 0; }));
    add(tu("T1411 D2: zurueck", [] {
        modellBedienungSetzen(0);
        mtab().auswahlArt = AuswahlArt::Objekt;
        mtab().bereich = ModellBereich::Ansehen;
    }));
    // D3 (Foto 27, Painting): die Geister aus Animation -> Motion -> Ghosts erscheinen nur im Bereich Animation
    auto geister = std::make_shared<std::array<int, kModellBereiche>>();
    add(tu("T1411 D3: Ghosts an (3 vor, 3 nach), SBD_WALK_NORMAL Bild 7", [] {
        mtab().geister.an = true;
        mtab().geister.davor = 3;
        mtab().geister.danach = 3;
        const int q = sequenz("SBD_WALK_NORMAL");
        if (q >= 0) {
            animWaehlen(q, false);
            g_app->animFrame = g_app->animList[uz(q)].firstFrame + std::min(7, std::max(g_app->animList[uz(q)].numFrames - 1, 0));
        }
    }));
    for (int b = 0; b < kModellBereiche; ++b) {
        add(tu(std::string("T1411 D3: Bereich ") + bereichName(static_cast<ModellBereich>(b)), [b] {
            mtab().bereich = static_cast<ModellBereich>(b);
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        add(tu("T1411 D3: gezeichnete Geister", [geister, b] {
            (*geister)[uz(b)] = mtab().geisterGezeichnet;
            melde(std::string("D3 ") + bereichName(static_cast<ModellBereich>(b)) + ": " + std::to_string((*geister)[uz(b)]) + " Geister");
        }));
    }
    add(pruefe("T1411 D3: Geister stehen nur im Bereich Animation (dort 6), nie in View/Check, Modeling, UV, Weights und Painting", [geister] {
        for (int b = 0; b < kModellBereiche; ++b) {
            const bool anim = static_cast<ModellBereich>(b) == ModellBereich::Animation;
            if (anim ? (*geister)[uz(b)] != 6 : (*geister)[uz(b)] != 0) { return false; }
        }
        return true;
    }));
    add(pruefe("T1411 D3: Ghosts ab Werk aus", [] { return !ModellGeisterEinst{}.an; }));
    add(tu("T1411 D3: Ghosts aus", [] {
        mtab().geister.an = false;
        mtab().bereich = ModellBereich::Ansehen;
        g_app->modelDirty = true;
    }));
}

// --- I: Knochen im Bereich Animation ohne Control Rig -------------------------------------------------------------------

struct IStand {
    int knochen = -1;
    std::string meldung;
    std::vector<int> auswahl;
};

// Winkel zwischen den Drehungen zweier Weltmatrizen (Grad), die Spalten normiert
float drehWinkel(const BoneMatrix& a, const BoneMatrix& b) {
    float spur = 0.0F;
    for (int i = 0; i < 3; ++i) {
        float la = 0.0F;
        float lb = 0.0F;
        float ab = 0.0F;
        for (int k = 0; k < 3; ++k) {
            la += a.m[k][i] * a.m[k][i];
            lb += b.m[k][i] * b.m[k][i];
            ab += a.m[k][i] * b.m[k][i];
        }
        spur += ab / std::sqrt(std::max(la * lb, 1e-12F));
    }
    const float c = std::clamp((spur - 1.0F) * 0.5F, -1.0F, 1.0F);
    return std::acos(c) * 180.0F / 3.14159265F;
}

struct IDreh {
    int knochen = -1;
    int bild = -1;
    std::string sequenz;
    BoneMatrix vorher;
    float gedreht = -1.0F;
    bool schluessel = false;
    float nachSpielen = -1.0F;
    bool knochenAn = false;
    int formen = 0;
};

void iSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<std::array<IStand, 2>>();
    auto dr = std::make_shared<IDreh>();
    const char* const ziele[2] = {"torso", "hips"};
    for (int k = 0; k < 2; ++k) {
        add(tu(std::string("T1411 I: View/Check, wie 3ds Max, kein Rig - Klick auf ") + ziele[k], [] {
            modellBedienungSetzen(0);
            mtab().bereich = ModellBereich::Ansehen;
        }));
        add(pause(0.2));
        add(tu("T1411 I: in View/Check die Knochen aus", [] { g_app->modellKnochen = false; }));
        add(pause(0.1));
        add(tu("T1411 I: Bereich Animation, SBD_WALK_NORMAL Bild 0", [] {
            ModellTab& t = mtab();
            t.bereich = ModellBereich::Animation;
            t.vierAnsichten = false;
            t.aktiv = 3;
            t.auswahlArt = AuswahlArt::Objekt;
            t.rig = modell::Rig{};
            t.rigFuer.clear();
            t.griffWahl = -1;
            t.knochenWahl = -1;
            t.auswahl.clear();
            t.meldung.clear();
            t.gizmo = 0;
            t.ansichten[3].art = AnsichtArt::Perspektive;
            const int q = sequenz("SBD_WALK_NORMAL");
            if (q >= 0) {
                animWaehlen(q, false);
                g_app->animFrame = g_app->animList[uz(q)].firstFrame;
            }
            modellAnsichtEinpassenAktiv();
            g_app->modelDirty = true;
        }));
        add(pause(0.5));
        if (k == 0) {
            add(tu("T1411 I: Knochen sichtbar?", [dr] {
                dr->knochenAn = g_app->modellKnochen;
                dr->formen = modellKdFormZahl();
                melde("I Knochen im Bereich Animation: Schalter " + std::to_string(dr->knochenAn ? 1 : 0) + ", " + std::to_string(dr->formen) + " Formen");
            }));
        }
        const std::string name = ziele[k];
        add(klickAn("T1411 I: auf " + name + " (Mitte ihres groessten Dreiecks auf dem Schirm)", [name] { return flaechenPunkt(name.c_str()); }));
        add(pause(0.3));
        add(tu("T1411 I: was der Klick gewaehlt hat", [st, k, name] {
            IStand& i = (*st)[uz(k)];
            i.knochen = mtab().knochenWahl;
            i.meldung = mtab().meldung;
            i.auswahl = mtab().auswahl;
            std::string fl;
            for (const int f : i.auswahl) {
                if (f >= 0 && uz(f) < mtab().dok.inhalt().teile.size()) { fl += " " + mtab().dok.inhalt().teile[uz(f)].name; }
            }
            melde("I Klick auf " + name + ": Knochen " + std::to_string(i.knochen) +
                  (i.knochen >= 0 && uz(i.knochen) < g_app->anim.bones.size() ? " (" + g_app->anim.bones[uz(i.knochen)].name + ")" : std::string()) + ", Flaechen" +
                  (fl.empty() ? std::string(" -") : fl) + ", Meldung '" + i.meldung + "'");
        }));
    }
    add(pruefe("T1411 I: im Bereich Animation sind die Knochen ab Werk sichtbar (auch wenn sie in View/Check aus sind)", [dr] { return dr->knochenAn && dr->formen > 0; }));
    add(pruefe("T1411 I: ohne Control Rig waehlt ein Klick auf die Figur einen Knochen (torso und hips)", [st] { return (*st)[0].knochen >= 0 && (*st)[1].knochen >= 0; }));
    add(pruefe("T1411 I: keine Meldung 'eingefroren' beim Klick auf die Figur", [st] {
        const std::string frost = tr(Str::MbXEingefrorenMeldung);
        const std::string kern = frost.substr(frost.find("%s") != std::string::npos ? frost.find("%s") + 2 : 0, 12);
        return (*st)[0].meldung.find(kern) == std::string::npos && (*st)[1].meldung.find(kern) == std::string::npos;
    }));
    // drehen (E und der Zug, den der Gizmo schickt), Set Key (K), abspielen
    add(tu("T1411 I: E (Drehen), den gewaehlten Knochen um 30 Grad um Z drehen", [dr] {
        ModellTab& t = mtab();
        dr->knochen = t.knochenWahl;
        dr->bild = g_app->animFrame;
        const AnimEntry* e = g_app->animIndex >= 0 && uz(g_app->animIndex) < g_app->animList.size() ? &g_app->animList[uz(g_app->animIndex)] : nullptr;
        dr->sequenz = e != nullptr ? e->name : std::string();
        if (dr->knochen < 0 || uz(dr->knochen) >= g_app->modellWelt.size()) { return; }
        dr->vorher = g_app->modellWelt[uz(dr->knochen)];
        t.gizmo = 2;
        const float wert[3] = {0.0F, 0.0F, 30.0F};
        modellRigZiehen(2, wert);
        modellRigZiehenEnde();
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("T1411 I: Drehung gemessen", [dr] {
        if (dr->knochen < 0 || uz(dr->knochen) >= g_app->modellWelt.size()) { return; }
        dr->gedreht = drehWinkel(dr->vorher, g_app->modellWelt[uz(dr->knochen)]);
        melde("I gedreht: " + std::to_string(dr->gedreht) + " Grad");
    }));
    add(taste(ImGuiKey_K, "K (Set Key)"));
    add(pause(0.3));
    add(tu("T1411 I: Schluessel da?", [dr] {
        const auto& sch = mtab().dok.inhalt().rig.schichten;
        const auto it = sch.find(dr->sequenz);
        dr->schluessel = it != sch.end() && it->second.hatSchluessel(dr->knochen, dr->bild);
        melde("I Schluessel am Bild " + std::to_string(dr->bild) + ": " + std::to_string(dr->schluessel ? 1 : 0));
    }));
    add(tu("T1411 I: abspielen", [] { g_app->animPlaying = true; }));
    add(pause(0.6));
    add(tu("T1411 I: anhalten, Bild des Schluessels", [dr] {
        g_app->animPlaying = false;
        g_app->animFrame = dr->bild;
        g_app->animTime = 0.0F;   // wie bildSetzen der Zeitleiste: genau das Bild, nicht zwischen ihm und dem naechsten
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("T1411 I: die Drehung gegen das Bild der GLA", [dr] {
        if (dr->knochen < 0 || uz(dr->knochen) >= g_app->modellWelt.size()) { return; }
        std::vector<BoneMatrix> gla;
        g_app->anim.worldMatrices(dr->bild, gla);
        dr->nachSpielen = uz(dr->knochen) < gla.size() ? drehWinkel(gla[uz(dr->knochen)], g_app->modellWelt[uz(dr->knochen)]) : -1.0F;
        melde("I nach dem Abspielen: " + std::to_string(dr->nachSpielen) + " Grad gegen die GLA");
    }));
    add(pruefe("T1411 I: E dreht den Knochen ohne Rig (30 Grad)", [dr] { return std::fabs(dr->gedreht - 30.0F) < 0.5F; }));
    add(pruefe("T1411 I: K setzt einen Schluessel auf dem Knochen", [dr] { return dr->schluessel; }));
    add(pruefe("T1411 I: nach dem Abspielen zeigt das Bild die Drehung (30 Grad gegen die GLA)", [dr] { return std::fabs(dr->nachSpielen - 30.0F) < 0.5F; }));
    add(tu("T1411 I: zurueck", [] {
        mtab().gizmo = 0;
        mtab().knochenWahl = -1;
        mtab().bereich = ModellBereich::Ansehen;
    }));
}

// --- H: Knochen als saubere Kette je Glied --------------------------------------------------------------------------------

struct HSeg {
    int knochen = -1;
    float a[3]{};
    float b[3]{};
    float breite = 0.0F;
};

struct HStand {
    std::string modell;
    int knochen = 0;
    int mehrfach = 0;          // Knochen mit mehr als einer Form
    int kaesten = 0;
    int ueberlappt = 0;        // Paare von Kaesten, die sich ausserhalb der Gelenke schneiden
    std::string paare;
    std::vector<std::string> kette;   // Pruefungen am _humanoid: "<knochen> -> <folge>: ok|<abstand>"
    bool hilfePunkt = true;    // lradiusX, lhumerusX, lfemurX ohne Kasten
    bool glaGleich = false;
    bool klickTrifft = false;
    std::uint64_t glaVorher = 0;
};

float hAbstandPunktStrecke(const float p[3], const float a[3], const float b[3], float& u) {
    float ab[3];
    float ap[3];
    for (int k = 0; k < 3; ++k) {
        ab[k] = b[k] - a[k];
        ap[k] = p[k] - a[k];
    }
    const float l2 = ab[0] * ab[0] + ab[1] * ab[1] + ab[2] * ab[2];
    u = l2 > 0.0F ? std::clamp((ap[0] * ab[0] + ap[1] * ab[1] + ap[2] * ab[2]) / l2, 0.0F, 1.0F) : 0.0F;
    float d = 0.0F;
    for (int k = 0; k < 3; ++k) {
        const float q = a[k] + ab[k] * u - p[k];
        d += q * q;
    }
    return std::sqrt(d);
}

float hAbstand(const float a[3], const float b[3]) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

// Schneiden sich zwei Kaesten (verjuengt von voller Breite am Gelenk auf 10 % am Ende) ausserhalb ihrer Gelenke? Abgetastet von 20 %
// bis 80 % der Laenge von x; ein Punkt nahe einem Gelenk von y (Anfang oder Ende, im Abstand der beiden vollen Breiten dort) ist die
// Beruehrung an diesem Gelenk - ein Ast beginnt dort (thoracic -> lclavical) oder die Kette geht weiter (l_d2_j1 -> l_d2_j2).
bool hUeberlappt(const HSeg& x, const HSeg& y) {
    for (int i = 4; i <= 16; ++i) {
        const float t = static_cast<float>(i) / 20.0F;
        float p[3];
        for (int k = 0; k < 3; ++k) { p[k] = x.a[k] + (x.b[k] - x.a[k]) * t; }
        float u = 0.0F;
        const float d = hAbstandPunktStrecke(p, y.a, y.b, u);
        const float hx = 0.5F * x.breite * (1.0F - 0.9F * t);
        const float hy = 0.5F * y.breite * (1.0F - 0.9F * u);
        const bool amGelenk = hAbstand(p, y.a) < x.breite + y.breite || hAbstand(p, y.b) < x.breite + y.breite * 0.1F;
        if (d < hx + hy && !amGelenk) { return true; }
    }
    return false;
}

std::uint64_t hGlaSumme() {
    const GlaAnimation& a = g_app->anim;
    std::uint64_t h = 1469598103934665603ULL;
    const auto dazu = [&h](const void* p, std::size_t n) {
        const auto* c = static_cast<const unsigned char*>(p);
        for (std::size_t i = 0; i < n; ++i) { h = (h ^ c[i]) * 1099511628211ULL; }
    };
    for (const GlaBone& b : a.bones) {
        dazu(b.name.data(), b.name.size());
        dazu(&b.parent, sizeof b.parent);
        dazu(&b.basePose, sizeof b.basePose);
        dazu(&b.flags, sizeof b.flags);
    }
    if (!a.bonePool.empty()) { dazu(a.bonePool.data(), a.bonePool.size()); }
    if (!a.indexes.empty()) { dazu(a.indexes.data(), a.indexes.size() * sizeof(std::uint32_t)); }
    return h;
}

// H.1: je Knochen Elternteil, Kinder, Teilbaum, Laenge zum Elternteil, Richtung der eigenen X-Achse zu jedem Kind (cos) - aus der
// Grundstellung der GLA; dazu der Kasten, den die Ansicht zeichnet (Anfang/Ende), ins Protokoll
void hListe(const std::string& modell) {
    const GlaAnimation& a = g_app->anim;
    const std::size_t n = a.bones.size();
    std::vector<std::vector<int>> kinder(n);
    for (std::size_t i = 0; i < n; ++i) {
        const int p = a.bones[i].parent;
        if (p >= 0 && uz(p) < n && uz(p) != i) { kinder[uz(p)].push_back(static_cast<int>(i)); }
    }
    std::vector<int> teil(n, 1);
    const std::vector<int> ord = a.topologicalOrder();
    for (auto it = ord.rbegin(); it != ord.rend(); ++it) {
        const int p = a.bones[uz(*it)].parent;
        if (p >= 0 && uz(p) < n && p != *it) { teil[uz(p)] += teil[uz(*it)]; }
    }
    melde("H Liste " + modell + " (" + a.name + ", " + std::to_string(n) + " Knochen, " + std::to_string(a.numFrames) + " Bilder):");
    for (std::size_t i = 0; i < n; ++i) {
        const BoneMatrix& m = a.bones[i].basePose;
        float x[3] = {m.m[0][0], m.m[1][0], m.m[2][0]};
        const float lx = std::sqrt(x[0] * x[0] + x[1] * x[1] + x[2] * x[2]);
        for (float& v : x) { v /= std::max(lx, 1e-9F); }
        const int p = a.bones[i].parent;
        float laengeEltern = 0.0F;
        if (p >= 0 && uz(p) < n) {
            for (int k = 0; k < 3; ++k) { laengeEltern += (m.m[k][3] - a.bones[uz(p)].basePose.m[k][3]) * (m.m[k][3] - a.bones[uz(p)].basePose.m[k][3]); }
            laengeEltern = std::sqrt(laengeEltern);
        }
        std::string z;
        for (const int c : kinder[i]) {
            const BoneMatrix& cm = a.bones[uz(c)].basePose;
            float d[3];
            float l = 0.0F;
            for (int k = 0; k < 3; ++k) {
                d[k] = cm.m[k][3] - m.m[k][3];
                l += d[k] * d[k];
            }
            l = std::sqrt(l);
            const float c0 = l > 1e-6F ? (d[0] * x[0] + d[1] * x[1] + d[2] * x[2]) / l : 0.0F;
            char t[96];
            std::snprintf(t, sizeof t, " %s(%d, %.1f, cos %.2f)", a.bones[uz(c)].name.c_str(), teil[uz(c)], static_cast<double>(l), static_cast<double>(c0));
            z += t;
        }
        char k[200];
        std::snprintf(k, sizeof k, "H %2d %-18s Eltern %-14s Laenge %5.2f, %d Kinder:", static_cast<int>(i), a.bones[i].name.c_str(),
                      p >= 0 && uz(p) < n ? a.bones[uz(p)].name.c_str() : "-", static_cast<double>(laengeEltern), static_cast<int>(kinder[i].size()));
        melde(k + z);
    }
}

void hMessen(HStand& h) {
    const GlaAnimation& an = g_app->anim;
    h.knochen = static_cast<int>(an.bones.size());
    std::vector<int> formen(an.bones.size(), 0);
    std::vector<HSeg> seg;
    for (int f = 0; f < modellKdFormZahl(); ++f) {
        const KdFormInfo i = modellKdFormInfo(f);
        if (i.knochen < 0 || uz(i.knochen) >= formen.size()) { continue; }
        ++formen[uz(i.knochen)];
        HSeg x;
        x.knochen = i.knochen;
        if (modellKdFormAchse(f, x.a, x.b, x.breite)) { seg.push_back(x); }
    }
    for (const int z : formen) { h.mehrfach += z > 1 ? 1 : 0; }
    h.kaesten = static_cast<int>(seg.size());
    for (std::size_t i = 0; i < seg.size(); ++i) {
        for (std::size_t j = 0; j < seg.size(); ++j) {
            if (i != j && hUeberlappt(seg[i], seg[j])) {
                ++h.ueberlappt;
                if (h.paare.size() < 600) { h.paare += " " + an.bones[uz(seg[i].knochen)].name + "/" + an.bones[uz(seg[j].knochen)].name; }
            }
        }
    }
    // am _humanoid: das Ende der Kaesten an den Gelenken der Kette, die Drehhilfen ohne Kasten
    const auto ende = [&](const char* k, const char* folge) {
        const int b = boneIndex(an, k);
        const int c = boneIndex(an, folge);
        if (b < 0 || c < 0 || uz(c) >= g_app->modellWelt.size()) { return; }
        const BoneMatrix& w = g_app->modellWelt[uz(c)];
        std::string z = std::string(k) + " -> " + folge + ": ";
        bool da = false;
        for (const HSeg& x : seg) {
            if (x.knochen != b) { continue; }
            da = true;
            const float d = std::sqrt((x.b[0] - w.m[0][3]) * (x.b[0] - w.m[0][3]) + (x.b[1] - w.m[1][3]) * (x.b[1] - w.m[1][3]) + (x.b[2] - w.m[2][3]) * (x.b[2] - w.m[2][3]));
            z += d < 0.01F ? "ok" : "Ende " + std::to_string(d) + " vom Gelenk";
        }
        h.kette.push_back(z + (da ? "" : "kein Kasten"));
    };
    ende("lhumerus", "lradius");
    ende("lradius", "lhand");
    ende("thoracic", "cervical");
    ende("lclavical", "lhumerus");
    ende("l_d1_j1", "l_d1_j2");
    ende("lfemurYZ", "ltibia");
    for (const char* hilfe : {"lhumerusX", "lradiusX", "lfemurX"}) {
        const int b = boneIndex(an, hilfe);
        for (const HSeg& x : seg) { h.hilfePunkt = h.hilfePunkt && x.knochen != b; }
    }
    std::string k;
    for (const std::string& z : h.kette) { k += " | " + z; }
    melde("H " + h.modell + ": " + std::to_string(h.knochen) + " Knochen, " + std::to_string(h.kaesten) + " Kaesten, mehrfach " + std::to_string(h.mehrfach) +
          ", ueberlappend " + std::to_string(h.ueberlappt) + (h.paare.empty() ? std::string() : " (" + h.paare + ")") + ", Drehhilfen als Punkt " + std::to_string(h.hilfePunkt ? 1 : 0) + k);
}

void hSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const char* const modelle[2] = {"models/players/alora/model.glm", "models/players/rancor/model.glm"};
    auto st = std::make_shared<std::array<HStand, 2>>();
    for (int m = 0; m < 2; ++m) {
        if (m > 0) {
            for (Schritt& x : mb1::laden(modelle[m])) { add(std::move(x)); }
        }
        add(tu(std::string("T1411 H: ") + modelle[m] + " - Bereich Animation, wie 3ds Max, Rig-Stil Standard, Knochen an", [st, m, modelle] {
            HStand& h = (*st)[uz(m)];
            h.modell = modelle[m];
            modellBedienungSetzen(0);
            ModellTab& t = mtab();
            t.bereich = ModellBereich::Animation;
            t.vierAnsichten = false;
            t.aktiv = 3;
            t.rig = modell::Rig{};
            t.rigFuer.clear();
            t.griffWahl = -1;
            t.knochenWahl = -1;
            modellRigStilSetzen(modell::RigStil::Standard);
            g_app->animIndex = -1;
            g_app->animFrame = 0;
            t.ansichten[3].art = AnsichtArt::Vorne;
            modellAnsichtEinpassenAktiv();
            h.glaVorher = hGlaSumme();
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        add(tu("T1411 H: Knochen an, eine Ansicht", [] {
            g_app->modellKnochen = true;
            mtab().vierAnsichten = false;
            mtab().aktiv = 3;
            g_app->modelDirty = true;
        }));
        add(pause(0.4));
        add(tu("T1411 H: Liste und Kaesten", [st, m] {
            hListe((*st)[uz(m)].modell);
            hMessen((*st)[uz(m)]);
        }));
        add(foto(std::string("t1411_h_") + (m == 0 ? "alora" : "rancor") + "_vorne"));
        if (m == 0) {
            add(tu("T1411 H: Seite", [] {
                mtab().ansichten[3].art = AnsichtArt::Links;
                modellAnsichtEinpassenAktiv();
                g_app->modelDirty = true;
            }));
            add(pause(0.3));
            add(foto("t1411_h_alora_seite"));
            add(tu("T1411 H: Perspektive", [] {
                mtab().ansichten[3].art = AnsichtArt::Perspektive;
                modellAnsichtEinpassenAktiv();
                g_app->modelDirty = true;
            }));
            add(pause(0.3));
            add(foto("t1411_h_alora_perspektive"));
            // ein Klick auf die Mitte des Kastens von lhumerus waehlt lhumerus
            add(klickAn("T1411 H: auf den Kasten von lhumerus (30 % der Laenge)", [] {
                const int b = boneIndex(g_app->anim, "lhumerus");
                for (int f = 0; f < modellKdFormZahl(); ++f) {
                    HSeg x;
                    if (modellKdFormInfo(f).knochen != b || !modellKdFormAchse(f, x.a, x.b, x.breite)) { continue; }
                    // bei 30 % der Laenge: in der Mitte liegt der Hilfspunkt der Drehhilfe lhumerusX
                    const float mitte[3] = {x.a[0] + (x.b[0] - x.a[0]) * 0.3F, x.a[1] + (x.b[1] - x.a[1]) * 0.3F, x.a[2] + (x.b[2] - x.a[2]) * 0.3F};
                    ImVec2 p;
                    if (modellAufSchirm(mtab().ansichten[mtab().aktiv], mitte, p)) { return ImVec2{std::floor(p.x), std::floor(p.y)}; }
                }
                return ansichtMitte();
            }));
            add(pause(0.3));
            add(tu("T1411 H: welcher Knochen gewaehlt ist", [st] {
                const int k = mtab().knochenWahl;
                (*st)[0].klickTrifft = k >= 0 && k == boneIndex(g_app->anim, "lhumerus");
                melde("H Klick auf den Kasten von lhumerus (30 %): " + std::string(k >= 0 && uz(k) < g_app->anim.bones.size() ? g_app->anim.bones[uz(k)].name : "-"));
            }));
        }
        add(tu("T1411 H: GLA unberuehrt", [st, m] {
            HStand& h = (*st)[uz(m)];
            h.glaGleich = hGlaSumme() == h.glaVorher;
        }));
    }
    add(pruefe("T1411 H: je Knochen hoechstens eine Form (_humanoid und rancor)", [st] { return (*st)[0].knochen > 0 && (*st)[1].knochen > 0 && (*st)[0].mehrfach == 0 && (*st)[1].mehrfach == 0; }));
    add(pruefe("T1411 H: kein Knochen-Kasten schneidet einen anderen ausser an den Gelenken (_humanoid und rancor)",
               [st] { return (*st)[0].kaesten > 0 && (*st)[1].kaesten > 0 && (*st)[0].ueberlappt == 0 && (*st)[1].ueberlappt == 0; }));
    add(pruefe("T1411 H: am _humanoid reicht jeder Kasten bis zum Gelenk der Kette (lhumerus -> lradius, lradius -> lhand, lclavical -> lhumerus, l_d1_j1 -> l_d1_j2 ...)",
               [st] {
                   bool ok = !(*st)[0].kette.empty();
                   for (const std::string& z : (*st)[0].kette) { ok = ok && z.size() > 2 && z.compare(z.size() - 2, 2, "ok") == 0; }
                   return ok;
               }));
    add(pruefe("T1411 H: die Drehhilfen lhumerusX, lradiusX, lfemurX sind ab Werk ein Hilfspunkt, kein Kasten", [st] { return (*st)[0].hilfePunkt; }));
    add(pruefe("T1411 H: ein Klick auf den Kasten von lhumerus waehlt lhumerus", [st] { return (*st)[0].klickTrifft; }));
    add(pruefe("T1411 H: die GLA bleibt unberuehrt (nur Darstellung)", [st] { return (*st)[0].glaGleich && (*st)[1].glaGleich; }));
    for (Schritt& x : mb1::laden("models/players/alora/model.glm")) { add(std::move(x)); }
}

// --- Fotos (BHED_FOTOS): D, F, I wie shanks Fotos ---------------------------------------------------------------------------

void fotoSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto zoom = [](int rasten) {
        return Schritt{"T1411 Foto: Rad-Rasten", [rasten](int b) {
                           if (b == 0) {
                               setzeMaus(ansichtMitte());
                               return false;
                           }
                           if (b <= std::abs(rasten)) {
                               ImGui::GetIO().AddMouseWheelEvent(0.0F, rasten < 0 ? -1.0F : 1.0F);
                               return false;
                           }
                           return b >= std::abs(rasten) + 6;
                       }};
    };
    // D wie Foto 14.11. (Modeling, Perspective, herausgezoomt): auf der Objektebene ohne Gitter, auf der Kanten-Ebene deckungsgleich
    add(tu("T1411 Foto D: Modeling, Perspective, Objektebene", [] {
        modellBedienungSetzen(0);
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Modellieren;
        t.vierAnsichten = false;
        t.aktiv = 3;
        t.auswahlArt = AuswahlArt::Objekt;
        t.ansichten[3].art = AnsichtArt::Perspektive;
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(zoom(-6));
    add(foto("t1411_d_modeling_objekt_herausgezoomt"));
    add(tu("T1411 Foto D: Kanten-Ebene (2)", [] {
        mtab().auswahlArt = AuswahlArt::Kante;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(foto("t1411_d_modeling_kanten_herausgezoomt"));
    add(tu("T1411 Foto D: wie Blender, Edit Mode Kanten", [] {
        modellBedienungSetzen(1);
        mtab().auswahlArt = AuswahlArt::Kante;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(foto("t1411_d_modeling_blender_kanten"));
    // I/D wie Foto 14.11. (Animation, Orthographic, SBD_WALK_NORMAL Bild 7): Knochen gewaehlt, herausgezoomt
    add(tu("T1411 Foto I: Animation, wie 3ds Max, Orthographic, SBD_WALK_NORMAL Bild 7", [] {
        modellBedienungSetzen(0);
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Animation;
        t.auswahlArt = AuswahlArt::Objekt;
        t.ansichten[3].art = AnsichtArt::Orthographisch;
        const int q = sequenz("SBD_WALK_NORMAL");
        if (q >= 0) {
            animWaehlen(q, false);
            g_app->animFrame = g_app->animList[uz(q)].firstFrame + 7;
        }
        t.knochenWahl = boneIndex(g_app->anim, "lradius");
        t.griffWahl = -1;
        t.gizmo = 2;
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.4));
    add(zoom(-4));
    add(foto("t1411_i_animation_knochen_gewaehlt"));
    add(tu("T1411 Foto: zurueck", [] {
        mtab().gizmo = 0;
        mtab().knochenWahl = -1;
        mtab().bereich = ModellBereich::Ansehen;
    }));
}

// --- G: Zeitleiste und Statuszeile wie 3ds Max ----------------------------------------------------------------------------

const ModellZeitProbe& zp() { return modellZeit().probe; }

struct GLage {
    std::string bereich;
    std::string hinweis;       // der Text unter der Leiste (leer: keiner)
    int spurZeilen = 0;        // Zeilen unter der Spurleiste (Primaer/Sekundaer)
    bool schieberOben = false; // der Zeitschieber steht ueber der Spurleiste
    bool knoepfeUnten = false; // Auto Key steht in der Statuszeile unter der Spurleiste
    float statusHoehe = 0.0F;
    bool fussExplorer = false; // Fusszeile unter dem Baum
};

struct GStand {
    std::vector<GLage> lagen;
    int nachZiehen = -1;       // Bild nach dem Ziehen des Schiebers ans Ende
    int letztes = -1;
    int nachKlickSkala = -1;   // Bild nach dem Klick auf Bild 10 der Spurleiste
    int nachTippen = -1;       // Bild nach "7" + Enter im Bildfeld
    int nachPunkt = -1;        // Bild nach "." (vor) von 7
    int nachKomma = -1;        // ... dann "," (zurueck)
    int knochen = -1;
    bool keyDa = false;        // Schluessel auf Bild 3
    bool keyGewaehlt = false;  // Klick auf den Schluessel in der Spurleiste waehlt ihn
    bool keyVerschoben = false;// Ziehen 3 -> 6: Schluessel auf 6, nicht mehr auf 3
    bool keyKopiert = false;   // Umschalt-Ziehen 6 -> 9: Schluessel auf 6 und 9
    bool keyMenue = false;     // Rechtsklick auf den Schluessel: ein Menue
    bool miniKurveAuf = false; // Mini Curve Editor oeffnet den Spurbereich ueber der Spurleiste
};

float skalaX(int rel) {
    const ModellZeitProbe& p = zp();
    const int q = g_app->animIndex;
    if (q < 0 || uz(q) >= g_app->animList.size()) { return p.bildX0; }
    const int n = std::max(g_app->animList[uz(q)].numFrames, 2);
    return std::floor(p.bildX0 + (p.bildX1 - p.bildX0) * static_cast<float>(rel) / static_cast<float>(n - 1));
}

int gRel() {
    const int q = g_app->animIndex;
    return q >= 0 && uz(q) < g_app->animList.size() ? g_app->animFrame - g_app->animList[uz(q)].firstFrame : -1;
}

bool gKey(int knochen, int rel) {
    const int q = g_app->animIndex;
    if (q < 0 || uz(q) >= g_app->animList.size()) { return false; }
    const auto& sch = mtab().dok.inhalt().rig.schichten;
    const auto it = sch.find(g_app->animList[uz(q)].name);
    return it != sch.end() && it->second.hatSchluessel(knochen, g_app->animList[uz(q)].firstFrame + rel);
}

// Ziehen mit der linken Taste von a nach b (Umschalt optional), in Schritten je Bild
Schritt gZiehen(const std::string& was, std::function<ImVec2()> von, std::function<ImVec2()> nach, bool umschalt) {
    auto a = std::make_shared<ImVec2>();
    auto b = std::make_shared<ImVec2>();
    return {"T1411 G: ziehen " + was, [=](int i) {
                ImGuiIO& io = ImGui::GetIO();
                if (i == 0) {
                    *a = von();
                    *b = nach();
                    setzeMaus(*a);
                    if (umschalt) { io.AddKeyEvent(ImGuiMod_Shift, true); }
                    return false;
                }
                if (i == 1) {
                    io.AddMouseButtonEvent(0, true);
                    return false;
                }
                if (i < 12) {
                    const float t = static_cast<float>(i - 1) / 10.0F;
                    setzeMaus(ImVec2{a->x + (b->x - a->x) * t, a->y + (b->y - a->y) * t});
                    return false;
                }
                if (i == 12) {
                    io.AddMouseButtonEvent(0, false);
                    return false;
                }
                if (i == 13 && umschalt) { io.AddKeyEvent(ImGuiMod_Shift, false); }
                return i >= 16;
            }};
}

void gSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<GStand>();
    // 1. Aufbau in allen sechs Bereichen wie 3ds Max
    for (int b = 0; b < kModellBereiche; ++b) {
        add(tu(std::string("T1411 G: wie 3ds Max, Bereich ") + bereichName(static_cast<ModellBereich>(b)), [b] {
            modellBedienungSetzen(0);
            mtab().bereich = static_cast<ModellBereich>(b);
            const int q = sequenz("SBD_WALK_NORMAL");
            if (q >= 0 && g_app->animIndex != q) { animWaehlen(q, false); }
        }));
        add(pause(0.4));
        add(tu("T1411 G: Lage der Teile", [st, b] {
            GLage l;
            l.bereich = bereichName(static_cast<ModellBereich>(b));
            const ModellZeitProbe& p = zp();
            l.hinweis = p.hinweisText;
            l.spurZeilen = p.spurZeilen;
            l.statusHoehe = p.status[3];   // Probe: x, y, Breite, Hoehe
            const Element* sch = finde("##mzschieber", "", 0, false);
            l.schieberOben = sch != nullptr && sch->rect.Max.y <= p.skala[1] + 1.0F;
            const Element* ak = finde(tr(Str::MzAutoKey), "", 0, false);
            l.knoepfeUnten = ak != nullptr && ak->rect.Min.y >= p.status[1] - 1.0F && ak->rect.Max.y <= p.status[1] + p.status[3] + 1.0F;
            l.fussExplorer = modellExplorer().fussHinweis || modellExplorer().fussEbenen;
            st->lagen.push_back(l);
            char z[300];
            std::snprintf(z, sizeof z, "G %s: Hinweis '%s', %d Zeilen unter der Spurleiste, Schieber oben %d, Knoepfe in der Statuszeile %d, Statuszeile %.0f px, Fusszeile Explorer %d",
                          l.bereich.c_str(), l.hinweis.c_str(), l.spurZeilen, l.schieberOben ? 1 : 0, l.knoepfeUnten ? 1 : 0, static_cast<double>(l.statusHoehe), l.fussExplorer ? 1 : 0);
            melde(z);
        }));
        add(foto(std::string("t1411_g_max_") + bereichName(static_cast<ModellBereich>(b))));
    }
    add(pruefe("T1411 G: wie 3ds Max in allen Bereichen kein Text unter der Leiste und keine Zeilen unter der Spurleiste", [st] {
        bool ok = st->lagen.size() == static_cast<std::size_t>(kModellBereiche);
        for (const GLage& l : st->lagen) { ok = ok && l.hinweis.empty() && l.spurZeilen == 0; }
        return ok;
    }));
    add(pruefe("T1411 G: der Zeitschieber steht ueber der Spurleiste, Auto Key in der einzeiligen Statuszeile darunter (alle Bereiche)", [st] {
        bool ok = st->lagen.size() == static_cast<std::size_t>(kModellBereiche);
        for (const GLage& l : st->lagen) { ok = ok && l.schieberOben && l.knoepfeUnten && l.statusHoehe <= ImGui::GetFrameHeight() + 8.0F; }
        return ok;
    }));
    add(pruefe("T1411 G: keine Fusszeile unter dem Baum des Szenen-Explorers", [st] {
        bool ok = !st->lagen.empty();
        for (const GLage& l : st->lagen) { ok = ok && !l.fussExplorer; }
        return ok;
    }));
    // 2. Verhalten (Bereich Animation, SBD_WALK_NORMAL)
    add(tu("T1411 G: Bereich Animation, Bild 0", [] {
        mtab().bereich = ModellBereich::Animation;
        const int q = sequenz("SBD_WALK_NORMAL");
        if (q >= 0) {
            animWaehlen(q, false);
            g_app->animFrame = g_app->animList[uz(q)].firstFrame;
        }
        g_app->animPlaying = false;
    }));
    add(pause(0.3));
    add(gZiehen("den Zeitschieber vom Anfang ans Ende", [] {
        const Element* e = finde("##mzschieber", "", 0, false);
        return e != nullptr ? ImVec2{e->rect.Min.x + ImGui::GetFontSize() * 2.0F, e->rect.GetCenter().y} : ansichtMitte();
    }, [] {
        const Element* e = finde("##mzschieber", "", 0, false);
        return e != nullptr ? ImVec2{e->rect.Max.x + ImGui::GetFontSize(), e->rect.GetCenter().y} : ansichtMitte();
    }, false));
    add(tu("T1411 G: Bild nach dem Ziehen", [st] {
        st->nachZiehen = gRel();
        const int q = g_app->animIndex;
        st->letztes = q >= 0 && uz(q) < g_app->animList.size() ? g_app->animList[uz(q)].numFrames - 1 : -2;
    }));
    add(klickAn("T1411 G: Spurleiste bei Bild 10", [] { return ImVec2{skalaX(10), std::floor(zp().skala[1] + zp().skala[3] * 0.5F)}; }));
    add(pause(0.2));
    add(tu("T1411 G: Bild nach dem Klick", [st] { st->nachKlickSkala = gRel(); }));
    add(klickAn("T1411 G: ins Bildfeld", [] {
        const Element* e = finde("##mzbildfeld", "", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ansichtMitte();
    }));
    add({"T1411 G: 7 und Enter tippen", [](int i) {
             ImGuiIO& io = ImGui::GetIO();
             if (i == 0) {
                 io.AddKeyEvent(ImGuiMod_Ctrl, true);
                 io.AddKeyEvent(ImGuiKey_A, true);
                 return false;
             }
             if (i == 1) {
                 io.AddKeyEvent(ImGuiKey_A, false);
                 io.AddKeyEvent(ImGuiMod_Ctrl, false);
                 return false;
             }
             if (i == 2) {
                 io.AddInputCharacter('7');
                 return false;
             }
             if (i == 3) {
                 io.AddKeyEvent(ImGuiKey_Enter, true);
                 return false;
             }
             if (i == 4) {
                 io.AddKeyEvent(ImGuiKey_Enter, false);
                 return false;
             }
             return i >= 7;
         }});
    add(tu("T1411 G: Bild nach dem Tippen", [st] { st->nachTippen = gRel(); }));
    add(klickAn("T1411 G: in die Ansicht (Fokus fuer die Tasten)", [] { return ImVec2{mtab().ansichten[mtab().aktiv].lo[0] + 30.0F, mtab().ansichten[mtab().aktiv].lo[1] + 60.0F}; }));
    add(taste(ImGuiKey_Period, ". (Bild vor)"));
    add(tu("T1411 G: Bild nach .", [st] { st->nachPunkt = gRel(); }));
    add(taste(ImGuiKey_Comma, ", (Bild zurueck)"));
    add(tu("T1411 G: Bild nach ,", [st] { st->nachKomma = gRel(); }));
    // Schluessel in der Spurleiste: Knochen lradius auf Bild 3 drehen, K
    add(tu("T1411 G: lradius waehlen, Bild 3, 20 Grad drehen", [st] {
        ModellTab& t = mtab();
        t.rig = modell::Rig{};
        t.rigFuer.clear();
        t.griffWahl = -1;
        st->knochen = boneIndex(g_app->anim, "lradius");
        t.knochenWahl = st->knochen;
        const int q = g_app->animIndex;
        if (q >= 0 && uz(q) < g_app->animList.size()) {
            g_app->animFrame = g_app->animList[uz(q)].firstFrame + 3;
            g_app->animTime = 0.0F;
        }
        const float wert[3] = {0.0F, 0.0F, 20.0F};
        modellRigZiehen(2, wert);
        modellRigZiehenEnde();
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(taste(ImGuiKey_K, "K (Set Key)"));
    add(pause(0.2));
    add(tu("T1411 G: Schluessel auf Bild 3?", [st] {
        st->keyDa = gKey(st->knochen, 3);
        mtab().kvWahl.clear();
    }));
    add(klickAn("T1411 G: auf den Schluessel in der Spurleiste (Bild 3)", [] { return ImVec2{skalaX(3), modellZeitSchluesselY(zp().skala[1])}; }));
    add(pause(0.2));
    add(tu("T1411 G: Schluessel gewaehlt?", [st] {
        for (const auto& w : mtab().kvWahl) { st->keyGewaehlt = st->keyGewaehlt || w.bild == g_app->animList[uz(g_app->animIndex)].firstFrame + 3; }
    }));
    add(gZiehen("den Schluessel von Bild 3 auf Bild 6", [] { return ImVec2{skalaX(3), modellZeitSchluesselY(zp().skala[1])}; },
                [] { return ImVec2{skalaX(6), modellZeitSchluesselY(zp().skala[1])}; }, false));
    add(tu("T1411 G: verschoben?", [st] { st->keyVerschoben = gKey(st->knochen, 6) && !gKey(st->knochen, 3); }));
    add(gZiehen("mit Umschalt den Schluessel von Bild 6 auf Bild 9 (kopieren)", [] { return ImVec2{skalaX(6), modellZeitSchluesselY(zp().skala[1])}; },
                [] { return ImVec2{skalaX(9), modellZeitSchluesselY(zp().skala[1])}; }, true));
    add(tu("T1411 G: kopiert?", [st] { st->keyKopiert = gKey(st->knochen, 6) && gKey(st->knochen, 9); }));
    add(klickAn("T1411 G: Rechtsklick auf den Schluessel (Bild 9)", [] { return ImVec2{skalaX(9), modellZeitSchluesselY(zp().skala[1])}; }, 1));
    add(pause(0.2));
    add(tu("T1411 G: Menue offen?", [st] {
        st->keyMenue = ImGui::IsPopupOpen("", ImGuiPopupFlags_AnyPopupId);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true);
    }));
    add(tu("T1411 G: Esc los", [] { ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, false); }));
    add(pause(0.2));
    add(klickAn("T1411 G: Mini Curve Editor", [] {
        const Element* e = finde(tr(Str::T1MiniKurve), "", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ansichtMitte();
    }));
    add(pause(0.3));
    add(tu("T1411 G: Spurbereich offen?", [st] { st->miniKurveAuf = zp().spuren[3] > ImGui::GetFrameHeight(); }));
    add(foto("t1411_g_max_animation_minikurve"));
    add(klickAn("T1411 G: Mini Curve Editor wieder zu", [] {
        const Element* e = finde(tr(Str::T1MiniKurve), "", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ansichtMitte();
    }));
    add(tu("T1411 G: Ergebnis", [st] {
        char z[300];
        std::snprintf(z, sizeof z, "G Verhalten: gezogen %d (letztes %d), Klick Spurleiste %d, getippt %d, . %d , %d; Key 3 %d, gewaehlt %d, verschoben %d, kopiert %d, Menue %d, Mini Curve Editor %d",
                      st->nachZiehen, st->letztes, st->nachKlickSkala, st->nachTippen, st->nachPunkt, st->nachKomma, st->keyDa ? 1 : 0, st->keyGewaehlt ? 1 : 0,
                      st->keyVerschoben ? 1 : 0, st->keyKopiert ? 1 : 0, st->keyMenue ? 1 : 0, st->miniKurveAuf ? 1 : 0);
        melde(z);
    }));
    add(pruefe("T1411 G: den Zeitschieber ziehen scrubbt bis zum letzten Bild", [st] { return st->letztes > 0 && st->nachZiehen == st->letztes; }));
    add(pruefe("T1411 G: ein Klick in die Spurleiste setzt das Bild", [st] { return st->nachKlickSkala == 10; }));
    add(pruefe("T1411 G: das Bildfeld eintippen springt (7)", [st] { return st->nachTippen == 7; }));
    add(pruefe("T1411 G: . und , gehen ein Bild weiter und zurueck", [st] { return st->nachPunkt == st->nachTippen + 1 && st->nachKomma == st->nachTippen; }));
    add(pruefe("T1411 G: Schluessel in der Spurleiste: anklicken waehlt, ziehen verschiebt, Umschalt-Ziehen kopiert, Rechtsklick zeigt das Menue",
               [st] { return st->keyDa && st->keyGewaehlt && st->keyVerschoben && st->keyKopiert && st->keyMenue; }));
    add(pruefe("T1411 G/I: der Mini Curve Editor oeffnet Dope Sheet/Kurveneditor ueber der Spurleiste", [st] { return st->miniKurveAuf; }));
    // 3. wie Blender: auch dort kein Text unter der Leiste
    add(tu("T1411 G: wie Blender, Bereich Animation", [] { modellBedienungSetzen(1); }));
    add(pause(0.4));
    add(pruefe("T1411 G: wie Blender kein erklaerender Text unter der Leiste", [] { return zp().hinweisText.empty(); }));
    add(foto("t1411_g_blender_animation"));
    add(tu("T1411 G: zurueck", [] {
        modellBedienungSetzen(0);
        mtab().knochenWahl = -1;
        mtab().bereich = ModellBereich::Ansehen;
    }));
}

// --- J: Rechtsklick wie das Quad-Menue von 3ds Max ----------------------------------------------------------------------

struct JStand {
    ImVec2 klick{};
    bool rotateDa = false;       // "Rotate" im Menue
    bool rechtsUnten = false;    // ... rechts unter dem Klick (Viertel transform)
    bool displayRechtsOben = false;   // "Hide Selection" rechts ueber dem Klick (Viertel display)
    int gizmo = -1;              // Werkzeug nach dem Klick auf Rotate
    bool blTransform = false;    // wie Blender: "Transform" im Kontextmenue
};

void jSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<JStand>();
    add(tu("T1411 J: wie 3ds Max, Bereich Animation, Werkzeug Auswaehlen", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Animation;
        mtab().gizmo = 0;
    }));
    add(pause(0.3));
    add(tu("T1411 J: eine Ansicht (nach dem Wechsel des Bereichs, der seine Wahl von Alt+W mitbringt)", [] {
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
    }));
    add(pause(0.3));
    add(tu("T1411 J: Klickstelle", [st] {
        const ModellAnsicht& a = mtab().ansichten[3];
        st->klick = ImVec2{std::floor(a.lo[0] + a.groesse[0] * 0.5F), std::floor(a.lo[1] + a.groesse[1] * 0.5F)};
    }));
    add(pause(0.3));
    add(klickAn("T1411 J: Rechtsklick in die Ansicht", [st] { return st->klick; }, 1));
    add(pause(0.3));
    add(tu("T1411 J: was das Menue zeigt", [st] {
        const Element* rot = finde(tr(Str::T1QuadRotate), "", 0, false);
        const Element* hide = finde(tr(Str::T1QuadHideSel), "", 0, false);
        st->rotateDa = rot != nullptr;
        st->rechtsUnten = rot != nullptr && rot->rect.Min.x >= st->klick.x && rot->rect.Min.y >= st->klick.y;
        st->displayRechtsOben = hide != nullptr && hide->rect.Min.x >= st->klick.x && hide->rect.Max.y <= st->klick.y;
        melde("J Quad-Menue: Rotate " + std::to_string(st->rotateDa ? 1 : 0) + ", rechts unten " + std::to_string(st->rechtsUnten ? 1 : 0) + ", Hide Selection rechts oben " +
              std::to_string(st->displayRechtsOben ? 1 : 0));
    }));
    add(foto("t1411_j_quadmenue"));
    add(klickAn("T1411 J: Rotate", [st] {
        const Element* rot = finde(tr(Str::T1QuadRotate), "", 0, false);
        return rot != nullptr ? rot->rect.GetCenter() : ImVec2{st->klick.x - 400.0F, st->klick.y};
    }));
    add(pause(0.3));
    add(tu("T1411 J: Werkzeug", [st] {
        st->gizmo = mtab().gizmo;
        melde("J Werkzeug nach Rotate: " + std::to_string(st->gizmo));
        if (ImGui::IsPopupOpen("##mtkontext")) { ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true); }
    }));
    add(tu("T1411 J: Esc los", [] { ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, false); }));
    add(tu("T1411 J: wie Blender", [] { modellBedienungSetzen(1); }));
    add(pause(0.3));
    add(klickAn("T1411 J: Rechtsklick in die Ansicht (Blender)", [st] { return st->klick; }, 1));
    add(pause(0.3));
    add(tu("T1411 J: Transform im Kontextmenue?", [st] {
        st->blTransform = finde(tr(Str::T1BlTransform), "", 0, false) != nullptr;
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true);
    }));
    add(tu("T1411 J: Esc los", [] { ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, false); }));
    add(pause(0.2));
    add(pruefe("T1411 J: wie 3ds Max zeigt der Rechtsklick das Quad-Menue mit Rotate rechts unter der Maus (transform) und Hide Selection rechts darueber (display)",
               [st] { return st->rotateDa && st->rechtsUnten && st->displayRechtsOben; }));
    add(pruefe("T1411 J: Rechtsklick -> Rotate setzt das Werkzeug Drehen (wie E)", [st] { return st->gizmo == 2; }));
    add(pruefe("T1411 J: wie Blender steht Transform (G/R/S) im Kontextmenue", [st] { return st->blTransform; }));
    add(tu("T1411 J: zurueck", [] {
        modellBedienungSetzen(0);
        mtab().gizmo = 0;
        mtab().bereich = ModellBereich::Ansehen;
    }));
}

// --- L: ViewCube wie 3ds Max --------------------------------------------------------------------------------------------

struct LStand {
    int buchstabe = -1;          // ein Buchstabe, der auf dem Wuerfel liegt
    float neigungVor = 0.0F;
    float neigungNach = 0.0F;
    float gierVor = 0.0F;
    float gierNach = 0.0F;
    std::vector<float> verlauf;  // Neigung je Bild im Uebergang
    bool ueberschiesst = true;
};

// Liegt der Schirmpunkt im Umriss des Wuerfels (konvexe Huelle der acht Ecken)?
bool imWuerfel(int ansicht, ImVec2 p) {
    std::vector<ImVec2> e;
    for (int c = 0; c < 8; ++c) {
        ImVec2 q;
        if (modellWuerfelOrt(ansicht, (c & 1) != 0 ? 1 : -1, (c & 2) != 0 ? 1 : -1, (c & 4) != 0 ? 1 : -1, q)) { e.push_back(q); }
    }
    if (e.size() < 3U) { return false; }
    // Huelle nach Andrew (monoton)
    std::sort(e.begin(), e.end(), [](const ImVec2& a, const ImVec2& b) { return a.x < b.x || (a.x == b.x && a.y < b.y); });
    const auto kr = [](ImVec2 o, ImVec2 a, ImVec2 b) { return (a.x - o.x) * (b.y - o.y) - (a.y - o.y) * (b.x - o.x); };
    std::vector<ImVec2> h(2 * e.size());
    std::size_t k = 0;
    for (std::size_t i = 0; i < e.size(); ++i) {
        while (k >= 2 && kr(h[k - 2], h[k - 1], e[i]) <= 0.0F) { --k; }
        h[k++] = e[i];
    }
    for (std::size_t i = e.size() - 1, t = k + 1; i > 0; --i) {
        while (k >= t && kr(h[k - 2], h[k - 1], e[i - 1]) <= 0.0F) { --k; }
        h[k++] = e[i - 1];
    }
    h.resize(k - 1);
    for (std::size_t i = 0; i < h.size(); ++i) {
        if (kr(h[i], h[(i + 1) % h.size()], p) < -0.5F) { return false; }
    }
    return true;
}

void lSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<LStand>();
    add(tu("T1411 L: wie 3ds Max, Bereich View/Check, Perspektive, Kompass an", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
        mtab().ansichten[3].art = AnsichtArt::Perspektive;
        g_app->settings.modellNavigation = true;
        g_app->settings.modellKompass = true;
        modellAnsichtEinpassenAktiv();
    }));
    add(pause(0.3));
    // von oben schraeg (wie das Foto: Seite Right) die Gier drehen, bis ein Buchstabe auf dem Wuerfel liegt
    add({"T1411 L: eine Stellung suchen, in der ein Buchstabe auf dem Wuerfel liegt", [st](int b) {
             if (b % 3 != 0) { return false; }
             const int i = b / 3;
             if (i > 0) {
                 for (int k = 0; k < 4; ++k) {
                     ImVec2 p;
                     if (modellKompassOrt(3, k, p) && imWuerfel(3, p)) {
                         st->buchstabe = k;
                         return true;
                     }
                 }
             }
             if (i >= 48) { return true; }
             const float w[3] = {35.0F, static_cast<float>(i) * 7.5F, 0.0F};
             modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
             g_app->modelDirty = true;
             return false;
         }});
    add(pause(0.2));
    add(foto("t1411_l_viewcube_buchstabe"));
    add(tu("T1411 L: Winkel vor dem Klick", [st] {
        float w[3];
        modellAnsichtWinkel(mtab().ansichten[3], w);
        st->neigungVor = w[0];
        st->gierVor = w[1];
        melde("L Buchstabe " + std::to_string(st->buchstabe) + " auf dem Wuerfel, Neigung " + std::to_string(w[0]) + ", Gier " + std::to_string(w[1]));
    }));
    add({"T1411 L: Klick an der Stelle des Buchstabens, Neigung je Bild", [st](int b) {
             ImGuiIO& io = ImGui::GetIO();
             if (st->buchstabe < 0) { return true; }
             ImVec2 p;
             if (b == 0) {
                 if (modellKompassOrt(3, st->buchstabe, p)) { setzeMaus(p); }
                 return false;
             }
             if (b == 1) {
                 io.AddMouseButtonEvent(0, true);
                 return false;
             }
             if (b == 2) {
                 io.AddMouseButtonEvent(0, false);
                 return false;
             }
             float w[3];
             modellAnsichtWinkel(mtab().ansichten[3], w);
             st->verlauf.push_back(w[0]);
             return b >= 60;
         }});
    add(tu("T1411 L: Winkel nach dem Klick", [st] {
        float w[3];
        modellAnsichtWinkel(mtab().ansichten[3], w);
        st->neigungNach = w[0];
        st->gierNach = w[1];
        // kein Ueberschiessen: die Neigung laeuft vom Anfang zum Ziel, ohne ueber das Ziel hinaus und zurueck zu gehen
        bool ok = !st->verlauf.empty();
        for (std::size_t i = 1; i < st->verlauf.size(); ++i) {
            const float d0 = std::fabs(st->verlauf[i - 1] - st->neigungNach);
            const float d1 = std::fabs(st->verlauf[i] - st->neigungNach);
            ok = ok && d1 <= d0 + 0.01F;
        }
        st->ueberschiesst = !ok;
        melde("L nach dem Klick: Neigung " + std::to_string(w[0]) + ", Gier " + std::to_string(w[1]) + ", Ueberschiessen " + std::to_string(st->ueberschiesst ? 1 : 0));
    }));
    // der Buchstabe haette die Neigung behalten und die Gier auf seine Himmelsrichtung gedreht (buchstabeKlick); eine Flaeche, Kante oder
    // Ecke des Wuerfels aendert die Neigung (0, 45, 90 bzw. 35,26 Grad an einer Ecke)
    add(pruefe("T1411 L: ein Klick an der Stelle eines Buchstabens hinter dem Wuerfel trifft den Wuerfel (Flaeche, Kante oder Ecke), nicht den Buchstaben", [st] {
        return st->buchstabe >= 0 && std::fabs(st->neigungNach - st->neigungVor) > 1.0F;
    }));
    add(pruefe("T1411 L: der Uebergang laeuft weich zum Ziel, ohne nachzuschwingen", [st] { return st->buchstabe >= 0 && !st->ueberschiesst; }));
}

// --- M: Beschriftung auf jedem Grund lesbar -------------------------------------------------------------------------------

// WCAG 2: relative Leuchtdichte und Kontrastverhaeltnis
float mLeucht(float r, float g, float b) {
    const auto lin = [](float c) { return c <= 0.03928F ? c / 12.92F : std::pow((c + 0.055F) / 1.055F, 2.4F); };
    return 0.2126F * lin(r) + 0.7152F * lin(g) + 0.0722F * lin(b);
}

float mKontrast(ImU32 vorne, const float grund[3]) {
    const float a = static_cast<float>((vorne >> IM_COL32_A_SHIFT) & 0xFFU) / 255.0F;
    float v[3];
    for (int k = 0; k < 3; ++k) {
        const float c = static_cast<float>((vorne >> (k * 8)) & 0xFFU) / 255.0F;   // R G B in Bit 0, 8, 16
        v[k] = c * a + grund[k] * (1.0F - a);
    }
    const float l1 = mLeucht(v[0], v[1], v[2]);
    const float l2 = mLeucht(grund[0], grund[1], grund[2]);
    return (std::max(l1, l2) + 0.05F) / (std::min(l1, l2) + 0.05F);
}

void mSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto kleinst = std::make_shared<float>(100.0F);
    auto alle = std::make_shared<std::string>();
    // dunkler Grund (Verlauf ab Werk), mittel, Weiss, Silhouette (0xE8E8E8)
    const int gruende[4] = {-1, 0x808080, 0xFFFFFF, -2};
    for (const int g : gruende) {
        add(tu("T1411 M: Grund " + std::to_string(g), [g] {
            mtab().silhouette = g == -2;
            g_app->settings.modellHintergrund = g == -2 ? -1 : g;
            mtab().bereich = ModellBereich::Animation;
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        if (g == -2) { add(foto("t1411_m_silhouette")); }
        if (g == 0xFFFFFF) { add(foto("t1411_m_weiss")); }
        add(tu("T1411 M: Kontrast der Beschriftung", [kleinst, alle, g] {
            float grund[3];
            modellHintergrundFarbe(grund);
            const float k1 = mKontrast(modellUeberGrund(1.0F, false), grund);
            const float k2 = mKontrast(modellUeberGrund(1.0F, true), grund);
            *kleinst = std::min({*kleinst, k1, k2});
            char z[160];
            std::snprintf(z, sizeof z, " Grund %d: Beschriftung %.2f:1, unter der Maus %.2f:1;", g, static_cast<double>(k1), static_cast<double>(k2));
            *alle += z;
        }));
    }
    add(tu("T1411 M: Ergebnis", [kleinst, alle] { melde("M Kontrast:" + *alle + " kleinster " + std::to_string(*kleinst)); }));
    add(pruefe("T1411 M: die Beschriftung ueber der Ansicht hat auf dunklem, mittlerem, weissem Grund und in der Silhouette mindestens 4,5:1 Kontrast (WCAG)",
               [kleinst] { return *kleinst >= 4.5F; }));
    add(tu("T1411 M: zurueck", [] {
        mtab().silhouette = false;
        g_app->settings.modellHintergrund = -1;
        mtab().bereich = ModellBereich::Ansehen;
        g_app->modelDirty = true;
    }));
}

// --- K: Fussplattform wie CAT -------------------------------------------------------------------------------------------

struct KStand {
    int fuss = -1;
    int ferse = -1;
    int zehe = -1;
    int gewaehlt = -2;
    float fussZ0 = 0.0F;
    float fussZ1 = 0.0F;
    float knie0 = 0.0F;        // Winkel am Knie (Grad) vor und nach dem Zug
    float knie1 = 0.0F;
    bool schluessel = false;
    float ferseZ0 = 0.0F;
    float ferseZ1 = 0.0F;
};

int kGriff(modell::GriffArt art) {
    const auto& gr = mtab().rig.griffe;
    for (std::size_t i = 0; i < gr.size(); ++i) {
        if (gr[i].art == art && gr[i].seite == modell::Seite::Links) { return static_cast<int>(i); }
    }
    return -1;
}

float kZ(int knochen) { return knochen >= 0 && uz(knochen) < g_app->modellWelt.size() ? g_app->modellWelt[uz(knochen)].m[2][3] : 0.0F; }

float kKnie() {
    const int o = boneIndex(g_app->anim, "lfemurYZ");
    const int k = boneIndex(g_app->anim, "ltibia");
    const int f = boneIndex(g_app->anim, "ltalus");
    const auto& w = g_app->modellWelt;
    if (o < 0 || k < 0 || f < 0 || uz(std::max({o, k, f})) >= w.size()) { return 0.0F; }
    float a[3];
    float b[3];
    for (int i = 0; i < 3; ++i) {
        a[i] = w[uz(o)].m[i][3] - w[uz(k)].m[i][3];
        b[i] = w[uz(f)].m[i][3] - w[uz(k)].m[i][3];
    }
    const float la = std::sqrt(a[0] * a[0] + a[1] * a[1] + a[2] * a[2]);
    const float lb = std::sqrt(b[0] * b[0] + b[1] * b[1] + b[2] * b[2]);
    return std::acos(std::clamp((a[0] * b[0] + a[1] * b[1] + a[2] * b[2]) / std::max(la * lb, 1e-6F), -1.0F, 1.0F)) * 180.0F / 3.14159265F;
}

// Ein Punkt auf der Platte zwischen Ferse und Zehe, abseits der Drehpunkte: 40 % von der Ferse, 85 % der halben Breite seitlich
ImVec2 kPlattenPunkt(const KStand& k) {
    float a[3];
    float b[3];
    ImVec2 out{0.0F, 0.0F};
    if (!modellRigGriffWelt(k.ferse, g_app->modellWelt, a) || !modellRigGriffWelt(k.zehe, g_app->modellWelt, b)) { return out; }
    const float d[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    const float l = std::sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    const float lq = std::sqrt(d[0] * d[0] + d[1] * d[1]);
    const float seite = lq > 1e-4F ? 0.45F * l * 0.5F * 0.85F / lq : 0.0F;
    const float p[3] = {a[0] + d[0] * 0.4F - d[1] * seite, a[1] + d[1] * 0.4F + d[0] * seite, a[2] + d[2] * 0.4F};
    (void)modellAufSchirm(mtab().ansichten[mtab().aktiv], p, out);
    return ImVec2{std::floor(out.x), std::floor(out.y)};
}

void kSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<KStand>();
    add(tu("T1411 K: Animation, wie 3ds Max, Rig-Stil Standard, Control Rig, SBD_WALK_NORMAL Bild 0, Ansicht von vorn", [st] {
        modellBedienungSetzen(0);
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Animation;
        t.vierAnsichten = false;
        t.aktiv = 3;
        modellRigStilSetzen(modell::RigStil::Standard);
        const int q = sequenz("SBD_WALK_NORMAL");
        if (q >= 0) {
            animWaehlen(q, false);
            g_app->animFrame = g_app->animList[uz(q)].firstFrame;
            g_app->animTime = 0.0F;
        }
        (void)modellAnimRigErstellen(false);
        t.rigZeigen = true;
        t.griffWahl = -1;
        t.knochenWahl = -1;
        t.ansichten[3].art = AnsichtArt::Perspektive;
        const float w[3] = {20.0F, 90.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(tu("T1411 K: Griffe des linken Fusses", [st] {
        st->fuss = kGriff(modell::GriffArt::Fuss);
        st->ferse = kGriff(modell::GriffArt::Ferse);
        st->zehe = kGriff(modell::GriffArt::Zehe);
        melde("K Griffe: Fuss " + std::to_string(st->fuss) + ", Ferse " + std::to_string(st->ferse) + ", Zehe " + std::to_string(st->zehe));
    }));
    add(foto("t1411_k_fussplattform"));
    add(klickAn("T1411 K: auf die Platte (40 % von der Ferse, seitlich)", [st] { return kPlattenPunkt(*st); }));
    add(pause(0.3));
    add(tu("T1411 K: gewaehlt, Fuss 6 Einheiten heben", [st] {
        st->gewaehlt = mtab().griffWahl;
        const int fussKnochen = st->fuss >= 0 ? mtab().rig.griffe[uz(st->fuss)].knochen : -1;
        st->fussZ0 = kZ(fussKnochen);
        st->knie0 = kKnie();
        if (st->gewaehlt == st->fuss && st->fuss >= 0) {
            const float weg[3] = {0.0F, 0.0F, 6.0F};
            mtab().gizmo = 1;
            modellRigZiehen(1, weg);
            modellRigZiehenEnde();
        }
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(taste(ImGuiKey_K, "K (Set Key)"));
    add(pause(0.3));
    add(tu("T1411 K: Fuss und Knie nach dem Zug, Schluessel", [st] {
        const int fussKnochen = st->fuss >= 0 ? mtab().rig.griffe[uz(st->fuss)].knochen : -1;
        st->fussZ1 = kZ(fussKnochen);
        st->knie1 = kKnie();
        const modell::RigSchicht* sch = modellRigSchichtJetzt();
        st->schluessel = false;
        for (std::size_t i = 0; sch != nullptr && i < sch->ik.size(); ++i) { st->schluessel = st->schluessel || !sch->ik[i].ziel.empty(); }
        char z[200];
        std::snprintf(z, sizeof z, "K Platte gewaehlt %d (Fuss %d), Fuss z %.2f -> %.2f, Knie %.1f -> %.1f Grad, IK-Schluessel %d", st->gewaehlt, st->fuss,
                      static_cast<double>(st->fussZ0), static_cast<double>(st->fussZ1), static_cast<double>(st->knie0), static_cast<double>(st->knie1), st->schluessel ? 1 : 0);
        melde(z);
    }));
    // Abrollen um die Zehe: die Ferse hebt sich
    add(tu("T1411 K: Zehe waehlen, um X drehen", [st] {
        float f[3] = {0.0F, 0.0F, 0.0F};
        if (st->ferse >= 0) { (void)modellRigGriffWelt(st->ferse, g_app->modellWelt, f); }
        st->ferseZ0 = f[2];
        modellRigGriffWaehlen(st->zehe);
        mtab().gizmo = 2;
        const float dreh[3] = {25.0F, 0.0F, 0.0F};
        modellRigZiehen(2, dreh);
        modellRigZiehenEnde();
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("T1411 K: Ferse nach dem Abrollen", [st] {
        float f[3] = {0.0F, 0.0F, 0.0F};
        if (st->ferse >= 0) { (void)modellRigGriffWelt(st->ferse, g_app->modellWelt, f); }
        st->ferseZ1 = f[2];
        if (st->ferseZ1 < st->ferseZ0) {   // in die andere Richtung gedreht: zurueck und andersherum
            const float dreh[3] = {-50.0F, 0.0F, 0.0F};
            modellRigZiehen(2, dreh);
            modellRigZiehenEnde();
        }
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("T1411 K: Ferse gemessen", [st] {
        float f[3] = {0.0F, 0.0F, 0.0F};
        if (st->ferse >= 0) { (void)modellRigGriffWelt(st->ferse, g_app->modellWelt, f); }
        st->ferseZ1 = std::max(st->ferseZ1, f[2]);
        melde("K Abrollen: Ferse z " + std::to_string(st->ferseZ0) + " -> " + std::to_string(st->ferseZ1));
    }));
    add(pruefe("T1411 K: ein Klick auf die Fussplattform waehlt den IK-Fuss", [st] { return st->fuss >= 0 && st->gewaehlt == st->fuss; }));
    add(pruefe("T1411 K: die Platte heben hebt den Fuss (6 Einheiten), das Knie beugt sich, ein IK-Schluessel ist gesetzt",
               [st] { return std::fabs(st->fussZ1 - st->fussZ0 - 6.0F) < 0.6F && std::fabs(st->knie1 - st->knie0) > 2.0F && st->schluessel; }));
    add(pruefe("T1411 K: Abrollen um die Zehe hebt die Ferse", [st] { return st->ferseZ1 - st->ferseZ0 > 1.0F; }));
    add(tu("T1411 K: zurueck", [] {
        mtab().gizmo = 0;
        mtab().griffWahl = -1;
        mtab().rig = modell::Rig{};
        mtab().rigFuer.clear();
        mtab().bereich = ModellBereich::Ansehen;
    }));
}

// --- C: Load .gla aus den pk3 -------------------------------------------------------------------------------------------

struct CStand {
    bool listeOffen = false;
    bool dateiKnopf = false;
    std::string glaVor;
    std::string glaNach;
};

void cSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<CStand>();
    add(tu("T1411 C: View/Check", [st] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        st->glaVor = g_app->anim.skeletonName + " " + std::to_string(g_app->anim.numFrames);
    }));
    add(pause(0.3));
    add(klickAn("T1411 C: Werkzeugknopf Load .gla...", [] {
        const Element* e = finde(tr(Str::ModelLoadGla), "", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ansichtMitte();
    }));
    add(pause(0.4));
    add(tu("T1411 C: die Liste offen?", [st] {
        st->listeOffen = finde("##glasuche", "", 0, false) != nullptr;
        st->dateiKnopf = finde(tr(Str::MeDateiOeffnen), "", 0, false) != nullptr;
        melde("C Liste offen " + std::to_string(st->listeOffen ? 1 : 0) + ", Knopf Datei oeffnen " + std::to_string(st->dateiKnopf ? 1 : 0));
    }));
    add(foto("t1411_c_gla_liste"));
    add(klickAn("T1411 C: _humanoid_luke in der Liste (eine andere GLA)", [] {
        // irgendeine GLA ausser der geladenen: die erste Zeile mit "_humanoid_" (Karten-GLA) oder sonst "rancor"
        for (const Element& e : g_elemente) {
            if (e.label.find(".gla") != std::string::npos && e.label.find("_humanoid/_humanoid.gla") == std::string::npos && e.label.find("models/") == 0) {
                return e.rect.GetCenter();
            }
        }
        return ansichtMitte();
    }));
    add(pause(0.6));
    add(tu("T1411 C: geladen?", [st] {
        st->glaNach = g_app->anim.skeletonName + " " + std::to_string(g_app->anim.numFrames);
        melde("C GLA vorher '" + st->glaVor + "', nachher '" + st->glaNach + "'");
    }));
    add(pruefe("T1411 C: Load .gla... oeffnet die Liste der .gla aus den pk3 (mit Open file... fuer eine lose)", [st] { return st->listeOffen && st->dateiKnopf; }));
    add(pruefe("T1411 C: ein Klick in der Liste laedt die GLA", [st] { return !st->glaNach.empty() && st->glaNach != st->glaVor; }));
    for (Schritt& x : mb1::laden("models/players/alora/model.glm")) { add(std::move(x)); }
}

// --- F: Knoepfe der Zeitleiste stehen still ------------------------------------------------------------------------------

struct FLage {
    std::vector<float> x;   // je Knopf die linke Kante
    std::string text;
};

std::vector<std::string> fKnoepfe() {
    return {tr(Str::MzAutoKey), tr(Str::MzSetKey), tr(Str::MzZeitKonfig)};
}

FLage fMessen() {
    FLage l;
    for (const std::string& k : fKnoepfe()) {
        const Element* e = finde(k, "", 0, false);
        l.x.push_back(e != nullptr ? e->rect.Min.x : -1.0F);
    }
    l.text = mtab().zaehlungText;
    return l;
}

void fSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto lagen = std::make_shared<std::vector<FLage>>();
    add(tu("T1411 F: Bereich Animation, wie 3ds Max, laengste Sequenz", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Animation;
        const int q = laengsteSequenz();
        if (q >= 0) {
            animWaehlen(q, false);
            melde("F Sequenz " + g_app->animList[uz(q)].name + ", " + std::to_string(g_app->animList[uz(q)].numFrames) + " Bilder");
        }
        g_app->animPlaying = false;
    }));
    add(pause(0.4));
    const int bilder[6] = {0, 9, 10, 99, 100, 1000};
    for (const int b : bilder) {
        add(tu("T1411 F: Bild " + std::to_string(b), [b] {
            const int q = g_app->animIndex;
            if (q < 0 || uz(q) >= g_app->animList.size()) { return; }
            const AnimEntry& e = g_app->animList[uz(q)];
            g_app->animFrame = e.firstFrame + std::min(b, std::max(e.numFrames - 1, 0));
            g_app->modelDirty = true;
        }));
        add(pause(0.15));
        add(tu("T1411 F: Lage der Knoepfe bei Bild " + std::to_string(b), [lagen] {
            lagen->push_back(fMessen());
            std::string z = "F \"" + lagen->back().text + "\":";
            for (const float x : lagen->back().x) { z += " " + std::to_string(static_cast<int>(x)); }
            melde(z);
        }));
    }
    add(pruefe("T1411 F: Auto Key, Set Key und die Zeitkonfiguration stehen bei Bild 0, 9, 10, 99, 100 und dem letzten an derselben Stelle", [lagen] {
        if (lagen->size() < 2U) { return false; }
        for (const FLage& l : *lagen) {
            for (std::size_t k = 0; k < l.x.size(); ++k) {
                if (l.x[k] < 0.0F || l.x[k] != lagen->front().x[k]) { return false; }
            }
        }
        return true;
    }));
}

// --- N: Motion-Feld mit Setup- und Animationsmodus wie CAT --------------------------------------------------------------------------
// Rot (vor TEST1411 N): das Motion-Feld zeigte alle 17 Gruppen untereinander (Foto 26 von shank; ap10 A14 zaehlte sie als Soll).

struct NStand {
    bool vorgabeAnimation = false;
    bool schalterDa = false;
    std::string gruppenAnimation;
    std::string gruppenSetup;
    std::string gruppenAnzeige;
    std::string gruppenHierarchie;
    int zahlAnimation = 0;
    int zahlSetup = 0;
    bool setupPose = false;
    bool keinGizmo = false;
    int schluesselVorher = -1;
    int schluesselSetup = -1;
    int schluesselAnimation = -1;
    bool meldungSetup = false;
    bool statusSetup = false;
    bool tasteSetup = false;
    bool tasteAnimation = false;
    bool posePose = false;
    bool aktionDa = false;
    bool blEdit = false;
    bool blPose = false;
};

int nSchluessel() {
    const modell::RigSchicht* sch = modellRigSchichtJetzt();
    return sch != nullptr ? sch->schluesselZahl() : 0;
}

bool nIn(const std::string& liste, Str t) { return ("|" + liste + "|").find("|" + std::string(tr(t)) + "|") != std::string::npos; }

void nSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<NStand>();
    add(tu("T1411 N: wie 3ds Max, Bereich Animation, Reiter Motion, eine Ansicht", [st] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Animation;
        mtab().vierAnsichten = false;
        mtab().griffWahl = -1;
        mtab().knochenWahl = -1;
        g_app->animPlaying = false;
        const int q = sequenz("SBD_WALK_NORMAL");
        if (q >= 0) {
            animWaehlen(q, false);
            g_app->animFrame = g_app->animList[uz(q)].firstFrame + 3;
        }
        modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
        st->vorgabeAnimation = !modellAnimSetupGewaehlt();
        g_app->modelDirty = true;
    }));
    add(pause(0.5));
    add(tu("T1411 N: Gruppen im Animationsmodus", [st] {
        st->gruppenAnimation = ap10::gruppen();
        st->zahlAnimation = ui::rolloutProbeZahl();
        st->schalterDa = finde(tr(Str::T1nSetup), "mtrollouts", 0, false) != nullptr && finde(tr(Str::T1nAnimation), "mtrollouts", 0, false) != nullptr;
        st->posePose = !modell::bindepose(g_app->anim.bones, g_app->modellWelt);
        melde("N Animationsmodus (" + std::to_string(st->zahlAnimation) + "): " + st->gruppenAnimation);
    }));
    add(foto("t1411_n_animationsmodus"));
    add(klick(tr(Str::T1nSetup), "mtrollouts"));
    add(pause(0.5));
    add(tu("T1411 N: Gruppen im Setup-Modus", [st] {
        st->gruppenSetup = ap10::gruppen();
        st->zahlSetup = ui::rolloutProbeZahl();
        st->setupPose = modellAnimSetupGewaehlt() && modell::bindepose(g_app->anim.bones, g_app->modellWelt);
        st->statusSetup = zp().auswahlText.find(tr(Str::T1nSetup)) != std::string::npos;
        melde("N Setup-Modus (" + std::to_string(st->zahlSetup) + "): " + st->gruppenSetup + " - Statuszeile: " + zp().auswahlText);
    }));
    add(tu("T1411 N: Maus in die Ansicht (kein Tooltip im Foto)", [] { setzeMaus(ansichtMitte()); }));
    add(pause(0.4));
    add(foto("t1411_n_setupmodus"));
    add(tu("T1411 N: Knochen lhumerus waehlen", [] {
        mtab().griffWahl = -1;
        mtab().knochenWahl = boneIndex(g_app->anim, "lhumerus");
        mtab().gizmo = 2;
        mtab().meldung.clear();
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
    add(tu("T1411 N: Schluessel vorher, Gizmo", [st] {
        st->schluesselVorher = nSchluessel();
        float m[3];
        st->keinGizmo = !modellAnimKnochenMitte(m) && !modellRigGriffMitte(m);
    }));
    add(taste(ImGuiKey_K, "K (Set Key) im Setup-Modus"));
    add(pause(0.3));
    add(tu("T1411 N: Schluessel nach K im Setup-Modus", [st] {
        st->schluesselSetup = nSchluessel();
        st->meldungSetup = mtab().meldung.find(tr(Str::T1nSetup)) != std::string::npos;
        melde("N K im Setup-Modus: Schluessel " + std::to_string(st->schluesselVorher) + " -> " + std::to_string(st->schluesselSetup) + ", Meldung: " + mtab().meldung);
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_M, "Strg+Umschalt+M"));
    add(pause(0.4));
    add(tu("T1411 N: Animationsmodus nach Strg+Umschalt+M", [st] { st->tasteAnimation = !modellAnimSetupGewaehlt(); }));
    add(taste(ImGuiKey_K, "K (Set Key) im Animationsmodus"));
    add(pause(0.3));
    add(tu("T1411 N: Schluessel nach K im Animationsmodus", [st] {
        st->schluesselAnimation = nSchluessel();
        melde("N K im Animationsmodus: Schluessel " + std::to_string(st->schluesselAnimation));
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Shift | ImGuiKey_M, "Strg+Umschalt+M"));
    add(pause(0.3));
    add(tu("T1411 N: Setup nach Strg+Umschalt+M, Reiter Anzeige und Hierarchie", [st] {
        st->tasteSetup = modellAnimSetupGewaehlt();
        modellAnimSetupSetzen(false);
        modellFeldReiter(MbReiter::Anzeige, MbReiterBl::Daten);
    }));
    add(pause(0.4));
    add(tu("T1411 N: Gruppen im Reiter Anzeige", [st] {
        st->gruppenAnzeige = ap10::gruppen();
        modellFeldReiter(MbReiter::Hierarchie, MbReiterBl::Constraints);
    }));
    add(pause(0.4));
    add(tu("T1411 N: Gruppen im Reiter Hierarchie (Animationsmodus)", [st] {
        st->gruppenHierarchie = ap10::gruppen();
        for (int i = 0; i < modellRigAktionZahl(); ++i) {
            st->aktionDa = st->aktionDa || (std::string(modellRigAktionText(i)) == tr(Str::T1nUmschalten) && std::string(modellRigAktionTaste(i, false)) == "Ctrl+Shift+M" &&
                                            std::string(modellRigAktionTaste(i, true)) == "Tab");
        }
        melde("N Anzeige: " + st->gruppenAnzeige + " - Hierarchie: " + st->gruppenHierarchie);
        modellFeldReiter(MbReiter::Bewegung, MbReiterBl::Knochen);
        modellBedienungSetzen(1);
    }));
    add(pause(0.5));
    add(taste(ImGuiKey_Tab, "Tab (Blender: Edit Mode der Armatur)"));
    add(pause(0.4));
    add(tu("T1411 N: Blender Edit Mode", [st] { st->blEdit = modellAnimSetupGewaehlt() && std::string(modellBlModusName(ModellBereich::Animation)) == tr(Str::MbBlEditModus); }));
    add(foto("t1411_n_blender_edit"));
    add(taste(ImGuiKey_Tab, "Tab (Blender: Pose Mode)"));
    add(pause(0.4));
    add(tu("T1411 N: Blender Pose Mode, zurueck wie 3ds Max", [st] {
        st->blPose = !modellAnimSetupGewaehlt() && std::string(modellBlModusName(ModellBereich::Animation)) == tr(Str::BzModusPose);
        modellBedienungSetzen(0);
        mtab().knochenWahl = -1;
        mtab().gizmo = 0;
    }));
    add(pause(0.3));
    add(pruefe("T1411 N: nach dem Laden einer Figur mit Animation steht das Motion-Feld im Animationsmodus, oben der Schalter Setup-/Animationsmodus",
               [st] { return st->vorgabeAnimation && st->schalterDa; }));
    add(pruefe("T1411 N: Animationsmodus - Sequenz, Vorlagen, Bearbeiten, Uebertragen, IK/FK, Pose, Pfade, Kurven, Posen-Bibliothek; nichts zum Aufbauen; hoechstens 10 von 17 Gruppen",
               [st] {
                   const auto& g = st->gruppenAnimation;
                   return nIn(g, Str::AzRAnimation) && nIn(g, Str::PrRollout) && nIn(g, Str::AzRBearbeiten) && nIn(g, Str::AzRUebertragen) && nIn(g, Str::MbIkRollout) &&
                          nIn(g, Str::MbRgGruppePose) && nIn(g, Str::MbRgPfade) && nIn(g, Str::MbKvRollout) && nIn(g, Str::MbKvGruppeBibliothek) && !nIn(g, Str::AzRRig) &&
                          !nIn(g, Str::AzRBauen) && !nIn(g, Str::MbRGla) && !nIn(g, Str::MbRCfg) && !nIn(g, Str::AzRAnbauten) && !nIn(g, Str::MbRgGhosts) && !nIn(g, Str::KdGruppe) &&
                          st->zahlAnimation <= 10;
               }));
    add(pruefe("T1411 N: Setup-Modus - Rig erstellen, Anbauten, Fuer das Spiel bauen, GLA, animation.cfg; keine Gruppe zum Animieren; hoechstens die Haelfte der Gruppen",
               [st] {
                   const auto& g = st->gruppenSetup;
                   return nIn(g, Str::AzRRig) && nIn(g, Str::AzRAnbauten) && nIn(g, Str::AzRBauen) && nIn(g, Str::MbRGla) && nIn(g, Str::MbRCfg) && !nIn(g, Str::AzRAnimation) &&
                          !nIn(g, Str::MbIkRollout) && !nIn(g, Str::MbRgGruppePose) && !nIn(g, Str::MbKvRollout) && st->zahlSetup <= 8;
               }));
    add(pruefe("T1411 N: Ghosts und Knochen- und Rig-Darstellung stehen wie in 3ds Max im Reiter Anzeige; Rig erstellen bleibt im Reiter Hierarchie erreichbar",
               [st] { return nIn(st->gruppenAnzeige, Str::MbRgGhosts) && nIn(st->gruppenAnzeige, Str::KdGruppe) && nIn(st->gruppenHierarchie, Str::AzRRig); }));
    add(pruefe("T1411 N: im Setup-Modus steht die Figur in der Setup-Pose (Grundstellung der GLA), im Animationsmodus in der Pose der Sequenz",
               [st] {
                   melde("N Pose: Setup " + std::to_string(st->setupPose ? 1 : 0) + ", Animation " + std::to_string(st->posePose ? 1 : 0));
                   return st->setupPose && st->posePose;
               }));
    add(pruefe("T1411 N: im Setup-Modus kein Gizmo am Knochen, K setzt keinen Schluessel und sagt warum", [st] {
        return st->keinGizmo && st->schluesselSetup == st->schluesselVorher && st->meldungSetup;
    }));
    add(pruefe("T1411 N: im Animationsmodus setzt K den Schluessel", [st] { return st->schluesselAnimation > st->schluesselVorher; }));
    add(pruefe("T1411 N: die Statuszeile nennt den Setup-Modus", [st] { return st->statusSetup; }));
    add(pruefe("T1411 N: Strg+Umschalt+M schaltet um (beide Richtungen), die Aktion steht im Menue und in der Suche mit Taste (Blender Tab)",
               [st] { return st->tasteAnimation && st->tasteSetup && st->aktionDa; }));
    add(pruefe("T1411 N: wie Blender schaltet Tab Edit Mode / Pose Mode der Armatur, der Kopf der Ansicht nennt den Modus", [st] { return st->blEdit && st->blPose; }));
}

// --- O: Painting - die Ziel-Textur folgt der Flaeche unter dem Pinsel -------------------------------------------------------------
// Rot (vor TEST1411 O): ein Strich vom Rumpf zum Gesicht malte nur in das Bild, auf dem er begann (tupferAn lieferte -1 fuer jedes
// Dreieck eines anderen Bilds); der Schalter "Nur Ziel-Textur bemalen" stellt genau diesen Weg her und dient hier als Gegenprobe.

struct OStand {
    std::string shaderA, shaderB;
    std::vector<std::string> bemaltStrich, bemaltNachUndo, bemaltNurZiel;
    int gruppeNachStrich = -1, gruppeExplorer = -1, gruppeAlt = -1, gruppeVorschau = -1;
    int vorschauZahl = 0;
    int gebaut = -1;
    int dateien = -1;
    int strichBilder = 0;
};

std::vector<std::string> oBemalt() {
    std::vector<std::string> v;
    for (const auto& [datei, bemalt] : modellMalenBildStaende()) {
        if (bemalt) { v.push_back(datei); }
    }
    return v;
}

std::string oListe(const std::vector<std::string>& v) {
    std::string s;
    for (const std::string& x : v) { s += (s.empty() ? "" : ", ") + x; }
    return s.empty() ? "-" : s;
}

// ein Strich in der aktiven Ansicht von der Mitte der Flaeche a zur Mitte der Flaeche b (20 Bilder, linke Taste)
Schritt oStrich(const std::string& was, const char* a, const char* b) {
    auto p = std::make_shared<std::pair<ImVec2, ImVec2>>();
    return Schritt{"T1411 O: Strich " + was, [=](int k) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (k == 0) {
                           *p = {flaechenPunkt(a), flaechenPunkt(b)};
                           setzeMaus(p->first);
                           return false;
                       }
                       if (k == 2) {
                           io.AddMouseButtonEvent(0, true);
                           return false;
                       }
                       if (k >= 3 && k <= 22) {
                           const float f = static_cast<float>(k - 2) / 20.0F;
                           setzeMaus(ImVec2{p->first.x + (p->second.x - p->first.x) * f, p->first.y + (p->second.y - p->first.y) * f});
                           return false;
                       }
                       if (k == 23) {
                           io.AddMouseButtonEvent(0, false);
                           return false;
                       }
                       return k >= 27;
                   }};
}

void oSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<OStand>();
    add(tu("T1411 O: wie 3ds Max, Bereich Painting, eine Ansicht von vorn, Pinsel", [st] {
        modellBedienungSetzen(0);
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Painting;
        t.auswahl.clear();
        t.malen.nurZiel = false;
        t.malen.werkzeug = 0;
        t.malen.radiusPx = 18.0F;
        t.malen.pinsel.farbe = {1.0F, 0.0F, 0.0F, 1.0F};
        const int a = teil("torso");
        const int b = teil("head_face");
        st->shaderA = a >= 0 ? t.anzeige.surfaces[uz(a)].shader : std::string("?");
        st->shaderB = b >= 0 ? t.anzeige.surfaces[uz(b)].shader : std::string("?");
        melde("O Flaechen: torso " + st->shaderA + ", head_face " + st->shaderB);
    }));
    add(pause(0.4));
    add(tu("T1411 O: eine Ansicht von vorn", [] {
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
        mtab().ansichten[3].art = AnsichtArt::Vorne;
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(oStrich("vom Rumpf zum Gesicht", "torso", "head_face"));
    add(pause(0.3));
    add(tu("T1411 O: bemalte Bilder nach dem Strich", [st] {
        st->bemaltStrich = oBemalt();
        st->gruppeNachStrich = mtab().malen.gruppe;
        st->strichBilder = mtab().malen.strichBilder;
        melde("O Strich: bemalt " + oListe(st->bemaltStrich) + ", Ziel danach Gruppe " + std::to_string(st->gruppeNachStrich) + ", Bilder im Strich " +
              std::to_string(st->strichBilder));
    }));
    add(foto("t1411_o_strich_zwei_bilder"));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(tu("T1411 O: nach Strg+Z", [st] {
        st->bemaltNachUndo = oBemalt();
        melde("O nach Strg+Z: bemalt " + oListe(st->bemaltNachUndo));
        mtab().auswahl = {teil("torso")};   // wie ein Klick auf die Flaeche im Szenen-Explorer
    }));
    add(pause(0.3));
    add(tu("T1411 O: Ziel nach der Auswahl im Explorer", [st] {
        st->gruppeExplorer = mtab().malen.gruppe;
        mtab().auswahl.clear();
    }));
    add(klickAn("T1411 O: Alt+Klick auf das Gesicht", [] {
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, true);
        ImGui::GetIO().AddKeyEvent(ImGuiKey_LeftAlt, true);
        return flaechenPunkt("head_face");
    }));
    add(tu("T1411 O: Alt los, Ziel nach Alt+Klick", [st] {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_LeftAlt, false);
        ImGui::GetIO().AddKeyEvent(ImGuiMod_Alt, false);
        st->gruppeAlt = mtab().malen.gruppe;
        st->vorschauZahl = static_cast<int>(mtab().malen.vorschauKaesten.size());
    }));
    add(tu("T1411 O: das Befehlsfeld nach oben (Rollout Ziel mit den Vorschaubildern)", [] {
        if (ImGuiWindow* w = mbefoto::kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(pause(0.3));
    add(klickAn("T1411 O: Klick auf das Vorschaubild des Rumpfs", [st] {
        const auto& k = mtab().malen.vorschauKaesten;
        return st->gruppeExplorer >= 0 && uz(st->gruppeExplorer) < k.size() ? k[uz(st->gruppeExplorer)].GetCenter() : ImVec2{0.0F, 0.0F};
    }));
    add(pause(0.3));
    add(tu("T1411 O: Ziel nach dem Vorschaubild; Nur Ziel-Textur bemalen an", [st] {
        st->gruppeVorschau = mtab().malen.gruppe;
        mtab().malen.nurZiel = true;
        melde("O Ziel: nach dem Strich " + std::to_string(st->gruppeNachStrich) + ", Explorer " + std::to_string(st->gruppeExplorer) + ", Alt+Klick " +
              std::to_string(st->gruppeAlt) + ", Vorschaubild " + std::to_string(st->gruppeVorschau) + ", " + std::to_string(st->vorschauZahl) + " Vorschaubilder");
    }));
    add(tu("T1411 O: Maus in die Ansicht, Befehlsfeld oben", [] {
        setzeMaus(ansichtMitte());
        if (ImGuiWindow* w = mbefoto::kindFenster("mtrollouts")) { ImGui::SetScrollY(w, 0.0F); }
    }));
    add(pause(0.3));
    add(foto("t1411_o_ziel_vorschaubilder"));
    add(oStrich("vom Rumpf zum Gesicht, nur Ziel", "torso", "head_face"));
    add(pause(0.3));
    add(tu("T1411 O: bemalt mit Nur Ziel", [st] {
        st->bemaltNurZiel = oBemalt();
        melde("O Nur Ziel: bemalt " + oListe(st->bemaltNurZiel));
        mtab().malen.nurZiel = false;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(oStrich("vom Rumpf zum Gesicht (fuer das Bauen)", "torso", "head_face"));
    add(pause(0.3));
    add(tu("T1411 O: Fuer das Spiel bauen in den Laufordner", [st] {
        const std::string ordner = std::filesystem::current_path().string() + "\\t1411_o_bauen";
        std::error_code ec;
        std::filesystem::remove_all(ordner, ec);
        std::snprintf(mtab().malen.ordner, sizeof mtab().malen.ordner, "%s", ordner.c_str());
        st->gebaut = modellMalenBauen();
        int n = 0;
        for (const auto& e : std::filesystem::recursive_directory_iterator(ordner, ec)) { n += e.is_regular_file() ? 1 : 0; }
        st->dateien = n;
        mtab().malen.ordner[0] = '\0';
        melde("O gebaut: " + std::to_string(st->gebaut) + " Bilder, " + std::to_string(st->dateien) + " Dateien");
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Z, "Strg+Z"));
    add(pause(0.3));
    add(tu("T1411 O: zurueck", [] {
        mtab().ansichten[3].art = AnsichtArt::Perspektive;
        modellAnsichtEinpassenAktiv();
        mtab().bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.3));
    const auto mitName = [](const std::vector<std::string>& v, const char* teilName) {
        for (const std::string& x : v) {
            if (klein(x).find(teilName) != std::string::npos) { return true; }
        }
        return false;
    };
    add(pruefe("T1411 O: ein Strich vom Rumpf zum Gesicht malt in jedes Bild unter dem Pinsel (Rumpf, Kopf, Gesicht; Rot vorher: nur das erste)", [st, mitName] {
        return st->bemaltStrich.size() >= 2U && mitName(st->bemaltStrich, "torso") && mitName(st->bemaltStrich, "face") &&
               st->strichBilder == static_cast<int>(st->bemaltStrich.size());
    }));
    add(pruefe("T1411 O: ein Strg+Z nimmt den ganzen Strich zurueck - beide Bilder wie geladen", [st] { return st->bemaltNachUndo.empty(); }));
    add(pruefe("T1411 O: das Ziel folgt der zuletzt bemalten Flaeche, der Explorer-Klick und Alt+Klick waehlen die Textur der Flaeche", [st] {
        return st->gruppeNachStrich >= 0 && st->gruppeAlt == st->gruppeNachStrich && st->gruppeExplorer >= 0 && st->gruppeExplorer != st->gruppeNachStrich;
    }));
    add(pruefe("T1411 O: im Rollout Ziel stehen die Bilder als Vorschaubilder, ein Klick waehlt", [st] { return st->vorschauZahl >= 2 && st->gruppeVorschau == st->gruppeExplorer; }));
    add(pruefe("T1411 O: Nur Ziel-Textur bemalen haelt den Strich im Ziel (der Weg bis rc612)", [st, mitName] {
        return st->bemaltNurZiel.size() == 1U && mitName(st->bemaltNurZiel, "torso");
    }));
    add(pruefe("T1411 O: Fuer das Spiel bauen schreibt genau die bemalten Bilder (die anderen bleiben das Original)", [st] {
        return st->gebaut == static_cast<int>(st->bemaltStrich.size()) && st->dateien == st->gebaut;
    }));
}

// --- P: Weights wie der Skin-Modifier von 3ds Max ---------------------------------------------------------------------------------
// Rot (vor TEST1411 P): im Rollout Weight tool stand zuoberst "Paint weights"; wie man Ecken waehlt, stand nirgends (nur Unterobjekt-Ebene 1
// bzw. Rechtsklick), es gab keine Liste der Knochen einer Ecke, keine Weight Table, kein Copy/Paste und kein "Mirror Paste" (Foto 28).

struct PStand {
    bool eckenAn = false;
    int ecken = 0;
    int punkt = -1;
    int partner = -1;
    modell::GewichtListe liste;
    float summe = 0.0F;
    int knochen = -1;
    bool knochenGewaehlt = false;
    float gesetzt = -1.0F;
    float summeNach = 0.0F;
    bool gespiegelt = false;
    bool eingefuegt = false;
    bool tabelle = false;
    int rahmenAlle = 0;
    int rahmenVorn = 0;
    bool spielFormat = false;
    bool blender = false;
};

modell::GewichtListe pListe(int punkt) {
    const ModellGewichte& g = modellGewichte();
    if (punkt < 0 || punkt >= g.netz.zahl()) { return {}; }
    return modell::punktGewichte(mtab().dok.inhalt(), mtab().lodJetzt, g.netz.ecken[uz(punkt)].front());
}

float pIn(const modell::GewichtListe& l, int k) {
    for (const modell::KnochenGewicht& g : l) {
        if (g.knochen == k) { return g.gewicht; }
    }
    return 0.0F;
}

float pSumme(const modell::GewichtListe& l) {
    float s = 0.0F;
    for (const modell::KnochenGewicht& g : l) { s += g.gewicht; }
    return s;
}

// ein Rahmen ueber die ganze aktive Ansicht (linke Taste)
Schritt pRahmen(const std::string& was) {
    auto p = std::make_shared<std::pair<ImVec2, ImVec2>>();
    return Schritt{"T1411 P: Rahmen " + was, [=](int k) {
                       ImGuiIO& io = ImGui::GetIO();
                       if (k == 0) {
                           const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
                           *p = {ImVec2{a.lo[0] + 40.0F, a.lo[1] + 60.0F}, ImVec2{a.lo[0] + a.groesse[0] - 40.0F, a.lo[1] + a.groesse[1] - 40.0F}};
                           setzeMaus(p->first);
                           return false;
                       }
                       if (k == 2) {
                           io.AddMouseButtonEvent(0, true);
                           return false;
                       }
                       if (k >= 3 && k <= 12) {
                           const float f = static_cast<float>(k - 2) / 10.0F;
                           setzeMaus(ImVec2{p->first.x + (p->second.x - p->first.x) * f, p->first.y + (p->second.y - p->first.y) * f});
                           return false;
                       }
                       if (k == 13) {
                           io.AddMouseButtonEvent(0, false);
                           return false;
                       }
                       return k >= 17;
                   }};
}

// die Ecke von l_arm, die der Mitte ihres groessten Dreiecks am Schirm am naechsten liegt
ImVec2 pEckeLArm() {
    const int i = teil("l_arm");
    const ImVec2 m = flaechenPunkt("l_arm");
    const auto& h = modellHaut();
    ImVec2 best = m;
    float bd = 1e30F;
    if (i >= 0 && uz(i) < h.size()) {
        for (const auto& e : h[uz(i)]) {
            ImVec2 sp;
            if (!modellAufSchirm(mtab().ansichten[mtab().aktiv], e.data(), sp)) { continue; }
            const float d = (sp.x - m.x) * (sp.x - m.x) + (sp.y - m.y) * (sp.y - m.y);
            if (d < bd) {
                bd = d;
                best = sp;
            }
        }
    }
    return best;
}

void pSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<PStand>();
    add(tu("T1411 P: wie 3ds Max, Bereich Weights, eine Ansicht von vorn", [] {
        modellBedienungSetzen(0);
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Weights;
        t.eckenWahl.clear();
        t.knochenWahl = -1;
        modellGewichte().rueckseitenAus = false;
        modellGewichte().tabelleOffen = false;
    }));
    add(pause(0.4));
    add(tu("T1411 P: eine Ansicht von vorn", [] {
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
        mtab().ansichten[3].art = AnsichtArt::Vorne;
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(klick(tr(Str::T1pEcken), "mtrollouts"));
    add(pause(0.3));
    add(tu("T1411 P: Ecken an?", [st] { st->eckenAn = !modellGewichte().malen && mtab().auswahlArt == AuswahlArt::Ecke; }));
    add(klickAn("T1411 P: Klick auf eine Ecke von l_arm", pEckeLArm));
    add(pause(0.4));
    add(tu("T1411 P: gewaehlte Ecke und die Liste des Weight Tools", [st] {
        ModellGewichte& g = modellGewichte();
        modellGewichteSpiegelStand();
        st->ecken = static_cast<int>(mtab().eckenWahl.size());
        if (!mtab().eckenWahl.empty()) {
            const modell::EckOrt o = mtab().eckenWahl.front();
            if (uz(o.teil) < g.netz.punktVon.size() && uz(o.ecke) < g.netz.punktVon[uz(o.teil)].size()) { st->punkt = g.netz.punktVon[uz(o.teil)][uz(o.ecke)]; }
        }
        st->partner = st->punkt >= 0 && uz(st->punkt) < g.partner.size() ? g.partner[uz(st->punkt)] : -1;
        st->liste = g.listeGezeigt;
        st->summe = pSumme(st->liste);
        st->knochen = st->liste.empty() ? -1 : st->liste.front().knochen;
        std::string z;
        for (const modell::KnochenGewicht& kg : st->liste) { z += g_app->anim.bones[uz(kg.knochen)].name + " " + std::to_string(kg.gewicht) + "; "; }
        melde("P Ecken " + std::to_string(st->ecken) + ", Punkt " + std::to_string(st->punkt) + ", Partner " + std::to_string(st->partner) + ", Liste: " + z);
    }));
    add(foto("t1411_p_ecke_weight_tool"));
    add(klickAn("T1411 P: Klick auf den ersten Knochen der Liste", [st] {
        const Element* e = st->knochen >= 0 ? finde(g_app->anim.bones[uz(st->knochen)].name, "mtrollouts", 0, false) : nullptr;
        return e != nullptr ? e->rect.GetCenter() : ImVec2{0.0F, 0.0F};
    }));
    add(pause(0.3));
    add(tu("T1411 P: Knochen gewaehlt?", [st] { st->knochenGewaehlt = st->knochen >= 0 && mtab().knochenWahl == st->knochen; }));
    add(klickAn("T1411 P: in das Feld des Gewichts", [] {
        const Element* e = finde("##t1pwert", "mtrollouts", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{0.0F, 0.0F};
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_A, "Strg+A"));
    add(tippe("0.6"));
    add(taste(ImGuiKey_Enter, "Enter"));
    add(pause(0.3));
    add(tu("T1411 P: Gewicht nach dem Tippen", [st] {
        const modell::GewichtListe l = pListe(st->punkt);
        st->gesetzt = pIn(l, st->knochen);
        st->summeNach = pSumme(l);
        melde("P getippt: " + std::to_string(st->gesetzt) + ", Summe " + std::to_string(st->summeNach));
    }));
    add(ap3::insFeld(tr(Str::T1pSpiegelPaste)));
    add(klick(tr(Str::T1pSpiegelPaste), "mtrollouts"));
    add(pause(0.3));
    add(tu("T1411 P: Gegenseite nach Mirror Paste", [st] {
        const modell::GewichtListe a = pListe(st->punkt);
        const modell::GewichtListe b = pListe(st->partner);
        std::vector<std::string> namen;
        for (const GlaBone& k : g_app->anim.bones) { namen.push_back(k.name); }
        bool ok = st->partner >= 0 && st->partner != st->punkt && !a.empty();
        std::string z;
        for (const modell::KnochenGewicht& kg : a) {
            const int gegen = modell::spiegelKnochen(namen, kg.knochen);
            const float w = pIn(b, gegen >= 0 ? gegen : kg.knochen);
            z += namen[uz(kg.knochen)] + " " + std::to_string(kg.gewicht) + " -> " + std::to_string(w) + "; ";
            ok = ok && std::fabs(w - kg.gewicht) < 0.002F;
        }
        st->gespiegelt = ok;
        melde("P Mirror Paste: " + z);
    }));
    add(ap3::insFeld(tr(Str::T1pKopieren)));
    add(klick(tr(Str::T1pKopieren), "mtrollouts"));
    add(pause(0.2));
    add(tu("T1411 P: die Gegenseite waehlen", [st] {
        const ModellGewichte& g = modellGewichte();
        if (st->partner >= 0 && st->partner < g.netz.zahl()) { mtab().eckenWahl = {g.netz.ecken[uz(st->partner)].front()}; }
    }));
    add(pause(0.3));
    add(ap3::insFeld(tr(Str::T1pEinfuegen)));
    add(klick(tr(Str::T1pEinfuegen), "mtrollouts"));
    add(pause(0.3));
    add(tu("T1411 P: nach Paste traegt die Gegenseite dieselbe Liste", [st] {
        const modell::GewichtListe a = pListe(st->punkt);
        const modell::GewichtListe b = pListe(st->partner);
        bool ok = !a.empty() && a.size() == b.size();
        for (const modell::KnochenGewicht& kg : a) { ok = ok && std::fabs(pIn(b, kg.knochen) - kg.gewicht) < 1e-6F; }
        st->eingefuegt = ok;
    }));
    add(ap3::insFeld(tr(Str::T1pTabelle)));
    add(klick(tr(Str::T1pTabelle), "mtrollouts"));
    add(pause(0.5));
    add(tu("T1411 P: Weight Table", [st] {
        const ModellGewichte& g = modellGewichte();
        st->tabelle = g.tabelleSichtbar && g.tabelleZeilen == 1 && g.tabelleSpalten == static_cast<int>(pListe(st->partner).size());
        melde("P Weight Table: sichtbar " + std::to_string(g.tabelleSichtbar ? 1 : 0) + ", " + std::to_string(g.tabelleZeilen) + " Zeilen, " + std::to_string(g.tabelleSpalten) +
              " Spalten");
    }));
    add(foto("t1411_p_weight_table"));
    add(tu("T1411 P: Tabelle zu", [] { modellGewichte().tabelleOffen = false; }));
    add(pRahmen("ueber die ganze Ansicht"));
    add(pause(0.3));
    add(tu("T1411 P: Ecken im Rahmen, dann Backface Cull Vertices an", [st] {
        st->rahmenAlle = static_cast<int>(mtab().eckenWahl.size());
        modellGewichte().rueckseitenAus = true;
    }));
    add(pRahmen("ueber die ganze Ansicht (Rueckseiten aus)"));
    add(pause(0.3));
    add(tu("T1411 P: Ecken im Rahmen ohne Rueckseiten; Gewichte uebernehmen (Format des Spiels)", [st] {
        st->rahmenVorn = static_cast<int>(mtab().eckenWahl.size());
        modellGewichte().rueckseitenAus = false;
        melde("P Rahmen: alle " + std::to_string(st->rahmenAlle) + ", ohne Rueckseiten " + std::to_string(st->rahmenVorn));
        (void)modellUebernehmen();
    }));
    add(pause(0.3));
    add(tu("T1411 P: die Ecke im Format des Spiels", [st] {
        const ModellGewichte& g = modellGewichte();
        if (st->punkt < 0 || st->punkt >= g.netz.zahl()) { return; }
        const modell::EckOrt o = g.netz.ecken[uz(st->punkt)].front();
        const modell::Netz& n = mtab().dok.inhalt().teile[uz(o.teil)].stufen[uz(mtab().lodJetzt)];
        const modell::GewichtListe l = modell::listeAus(n.ecken[uz(o.ecke)]);
        st->spielFormat = !l.empty() && l.size() <= 4U && std::fabs(pSumme(l) - 1.0F) < 2.0F / 1023.0F && n.mal.empty();
        melde("P Format des Spiels: " + std::to_string(l.size()) + " Gewichte, Summe " + std::to_string(pSumme(l)));
        mtab().eckenWahl = {o};
        modellBedienungSetzen(1);
    }));
    add(pause(0.6));
    add(tu("T1411 P: wie Blender das Panel Vertex-Gewichte", [st] {
        st->blender = ap10::gruppe(Str::T1pBlVertexGewichte) != nullptr && !modellGewichte().listeGezeigt.empty();
    }));
    add(foto("t1411_p_blender_vertex_gewichte"));
    add(tu("T1411 P: zurueck", [] {
        modellBedienungSetzen(0);
        mtab().eckenWahl.clear();
        modellGewichteMalenSetzen(true);
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        mtab().ansichten[3].art = AnsichtArt::Perspektive;
        modellAnsichtEinpassenAktiv();
        mtab().bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.3));
    add(pruefe("T1411 P: im Rollout steht die Auswahl wie Skin (Ecken) - ein Klick waehlt eine Ecke statt zu malen", [st] { return st->eckenAn && st->ecken >= 1 && st->punkt >= 0; }));
    add(pruefe("T1411 P: die Liste des Weight Tools zeigt die Knochen der Ecke, Summe 1", [st] { return !st->liste.empty() && std::fabs(st->summe - 1.0F) < 0.002F; }));
    add(pruefe("T1411 P: Klick auf den Knochen waehlt ihn, getippt 0,6 steht an der Ecke, die Summe bleibt 1", [st] {
        return st->knochenGewaehlt && std::fabs(st->gesetzt - 0.6F) < 0.002F && std::fabs(st->summeNach - 1.0F) < 0.002F;
    }));
    add(pruefe("T1411 P: Mirror Paste - die Gegenseite traegt dieselben Gewichte auf den gespiegelten Knochen", [st] { return st->gespiegelt; }));
    add(pruefe("T1411 P: Copy/Paste uebertraegt die Gewichte auf die gewaehlte Ecke", [st] { return st->eingefuegt; }));
    add(pruefe("T1411 P: die Weight Table zeigt die gewaehlte Ecke mit ihren Knochen", [st] { return st->tabelle; }));
    add(pruefe("T1411 P: Backface Cull Vertices - der Rahmen nimmt nur die Ecken der Vorderseite", [st] { return st->rahmenVorn > 0 && st->rahmenVorn < st->rahmenAlle; }));
    add(pruefe("T1411 P: uebernommen traegt die Ecke das Format des Spiels (hoechstens 4 Gewichte, 10 Bit, Summe 1)", [st] { return st->spielFormat; }));
    add(pruefe("T1411 P: wie Blender steht das Panel Vertex-Gewichte mit der Liste der gewaehlten Ecke", [st] { return st->blender; }));
}

// --- TEST1411e: Knochenpaare des Weights-Mirror von Hand (wie die Zuordnung im Mirror Mode von 3ds Max) ---------------------------
// Rot (vor TEST1411e, am Code): die Tabelle "Bone pairs" zeigte die Paare nach den Namen nur an (Text), aendern liess sich nichts - shank
// (P): "Die Zuordnung der Knochenpaare ist sichtbar und aenderbar". Hier: der staerkste Knochen b0 einer Ecke von l_arm bekommt in seiner
// Klappliste einen anderen Partner T; danach haben sein alter Partner und der alte Partner von T keinen mehr (Befund), Mirror Paste
// schreibt das Gewicht von b0 auf der Gegenseite an T; die letzte Zeile paart zwei beliebige Knochen; "Paare nach den Namen" stellt alles
// wieder her.

struct WStand {
    int punkt = -1;
    int partner = -1;
    int b0 = -1;          // der Knochen der Ecke mit Seite und dem groessten Gewicht
    int p0 = -1;          // sein Partner nach den Namen
    int ziel = -1;        // der neue Partner von Hand
    int zielAlt = -1;     // dessen Partner nach den Namen
    float w0 = 0.0F;      // Gewicht von b0 an der Ecke
    modell::SpiegelBefund vorher;
    bool paar = false;
    bool befund = false;
    bool stern = false;
    bool paste = false;
    bool neuesPaar = false;
    bool zurueck = false;
    int zeilen = -1;
};

std::vector<std::string> wNamen() {
    std::vector<std::string> n;
    for (const GlaBone& b : g_app->anim.bones) { n.push_back(b.name); }
    return n;
}

// den Abschnitt "Bone pairs" im Rollout Mirror aufklappen, falls er zu ist (die Tabelle meldet sich mit paareBild)
Schritt wPaareAuf() {
    auto innen = std::make_shared<Schritt>();
    auto offen = std::make_shared<bool>(false);
    return {"T1411 w: Bone pairs aufklappen", [=](int b) {
                if (b == 0) {
                    *offen = modellGewichte().paareBild >= ImGui::GetFrameCount() - 2;
                    *innen = klick(tr(Str::T1pPaare), "mtrollouts");
                }
                return *offen || innen->tun(b);
            }};
}

// ein Element der Tabelle ins Bild: erst die Tabelle (ein eigenes Kindfenster mit Rollbalken), dann das Befehlsfeld darum
Schritt wInsBild(const std::string& was, const std::function<std::string()>& label) {
    return tu("T1411 w: ins Bild - " + was, [label] {
        const Element* e = finde(label(), "mtrollouts", 0, false);
        if (e == nullptr) { return; }
        ImGuiWindow* innen = ImGui::FindWindowByName(e->innen.c_str());
        ImGuiWindow* feld = mbefoto::kindFenster("mtrollouts");
        for (ImGuiWindow* w : {innen, feld}) {
            if (w == nullptr || !(w->ScrollMax.y > 0.0F)) { continue; }
            float d = 0.0F;
            if (e->rect.Max.y > w->InnerClipRect.Max.y) {
                d = e->rect.Max.y - w->InnerClipRect.Max.y + ImGui::GetFontSize() * 0.5F;
            } else if (e->rect.Min.y < w->InnerClipRect.Min.y) {
                d = e->rect.Min.y - w->InnerClipRect.Min.y - ImGui::GetFontSize() * 0.5F;
            }
            if (d != 0.0F) {
                ImGui::SetScrollY(w, std::clamp(w->Scroll.y + d, 0.0F, w->ScrollMax.y));
                return;   // je Schritt ein Fenster: das naechste Bild zeigt die neue Lage
            }
        }
    });
}

void wSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<WStand>();
    add(tu("T1411 w: wie 3ds Max, Bereich Weights, eine Ansicht von vorn, keine Paare von Hand", [] {
        modellBedienungSetzen(0);
        ModellTab& t = mtab();
        t.bereich = ModellBereich::Weights;
        t.eckenWahl.clear();
        t.knochenWahl = -1;
        modellGewichteMalenSetzen(true);   // der Klick auf "Vertices" schaltet die Auswahl der Ecken ein
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        modellKnochenPaareNachNamen();
    }));
    add(pause(0.4));
    // wie P in einem eigenen Schritt: der Wechsel des Bereichs stellt im naechsten Bild dessen Aufteilung her (Weights: vier Ansichten)
    add(tu("T1411 w: eine Ansicht von vorn", [] {
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
        mtab().ansichten[3].art = AnsichtArt::Vorne;
        modellAnsichtEinpassenAktiv();
        g_app->modelDirty = true;
    }));
    add(pause(0.6));
    add(ap3::insFeld(tr(Str::T1pEcken)));
    add(klick(tr(Str::T1pEcken), "mtrollouts"));   // wie P: die Auswahl der Ecken im Rollout
    add(pause(0.3));
    add(klickAn("T1411 w: Klick auf eine Ecke von l_arm", pEckeLArm));
    add(pause(0.4));
    add(tu("T1411 w: die Ecke, ihr staerkster Knochen mit Seite und ein anderer Partner", [st] {
        ModellGewichte& g = modellGewichte();
        modellGewichteSpiegelStand();
        if (!mtab().eckenWahl.empty()) {
            const modell::EckOrt o = mtab().eckenWahl.front();
            if (uz(o.teil) < g.netz.punktVon.size() && uz(o.ecke) < g.netz.punktVon[uz(o.teil)].size()) { st->punkt = g.netz.punktVon[uz(o.teil)][uz(o.ecke)]; }
        }
        st->partner = st->punkt >= 0 && uz(st->punkt) < g.partner.size() ? g.partner[uz(st->punkt)] : -1;
        const std::vector<std::string> namen = wNamen();
        const modell::KnochenSpiegel nachNamen(namen);
        for (const modell::KnochenGewicht& kg : pListe(st->punkt)) {
            if (nachNamen.seiteVon(kg.knochen) != 0 && nachNamen.partnerVon(kg.knochen) >= 0 && kg.gewicht > st->w0) {
                st->b0 = kg.knochen;
                st->w0 = kg.gewicht;
            }
        }
        st->p0 = nachNamen.partnerVon(st->b0);
        // der neue Partner: ein Knochen der Gegenseite mit eigenem Partner, nicht p0 und nicht b0 (rhand, sonst rfemur, rtibia)
        for (const char* n : {"rhand", "rfemur", "rtibia"}) {
            const int k = boneIndex(g_app->anim, n);
            if (k >= 0 && k != st->p0 && k != st->b0 && nachNamen.partnerVon(k) >= 0 && st->ziel < 0) { st->ziel = k; }
        }
        st->zielAlt = nachNamen.partnerVon(st->ziel);
        st->vorher = g.befund;
        melde("w Punkt " + std::to_string(st->punkt) + ", Partner " + std::to_string(st->partner) + ", b0 " + (st->b0 >= 0 ? namen[uz(st->b0)] : "-") + " " +
              std::to_string(st->w0) + ", p0 " + (st->p0 >= 0 ? namen[uz(st->p0)] : "-") + ", neu " + (st->ziel >= 0 ? namen[uz(st->ziel)] : "-") + " (bisher mit " +
              (st->zielAlt >= 0 ? namen[uz(st->zielAlt)] : "-") + "), Befund " + std::to_string(st->vorher.knochenPaare) + " / " + std::to_string(st->vorher.knochenPaareSoll));
    }));
    add(ap3::insFeld(tr(Str::T1pPaare)));
    add(wPaareAuf());
    add(pause(0.3));
    // erst die Zeile unter der Tabelle ins Bild (dann steht die ganze Tabelle im Feld - ausserhalb zeichnet sie keine Zeilen), dann in der
    // Tabelle die Zeile von b0
    add(ap3::insFeld("##paarneu1"));
    add(pause(0.2));
    const auto pk = [st] { return "##pk" + std::to_string(st->b0); };
    add(wInsBild("die Zeile von b0", pk));
    add(pause(0.2));
    add(wInsBild("die Zeile von b0", pk));
    add(pause(0.2));
    add(klickAn("T1411 w: Klappliste des Partners von b0", [st] {
        const Element* e = st->b0 >= 0 ? finde("##pk" + std::to_string(st->b0), "mtrollouts", 0, false) : nullptr;
        if (e == nullptr) { melde("w die Klappliste von b0 steht nicht im Bild"); }
        return e != nullptr ? e->rect.GetCenter() : ImVec2{0.0F, 0.0F};
    }));
    add(pause(0.3));
    {
        auto innen = std::make_shared<Schritt>();
        add({"T1411 w: den neuen Partner in der Liste waehlen", [st, innen](int b) {
                 if (b == 0) { *innen = waehleInListe(st->ziel >= 0 ? g_app->anim.bones[uz(st->ziel)].name : std::string("?"), false); }
                 return innen->tun(b);
             }});
    }
    add(pause(0.4));
    add(tu("T1411 w: Paare und Befund nach der Wahl", [st] {
        const modell::KnochenSpiegel p = modellKnochenSpiegel();
        const ModellGewichte& g = modellGewichte();
        modellGewichteSpiegelStand();
        st->paar = st->b0 >= 0 && st->ziel >= 0 && p.partnerVon(st->b0) == st->ziel && p.partnerVon(st->ziel) == st->b0 && p.partnerVon(st->p0) == -1 &&
                   p.partnerVon(st->zielAlt) == -1;
        // vorher zwei Paare (b0 - p0, Ziel - sein Partner), jetzt eines; p0 und der alte Partner des Ziels stehen ohne Partner im Soll
        st->befund = g.befund.knochenPaare == st->vorher.knochenPaare - 1 && g.befund.knochenPaareSoll == st->vorher.knochenPaareSoll + 1;
        st->zeilen = g.paareZeilen;
        melde("w nach der Wahl: Befund " + std::to_string(g.befund.knochenPaare) + " / " + std::to_string(g.befund.knochenPaareSoll) + ", Zeilen " +
              std::to_string(g.paareZeilen) + ", Meldung \"" + mtab().meldung + "\"");
    }));
    add(tu("T1411 w: das Paar traegt den Stern", [st] { st->stern = modellGewichte().paareStern == 1; }));
    add(foto("t1411e_w_paare"));
    add(ap3::insFeld(tr(Str::T1pSpiegelPaste)));
    add(klick(tr(Str::T1pSpiegelPaste), "mtrollouts"));
    add(pause(0.3));
    add(tu("T1411 w: Mirror Paste folgt dem Paar von Hand", [st] {
        const modell::GewichtListe a = pListe(st->punkt);
        const modell::GewichtListe b = pListe(st->partner);
        const modell::KnochenSpiegel p = modellKnochenSpiegel();
        bool ok = st->partner >= 0 && st->partner != st->punkt && !a.empty();
        std::string z;
        for (const modell::KnochenGewicht& kg : a) {
            const int gegen = p.partnerVon(kg.knochen);
            const float w = pIn(b, gegen >= 0 ? gegen : kg.knochen);
            z += g_app->anim.bones[uz(kg.knochen)].name + " " + std::to_string(kg.gewicht) + " -> " + std::to_string(w) + "; ";
            ok = ok && std::fabs(w - kg.gewicht) < 0.002F;
        }
        // b0 landet an seinem Partner von Hand, sein Partner nach den Namen bekommt nichts von ihm
        st->paste = ok && std::fabs(pIn(b, st->ziel) - pIn(a, st->b0)) < 0.002F && pIn(b, st->ziel) > 0.0F && pIn(b, st->p0) < 0.002F;
        melde("w Mirror Paste: " + z + " | am Ziel " + std::to_string(pIn(b, st->ziel)) + ", an p0 " + std::to_string(pIn(b, st->p0)));
    }));
    // ein neues Paar aus zwei beliebigen Knochen in der letzten Zeile: p0 (jetzt ohne Partner) mit dem alten Partner des Ziels
    add(ap3::insFeld("##paarneu1"));
    add(klickAn("T1411 w: neues Paar - erster Knochen", [] {
        const Element* e = finde("##paarneu1", "mtrollouts", 0, false);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{0.0F, 0.0F};
    }));
    add(pause(0.3));
    {
        auto innen = std::make_shared<Schritt>();
        add({"T1411 w: p0 als ersten Knochen", [st, innen](int b) {
                 if (b == 0) { *innen = waehleInListe(st->p0 >= 0 ? g_app->anim.bones[uz(st->p0)].name : std::string("?"), false); }
                 return innen->tun(b);
             }});
    }
    add(pause(0.3));
    add(klickAn("T1411 w: neues Paar - zweiter Knochen", [] {
        const Element* e = finde("##paarneu2", "mtrollouts", 0, false);
        char z[200];
        std::snprintf(z, sizeof z, "w neues Paar: erster Knochen %d, zweite Liste %s bei %.0f/%.0f%s", modellGewichte().paarNeu, e != nullptr ? "da" : "fehlt",
                      e != nullptr ? static_cast<double>(e->rect.GetCenter().x) : 0.0, e != nullptr ? static_cast<double>(e->rect.GetCenter().y) : 0.0,
                      e != nullptr && e->gesperrt ? " (gesperrt)" : "");
        melde(z);
        return e != nullptr ? e->rect.GetCenter() : ImVec2{0.0F, 0.0F};
    }));
    add(pause(0.3));
    {
        auto innen = std::make_shared<Schritt>();
        add({"T1411 w: den alten Partner des Ziels als zweiten", [st, innen](int b) {
                 if (b == 0) { *innen = waehleInListe(st->zielAlt >= 0 ? g_app->anim.bones[uz(st->zielAlt)].name : std::string("?"), false); }
                 return innen->tun(b);
             }});
    }
    add(pause(0.3));
    add(tu("T1411 w: das neue Paar steht", [st] {
        const modell::KnochenSpiegel p = modellKnochenSpiegel();
        modellGewichteSpiegelStand();
        st->neuesPaar = st->p0 >= 0 && p.partnerVon(st->p0) == st->zielAlt && p.partnerVon(st->zielAlt) == st->p0 && p.partnerVon(st->b0) == st->ziel &&
                        modellGewichte().befund.knochenPaare == st->vorher.knochenPaare && modellGewichte().handPaare.size() == 2U;
    }));
    add(ap3::insFeld(tr(Str::T1ePaareNamen)));
    add(klick(tr(Str::T1ePaareNamen), "mtrollouts"));
    add(pause(0.3));
    add(tu("T1411 w: nach den Namen wie vorher", [st] {
        const modell::KnochenSpiegel p = modellKnochenSpiegel();
        const modell::KnochenSpiegel nachNamen(wNamen());
        modellGewichteSpiegelStand();
        const ModellGewichte& g = modellGewichte();
        st->zurueck = g.handPaare.empty() && p.partner == nachNamen.partner && p.seite == nachNamen.seite && g.befund.knochenPaare == st->vorher.knochenPaare &&
                      g.befund.knochenPaareSoll == st->vorher.knochenPaareSoll;
    }));
    add(tu("T1411 w: zurueck", [] {
        mtab().eckenWahl.clear();
        modellGewichteMalenSetzen(true);
        modellAuswahlArtSetzen(AuswahlArt::Objekt);
        mtab().ansichten[3].art = AnsichtArt::Perspektive;
        modellAnsichtEinpassenAktiv();
        mtab().bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.3));
    add(pruefe("T1411 w: die Klappliste in der Tabelle paart b0 von Hand mit einem anderen Knochen, beide alten Partner stehen ohne (Befund: ein Paar weniger, "
               "einer mehr im Soll), die Zeile traegt den Stern", [st] { return st->paar && st->befund && st->stern && st->zeilen > 0; }));
    add(pruefe("T1411 w: Mirror Paste folgt dem Paar von Hand (das Gewicht von b0 steht auf der Gegenseite am neuen Partner)", [st] { return st->paste; }));
    add(pruefe("T1411 w: die letzte Zeile paart zwei beliebige Knochen", [st] { return st->neuesPaar; }));
    add(pruefe("T1411 w: Paare nach den Namen stellt die Paare und den Befund von vorher her", [st] { return st->zurueck; }));
}

// --- A: "Mission laden" ohne pk3-Dialog ------------------------------------------------------------------------------------------
// Rot (vor TEST1411 A, am Code): loadMissionArchive oeffnete den Dateidialog "Game archives (*.pk3)" - im Selbsttest unterdrueckt (wie
// Abbrechen), die Auswahl ging nicht auf; aus einer Liste ohne gewaehltes Archiv entstand kein Eintrag "zuletzt geoeffnet" (missionArchive
// leer). Grün: die Auswahl geht ohne Dialog auf, nach Archiv gruppiert, und der Eintrag traegt den vollen Pfad des Archivs der .bsp.

struct AStand {
    bool offenOhneDialog = false;
    std::vector<std::string> gruppen;
    std::vector<std::string> gruppenFilter;
    std::string filter;
    std::string mission;
    std::string archivPfad;
    std::string zuletzt;
    bool ordnerSitzung = false;
    std::vector<std::string> gruppenOrdner;
    bool nichtsEingerichtet = false;
    bool archivVorn = false;
    std::string jaBase;
};

bool aSucheFertig() { return !g_app->missionsSuche || g_app->missionsSuche->fertig(); }

// --- TEST1411e: Load mission - waehrend des Lesens springt unter der Maus nichts ------------------------------------------------
// Rot (Lauf t1411e_jk2_rot, Gruppe jk2): der Klick auf md_am_sith kam bei md_afif_sith an - die Suche im Hintergrund fuegte Karten
// sortiert in die Auswahl ein, die Zeilen rutschten. Das trifft ebenso einen Nutzer. Gewollt (Hauptchat 15.11.): unter der Maus springt
// nichts - neue Karten duerfen kommen, eine gezeigte Zeile behaelt ihren Platz, solange gelesen wird; am Ende ordnet sich die Liste
// einmal, mit der Maus darueber erst, wenn sie sie verlaesst. Hier: der ganze Spielordner kalt (Zwischenspeicher leer), die Maus auf
// die unterste ganz sichtbare spielbare Zeile, solange gelesen wird; je Bild die Zeile unter der Maus.

struct SStand {
    std::string unterMaus;      // die Zeile unter der Maus beim Ansetzen
    ImVec2 maus{};
    int bilder = 0;             // Bilder mit der Maus darueber, waehrend gelesen wurde
    std::size_t vorher = 0;     // Karten in der Auswahl beim Ansetzen
    std::size_t nachher = 0;    // ... am Ende der Suche
    int gesprungen = 0;         // Bilder, in denen eine andere Zeile unter der Maus lag
    std::string anders;         // die erste andere
    bool nachEndeStill = false; // Suche fertig, Maus noch darueber: dieselbe Zeile
    bool geordnet = false;      // Maus weg: nach Namen geordnet und in Gruppen
};

// die Beschriftung der Zeile unter der Maus (Kennung aus dem letzten Bild)
std::string sUnterMaus() {
    const Element* e = findeKennung(GImGui->HoveredIdPreviousFrame);
    return e != nullptr && e->innen.find("##missionliste") != std::string::npos ? e->label : std::string();
}

bool sGeordnet() {
    std::vector<Mission> kopie = g_app->missions;
    sortiereMissionen(kopie);
    for (std::size_t i = 0; i < kopie.size(); ++i) {
        if (kopie[i].name != g_app->missions[i].name) { return false; }
    }
    return !kopie.empty();
}

void sSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<SStand>();
    add(tu("T1411 s: kalt - Zwischenspeicher leer, Load mission ueber den ganzen Spielordner", [] {
        missionsSucheBeenden();
        g_app->missionPickOpen = false;
        g_app->missionsVerweise.clear();
        g_app->missionsVerweiseGeladen = false;
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(missionsVerweisePfad()), ec);
        g_app->missionsFilter[0] = '\0';
        missionenAusArchiv("");
    }));
    add(warteBis("T1411 s: die Liste fuellt sich, die Suche laeuft noch", [] {
        return (g_app->missions.size() >= 30U && g_app->missionsSuche && !g_app->missionsSuche->fertig()) || aSucheFertig();
    }, 6000));
    add(pause(0.2));
    add(tu("T1411 s: Maus auf die unterste ganz sichtbare spielbare Zeile", [st] {
        const Element* best = nullptr;
        for (const Element& e : g_elemente) {
            if (e.innen.find("##missionliste") == std::string::npos || e.label.empty() || !ganzSichtbar(e)) { continue; }
            const bool mission = std::any_of(g_app->missions.begin(), g_app->missions.end(), [&](const Mission& m) { return m.name == e.label && m.playable(); });
            if (mission && (best == nullptr || e.rect.Min.y > best->rect.Min.y)) { best = &e; }
        }
        if (best != nullptr) {
            st->maus = ImVec2{std::floor(best->rect.Min.x + 20.0F), std::floor(best->rect.GetCenter().y)};
            st->unterMaus = best->label;
            setzeMaus(st->maus);
        }
        st->vorher = g_app->missions.size();
        melde("s Ansatz: \"" + st->unterMaus + "\", " + std::to_string(st->vorher) + " Karten, Suche " + (aSucheFertig() ? "fertig" : "laeuft"));
    }));
    add({"T1411 s: je Bild die Zeile unter der Maus, bis die Suche fertig ist", [st](int b) {
             setzeMaus(st->maus);
             if (b >= 2) {
                 const std::string jetzt = sUnterMaus();
                 ++st->bilder;
                 if (jetzt != st->unterMaus) {
                     if (st->gesprungen == 0) { st->anders = jetzt + " (Bild " + std::to_string(b) + ", " + std::to_string(g_app->missions.size()) + " Karten)"; }
                     ++st->gesprungen;
                 }
             }
             if (aSucheFertig() || b > 12000) {
                 st->nachher = g_app->missions.size();
                 melde("s gelesen: " + std::to_string(st->bilder) + " Bilder, " + std::to_string(st->vorher) + " -> " + std::to_string(st->nachher) + " Karten, " +
                       std::to_string(st->gesprungen) + " mal eine andere Zeile" + (st->anders.empty() ? std::string() : ", zuerst " + st->anders));
                 return true;
             }
             return false;
         }});
    add(pause(0.4));
    add(tu("T1411 s: Suche fertig, die Maus noch ueber der Liste", [st] {
        st->nachEndeStill = sUnterMaus() == st->unterMaus;
        melde("s nach dem Ende: unter der Maus \"" + sUnterMaus() + "\", geordnet " + std::to_string(sGeordnet() ? 1 : 0));
    }));
    add(tu("T1411 s: die Maus verlaesst die Liste (auf das Suchfeld)", [] {
        const Element* e = finde("##missionfilter", "##missionpick", 0, false);
        if (e != nullptr) { setzeMaus(e->rect.GetCenter()); }
    }));
    add(pause(0.3));
    add(tu("T1411 s: geordnet?", [st] {
        st->geordnet = sGeordnet() && !g_app->missionsGruppen.empty();
        melde("s Maus weg: geordnet " + std::to_string(sGeordnet() ? 1 : 0) + ", Gruppen " + std::to_string(g_app->missionsGruppen.size()));
        g_app->missionPickOpen = false;
        missionsSucheBeenden();
    }));
    add(pause(0.3));
    add(pruefe("T1411 s: waehrend die Suche Karten bringt, bleibt die Zeile unter der Maus dieselbe", [st] {
        return !st->unterMaus.empty() && st->gesprungen == 0 && st->bilder >= 10 && st->nachher > st->vorher;
    }));
    add(pruefe("T1411 s: am Ende der Suche, die Maus noch ueber der Liste: dieselbe Zeile", [st] { return st->nachEndeStill; }));
    add(pruefe("T1411 s: verlaesst die Maus die Liste, ordnet sie sich (Namen, Gruppen)", [st] { return st->geordnet; }));
}

// --- TEST1411d (Rest von A): die Missionsliste aus dem Zwischenspeicher der Platte ------------------------------------------
struct ZStand {
    std::vector<std::string> frisch;        // je Mission Name | Gruppe | Archiv | .bsp | .ent | Skripte - frisch gelesen
    std::vector<std::string> ausPlatte;     // ... nach einem Neustart des Zwischenspeichers (nur die Datei)
    std::vector<std::string> nachAenderung; // ... nachdem ein Archiv eine andere Zeit hat
    std::size_t sofort = 0;                 // Karten in der Auswahl gleich nach Load mission (kalt)
    std::size_t zwischen = 0;               // die meisten Karten in der Auswahl, waehrend noch gelesen wurde
    std::size_t zwischenGruppen = 0;        // ... und ihre Gruppen
    double kaltMs = 0.0;
    double warmMs = 0.0;
    bool warmOhneSuche = false;
    std::size_t geaendert = 0;              // Karten, deren Archiv eine andere Zeit bekam
    std::size_t neuGelesen = 0;             // ... die die Hintergrundsuche danach las
    bool bereinigt = false;                 // die alte Zeit steht danach nicht mehr in der Datei
    std::string archiv;                     // das veraenderte Archiv
    std::chrono::steady_clock::time_point ab;
};

std::vector<std::string> zListe() {
    std::vector<std::string> l;
    for (const Mission& m : g_app->missions) {
        std::string z = m.name + "|" + m.gruppe + "|" + m.archivPfad + "|" + m.mapFile + "|" + m.entFile;
        for (const Mission::Script& sc : m.scripts) { z += "|" + sc.ref + ">" + sc.file + ":" + sc.key; }
        l.push_back(std::move(z));
    }
    std::sort(l.begin(), l.end());
    return l;
}

std::string zDatei() {
    std::ifstream f(::bhed::utf8Pfad(missionsVerweisePfad()), std::ios::binary);
    return std::string((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
}

void zSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<ZStand>();
    add(tu("T1411 Z: kalt - Zwischenspeicher leer (Speicher und Datei), Load mission", [st] {
        missionsSucheBeenden();
        g_app->missionPickOpen = false;
        g_app->missionsVerweise.clear();
        g_app->missionsVerweiseGeladen = false;
        std::error_code ec;
        std::filesystem::remove(::bhed::utf8Pfad(missionsVerweisePfad()), ec);
        st->ab = std::chrono::steady_clock::now();
        loadMissionArchive();
        st->sofort = g_app->missions.size();
    }));
    add(warteBis("T1411 Z: kalt gelesen (Karten schon waehrend des Lesens in der Auswahl?)", [st] {
        if (g_app->missionsSuche && !g_app->missionsSuche->fertig() && g_app->missions.size() > st->zwischen) {
            st->zwischen = g_app->missions.size();
            std::set<std::string> g;
            for (const Mission& m : g_app->missions) { g.insert(m.gruppe); }
            st->zwischenGruppen = g.size();
        }
        if (!aSucheFertig()) { return false; }
        st->kaltMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - st->ab).count();
        return true;
    }, 9000));
    add(pause(0.5));   // das Bild uebernimmt das Ergebnis (missionsSucheUebernehmen) und sichert die Datei
    add(tu("T1411 Z: frische Liste, warm - nur die Datei (Speicher leer), Load mission", [st] {
        st->frisch = zListe();
        missionsSucheBeenden();
        g_app->missionPickOpen = false;
        g_app->missionsVerweise.clear();
        g_app->missionsVerweiseGeladen = false;
        st->ab = std::chrono::steady_clock::now();
        loadMissionArchive();
        st->warmOhneSuche = !g_app->missionsSuche;
        st->warmMs = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - st->ab).count();
        st->ausPlatte = zListe();
    }));
    add(pause(0.3));
    add(tu("T1411 Z: ein Archiv bekommt in der Datei eine andere Zeit (wie eine neu gepackte pk3), Load mission", [st] {
        std::string text = zDatei();
        // das Archiv der ersten Karte mit Skripten (Schluessel "K<Tab>pfad@zeit|...")
        for (const Mission& m : g_app->missions) {
            if (m.playable()) {
                st->archiv = m.archivPfad;
                break;
            }
        }
        std::string esc;   // so steht der Pfad in der Datei (Backslash verdoppelt, missionsVerweiseText)
        for (const char c : st->archiv) { esc += c == '\\' ? std::string("\\\\") : std::string(1, c); }
        st->archiv = esc;
        const std::string such = "K\t" + esc + "@";
        std::string neu;
        std::size_t ab = 0;
        for (std::size_t i = text.find(such); i != std::string::npos; i = text.find(such, ab)) {
            const std::size_t zeit = i + such.size();
            const std::size_t bis = text.find('|', zeit);
            neu += text.substr(ab, zeit - ab) + "1";   // Zeit 1: kein Archiv hat sie
            ab = bis;
            ++st->geaendert;
        }
        neu += text.substr(ab);
        std::string fehler;
        (void)::bhed::sicherSchreiben(missionsVerweisePfad(), neu, &fehler);
        missionsSucheBeenden();
        g_app->missionPickOpen = false;
        g_app->missionsVerweise.clear();
        g_app->missionsVerweiseGeladen = false;
        loadMissionArchive();
        st->neuGelesen = g_app->missionsSuche ? g_app->missionsSuche->gesamt() : 0U;
    }));
    add(warteBis("T1411 Z: die Karten des veraenderten Archivs neu gelesen", [] { return aSucheFertig(); }, 6000));
    add(pause(0.5));
    add(tu("T1411 Z: Liste und Datei nach der Aenderung", [st] {
        st->nachAenderung = zListe();
        st->bereinigt = zDatei().find("K\t" + st->archiv + "@1|") == std::string::npos;
        g_app->missionPickOpen = false;
        char z[400];
        std::snprintf(z, sizeof z, "Z Zwischenspeicher: %zu Karten, kalt %.0f ms (sofort %zu, waehrend des Lesens bis %zu in %zu Gruppen), warm %.1f ms "
                      "(ohne Suche %d), %s: %zu Schluessel mit anderer Zeit -> %zu neu gelesen, bereinigt %d", st->frisch.size(), st->kaltMs, st->sofort,
                      st->zwischen, st->zwischenGruppen, st->warmMs, st->warmOhneSuche ? 1 : 0, st->archiv.c_str(), st->geaendert, st->neuGelesen,
                      st->bereinigt ? 1 : 0);
        melde(z);
    }));
    add(pruefe("T1411 Z: die Liste aus dem Zwischenspeicher der Platte ist gleich der frisch gelesenen, ohne Hintergrundsuche", [st] {
        return !st->frisch.empty() && st->ausPlatte == st->frisch && st->warmOhneSuche;
    }));
    add(pruefe("T1411 Z: eine veraenderte pk3 (andere Zeit) wird neu gelesen - genau ihre Karten, danach dieselbe Liste, die alte Zeit ist aus der Datei",
               [st] { return st->geaendert > 0U && st->neuGelesen == st->geaendert && st->nachAenderung == st->frisch && st->bereinigt; }));
    add(pruefe("T1411 Z: waehrend des Lesens stehen schon Karten in ihren Gruppen in der Auswahl (nicht erst am Ende)",
               [st] { return st->zwischen > st->sofort && st->zwischenGruppen > 0U && st->zwischen < st->frisch.size(); }));
}

void aSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<AStand>();
    add(tu("T1411 A: Map-Tab, Load mission (Menue)", [st] {
        g_app->missionPickOpen = false;
        g_app->settings.recentMissions.clear();
        g_app->leftMode = 1;
        loadMissionArchive();
        st->offenOhneDialog = g_app->missionPickOpen && g_app->missionArchive.empty();
    }));
    add(warteBis("T1411 A: Missionen des Spiels gelesen", [] { return aSucheFertig(); }, 6000));
    add(pause(0.6));
    add(tu("T1411 A: Gruppen", [st] {
        st->gruppen = g_app->missionsGruppen;
        std::string z;
        for (const std::string& g : st->gruppen) { z += g + ", "; }
        melde("A Gruppen (" + std::to_string(st->gruppen.size()) + "): " + z + " - " + std::to_string(g_app->missions.size()) + " Karten");
        // der Filter auf einen Teil des Archivnamens der zweiten Gruppe (ohne Endung)
        if (st->gruppen.size() >= 2U) {
            std::string f = st->gruppen[1];
            const std::size_t p = f.find_last_of('.');
            if (p != std::string::npos) { f.resize(p); }
            st->filter = f.size() > 4U ? f.substr(f.size() - 4U) : f;
        }
    }));
    add(foto("t1411_a_missionen_gruppen"));
    add(tu("T1411 A: Filter auf den Archivnamen", [st] { std::snprintf(g_app->missionsFilter, sizeof g_app->missionsFilter, "%s", st->filter.c_str()); }));
    add(pause(0.5));
    add(tu("T1411 A: Gruppen mit Filter, Filter leer, erste Mission der ersten Gruppe", [st] {
        st->gruppenFilter = g_app->missionsGruppen;
        melde("A Filter \"" + st->filter + "\": " + std::to_string(st->gruppenFilter.size()) + " Gruppen" + (st->gruppenFilter.empty() ? std::string() : ", " + st->gruppenFilter.front()));
        g_app->missionsFilter[0] = '\0';
        for (const Mission& m : g_app->missions) {
            if (!st->gruppen.empty() && m.playable() && m.gruppe == st->gruppen.front()) {
                st->mission = m.name;
                st->archivPfad = m.archivPfad;
                break;
            }
        }
        melde("A Mission " + st->mission + " aus " + st->archivPfad);
    }));
    add(pause(0.3));
    add(tu("T1411 A: Mission in der Liste waehlen", [st] {
        if (g_app->missionPickOpen && !st->mission.empty()) { schritteEinfuegen({waehleInListe(st->mission, false, 0, "##missionpick")}); }
    }));
    add(warteBis("T1411 A: Mission geladen", [] { return !g_app->map.empty() && g_app->camTrackValid; }, 3000));
    add(tu("T1411 A: zuletzt geoeffnet", [st] {
        st->zuletzt = g_app->settings.recentMissions.empty() ? std::string() : g_app->settings.recentMissions.front();
        melde("A zuletzt geoeffnet: " + st->zuletzt);
        // Ordner oeffnen: ein Ordner, der nicht eingerichtet ist (JKA base), mit der Antwort des ersetzten Ordnerdialogs
        const char* ja = std::getenv("BHED_JA_PFAD");
        st->jaBase = ja != nullptr && *ja != '\0' ? std::string(ja) : std::string("C:\\Program Files (x86)\\Steam\\steamapps\\common\\Jedi Academy\\GameData\\base");
        std::error_code ec;
        if (!std::filesystem::is_directory(std::filesystem::path(st->jaBase), ec)) { st->jaBase.clear(); }
        if (!st->jaBase.empty()) {
            platform::dialogAntwortImSelbsttest(st->jaBase);
            missionSpielordnerOeffnen();
        }
    }));
    add(warteBis("T1411 A: Missionen des Ordners gelesen", [] { return aSucheFertig(); }, 6000));
    add(pause(0.5));
    add(tu("T1411 A: nach Ordner oeffnen", [st] {
        st->ordnerSitzung = st->jaBase.empty() || (!g_app->sitzungsSpielordner.empty() && g_app->missionPickOpen);
        st->gruppenOrdner = g_app->missionsGruppen;
        melde("A Ordner " + (st->jaBase.empty() ? std::string("(JKA nicht da - uebersprungen)") : st->jaBase) + ": Sitzung " +
              std::to_string(g_app->sitzungsSpielordner.size()) + " Ordner, " + std::to_string(st->gruppenOrdner.size()) + " Gruppen");
        // zurueck zu den eingerichteten Ordnern
        setzeSitzungsSpielordner(g_app->settings.gamePaths);
        g_app->missionPickOpen = false;
        missionsSucheBeenden();
    }));
    add(pause(0.5));
    {
        auto weg = std::make_shared<std::vector<GamePath>>();
        add(tu("T1411 A: Spielordner beiseite, Load mission", [weg] {
            std::swap(g_app->gamePaths, *weg);
            loadMissionArchive();
        }));
        add(pause(0.5));
        add(tu("T1411 A: Hinweis und drei Knoepfe", [st, weg] {
            st->nichtsEingerichtet = g_app->missionPickOpen && g_app->missions.empty() && finde(tr(Str::T1aEinrichten), "", 0, false) != nullptr &&
                                     finde(tr(Str::T1aOrdnerOeffnen), "", 0, false) != nullptr && finde(tr(Str::T1aArchivOeffnen), "", 0, false) != nullptr;
        }));
        add(foto("t1411_a_nichts_eingerichtet"));
        add(tu("T1411 A: Spielordner zurueck, Archiv oeffnen (ersetzter Dateidialog)", [st, weg] {
            std::swap(g_app->gamePaths, *weg);
            if (!st->archivPfad.empty()) { platform::dialogAntwortImSelbsttest(st->archivPfad); }
        }));
    }
    add(klick(tr(Str::T1aArchivOeffnen)));
    add(warteBis("T1411 A: Missionen gelesen", [] { return aSucheFertig(); }, 6000));
    add(pause(0.5));
    add(tu("T1411 A: das Archiv steht vorn", [st] {
        st->archivVorn = g_app->missionPickOpen && !g_app->missions.empty() && g_app->missions.front().imGewaehlten && g_app->missionArchive == st->archivPfad;
        g_app->missionPickOpen = false;
        missionsSucheBeenden();
        g_app->leftMode = 2;
    }));
    add(pause(0.5));
    add(pruefe("T1411 A: Load mission oeffnet ohne Dateidialog die Missionen des eingerichteten Spiels (kein Archiv gewaehlt)", [st] { return st->offenOhneDialog; }));
    add(pruefe("T1411 A: die Missionen stehen nach Archiv gruppiert (mindestens zwei Gruppen)", [st] { return st->gruppen.size() >= 2U; }));
    add(pruefe("T1411 A: der Filter trifft den Archivnamen", [st] {
        return !st->filter.empty() && !st->gruppenFilter.empty() && st->gruppenFilter.size() < st->gruppen.size() &&
               std::find(st->gruppenFilter.begin(), st->gruppenFilter.end(), st->gruppen[1]) != st->gruppenFilter.end();
    }));
    add(pruefe("T1411 A: aus der Liste geladen steht die Mission in zuletzt geoeffnet - mit dem vollen Pfad des Archivs ihrer .bsp", [st] {
        return !st->mission.empty() && st->zuletzt == "pk3|" + st->archivPfad + "|" + st->mission && st->archivPfad.find_first_of("/\\") != std::string::npos;
    }));
    add(pruefe("T1411 A: Ordner oeffnen (ersetzter Dialog) zeigt die Missionen des Ordners, nur diese Sitzung", [st] { return st->ordnerSitzung; }));
    add(pruefe("T1411 A: ohne Spielordner geht die Auswahl trotzdem auf - Hinweis und Ordner oeffnen, Archiv oeffnen, Spielordner einrichten", [st] {
        return st->nichtsEingerichtet;
    }));
    add(pruefe("T1411 A: Archiv oeffnen (ersetzter Dialog) - die Missionen des Archivs stehen vorn", [st] { return st->archivVorn; }));
}

// --- E: Sequenzen als eigener Reiter mit Suche und Spalten ----------------------------------------------------------------------
// Rot (vor TEST1411 E): die Sequenzen (MD: 2635) waren ein Ast im Baum unter Flaechen/Tags/Knochen, ohne eigene Suche; "SBD_RUNBACK_NORMAL
// 15 · l..." abgeschnitten (Foto von shank). ap2 X9 pruefte genau diesen Ast (Zeilen des Baums > Zahl der Sequenzen).

struct EStand {
    int baumZeilen = 0;
    int baumSequenzen = 0;
    bool reiter = false;
    int treffer = 0;
    bool alleTreffen = false;
    bool nurBoth = false;
    bool sortiert = false;
    int geklickt = -1;
    bool menue = false;
    bool zeigen = false;
};

std::vector<const ExplorerLage*> eZeilen() {
    std::vector<const ExplorerLage*> v;
    for (const ExplorerLage& l : modellExplorer().lage) {
        if (l.art == ExplorerArt::Sequenz) { v.push_back(&l); }
    }
    return v;
}

void eSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<EStand>();
    add(tu("T1411 E: wie 3ds Max, View/Check, Explorer nach Hierarchie", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        modellExplorer().art = 0;
        modellExplorer().seqGruppe = 0;
        modellExplorer().seqSuche[0] = '\0';
    }));
    add(pause(0.5));
    add(tu("T1411 E: der Baum ohne den Ast der Sequenzen", [st] {
        st->baumZeilen = modellExplorer().zeilen;
        st->baumSequenzen = static_cast<int>(eZeilen().size());
        melde("E Baum: " + std::to_string(st->baumZeilen) + " Zeilen, " + std::to_string(st->baumSequenzen) + " Sequenzzeilen, " + std::to_string(g_app->animList.size()) +
              " Sequenzen");
    }));
    add(klick(tr(Str::T1eSequenzen), "mtexplorer"));
    add(pause(0.4));
    add(tu("T1411 E: Reiter Sequenzen, Maus ueber den Explorer", [st] {
        st->reiter = modellExplorer().art == 2;
        if (ImGuiWindow* w = mbefoto::kindFenster("mtexplorer")) { setzeMaus(ImVec2{w->InnerRect.GetCenter().x, w->InnerRect.Max.y - 40.0F}); }
    }));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_F, "Strg+F"));
    add(pause(0.2));
    add(tippe("walk"));
    add(pause(0.5));
    add(tu("T1411 E: Treffer der Suche", [st] {
        st->treffer = modellExplorer().seqZeilen;
        bool ok = st->treffer > 0;
        for (const ExplorerLage* l : eZeilen()) { ok = ok && klein(l->name).find("walk") != std::string::npos; }
        st->alleTreffen = ok && !eZeilen().empty();
        melde("E Suche walk: " + std::to_string(st->treffer) + " Treffer, " + std::to_string(eZeilen().size()) + " gezeichnet");
        modellExplorer().seqGruppe = 1;
    }));
    add(pause(0.4));
    add(tu("T1411 E: Gruppe BOTH", [st] {
        bool ok = !eZeilen().empty();
        for (const ExplorerLage* l : eZeilen()) { ok = ok && l->name.rfind("BOTH_", 0) == 0; }
        st->nurBoth = ok;
    }));
    add(foto("t1411_e_sequenzen_liste"));
    add(klick(tr(Str::T1eBilder), "mtexplorer"));
    add(pause(0.4));
    add(tu("T1411 E: nach Bildern sortiert", [st] {
        const auto v = eZeilen();
        bool auf = true;
        bool ab = true;
        for (std::size_t k = 1; k < v.size(); ++k) {
            const int a = g_app->animList[uz(v[k - 1]->index)].numFrames;
            const int b = g_app->animList[uz(v[k]->index)].numFrames;
            auf = auf && a <= b;
            ab = ab && a >= b;
        }
        st->sortiert = v.size() >= 2U && (auf || ab);
    }));
    add(klickAn("T1411 E: Klick auf die zweite Zeile", [st] {
        const auto v = eZeilen();
        if (v.size() < 2U) { return ImVec2{0.0F, 0.0F}; }
        st->geklickt = v[1]->index;
        return ImVec2{v[1]->xName + 20.0F, (v[1]->y0 + v[1]->y1) * 0.5F};
    }));
    add(pause(0.3));
    add(klickAn("T1411 E: Rechtsklick auf dieselbe Zeile", [st] {
        for (const ExplorerLage* l : eZeilen()) {
            if (l->index == st->geklickt) { return ImVec2{l->xName + 20.0F, (l->y0 + l->y1) * 0.5F}; }
        }
        return ImVec2{0.0F, 0.0F};
    }, 1));
    add(pause(0.3));
    add(tu("T1411 E: Menue der Sequenz", [st] {
        st->menue = finde(tr(Str::MbAlsPrimaer), "", 0, false) != nullptr;
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, true);
    }));
    add(tu("T1411 E: Esc los, Suche leer, zur gewaehlten Sequenz", [st] {
        ImGui::GetIO().AddKeyEvent(ImGuiKey_Escape, false);
        modellExplorer().seqSuche[0] = '\0';
        modellExplorer().seqGruppe = 0;
        modellExplorer().art = 0;
        modellExplorerZeigen(ExplorerArt::Sequenz, g_app->animIndex);
    }));
    add(pause(0.5));
    add(tu("T1411 E: die Sequenz steht im Bild", [st] {
        // die Zeile steht sichtbar im Explorer (nicht nur ausserhalb mitgezeichnet)
        const ImGuiWindow* w = mbefoto::kindFenster("mtexplorer");
        for (const ExplorerLage* l : eZeilen()) {
            st->zeigen = st->zeigen || (l->index == g_app->animIndex && w != nullptr && l->y0 >= w->InnerRect.Min.y && l->y1 <= w->InnerRect.Max.y);
        }
        st->zeigen = st->zeigen && modellExplorer().art == 2;
        modellExplorer().art = 0;
    }));
    add(pause(0.3));
    add(pruefe("T1411 E: der Baum verliert den langen Ast - ein Eintrag Sequenzen, keine Sequenzzeile", [st] {
        return st->baumSequenzen == 0 && st->baumZeilen < static_cast<int>(g_app->animList.size());
    }));
    add(pruefe("T1411 E: Reiter Sequenzen neben By hierarchy | By layers; Strg+F und Tippen filtern sofort (Teilwort, ohne Gross/Klein)", [st] {
        return st->reiter && st->alleTreffen;
    }));
    add(pruefe("T1411 E: die Gruppe (BOTH) bleibt", [st] { return st->nurBoth; }));
    add(pruefe("T1411 E: Klick auf den Kopf Bilder sortiert", [st] { return st->sortiert; }));
    add(pruefe("T1411 E: Klick spielt die Sequenz, Rechtsklick zeigt das Menue wie bisher", [st] { return st->geklickt >= 0 && g_app->animIndex == st->geklickt && st->menue; }));
    add(pruefe("T1411 E: Zeigen (Laden, NPC-Browser) springt in der Liste zur Sequenz", [st] { return st->zeigen; }));
}

// --- TEST1411e: der Mauspunkt auf dem Raster in der Statuszeile (wie 3ds Max) ------------------------------------------------
// shank (G): in der Mitte der Statuszeile "Koordinaten X/Y/Z der Auswahl bzw. des Mauspunkts". Ein Weltpunkt der Rasterebene einer
// Ansicht (oben und Perspektive: XY in Bodenhoehe, vorne XZ, links YZ) wird auf den Schirm gerechnet (modellAufSchirm, die Matrix des
// letzten Bilds), die Maus dorthin gesetzt - die Statuszeile muss ihn wieder nennen. ImGui setzt die Maus auf ganze Bildpunkte
// (AddMousePosEvent: ImFloorSigned, erster Lauf t1411e_sxw_rot bis 0,05 daneben): erwartet wird darum der Punkt der Ebene unter dem
// abgerundeten Bildpunkt, ueber die Ableitung der Abbildung am Weltpunkt (je eine Einheit entlang der beiden Achsen der Ebene).

struct XyzStand {
    float abweichung = 0.0F;   // groesste Abweichung der Anzeige vom Weltpunkt
    int geprueft = 0;
    int faelle = 0;
    std::string vorher;        // der Text, bevor die Maus die Ansichten verlaesst
    bool bleibt = false;
    bool auswahl = false;
    bool wieder = false;
    bool knochenLeer = false;  // ein gewaehlter Knochen ohne Mitte (Bereich Ansehen): die Felder leer wie bisher, kein Mauspunkt
};

bool xyzLesen(const std::string& text, float v[3]) { return std::sscanf(text.c_str(), "X: %f Y: %f Z: %f", &v[0], &v[1], &v[2]) == 3; }

// Den Weltpunkt `p` (die Achse der Ebene setzt der Fall) in Ansicht n anfahren und die Anzeige mit ihm vergleichen
void xyzFall(std::vector<Schritt>& s, const std::shared_ptr<XyzStand>& st, const std::string& name, int n, float u, float v) {
    auto ziel = std::make_shared<std::array<float, 3>>();
    auto da = std::make_shared<bool>(false);
    s.push_back(tu("T1411 x: " + name + " - Maus auf den Rasterpunkt", [=] {
        const ModellAnsicht& a = mtab().ansichten[n];
        const int k = modellRasterNormale(a.art);
        const int au = k == 0 ? 1 : 0;   // die beiden Achsen der Ebene wie rasterFuer
        const int av = k == 2 ? 1 : 2;
        (*ziel)[uz(k)] = k == 2 ? mtab().an.bodenZ : 0.0F;
        (*ziel)[uz(au)] = u;
        (*ziel)[uz(av)] = v;
        ImVec2 q;
        ImVec2 qu;
        ImVec2 qv;
        std::array<float, 3> pu = *ziel;
        std::array<float, 3> pv = *ziel;
        pu[uz(au)] += 1.0F;
        pv[uz(av)] += 1.0F;
        *da = modellAufSchirm(a, ziel->data(), q) && modellAufSchirm(a, pu.data(), qu) && modellAufSchirm(a, pv.data(), qv) && q.x > a.lo[0] &&
              q.x < a.lo[0] + a.groesse[0] && q.y > a.lo[1] && q.y < a.lo[1] + a.groesse[1];
        const ImVec2 m{std::floor(q.x), std::floor(q.y)};
        // m - q = J (du, dv), J = [qu - q, qv - q]
        const float j00 = qu.x - q.x;
        const float j10 = qu.y - q.y;
        const float j01 = qv.x - q.x;
        const float j11 = qv.y - q.y;
        const float det = j00 * j11 - j01 * j10;
        if (*da && std::fabs(det) > 1e-6F) {
            const float dx = m.x - q.x;
            const float dy = m.y - q.y;
            (*ziel)[uz(au)] += (j11 * dx - j01 * dy) / det;
            (*ziel)[uz(av)] += (-j10 * dx + j00 * dy) / det;
            setzeMaus(m);
        } else {
            *da = false;
        }
        ++st->faelle;
        char z[220];
        std::snprintf(z, sizeof z, "x %s: Weltpunkt unter dem Bildpunkt %.0f %.0f: %.4f %.4f %.4f (%s)", name.c_str(), static_cast<double>(m.x),
                      static_cast<double>(m.y), static_cast<double>((*ziel)[0]), static_cast<double>((*ziel)[1]), static_cast<double>((*ziel)[2]),
                      *da ? "in der Ansicht" : "AUSSERHALB");
        melde(z);
    }));
    s.push_back(pause(0.2));
    s.push_back(tu("T1411 x: " + name + " - Anzeige lesen", [=] {
        float w[3];
        const std::string text = zp().koordText;
        if (!*da || !xyzLesen(text, w)) {
            melde("x " + name + ": Anzeige \"" + text + "\" nicht lesbar");
            return;
        }
        float d = 0.0F;
        for (std::size_t k = 0; k < 3; ++k) { d = std::max(d, std::fabs(w[k] - (*ziel)[k])); }
        st->abweichung = std::max(st->abweichung, d);
        ++st->geprueft;
        char z[160];
        std::snprintf(z, sizeof z, "x %s: Anzeige \"%s\", Abweichung %.5f", name.c_str(), text.c_str(), static_cast<double>(d));
        melde(z);
    }));
}

void xSchritte(std::vector<Schritt>& s) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<XyzStand>();
    const auto einrichten = [](bool vier, AnsichtArt art) {
        return [vier, art] {
            modellBedienungSetzen(0);
            ModellTab& t = mtab();
            t.bereich = ModellBereich::Ansehen;
            t.auswahlArt = AuswahlArt::Objekt;
            t.auswahl.clear();
            t.vierAnsichten = vier;
            if (vier) {
                const AnsichtArt arten[4] = {AnsichtArt::Oben, AnsichtArt::Vorne, AnsichtArt::Links, AnsichtArt::Perspektive};
                for (int n = 0; n < 4; ++n) { t.ansichten[n].art = arten[n]; }
            } else {
                t.aktiv = 3;
                t.ansichten[3].art = art;
                modellAnsichtEinpassenAktiv();
            }
            g_app->modelDirty = true;
        };
    };
    const struct {
        const char* name;
        AnsichtArt art;
        float u;
        float v;
    } kEine[4] = {{"oben", AnsichtArt::Oben, 13.37F, -21.53F},
                  {"vorne", AnsichtArt::Vorne, 13.37F, 20.29F},
                  {"links", AnsichtArt::Links, -13.37F, 20.29F},
                  {"Perspektive", AnsichtArt::Perspektive, 2.37F, -1.53F}};
    for (const auto& f : kEine) {
        add(tu(std::string("T1411 x: eine Ansicht ") + f.name + ", wie 3ds Max, nichts gewaehlt", einrichten(false, f.art)));
        add(pause(0.5));
        xyzFall(s, st, std::string("eine Ansicht ") + f.name, 3, f.u, f.v);
    }
    // vier Ansichten: es zaehlt die Ansicht unter der Maus
    add(tu("T1411 x: vier Ansichten (oben, vorne, links, Perspektive)", einrichten(true, AnsichtArt::Perspektive)));
    add(pause(0.3));
    for (int n = 0; n < 3; ++n) {
        add(tu("T1411 x: vier Ansichten - Ansicht " + std::to_string(n) + " einpassen", [n] {
            mtab().aktiv = n;
            modellAnsichtEinpassenAktiv();
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
    }
    xyzFall(s, st, "vier Ansichten oben", 0, -9.71F, 15.13F);
    xyzFall(s, st, "vier Ansichten vorne", 1, -9.71F, 15.13F);
    xyzFall(s, st, "vier Ansichten links", 2, 9.71F, -7.43F);
    // die Maus verlaesst die Ansichten (ueber die Statuszeile): der letzte Punkt bleibt stehen
    add(tu("T1411 x: Maus auf die Statuszeile", [st] {
        st->vorher = zp().koordText;
        const ModellZeitProbe& p = zp();
        setzeMaus(ImVec2{std::floor(p.status[0] + p.status[2] * 0.5F), std::floor(p.status[1] + p.status[3] * 0.5F)});   // merke: x, y, Breite, Hoehe
    }));
    add(pause(0.2));
    add(tu("T1411 x: der letzte Punkt bleibt", [st] {
        st->bleibt = !st->vorher.empty() && zp().koordText == st->vorher;
        melde("x Statuszeile: vorher \"" + st->vorher + "\", jetzt \"" + zp().koordText + "\"");
    }));
    add(foto("t1411e_x_status"));
    // mit Auswahl: die Mitte der Auswahl wie bisher, auch mit der Maus in einer Ansicht
    add(tu("T1411 x: torso waehlen, Maus in die Ansicht vorne", [] {
        mtab().auswahl = {teil("torso")};
        const ModellAnsicht& a = mtab().ansichten[1];
        setzeMaus(ImVec2{std::floor(a.lo[0] + a.groesse[0] * 0.3F), std::floor(a.lo[1] + a.groesse[1] * 0.7F)});
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(tu("T1411 x: Anzeige = Mitte der Auswahl", [st] {
        float m[3];
        char soll[160] = "";
        if (modellAuswahlMitteJetzt(m)) {
            std::snprintf(soll, sizeof soll, "X: %.2f   Y: %.2f   Z: %.2f", static_cast<double>(m[0]), static_cast<double>(m[1]), static_cast<double>(m[2]));
        }
        st->auswahl = soll[0] != '\0' && zp().koordText == soll;
        melde(std::string("x Auswahl: soll \"") + soll + "\", Anzeige \"" + zp().koordText + "\"");
    }));
    add(tu("T1411 x: Auswahl weg", [] {
        mtab().auswahl.clear();
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(tu("T1411 x: ohne Auswahl wieder der Mauspunkt", [st] {
        float w[3];
        st->wieder = xyzLesen(zp().koordText, w) && zp().koordText != st->vorher;
        melde("x wieder: \"" + zp().koordText + "\"");
    }));
    // rot (Handbuchbild h_ap1_pruefen aus Bau bau_e10): "Bone rhand Selected" links, in der Mitte der Mauspunkt - als laege dort der Knochen
    add(tu("T1411 x: einen Knochen waehlen (Bereich Ansehen: keine Mitte)", [] {
        mtab().knochenWahl = boneIndex(g_app->anim, "rhand");
        g_app->modelDirty = true;
    }));
    add(pause(0.2));
    add(tu("T1411 x: mit dem Knochen die Felder leer", [st] {
        float m[3];
        st->knochenLeer = mtab().knochenWahl >= 0 && !modellAuswahlMitteJetzt(m) && zp().koordText == "X:    Y:    Z:";
        melde("x Knochen ohne Mitte: \"" + zp().koordText + "\", links \"" + zp().auswahlText + "\"");
        mtab().knochenWahl = -1;
    }));
    add(pruefe("T1411 x: ohne Auswahl nennt die Statuszeile den Mauspunkt auf dem Raster (oben, vorne, links, Perspektive, vier Ansichten) "
               "auf 0,01 genau (zwei Nachkommastellen)", [st] {
        char z[120];
        std::snprintf(z, sizeof z, "x: %d von %d Faellen gelesen, groesste Abweichung %.5f", st->geprueft, st->faelle, static_cast<double>(st->abweichung));
        melde(z);
        return st->faelle == 7 && st->geprueft == st->faelle && st->abweichung <= 0.0051F;
    }));
    add(pruefe("T1411 x: ausserhalb der Ansichten bleibt der letzte Punkt stehen", [st] { return st->bleibt; }));
    add(pruefe("T1411 x: mit Auswahl die Mitte der Auswahl wie bisher, danach wieder der Mauspunkt", [st] { return st->auswahl && st->wieder; }));
    add(pruefe("T1411 x: ein gewaehlter Knochen ohne Mitte (Bereich Ansehen) laesst die Felder leer wie bisher - der Mauspunkt nur ohne Auswahl",
               [st] { return st->knochenLeer; }));
    add(tu("T1411 x: zurueck", [] {
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
        mtab().ansichten[3].art = AnsichtArt::Perspektive;
        g_app->modelDirty = true;
    }));
    add(pause(0.3));
}

// --- B: der Model-Tab ohne Modell wie mit Modell -------------------------------------------------------------------------------
// Rot (vor TEST1411 B, Foto 15 von shank): ohne Modell links nur "No model yet..." mit "Show me how", rechts eine leere Flaeche mit "Load a
// model" - kein Raster, kein ViewCube, keine Kopfzeile der Ansicht, kein Befehlsfeld. Soll (Vorbild die leere Szene in 3ds Max, Foto 21):
// je Bereich und Bedienmodus dieselben Rahmen wie mit Modell (Explorer, Ansichten, Befehlsfeld, UV-Editor bzw. 2D-Bild), im Befehlsfeld
// dieselben belegten Reiter und im vorgegebenen Reiter dieselben Gruppen - ohne Modell zu und gesperrt, mit einem kurzen Hinweis; die Ansicht
// mit ViewCube und Kopfzeile, der Ladehinweis mittig mit "NPC-Browser" und ".glm oeffnen"; der Explorer mit Reitern, Suche und Filter,
// gesperrt, der Hinweis und "Show me how" im leeren Baum.

struct BLage {
    bool da = false;
    ImRect explorer;
    ImRect ansichten;
    ImRect feld;
    ImRect editor;          // UV-Editor bzw. 2D-Bild (leer: keiner)
    unsigned belegt = 0;    // Reiter des Befehlsfelds mit Gruppen
    std::vector<std::string> titel;   // die Gruppen im vorgegebenen Reiter
    int offen = 0;          // davon offen
};

struct BStand {
    std::map<int, BLage> mit;
    std::map<int, BLage> ohne;
    bool leer = false;
    bool explorer = false;
    bool ansicht = false;
    bool knoepfe = false;
    bool npc = false;
};

ImRect bRect(const char* name) {
    const ImGuiWindow* w = mbefoto::kindFenster(name);
    return w != nullptr ? w->Rect() : ImRect{};
}

BLage bLage() {
    BLage l;
    l.da = true;
    l.explorer = bRect("mtexplorer");
    l.ansichten = bRect("mtansichten");
    l.feld = bRect("mtbefehlsfeld");
    l.editor = mbefoto::kindFenster("mtuveditor") != nullptr ? bRect("mtuveditor") : bRect("mtmaleditor");
    l.belegt = ui::feldProbe().reiterBelegt;
    for (int i = 0; i < ui::rolloutProbeZahl(); ++i) {
        const ui::RolloutProbe& p = ui::rolloutProbe(i);
        // die Spalte des Felds (auch Gruppen unter dem Rand des Felds: mit Modell schieben offene Gruppen die folgenden hinaus)
        if (p.kopf.GetCenter().x < l.feld.Min.x || p.kopf.GetCenter().x > l.feld.Max.x) { continue; }
        l.titel.push_back(p.titel);
        l.offen += p.offen ? 1 : 0;
    }
    return l;
}

std::string bText(const ImRect& r) {
    char z[80];
    std::snprintf(z, sizeof z, "%.0f,%.0f-%.0f,%.0f", static_cast<double>(r.Min.x), static_cast<double>(r.Min.y), static_cast<double>(r.Max.x), static_cast<double>(r.Max.y));
    return z;
}

bool bGleich(const ImRect& a, const ImRect& b) {
    return std::fabs(a.Min.x - b.Min.x) <= 1.0F && std::fabs(a.Min.y - b.Min.y) <= 1.0F && std::fabs(a.Max.x - b.Max.x) <= 1.0F && std::fabs(a.Max.y - b.Max.y) <= 1.0F;
}

// Schluessel: Bedienmodus * 10 + Bereich, der Setup-Modus der Animation + 100
const int kBBereiche[] = {0, 1, 2, 3, 4, 5, 105};

void bDurchlauf(std::vector<Schritt>& s, const std::shared_ptr<BStand>& st, bool mitModell) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (int modus = 0; modus < 2; ++modus) {
        for (const int bb : kBBereiche) {
            const int bereich = bb % 100;
            const bool setup = bb >= 100;
            const int schluessel = modus * 10 + bb;
            add(tu("T1411 B: " + std::string(mitModell ? "mit" : "ohne") + " Modell, " + (modus == 0 ? "3ds Max" : "Blender") + ", Bereich " + std::to_string(bereich) +
                       (setup ? " Setup" : ""),
                   [modus, bereich, setup] {
                       modellBedienungSetzen(modus);
                       mtab().bereich = static_cast<ModellBereich>(bereich);
                       if (bereich == static_cast<int>(ModellBereich::Animation)) { modellAnimSetupSetzen(setup); }
                   }));
            add(pause(0.6));
            add(tu("T1411 B: Lage merken", [st, mitModell, schluessel] {
                const BLage l = bLage();
                (mitModell ? st->mit : st->ohne)[schluessel] = l;
                std::string z = "B " + std::string(mitModell ? "mit " : "ohne ") + std::to_string(schluessel) + ": Explorer " + bText(l.explorer) + ", Ansichten " +
                                bText(l.ansichten) + ", Feld " + bText(l.feld) + ", Editor " + bText(l.editor) + ", Reiter " + std::to_string(l.belegt) + ", " +
                                std::to_string(l.titel.size()) + " Gruppen (" + std::to_string(l.offen) + " offen):";
                for (const std::string& t : l.titel) { z += " " + t + ";"; }
                melde(z);
            }));
            if (modus == 0 || bereich == static_cast<int>(ModellBereich::Ansehen) || bereich == static_cast<int>(ModellBereich::Animation)) {
                add(foto(std::string("t1411_b_") + (mitModell ? "mit" : "leer") + "_" + (modus == 0 ? "max" : "blender") + "_" + std::to_string(bereich) + (setup ? "s" : "")));
            }
        }
    }
}

void bSchritte(std::vector<Schritt>& s, const std::string& modellPfad) {
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<BStand>();
    const auto grund = [] {
        mtab().vierAnsichten = false;
        mtab().aktiv = 3;
        mtab().malen.editorOffen = true;
        modellExplorer().art = 0;
        g_app->npcBrowserOffen = false;
    };
    add(tu("T1411 B: wie 3ds Max, eine Ansicht, Explorer nach Hierarchie", grund));
    add(pause(0.5));
    bDurchlauf(s, st, true);
    add(tu("T1411 B: den letzten Reiter schliessen (gespeichert) - der Model-Tab ist leer", [] {
        modellBedienungSetzen(0);
        mtab().gespeichertStand = mtab().dok.stand();
        modellReiterSchliessen(0);
    }));
    add(pause(0.8));
    add(tu("T1411 B: leer, eine Ansicht", [st, grund] {
        st->leer = g_app->model.empty() && modellReiterZahl() == 1;
        grund();
    }));
    add(pause(0.5));
    bDurchlauf(s, st, false);
    // die leere Ansicht, der Explorer und die zwei Knoepfe (wie 3ds Max, Ansehen/Pruefen)
    add(tu("T1411 B: leer, wie 3ds Max, Ansehen/Pruefen", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
    }));
    add(pause(0.6));
    add(tu("T1411 B: Ansicht und Explorer ohne Modell", [st] {
        const ModellAnsicht& a = mtab().ansichten[mtab().aktiv];
        st->ansicht = modellNavigationSichtbar(mtab().aktiv) && a.wuerfel[2] > a.wuerfel[0] && a.kopfUnten > a.lo[1] && a.kopfRechts > a.lo[0];
        const ModellExplorerZustand& x = modellExplorer();
        st->explorer = x.leerHinweis && x.filterReihe && finde(tr(Str::MbNachHierarchie), "mtexplorer", 0, false) != nullptr &&
                       finde(tr(Str::T1eSequenzen), "mtexplorer", 0, false) != nullptr && finde(tr(Str::EmptyShowHow), "mtexplorer", 0, false) != nullptr;
        const Element* npc = finde(tr(Str::T1bNpcBrowser), "mtansicht", 0, false);
        const Element* glm = finde(tr(Str::OpenGlm), "mtansicht", 0, false);
        const ImVec2 mitte{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.5F};
        st->knoepfe = npc != nullptr && glm != nullptr && std::fabs((npc->rect.Min.x + glm->rect.Max.x) * 0.5F - mitte.x) < 2.0F &&
                      std::fabs(npc->rect.GetCenter().y - mitte.y) < ImGui::GetFontSize() * 4.0F;
        melde(std::string("B leer: ViewCube ") + bText(ImRect{a.wuerfel[0], a.wuerfel[1], a.wuerfel[2], a.wuerfel[3]}) + ", Kopfzeile bis " +
              std::to_string(static_cast<int>(a.kopfUnten)) + ", Explorer-Hinweis " + std::to_string(x.leerHinweis ? 1 : 0) + ", Knoepfe " +
              (npc != nullptr ? bText(npc->rect) : std::string("-")) + " / " + (glm != nullptr ? bText(glm->rect) : std::string("-")) + ", Mitte " +
              std::to_string(static_cast<int>(mitte.x)) + "/" + std::to_string(static_cast<int>(mitte.y)));
    }));
    add(klick(tr(Str::T1bNpcBrowser), "mtansicht"));
    add(pause(0.4));
    add(tu("T1411 B: NPC-Browser offen", [st] {
        st->npc = g_app->npcBrowserOffen;
        g_app->npcBrowserOffen = false;
    }));
    add(pause(0.3));
    add(pruefe("T1411 B: der letzte Reiter geschlossen, der Model-Tab ist leer", [st] { return st->leer; }));
    add(pruefe("T1411 B: je Bereich und Bedienmodus stehen Explorer, Ansichten, Befehlsfeld und UV-Editor bzw. 2D-Bild ohne Modell an derselben Stelle wie mit Modell", [st] {
        bool ok = st->mit.size() == st->ohne.size() && !st->mit.empty();
        for (const auto& [k, m] : st->mit) {
            const auto it = st->ohne.find(k);
            if (it == st->ohne.end()) { return false; }
            const BLage& o = it->second;
            const bool gleich = bGleich(m.explorer, o.explorer) && bGleich(m.ansichten, o.ansichten) && bGleich(m.feld, o.feld) && bGleich(m.editor, o.editor) &&
                                m.explorer.GetWidth() > 0.0F && m.ansichten.GetWidth() > 0.0F && m.feld.GetWidth() > 0.0F;
            if (!gleich) { melde("B Rahmen anders bei " + std::to_string(k)); }
            ok = ok && gleich;
        }
        return ok;
    }));
    add(pruefe("T1411 B: im Befehlsfeld dieselben belegten Reiter und im vorgegebenen Reiter dieselben Gruppen wie mit Modell; ohne Modell alle zu (Modellieren zeichnet seine eigenen)", [st] {
        bool ok = !st->mit.empty();
        for (const auto& [k, m] : st->mit) {
            if (k % 100 % 10 == static_cast<int>(ModellBereich::Modellieren)) { continue; }
            const auto it = st->ohne.find(k);
            if (it == st->ohne.end()) { return false; }
            const BLage& o = it->second;
            const bool gleich = m.belegt == o.belegt && m.titel == o.titel && o.offen == 0 && !m.titel.empty();
            if (!gleich) {
                melde("B Gruppen anders bei " + std::to_string(k) + ": Reiter " + std::to_string(m.belegt) + "/" + std::to_string(o.belegt) + ", Gruppen " +
                      std::to_string(m.titel.size()) + "/" + std::to_string(o.titel.size()) + ", offen " + std::to_string(o.offen));
            }
            ok = ok && gleich;
        }
        return ok;
    }));
    add(pruefe("T1411 B: die leere Ansicht hat ViewCube und Kopfzeile [+] [Perspective] [Standard]", [st] { return st->ansicht; }));
    add(pruefe("T1411 B: der Explorer zeigt Reiter (By hierarchy | By layers | Sequences), Suche und Filter, der Hinweis und Show me how stehen im leeren Baum",
               [st] { return st->explorer; }));
    add(pruefe("T1411 B: der Ladehinweis steht mittig in der Ansicht mit NPC-Browser und .glm oeffnen; NPC-Browser oeffnet ihn", [st] { return st->knoepfe && st->npc; }));
    // das Modell wieder laden (fuer die folgenden Abschnitte)
    for (Schritt& x : mb1::laden(modellPfad.c_str())) { add(std::move(x)); }
    add(tu("T1411 B: zurueck, wie 3ds Max, Ansehen/Pruefen", [] {
        modellBedienungSetzen(0);
        mtab().bereich = ModellBereich::Ansehen;
        mtab().vierAnsichten = false;
    }));
    add(pause(0.5));
}

std::vector<Schritt> schritte(const char* nur) {
    std::vector<Schritt> s;
    const std::string n = nur != nullptr ? nur : "";
    if (n != "test1411") { return s; }
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    int b = 3840;
    int h = 2160;
    float z = 1.5F;
    if (const char* f = std::getenv("BHED_T1411_FENSTER")) { (void)std::sscanf(f, "%dx%d@%f", &b, &h, &z); }
    auto skala = std::make_shared<float>(1.0F);
    auto teiler = std::make_shared<float>(0.0F);
    auto bedienung = std::make_shared<int>(0);
    add(tu("T1411: Fenster, Skript fuer den Test", [=] {
        *skala = g_app->uiScale;
        (void)platform::hauptfensterGroesse(b, h);
        g_app->uiScale = z;
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "t1411.txt");
    }));
    add(pause(0.8));
    const char* modell = std::getenv("BHED_T1411_MODELL");
    for (Schritt& x : mb1::laden(modell != nullptr && *modell != '\0' ? modell : "models/players/alora/model.glm")) { add(std::move(x)); }
    add(tu("T1411: Aufteilung wie neue Einstellungen", [=] {
        *teiler = g_app->settings.splitModel;
        *bedienung = g_app->settings.modellBedienung;
        g_app->settings.splitModel = Settings{}.splitModel;
        g_app->settings.modellFeldBreite = 0.0F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->mtab.vierAnsichten = false;
        melde(g_app->modelPath + ", " + std::to_string(g_app->anim.bones.size()) + " Knochen, " + std::to_string(g_app->animList.size()) + " Sequenzen");
    }));
    add(pause(1.0));
    if (abschnitt("d")) { dSchritte(s); }
    if (abschnitt("i")) { iSchritte(s); }
    if (abschnitt("f")) { fSchritte(s); }
    if (abschnitt("h")) { hSchritte(s); }
    if (abschnitt("g")) { gSchritte(s); }
    if (abschnitt("j")) { jSchritte(s); }
    if (abschnitt("l")) { lSchritte(s); }
    if (abschnitt("m")) { mSchritte(s); }
    if (abschnitt("k")) { kSchritte(s); }
    if (abschnitt("c")) { cSchritte(s); }
    if (abschnitt("n")) { nSchritte(s); }
    if (abschnitt("o")) { oSchritte(s); }
    if (abschnitt("p")) { pSchritte(s); }
    if (abschnitt("e")) { eSchritte(s); }
    if (abschnitt("a")) { aSchritte(s); }
    if (abschnitt("z")) { zSchritte(s); }   // TEST1411d: die Missionsliste aus dem Zwischenspeicher der Platte
    if (abschnitt("x")) { xSchritte(s); }   // TEST1411e: der Mauspunkt auf dem Raster in der Statuszeile
    if (abschnitt("s")) { sSchritte(s); }   // TEST1411e: Load mission - waehrend des Lesens springt unter der Maus nichts
    if (abschnitt("w")) { wSchritte(s); }   // TEST1411e: Knochenpaare des Weights-Mirror von Hand
    if (abschnitt("b")) { bSchritte(s, modell != nullptr && *modell != '\0' ? modell : "models/players/alora/model.glm"); }
    if (abschnitt("fotos")) { fotoSchritte(s); }
    add(tu("T1411: zurueck", [=] {
        modellBedienungSetzen(*bedienung);
        g_app->settings.splitModel = *teiler;
        g_app->mtab.bereich = ModellBereich::Ansehen;
        g_app->uiScale = *skala;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.3));
    return s;
}

}  // namespace t1411
