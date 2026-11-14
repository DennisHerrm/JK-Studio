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
//   fotos  Fotos fuer D, I, H (BHED_FOTOS)
//   F  Zeitleiste: die Knoepfe rechts der Bildanzeige stehen still, wenn die Bildzahl waechst (0, 9, 10, 99, 100, letztes Bild der
//      laengsten Sequenz - unter Movie Duels BOTH_MD_CIN_51 mit 660 Bildern)
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
