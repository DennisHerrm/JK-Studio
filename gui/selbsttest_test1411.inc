// selbsttest_test1411.inc - TEST1411 (14.11.): die Testrueckmeldungen von shank zu rc612, je Punkt erst rot im laufenden Programm,
// dann gruen (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=test1411; BHED_T1411=<d,i,f,...> nur diese Punkte, Vorgabe alle)
//
//   D  Modell doppelt beim Zoomen: das Bild der Ansicht gegen ein frisch gezeichnetes Bild derselben Kamera (Bildpunkte des
//      Umrisses) und gegen die Ueberlagerung (Ecken der Haut auf dem Schirm), in drei Bereichen, fuenf Ansichtsarten, nach
//      Rad-Rasten heraus und herein; dazu das Drahtgitter ab Werk im Bereich Modellieren (Objektebene ohne Kanten) und die
//      Geister (Ghosts) nur im Bereich Animation
//   I  Bereich Animation ohne Control Rig: ein Klick auf die Figur waehlt einen Knochen, keine Meldung "eingefroren"
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
        add(tu(std::string("T1411 I: View/Check mit Knochen aus, dann Animation, wie 3ds Max, kein Rig - Klick auf ") + ziele[k], [] {
            modellBedienungSetzen(0);
            mtab().bereich = ModellBereich::Ansehen;
            g_app->modellKnochen = false;
        }));
        add(pause(0.2));
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
        add(klickAn("T1411 I: auf " + name + " (Mitte der Flaeche auf dem Schirm)", [name] {
            ImVec2 p{0.0F, 0.0F};
            (void)modellFlaecheAufSchirm(teil(name.c_str()), p);
            return ImVec2{std::floor(p.x), std::floor(p.y)};
        }));
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
