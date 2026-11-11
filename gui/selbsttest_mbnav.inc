// selbsttest_mbnav.inc - MBNAV (11.11.): Navigation in den Ansichten des Model-Tabs 1:1 wie 3ds Max bzw. Blender
// (BHED_EDITORTEST=modellblock, BHED_MODELL_NUR=nav; die Bilder des Handbuchs: BHED_EDITORTEST=handbuch BHED_HB_NUR=mbnav)
//   nav   Max: Mitteltaste schiebt 1:1 (Umschalt: eine Achse), Alt+Mitte kreist (auch aus einer Achsenansicht, Umschalt: eine
//         Achse, um die Auswahl), Strg+Alt+Mitte zoomt, Rad und [ ] um den Mauspunkt, Z / Strg+Alt+Z / Strg+Umschalt+Z, 2x,
//         Alt+Z, Strg+W, Strg+P, Strg+R, I, Umschalt+Z / Umschalt+Y, T/B/F/L/P/U, V, Alt+Pos1, Alt+Strg+H, Alt+Strg+V; ViewCube:
//         alle 26 Ziele, Ziehen in einer Achsenansicht. Blender: Mitte kreist (Turntable 0,4 Grad je Bildpunkt, ueber den Pol,
//         Auto Perspective), Umschalt+Mitte schiebt, Strg+Mitte zoomt (Dolly), Alt+Mitte ziehen rastet, Alt+Mitte klicken
//         zentriert, Rad und Ziffernblock +/- (1,2), Ziffernblock 1/5/4/6/8/2 (+Strg, +Umschalt), Komma, Pos1, Umschalt+C, `,
//         Strg+Alt+Q, Gizmo (Zoom, Verschieben, Persp/Ortho, Mitte bei gerader Sicht).
// Jede Pruefung liest nur den Zustand der Ansicht (Winkel, Drehpunkt, Bildhoehe am Drehpunkt): derselbe Test lief vor der
// Umsetzung rot (docs/intern/NOTIZEN_mbnav_1011.md) und belegt die Spalte "heute" der Tabelle docs/intern/navigation_abgleich.md.
// Die Sollwerte stammen aus dem Blender-Quelltext (5.0: view3d_navigate*.cc, blender_default.py, userdef_default.c) und der
// Autodesk-Hilfe; wo 3ds Max keine Zahl nennt, prueft der Test nur Richtung und Drehpunkt.
// Eingebunden in selbsttest.cpp (nach selbsttest_mbraster.inc).
#include <cstdarg>

namespace mbnav {

using mbkopf::ansicht;
using mbkopf::gizmoOrt;
using mbkopf::wuerfelOrt;

constexpr float kPi = 3.14159265F;
constexpr int kMitte = 2;   // ImGuiMouseButton_Middle

ModellTab& t() { return g_app->mtab; }

// Was eine Ansicht zeigt: Art, Winkel (pitch, yaw, roll), der Punkt in der Bildmitte (Perspektive: der Drehpunkt), die halbe
// Bildhoehe in Einheiten in dessen Tiefe, die Achsen.
struct Lage {
    AnsichtArt art = AnsichtArt::Perspektive;
    float w[3]{};
    float p[3]{};
    float halb = 0.0F;
    float r[3]{};
    float h[3]{};
    float v[3]{};
};

Lage lageVon(int n = -1) {
    const ModellAnsicht& a = ansicht(n);
    Lage l;
    l.art = a.art;
    modellAnsichtWinkel(a, l.w);
    modellAnsichtAchsenVon(a.art, l.r, l.h, l.v);
    if (a.art == AnsichtArt::Perspektive) {
        g_app->modelOrbit.pivot(g_app->modelCam, l.p);
        l.halb = g_app->modelOrbit.distance * std::tan(g_app->modelCam.fovY * kPi / 360.0F);
    } else {
        for (int k = 0; k < 3; ++k) { l.p[k] = a.mitte[k]; }
        l.halb = a.halbHoehe;
    }
    return l;
}

void melde(const char* fmt, ...) {
    char z[400];
    va_list ap;
    va_start(ap, fmt);
    std::vsnprintf(z, sizeof z, fmt, ap);
    va_end(ap);
    diag::detail(std::string("Knopftest: MBNAV ") + z);
}

ImVec2 mitteVon(int n = -1) {
    const ModellAnsicht& a = ansicht(n);
    return ImVec2{a.lo[0] + a.groesse[0] * 0.5F, a.lo[1] + a.groesse[1] * 0.5F};
}

std::function<ImVec2()> neben(float dx, float dy, int n = -1) {
    return [=] {
        const ImVec2 m = mitteVon(n);
        return ImVec2{m.x + dx, m.y + dy};
    };
}

// Ein Weltpunkt auf dem Schirm, gerechnet aus der Lage (nicht aus der Matrix des letzten Bilds).
ImVec2 schirmVon(const Lage& l, const float q[3], int n = -1) {
    const ModellAnsicht& a = ansicht(n);
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
    for (int k = 0; k < 3; ++k) {
        const float d = q[k] - l.p[k];
        x += d * l.r[k];
        y += d * l.h[k];
        z += d * l.v[k];
    }
    float proEinheit = a.groesse[1] / (2.0F * l.halb);
    if (l.art == AnsichtArt::Perspektive) {
        const float abstand = l.halb / std::tan(g_app->modelCam.fovY * kPi / 360.0F);
        proEinheit *= abstand / std::max(abstand + z, 1e-3F);
    }
    const ImVec2 m = mitteVon(n);
    return ImVec2{m.x + x * proEinheit, m.y - y * proEinheit};
}

// Der Punkt der Ebene durch den Drehpunkt (senkrecht zum Blick) unter einer Schirmstelle.
void weltVon(const Lage& l, ImVec2 ort, float q[3], int n = -1) {
    const ModellAnsicht& a = ansicht(n);
    const ImVec2 m = mitteVon(n);
    const float jePunkt = 2.0F * l.halb / a.groesse[1];
    const float x = (ort.x - m.x) * jePunkt;
    const float y = (m.y - ort.y) * jePunkt;
    for (int k = 0; k < 3; ++k) { q[k] = l.p[k] + l.r[k] * x + l.h[k] * y; }
}

// Winkel in Grad zwischen der Blickrichtung und der zu (pitch, yaw) gehoerenden.
float blickNeben(const Lage& l, float pitch, float yaw) {
    const float p = pitch * kPi / 180.0F;
    const float y = yaw * kPi / 180.0F;
    const float f[3] = {std::cos(p) * std::cos(y), std::cos(p) * std::sin(y), -std::sin(p)};
    const float d = std::clamp(l.v[0] * f[0] + l.v[1] * f[1] + l.v[2] * f[2], -1.0F, 1.0F);
    return std::acos(d) * 180.0F / kPi;
}

float abstandPunkte(const float a[3], const float b[3]) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

float winkelWeg(float von, float nach) { return std::remainder(nach - von, 360.0F); }

// Die halbe kleinere Bildseite in Einheiten in der Tiefe des Drehpunkts.
float halbKlein(const Lage& l, int n = -1) {
    const ModellAnsicht& a = ansicht(n);
    return a.groesse[0] < a.groesse[1] ? l.halb * a.groesse[0] / a.groesse[1] : l.halb;
}

// Der Kasten der gezeichneten Flaechen in der Pose (nurAuswahl: der gewaehlten); false ohne Ecke.
bool kasten(bool nurAuswahl, float mn[3], float mx[3]) {
    const auto& haut = modellHaut();
    bool da = false;
    for (std::size_t i = 0; i < haut.size(); ++i) {
        if (!modellTeilGezeichnet(static_cast<int>(i))) { continue; }
        if (nurAuswahl && std::find(t().auswahl.begin(), t().auswahl.end(), static_cast<int>(i)) == t().auswahl.end()) { continue; }
        for (const auto& e : haut[i]) {
            for (int k = 0; k < 3; ++k) {
                mn[k] = da ? std::min(mn[k], e[static_cast<std::size_t>(k)]) : e[static_cast<std::size_t>(k)];
                mx[k] = da ? std::max(mx[k], e[static_cast<std::size_t>(k)]) : e[static_cast<std::size_t>(k)];
            }
            da = true;
        }
    }
    return da;
}

// Die gezeichnete Flaeche, deren Mitte am weitesten neben der Hochachse durch den Drehpunkt liegt (eine Hand).
int fernsterTeil() {
    const auto& haut = modellHaut();
    int best = -1;
    float weit = -1.0F;
    for (std::size_t i = 0; i < haut.size(); ++i) {
        if (!modellTeilGezeichnet(static_cast<int>(i)) || haut[i].empty()) { continue; }
        float m[2] = {0.0F, 0.0F};
        for (const auto& e : haut[i]) {
            m[0] += e[0];
            m[1] += e[1];
        }
        const float d = std::hypot(m[0], m[1]) / static_cast<float>(haut[i].size());
        if (d > weit) {
            weit = d;
            best = static_cast<int>(i);
        }
    }
    return best;
}

// Mit gedrueckter Maustaste und beliebig vielen Modifikatoren von einer Stelle um (dx, dy) ziehen.
Schritt zieh(const std::string& was, std::function<ImVec2()> ort, float dx, float dy, int knopf, ImGuiKeyChord mods = 0, int bilder = 12) {
    auto z = std::make_shared<ImVec2>();
    return {"Ziehe " + was, [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                const auto modifikatoren = [&](bool an) {
                    if ((mods & ImGuiMod_Ctrl) != 0) { io.AddKeyEvent(ImGuiMod_Ctrl, an); }
                    if ((mods & ImGuiMod_Shift) != 0) { io.AddKeyEvent(ImGuiMod_Shift, an); }
                    if ((mods & ImGuiMod_Alt) != 0) { io.AddKeyEvent(ImGuiMod_Alt, an); }
                };
                if (b == 0) {
                    *z = ort();
                    setzeMaus(*z);
                    return false;
                }
                if (b == 1) {
                    modifikatoren(true);
                    return false;
                }
                if (b == 2) {
                    io.AddMouseButtonEvent(knopf, true);
                    return false;
                }
                if (b >= 3 && b < 3 + bilder) {
                    const float f = static_cast<float>(b - 2) / static_cast<float>(bilder);
                    setzeMaus(ImVec2{z->x + dx * f, z->y + dy * f});
                    return false;
                }
                if (b == 3 + bilder) {
                    io.AddMouseButtonEvent(knopf, false);
                    return false;
                }
                if (b == 4 + bilder) {
                    modifikatoren(false);
                    return false;
                }
                return b >= 8 + bilder;
            }};
}

// Klick mit Modifikatoren (ohne Bewegung).
Schritt klickMit(const std::string& was, std::function<ImVec2()> ort, int knopf, ImGuiKeyChord mods) { return zieh(was, std::move(ort), 0.0F, 0.0F, knopf, mods, 2); }

// Maus an eine Stelle, dann eine Taste (die Befehle "um den Mauspunkt" und "zum Mauszeiger").
Schritt mausDann(const std::string& was, std::function<ImVec2()> ort) {
    return tu("MBNAV: Maus " + was, [=] { setzeMaus(ort()); });
}

Schritt start(bool blender, AnsichtArt art = AnsichtArt::Perspektive, bool vier = false) {
    return tu(std::string("MBNAV: Start wie ") + (blender ? "Blender" : "3ds Max") + ", Ansicht " + std::to_string(static_cast<int>(art)) + (vier ? ", vier Ansichten" : ""),
              [=] {
                  modellBedienungSetzen(blender ? 1 : 0);
                  ModellTab& m = t();
                  m.vierAnsichten = vier;
                  m.aktiv = 3;
                  m.auswahl.clear();
                  m.eckenWahl.clear();
                  m.gizmo = 0;
                  static const AnsichtArt kArt[3] = {AnsichtArt::Oben, AnsichtArt::Vorne, AnsichtArt::Links};
                  for (int i = 0; i < 3; ++i) {
                      m.ansichten[i].art = kArt[i];
                      m.ansichten[i].animStart = -1.0;
                      modellAllesEinpassenIn(i);
                  }
                  modellStartansicht(3);
                  if (art != AnsichtArt::Perspektive) { modellBlickSetzen(3, art, nullptr, false); }
                  m.fokus = true;
                  g_app->settings.modellNavigation = true;
                  g_app->modelDirty = true;
                  setzeMaus(neben(0.0F, 0.0F)());
              });
}

// Ein Fall: Start, merken, Aktion(en), warten, pruefen (vorher/nachher).
struct Faelle {
    std::vector<Schritt>& s;
    std::shared_ptr<Lage> vor = std::make_shared<Lage>();
    void add(Schritt x) const { s.push_back(std::move(x)); }
    void beginn(Schritt st) const {
        add(std::move(st));
        add(pause(0.35));
        const std::shared_ptr<Lage> v = vor;
        add(tu("MBNAV: Lage merken", [v] { *v = lageVon(); }));
    }
    void pruefung(const std::string& text, std::function<bool(const Lage&, const Lage&)> ok, double warte = 0.35) const {
        add(pause(warte));
        const std::shared_ptr<Lage> v = vor;
        add(pruefe("MBNAV " + text, [v, ok] { return ok(*v, lageVon()); }));
    }
};

// Der alte Drehpunkt steht nach einer Verschiebung um (dx, dy) Bildpunkte neben der Bildmitte.
bool verschobenUm(const Lage& v, const Lage& n, float dx, float dy, const char* was) {
    const ImVec2 o = schirmVon(n, v.p);
    const ImVec2 m = mitteVon();
    melde("%s: der alte Drehpunkt steht %.2f, %.2f Bildpunkte neben der Bildmitte (Soll %.0f, %.0f)", was, static_cast<double>(o.x - m.x),
          static_cast<double>(o.y - m.y), static_cast<double>(dx), static_cast<double>(dy));
    return std::fabs(o.x - m.x - dx) < 1.5F && std::fabs(o.y - m.y - dy) < 1.5F;
}

bool gedrehtUm(const Lage& v, const Lage& n, float nick, float gier, const char* was) {
    const float neb = blickNeben(n, v.w[0] + nick, v.w[1] + gier);
    melde("%s: Winkel %.2f %.2f %.2f -> %.2f %.2f %.2f, %.3f Grad neben dem Soll (Nick %+.1f, Gier %+.1f), Drehpunkt %.3f Einheiten gewandert, Art %d", was,
          static_cast<double>(v.w[0]), static_cast<double>(v.w[1]), static_cast<double>(v.w[2]), static_cast<double>(n.w[0]), static_cast<double>(n.w[1]),
          static_cast<double>(n.w[2]), static_cast<double>(neb), static_cast<double>(nick), static_cast<double>(gier), static_cast<double>(abstandPunkte(v.p, n.p)),
          static_cast<int>(n.art));
    return neb < 0.05F && abstandPunkte(v.p, n.p) < 0.01F * std::max(v.halb, 1.0F);
}

bool gezoomtUm(const Lage& v, const Lage& n, float faktor, const char* was) {
    melde("%s: halbe Bildhoehe %.4f -> %.4f, Faktor %.5f (Soll %.5f)", was, static_cast<double>(v.halb), static_cast<double>(n.halb),
          static_cast<double>(n.halb / v.halb), static_cast<double>(faktor));
    return std::fabs(n.halb / v.halb - faktor) < 0.002F * faktor;
}

// Blender view3d.rotate: U.view_rotate_sensitivity_turntable / UI_SCALE_FAC = 0,4 Grad je Bildpunkt beim Massstab 1.
float blGrad(float punkte) { return 0.4F * punkte / g_app->settings.uiScale; }

// Blender view3d.zoom, Dolly (viewzoom_scale_value): dist = dist0 * (2 * (len_neu / len_alt - 1) + 1) mit len = 5 * U + Abstand
// der Maus zur Oberkante der Ansicht / U (U = UI_SCALE_FAC = Massstab der Oberflaeche).
float dollyFaktor(float obenAlt, float obenNeu) {
    const float u = g_app->settings.uiScale;
    const float alt = 5.0F * u + obenAlt / u;
    const float neu = 5.0F * u + obenNeu / u;
    return 2.0F * (neu / std::max(alt, 1.0F) - 1.0F) + 1.0F;
}

bool popupOffen() { return !GImGui->OpenPopupStack.empty(); }

Schritt popupsZu() {
    return tu("MBNAV: offene Menues schliessen", [] {
        if (popupOffen()) { ImGui::ClosePopupToLevel(0, false); }
    });
}

// Eine Taste druecken und im Bild nach ihrer Wirkung nachsehen, ob die Ansicht weich dreht (ein Wechsel laeuft).
Schritt tasteUndWeich(ImGuiKey k, const std::string& name, std::shared_ptr<bool> weich) {
    return {"Taste " + name + " (weich?)", [=](int b) {
                ImGuiIO& io = ImGui::GetIO();
                if (b == 0) {
                    *weich = false;
                    io.AddKeyEvent(k, true);
                    return false;
                }
                if (b == 1) {
                    io.AddKeyEvent(k, false);
                    return false;
                }
                if (b <= 4) {
                    *weich = *weich || ansicht().animStart >= 0.0;
                    return false;
                }
                return true;
            }};
}

// Die Ansicht (0..3), deren Feld die Ecke oben/unten, links/rechts belegt (Vierer-Ansicht).
int ansichtIn(bool unten, bool rechts) {
    int best = -1;
    float wert = -1e9F;
    for (int i = 0; i < 4; ++i) {
        const ModellAnsicht& a = ansicht(i);
        if (!a.sichtbar) { continue; }
        const float x = (rechts ? 1.0F : -1.0F) * a.lo[0] + (unten ? 1.0F : -1.0F) * a.lo[1];
        if (x > wert) {
            wert = x;
            best = i;
        }
    }
    return best;
}

void maxSchritte(std::vector<Schritt>& s) {
    const Faelle f{s};
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // --- Mitteltaste: schieben -------------------------------------------------------------------------------------------
    f.beginn(start(false));
    add(zieh("Max: Mitteltaste in der Perspektive", neben(0.0F, 0.0F), 120.0F, 60.0F, kMitte));
    f.pruefung("Max M1: Mitteltaste schiebt die Perspektive 1:1 - der Punkt in der Tiefe des Drehpunkts folgt der Maus",
               [](const Lage& v, const Lage& n) { return verschobenUm(v, n, 120.0F, 60.0F, "M1 Perspektive") && blickNeben(n, v.w[0], v.w[1]) < 0.01F; });
    f.beginn(start(false, AnsichtArt::Vorne));
    add(zieh("Max: Mitteltaste in Vorne", neben(0.0F, 0.0F), 120.0F, 60.0F, kMitte));
    f.pruefung("Max M1: Mitteltaste schiebt die Ansicht Vorne 1:1", [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Vorne && verschobenUm(v, n, 120.0F, 60.0F, "M1 Vorne"); });
    f.beginn(start(false));
    add(zieh("Max: Umschalt+Mitteltaste, zuerst waagerecht", neben(0.0F, 0.0F), 120.0F, 40.0F, kMitte, ImGuiMod_Shift));
    f.pruefung("Max M2: Umschalt+Mitteltaste schiebt nur entlang der zuerst bewegten Achse (waagerecht)",
               [](const Lage& v, const Lage& n) { return verschobenUm(v, n, 120.0F, 0.0F, "M2 waagerecht"); });
    f.beginn(start(false, AnsichtArt::Vorne));
    add(zieh("Max: Umschalt+Mitteltaste, zuerst senkrecht", neben(0.0F, 0.0F), 30.0F, 90.0F, kMitte, ImGuiMod_Shift));
    f.pruefung("Max M2: Umschalt+Mitteltaste schiebt nur entlang der zuerst bewegten Achse (senkrecht, Ansicht Vorne)",
               [](const Lage& v, const Lage& n) { return verschobenUm(v, n, 0.0F, 90.0F, "M2 senkrecht"); });
    // --- Alt+Mitteltaste: kreisen ------------------------------------------------------------------------------------------
    f.beginn(start(false));
    add(zieh("Max: Alt+Mitteltaste nach rechts", neben(0.0F, 0.0F), 50.0F, 0.0F, kMitte, ImGuiMod_Alt));
    f.pruefung("Max M4: Alt+Mitteltaste nach rechts kreist um den Drehpunkt, die Szene folgt der Maus (Gier -20 bei 0,4 Grad je Bildpunkt)",
               [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 0.0F, -20.0F, "M4 rechts"); });
    f.beginn(start(false));
    add(zieh("Max: Alt+Mitteltaste nach unten", neben(0.0F, 0.0F), 0.0F, 50.0F, kMitte, ImGuiMod_Alt));
    f.pruefung("Max M4: Alt+Mitteltaste nach unten - die Szene folgt der Maus, die Kamera steigt (Nick +20: von oben)",
               [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 20.0F, 0.0F, "M4 unten"); });
    f.beginn(start(false, AnsichtArt::Vorne));
    add(zieh("Max: Alt+Mitteltaste in der Ansicht Vorne", neben(0.0F, 0.0F), 50.0F, 30.0F, kMitte, ImGuiMod_Alt));
    f.pruefung("Max M4: Alt+Mitteltaste in einer Achsenansicht macht daraus die orthographische Benutzeransicht und kreist",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Orthographisch && gedrehtUm(v, n, 12.0F, -20.0F, "M4 Vorne") && std::fabs(n.halb - v.halb) < 0.01F; });
    f.beginn(start(false));
    add(zieh("Max: Umschalt+Alt+Mitteltaste, zuerst waagerecht", neben(0.0F, 0.0F), 60.0F, 20.0F, kMitte, ImGuiMod_Alt | ImGuiMod_Shift));
    f.pruefung("Max M5: Umschalt beim Kreisen beschraenkt auf die zuerst benutzte Achse (nur Gier)",
               [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 0.0F, -24.0F, "M5 waagerecht"); });
    f.beginn(start(false));
    add(zieh("Max: Umschalt+Alt+Mitteltaste, zuerst senkrecht", neben(0.0F, 0.0F), 15.0F, 60.0F, kMitte, ImGuiMod_Alt | ImGuiMod_Shift));
    f.pruefung("Max M5: Umschalt beim Kreisen beschraenkt auf die zuerst benutzte Achse (nur Nick)",
               [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 24.0F, 0.0F, "M5 senkrecht"); });
    // --- Strg+Alt+Mitteltaste, Rad, [ ] -------------------------------------------------------------------------------------
    f.beginn(start(false));
    add(zieh("Max: Strg+Alt+Mitteltaste nach oben", neben(0.0F, 0.0F), 0.0F, -50.0F, kMitte, ImGuiMod_Ctrl | ImGuiMod_Alt));
    f.pruefung("Max M6: Strg+Alt+Mitteltaste nach oben zoomt heran, der Drehpunkt bleibt", [](const Lage& v, const Lage& n) {
        melde("M6: halbe Bildhoehe %.3f -> %.3f (Faktor %.4f fuer 50 Bildpunkte)", static_cast<double>(v.halb), static_cast<double>(n.halb), static_cast<double>(n.halb / v.halb));
        return n.halb < v.halb * 0.95F && abstandPunkte(v.p, n.p) < 0.01F * v.halb;
    });
    for (int ortho = 0; ortho < 2; ++ortho) {
        auto unter = std::make_shared<std::array<float, 3>>();
        f.beginn(start(false, ortho != 0 ? AnsichtArt::Vorne : AnsichtArt::Perspektive));
        add(tu("MBNAV: Punkt unter der Maus merken", [=] { weltVon(lageVon(), neben(150.0F, 100.0F)(), unter->data()); }));
        add(radAn("Max: Rad neben der Mitte", neben(150.0F, 100.0F), 1.0F));
        f.pruefung(std::string("Max M7: das Rad zoomt um den Mauspunkt (Zoom About Mouse Point, ") + (ortho != 0 ? "Achsenansicht" : "Perspektive") + ")",
                   [=](const Lage& v, const Lage& n) {
                       const ImVec2 o = schirmVon(n, unter->data());
                       const ImVec2 m = mitteVon();
                       melde("M7 %s: halbe Bildhoehe %.3f -> %.3f (Faktor %.4f je Raste), der Punkt unter der Maus steht %.2f, %.2f neben der Mitte (Soll 150, 100)",
                             ortho != 0 ? "Vorne" : "Perspektive", static_cast<double>(v.halb), static_cast<double>(n.halb), static_cast<double>(n.halb / v.halb),
                             static_cast<double>(o.x - m.x), static_cast<double>(o.y - m.y));
                       return n.halb < v.halb * 0.97F && std::fabs(o.x - m.x - 150.0F) < 1.5F && std::fabs(o.y - m.y - 100.0F) < 1.5F;
                   });
    }
    {
        auto zwischen = std::make_shared<float>(0.0F);
        f.beginn(start(false));
        add(tasteKombi(ImGuiKey_LeftBracket, "["));
        add(pause(0.2));
        add(tu("MBNAV: nach [", [=] { *zwischen = lageVon().halb; }));
        add(tasteKombi(ImGuiKey_RightBracket, "]"));
        f.pruefung("Max M8: [ zoomt die Ansicht heran, ] wieder hinaus (Zoom Viewport In / Out)", [=](const Lage& v, const Lage& n) {
            melde("M8: halbe Bildhoehe %.3f, nach [ %.3f, nach ] %.3f", static_cast<double>(v.halb), static_cast<double>(*zwischen), static_cast<double>(n.halb));
            return *zwischen < v.halb * 0.97F && std::fabs(n.halb / v.halb - 1.0F) < 0.002F;
        });
        f.beginn(start(false, AnsichtArt::Vorne));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Equal, "Strg+="));
        add(pause(0.2));
        add(tu("MBNAV: nach Strg+=", [=] { *zwischen = lageVon().halb; }));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_Minus, "Strg+-"));
        f.pruefung("Max M8: Strg+= und Strg+- zoomen ebenso (zweite Belegung der Autodesk-Tabelle)", [=](const Lage& v, const Lage& n) {
            melde("M8 Strg: halbe Bildhoehe %.3f, nach Strg+= %.3f, nach Strg+- %.3f", static_cast<double>(v.halb), static_cast<double>(*zwischen), static_cast<double>(n.halb));
            return *zwischen < v.halb * 0.97F && std::fabs(n.halb / v.halb - 1.0F) < 0.002F;
        });
    }
    // --- Zoom Grenzen ------------------------------------------------------------------------------------------------------
    {
        auto soll = std::make_shared<std::array<float, 4>>();
        const auto verstellen = [] {
            for (int i = 0; i < 3; ++i) {
                ansicht(i).halbHoehe *= 3.0F;
                ansicht(i).mitte[0] += 40.0F;
                ansicht(i).mitte[2] += 40.0F;
            }
            g_app->modelOrbit.zoom(g_app->modelCam, -8.0F);
            g_app->modelDirty = true;
        };
        const auto halbe = [] {
            std::array<float, 4> x{};
            for (int i = 0; i < 4; ++i) { x[static_cast<std::size_t>(i)] = lageVon(i).halb; }
            return x;
        };
        const auto wieSoll = [=](const std::array<float, 4>& ist, int i) { return std::fabs(ist[static_cast<std::size_t>(i)] / (*soll)[static_cast<std::size_t>(i)] - 1.0F) < 0.01F; };
        add(start(false, AnsichtArt::Perspektive, true));
        add(pause(0.5));
        // erst jetzt kennen die vier Ansichten ihre Groesse (Seitenverhaeltnis): noch einmal einpassen, dann merken
        add(tu("MBNAV: die vier Ansichten in ihrer Groesse einpassen", [] {
            for (int i = 0; i < 4; ++i) { modellAllesEinpassenIn(i); }
            g_app->modelDirty = true;
        }));
        add(pause(0.3));
        add(tu("MBNAV: eingepasste Groessen merken, alle vier verstellen", [=] {
            *soll = halbe();
            verstellen();
        }));
        add(pause(0.3));
        add(tasteKombi(ImGuiKey_Z, "Z"));
        add(pause(0.4));
        add(pruefe("MBNAV Max M9: Z passt ALLE Ansichten ein (Zoom Extents All Selected; ohne Auswahl die ganze Figur)", [=] {
            const std::array<float, 4> ist = halbe();
            melde("M9 Z: halbe Bildhoehen %.2f %.2f %.2f %.2f, eingepasst %.2f %.2f %.2f %.2f", static_cast<double>(ist[0]), static_cast<double>(ist[1]),
                  static_cast<double>(ist[2]), static_cast<double>(ist[3]), static_cast<double>((*soll)[0]), static_cast<double>((*soll)[1]),
                  static_cast<double>((*soll)[2]), static_cast<double>((*soll)[3]));
            return wieSoll(ist, 0) && wieSoll(ist, 1) && wieSoll(ist, 2) && wieSoll(ist, 3);
        }));
        add(tu("MBNAV: Hand waehlen, alle vier verstellen", [=] {
            t().auswahl = {fernsterTeil()};
            verstellen();
        }));
        add(pause(0.3));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_Z, "Strg+Alt+Z"));
        add(pause(0.4));
        add(pruefe("MBNAV Max M9: Strg+Alt+Z passt nur die aktive Ansicht ein, und zwar auf die ganze Figur trotz Auswahl (Zoom Extents)", [=] {
            const std::array<float, 4> ist = halbe();
            melde("M9 Strg+Alt+Z: halbe Bildhoehen %.2f %.2f %.2f %.2f", static_cast<double>(ist[0]), static_cast<double>(ist[1]), static_cast<double>(ist[2]),
                  static_cast<double>(ist[3]));
            return wieSoll(ist, 3) && !wieSoll(ist, 0) && !wieSoll(ist, 1) && !wieSoll(ist, 2);
        }));
        // Zoom Extents All hat in 3ds Max Strg+Umschalt+Z - im Model-Tab ist das Wiederholen (modellDateiTasten); der Befehl
        // steht im Menue [Ansicht] > Navigation und in der Suche (Kollision im Bericht, nicht still umbelegt)
        add(klickAn("MBNAV: Kopfzeile [Ansicht] der Perspektive", mbkopf::feldOrt(3, 1)));
        add(pause(0.4));
        add(klick(tr(Str::MkBlNavigation)));
        add(pause(0.5));
        add(klick(tr(Str::MnGrenzenAlle)));
        add(pause(0.4));
        add(popupsZu());
        add(pruefe("MBNAV Max M9: [Ansicht] > Navigation > Zoom Grenzen: Alle passt alle Ansichten auf die ganze Figur ein (Zoom Extents All)", [=] {
            const std::array<float, 4> ist = halbe();
            melde("M9 Zoom Extents All: halbe Bildhoehen %.2f %.2f %.2f %.2f", static_cast<double>(ist[0]), static_cast<double>(ist[1]), static_cast<double>(ist[2]),
                  static_cast<double>(ist[3]));
            return wieSoll(ist, 0) && wieSoll(ist, 1) && wieSoll(ist, 2) && wieSoll(ist, 3);
        }));
        add(tu("MBNAV: Fokus zurueck in die Ansicht", [] { t().fokus = true; }));
        add(tasteKombi(ImGuiKey_Z, "Z"));
        add(pause(0.4));
        add(pruefe("MBNAV Max M9: Z mit gewaehlter Hand passt alle Ansichten auf die Auswahl ein", [=] {
            const std::array<float, 4> ist = halbe();
            melde("M9 Z mit Auswahl: halbe Bildhoehen %.2f %.2f %.2f %.2f", static_cast<double>(ist[0]), static_cast<double>(ist[1]), static_cast<double>(ist[2]),
                  static_cast<double>(ist[3]));
            bool ok = true;
            for (std::size_t i = 0; i < 4; ++i) { ok = ok && ist[i] < (*soll)[i] * 0.6F; }
            return ok;
        }));
    }
    f.beginn(start(false, AnsichtArt::Vorne));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_Z, "Strg+Alt+Umschalt+Z"));
    f.pruefung("Max M25: Strg+Alt+Umschalt+Z vergroessert 2x (Zoom In 2X)", [](const Lage& v, const Lage& n) { return gezoomtUm(v, n, 0.5F, "M25 2x heran"); });
    add(tasteKombi(ImGuiMod_Alt | ImGuiMod_Shift | ImGuiKey_Z, "Alt+Umschalt+Z"));
    f.pruefung("Max M25: Alt+Umschalt+Z verkleinert 2x (Zoom Out 2X)", [](const Lage& v, const Lage& n) { return gezoomtUm(v, n, 1.0F, "M25 2x hinaus"); });
    // --- Modi: Zoom (Alt+Z), Bereich (Strg+W), Schieben (Strg+P), Kreisen (Strg+R) -------------------------------------------
    f.beginn(start(false));
    add(tasteKombi(ImGuiMod_Alt | ImGuiKey_Z, "Alt+Z"));
    add(zieh("Max: Zoom-Modus, links nach oben ziehen", neben(0.0F, 0.0F), 0.0F, -60.0F, 0));
    f.pruefung("Max M10: Alt+Z schaltet den Zoom-Modus ein - Ziehen mit links nach oben zoomt heran und waehlt nichts", [](const Lage& v, const Lage& n) {
        melde("M10: halbe Bildhoehe %.3f -> %.3f, Auswahl %d", static_cast<double>(v.halb), static_cast<double>(n.halb), static_cast<int>(t().auswahl.size()));
        return n.halb < v.halb * 0.95F && t().auswahl.empty();
    });
    add(klickAn("Max: Rechtsklick beendet den Modus", neben(0.0F, 0.0F), 1));
    add(pause(0.3));
    add(pruefe("MBNAV Max M10: der Rechtsklick beendet den Modus, ohne das Menue der Ansicht zu oeffnen", [] { return !popupOffen(); }));
    add(popupsZu());
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    f.beginn(tu("MBNAV: nach dem Modus", [] {}));
    add(zieh("Max: nach dem Modus mit links ziehen", neben(0.0F, 0.0F), 0.0F, -60.0F, 0));
    f.pruefung("Max M10: nach dem Rechtsklick zoomt das Ziehen mit links nicht mehr", [](const Lage& v, const Lage& n) { return std::fabs(n.halb / v.halb - 1.0F) < 1e-4F; });
    {
        auto unter = std::make_shared<std::array<float, 3>>();
        f.beginn(start(false, AnsichtArt::Vorne));
        add(tu("MBNAV: Mitte des Bereichs merken", [=] { weltVon(lageVon(), neben(150.0F, 100.0F)(), unter->data()); }));
        add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_W, "Strg+W"));
        add(zieh("Max: Bereich aufziehen", neben(50.0F, 20.0F), 200.0F, 160.0F, 0));
        f.pruefung("Max M11: Strg+W - der aufgezogene Bereich fuellt die Ansicht (Zoom Region)", [=](const Lage& v, const Lage& n) {
            const ModellAnsicht& a = ansicht();
            const float soll = std::max(200.0F / a.groesse[0], 160.0F / a.groesse[1]);
            melde("M11: halbe Bildhoehe %.3f -> %.3f (Faktor %.4f, Soll %.4f), Mitte %.2f Einheiten neben der Mitte des Bereichs", static_cast<double>(v.halb),
                  static_cast<double>(n.halb), static_cast<double>(n.halb / v.halb), static_cast<double>(soll), static_cast<double>(abstandPunkte(n.p, unter->data())));
            return std::fabs(n.halb / v.halb - soll) < 0.01F * soll && abstandPunkte(n.p, unter->data()) < 0.02F * v.halb;
        });
        add(klickAn("Max: Rechtsklick beendet den Modus", neben(0.0F, 0.0F), 1));
        add(pause(0.2));
        add(popupsZu());
        add(tasteKombi(ImGuiKey_Escape, "Esc"));
    }
    f.beginn(start(false));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_P, "Strg+P"));
    add(zieh("Max: Schieben-Modus, mit links ziehen", neben(0.0F, 0.0F), 80.0F, 40.0F, 0));
    f.pruefung("Max M12: Strg+P schaltet den Schieben-Modus ein - Ziehen mit links schiebt 1:1 (Pan View)",
               [](const Lage& v, const Lage& n) { return verschobenUm(v, n, 80.0F, 40.0F, "M12") && t().auswahl.empty(); });
    add(klickAn("Max: Rechtsklick beendet den Modus", neben(0.0F, 0.0F), 1));
    add(pause(0.2));
    add(popupsZu());
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    f.beginn(start(false));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_R, "Strg+R"));
    add(zieh("Max: Orbit-Modus, mit links ziehen", neben(0.0F, 0.0F), 50.0F, 0.0F, 0));
    f.pruefung("Max M13: Strg+R schaltet den Orbit-Modus ein - Ziehen mit links im Kreis kreist frei", [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 0.0F, -20.0F, "M13"); });
    add(klickAn("Max: Rechtsklick beendet den Modus", neben(0.0F, 0.0F), 1));
    add(pause(0.2));
    add(popupsZu());
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    f.beginn(start(false));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_R, "Strg+R"));
    add(zieh("Max: Orbit-Modus, am rechten Griff ziehen", [] {
        const ModellAnsicht& a = ansicht();
        return neben(0.38F * std::min(a.groesse[0], a.groesse[1]), 0.0F)();
    }, 40.0F, 30.0F, 0));
    f.pruefung("Max M13: ein seitlicher Griff des Orbit-Kreises dreht nur um die Hochachse", [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 0.0F, -16.0F, "M13 Griff"); });
    add(klickAn("Max: Rechtsklick beendet den Modus", neben(0.0F, 0.0F), 1));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    f.beginn(start(false));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiKey_R, "Strg+R"));
    add(zieh("Max: Orbit-Modus, ausserhalb des Kreises ziehen", [] {
        const ModellAnsicht& a = ansicht();
        return neben(0.46F * std::min(a.groesse[0], a.groesse[1]), 0.0F)();
    }, 0.0F, 120.0F, 0));
    f.pruefung("Max M13: ausserhalb des Orbit-Kreises rollt die Ansicht um die Blickachse - die Szene folgt der Maus im Uhrzeigersinn", [](const Lage& v, const Lage& n) {
        const float soll = -std::atan2(120.0F, 0.46F * std::min(ansicht().groesse[0], ansicht().groesse[1])) * 180.0F / kPi;
        melde("M13 Rollen: Roll %.2f -> %.2f (Soll %.2f), Blick %.3f Grad gewandert", static_cast<double>(v.w[2]), static_cast<double>(n.w[2]), static_cast<double>(soll),
              static_cast<double>(blickNeben(n, v.w[0], v.w[1])));
        return std::fabs(winkelWeg(v.w[2], n.w[2]) - soll) < 0.1F && blickNeben(n, v.w[0], v.w[1]) < 0.05F;
    });
    add(tasteKombi(ImGuiKey_Escape, "Esc"));
    add(pause(0.2));
    add(pruefe("MBNAV Max M13: Esc beendet den Modus", [] { return t().navModus == 0; }));
    {
        // Orbit Point of Interest: der Punkt unter dem Mauszeiger (neben der Figur: in der Tiefe des Drehpunkts) bleibt stehen
        auto unter = std::make_shared<std::array<float, 3>>();
        f.beginn(start(false));
        add(tu("MBNAV: Orbit Point of Interest, Punkt unter der Maus merken", [=] {
            g_app->settings.modellOrbitArt = 3;
            weltVon(lageVon(), neben(-ansicht().groesse[0] * 0.38F, 80.0F)(), unter->data());
        }));
        add(zieh("Max: Alt+Mitteltaste neben der Figur", [] { return neben(-ansicht().groesse[0] * 0.38F, 80.0F)(); }, 50.0F, 0.0F, kMitte, ImGuiMod_Alt));
        f.pruefung("Max M15: Orbit Point of Interest kreist um den Punkt unter dem Mauszeiger", [=](const Lage&, const Lage& n) {
            const ImVec2 o = schirmVon(n, unter->data());
            const ImVec2 soll = neben(-ansicht().groesse[0] * 0.38F, 80.0F)();
            melde("M15 POI: der Punkt steht %.2f, %.2f neben seiner Stelle, Gier %.2f", static_cast<double>(o.x - soll.x), static_cast<double>(o.y - soll.y), static_cast<double>(n.w[1]));
            return std::hypot(o.x - soll.x, o.y - soll.y) < 1.5F && std::fabs(winkelWeg(90.0F, n.w[1]) + 20.0F) < 0.05F;
        });
        add(tu("MBNAV: Orbit SubObject (Vorgabe)", [] { g_app->settings.modellOrbitArt = 2; }));
    }
    f.beginn(start(false));
    add(tu("MBNAV: Zoom About Mouse Point (Perspektive) aus", [] { g_app->settings.modellZoomMausPersp = false; }));
    add(radAn("Max: Rad neben der Mitte, Einstellung aus", neben(150.0F, 100.0F), 1.0F));
    f.pruefung("Max M7: ohne Zoom About Mouse Point zoomt das Rad um die Bildmitte", [](const Lage& v, const Lage& n) { return n.halb < v.halb * 0.97F && abstandPunkte(v.p, n.p) < 0.001F * v.halb; });
    add(tu("MBNAV: Zoom About Mouse Point wieder an", [] { g_app->settings.modellZoomMausPersp = true; }));
    // --- I: zum Mauszeiger schieben -------------------------------------------------------------------------------------------
    for (int ortho = 0; ortho < 2; ++ortho) {
        auto unter = std::make_shared<std::array<float, 3>>();
        f.beginn(start(false, ortho != 0 ? AnsichtArt::Links : AnsichtArt::Perspektive));
        add(tu("MBNAV: Punkt unter der Maus merken", [=] { weltVon(lageVon(), neben(150.0F, 100.0F)(), unter->data()); }));
        add(mausDann("neben die Mitte", neben(150.0F, 100.0F)));
        add(pause(0.2));
        add(tasteKombi(ImGuiKey_I, "I"));
        f.pruefung(std::string("Max M14: I schiebt den Punkt unter dem Mauszeiger in die Bildmitte (Pan Viewport, ") + (ortho != 0 ? "Ansicht Links" : "Perspektive") + ")",
                   [=](const Lage& v, const Lage& n) {
                       melde("M14 %s: der Punkt unter der Maus liegt %.3f Einheiten neben der neuen Bildmitte (halbe Bildhoehe %.2f)", ortho != 0 ? "Links" : "Perspektive",
                             static_cast<double>(abstandPunkte(n.p, unter->data())), static_cast<double>(v.halb));
                       return abstandPunkte(n.p, unter->data()) < 0.01F * v.halb && std::fabs(n.halb / v.halb - 1.0F) < 1e-3F;
                   });
    }
    // --- Drehpunkt beim Kreisen: die Auswahl (Orbit SubObject / Selected) -------------------------------------------------------
    {
        auto ortVor = std::make_shared<ImVec2>();
        add(start(false));
        add(tu("MBNAV: Hand waehlen", [] {
            t().auswahl = {fernsterTeil()};
            g_app->modelDirty = true;
        }));
        add(pause(0.4));
        add(tu("MBNAV: Schirmort der Auswahl merken", [=] {
            float m[3];
            if (!modellAuswahlMitte(m) || !modellAufSchirm(ansicht(), m, *ortVor)) { meldeFehler("MBNAV M15: keine Auswahl"); }
        }));
        add(zieh("Max: Alt+Mitteltaste mit Auswahl", neben(0.0F, 120.0F), 60.0F, 0.0F, kMitte, ImGuiMod_Alt));
        add(pause(0.4));
        add(pruefe("MBNAV Max M15: mit Auswahl kreist Alt+Mitteltaste um die Mitte der Auswahl - sie bleibt an ihrer Stelle im Bild (Orbit SubObject)", [=] {
            float m[3];
            ImVec2 o{0.0F, 0.0F};
            if (!modellAuswahlMitte(m) || !modellAufSchirm(ansicht(), m, o)) { return false; }
            melde("M15: Mitte der Auswahl im Bild %.1f, %.1f -> %.1f, %.1f (Gier jetzt %.2f)", static_cast<double>(ortVor->x), static_cast<double>(ortVor->y),
                  static_cast<double>(o.x), static_cast<double>(o.y), static_cast<double>(g_app->modelCam.angles[1]));
            return std::hypot(o.x - ortVor->x, o.y - ortVor->y) < 2.0F && std::fabs(winkelWeg(90.0F, g_app->modelCam.angles[1]) + 24.0F) < 0.1F;
        }));
    }
    // --- Ansichtsaenderung rueckgaengig / wiederholen ---------------------------------------------------------------------------
    f.beginn(start(false));
    add(zieh("Max: Alt+Mitteltaste", neben(0.0F, 0.0F), 50.0F, 0.0F, kMitte, ImGuiMod_Alt));
    add(pause(0.2));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Z, "Umschalt+Z"));
    f.pruefung("Max M16: Umschalt+Z nimmt die Ansichtsaenderung zurueck (Undo Viewport Operation)", [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 0.0F, 0.0F, "M16 zurueck"); });
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Y, "Umschalt+Y"));
    f.pruefung("Max M16: Umschalt+Y stellt sie wieder her (Redo Viewport Operation)", [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 0.0F, -20.0F, "M16 wieder"); });
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Y, "Umschalt+Y"));
    add(pause(0.2));
    add(pruefe("MBNAV Max M16: ohne weiteren Schritt sagt Umschalt+Y das in der Statuszeile, statt still nichts zu tun", [] { return t().meldung == tr(Str::MnVerlaufLeer); }));
    // Rechtsklick-Menue der Ansicht und Suche (X): die Navigation ist an beiden Stellen
    f.beginn(start(false, AnsichtArt::Vorne));
    add(klickAn("Max: Rechtsklick in die Ansicht", neben(-150.0F, 60.0F), 1));
    add(pause(0.4));
    add(klick(tr(Str::MkBlNavigation)));
    add(pause(0.5));
    add(klick(tr(Str::MnZoom2xHeran)));
    add(pause(0.3));
    add(popupsZu());
    f.pruefung("Max: das Rechtsklick-Menue der Ansicht hat die Navigation (Vergroessern 2x ausgefuehrt)", [](const Lage& v, const Lage& n) { return gezoomtUm(v, n, 0.5F, "Rechtsklick 2x"); });
    f.beginn(start(false, AnsichtArt::Vorne));
    add(tasteKombi(ImGuiKey_X, "X"));
    add(pause(0.4));
    add(tippe(tr(Str::MnZoom2xHinaus)));
    add(pause(0.3));
    add(tasteKombi(ImGuiKey_Enter, "Enter"));
    f.pruefung("Max: die Suche (X) findet die Navigation und fuehrt sie aus (Verkleinern 2x)", [](const Lage& v, const Lage& n) { return gezoomtUm(v, n, 2.0F, "Suche 2x"); });
    add(popupsZu());
    add(tu("MBNAV: Fokus zurueck in die Ansicht", [] { t().fokus = true; }));
    // --- Ansichten T/B/F/L/P/U, V, Home ----------------------------------------------------------------------------------------
    {
        static const struct {
            ImGuiKey k;
            const char* name;
            AnsichtArt art;
        } kTasten[6] = {{ImGuiKey_T, "T", AnsichtArt::Oben},   {ImGuiKey_B, "B", AnsichtArt::Unten},       {ImGuiKey_F, "F", AnsichtArt::Vorne},
                        {ImGuiKey_L, "L", AnsichtArt::Links}, {ImGuiKey_P, "P", AnsichtArt::Perspektive}, {ImGuiKey_U, "U", AnsichtArt::Orthographisch}};
        add(start(false));
        for (const auto& k : kTasten) {
            add(tasteKombi(k.k, k.name));
            add(pause(0.2));
            add(pruefe(std::string("MBNAV Max M18: ") + k.name + " schaltet die aktive Ansicht um", [art = k.art] { return ansicht().art == art; }));
        }
    }
    add(start(false));
    add(mausDann("in die Ansicht", neben(-150.0F, 60.0F)));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_V, "V"));
    add(pause(0.4));
    add(pruefe("MBNAV Max M19: V oeffnet das Menue der Ansichten am Mauszeiger (Viewports-Quad)", [] { return popupOffen(); }));
    add(klick(tr(Str::MbAnsichtLinks)));
    add(pause(0.4));
    add(pruefe("MBNAV Max M19: der Eintrag Links im Menue schaltet die Ansicht um", [] { return ansicht().art == AnsichtArt::Links && !popupOffen(); }));
    add(popupsZu());
    for (int welche = 0; welche < 2; ++welche) {
        f.beginn(start(false));
        add(tu("MBNAV: Ansicht verdrehen und wegschieben", [] {
            g_app->modelOrbit.turn(g_app->modelCam, 60.0F, 25.0F);
            g_app->modelOrbit.pan(g_app->modelCam, 200.0F, 80.0F);
            g_app->modelDirty = true;
        }));
        add(pause(0.2));
        add(welche == 0 ? tasteKombi(ImGuiMod_Alt | ImGuiKey_Home, "Alt+Pos1") : tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_H, "Alt+Strg+H"));
        f.pruefung(std::string("Max M21: ") + (welche == 0 ? "Alt+Pos1 (Home View)" : "Alt+Strg+H (ViewCube Home)") + " fliegt zur Home-Ansicht",
                   [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 0.0F, 0.0F, "M21") && std::fabs(n.halb / v.halb - 1.0F) < 1e-3F; }, 0.9);
    }
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_V, "Alt+Strg+V"));
    add(pause(0.2));
    add(pruefe("MBNAV Max M21: Alt+Strg+V blendet den ViewCube aus (Toggle ViewCube Visibility)", [] { return !g_app->settings.modellNavigation; }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_V, "Alt+Strg+V"));
    add(pause(0.2));
    add(pruefe("MBNAV Max M21: Alt+Strg+V blendet ihn wieder ein", [] { return g_app->settings.modellNavigation; }));
    // --- ViewCube: jedes der 26 Ziele, Ziehen in einer Achsenansicht -------------------------------------------------------------
    for (int x = -1; x <= 1; ++x) {
        for (int y = -1; y <= 1; ++y) {
            for (int z = -1; z <= 1; ++z) {
                if (x == 0 && y == 0 && z == 0) { continue; }
                const std::string name = std::to_string(x) + " " + std::to_string(y) + " " + std::to_string(z);
                add(tu("MBNAV VC: Blick so, dass das Ziel " + name + " vorne liegt", [=] {
                    modellBedienungSetzen(0);
                    // ohne Kompass: von unten gesehen liegt der Ring vor der Unterseite des Wuerfels und geht dort vor (wie in
                    // der Vorlage: erst Ring und Buchstaben vor dem Wuerfel, dann der Wuerfel)
                    g_app->settings.modellKompass = false;
                    t().vierAnsichten = false;
                    t().aktiv = 3;
                    modellStartansicht(3);
                    // von schraeg vor dem Ziel: die Richtung zum Auge ist das Ziel plus ein Versatz, der keine Achse genau trifft
                    const float e[3] = {static_cast<float>(x) + 0.23F, static_cast<float>(y) - 0.31F, static_cast<float>(z) + 0.27F};
                    const float len = std::sqrt(e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
                    const float w[3] = {std::asin(e[2] / len) * 180.0F / kPi, std::atan2(-e[1], -e[0]) * 180.0F / kPi, 0.0F};
                    modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
                }));
                add(pause(0.3));
                add(klickAn("MBNAV VC: Ziel " + name, wuerfelOrt(x, y, z)));
                add(pause(0.6));
                add(pruefe("MBNAV Max VC: Klick auf das Ziel " + name + " des ViewCube schaut von dort (Perspektive bleibt)", [=] {
                    return ansicht().art == AnsichtArt::Perspektive && mbkopf::winkelZu(static_cast<float>(-x), static_cast<float>(-y), static_cast<float>(-z)) < 0.5F;
                }));
            }
        }
    }
    add(tu("MBNAV VC: Kompass wieder an", [] { g_app->settings.modellKompass = true; }));
    f.beginn(start(false, AnsichtArt::Vorne));
    add(zieh("Max: am ViewCube ziehen (Ansicht Vorne)", wuerfelOrt(0, -1, 0), 50.0F, 30.0F, 0));
    f.pruefung("Max VC: Ziehen am ViewCube in einer Achsenansicht macht daraus Orthographisch und kreist wie Alt+Mitteltaste",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Orthographisch && gedrehtUm(v, n, 12.0F, -20.0F, "VC Ziehen Vorne"); });
}

void blenderSchritte(std::vector<Schritt>& s) {
    const Faelle f{s};
    const auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    // --- Mitteltaste: kreisen (Turntable) --------------------------------------------------------------------------------------
    f.beginn(start(true));
    add(zieh("Blender: Mitteltaste nach rechts", neben(0.0F, 0.0F), 50.0F, 0.0F, kMitte));
    f.pruefung("Blender B1: Mitteltaste kreist (view3d.rotate, Turntable 0,4 Grad je Bildpunkt: 50 Punkte = Gier -20)",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Perspektive && gedrehtUm(v, n, 0.0F, -blGrad(50.0F), "B1 rechts"); });
    f.beginn(start(true));
    add(zieh("Blender: Mitteltaste nach unten", neben(0.0F, 0.0F), 0.0F, 50.0F, kMitte));
    f.pruefung("Blender B1: Mitteltaste nach unten - die Kamera steigt (Nick +20)", [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, blGrad(50.0F), 0.0F, "B1 unten"); });
    f.beginn(start(true));
    add(zieh("Blender: Mitteltaste 300 nach unten", neben(0.0F, -150.0F), 0.0F, 300.0F, kMitte, 0, 30));
    f.pruefung("Blender B1: das Kreisen geht ueber den Pol (kein Kappen bei 89 Grad: Nick 120)", [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, blGrad(300.0F), 0.0F, "B1 Pol"); });
    f.beginn(start(true, AnsichtArt::Vorne));
    add(zieh("Blender: Mitteltaste in Vorne", neben(0.0F, 0.0F), 50.0F, 0.0F, kMitte));
    f.pruefung("Blender B1: Kreisen aus einer Achsenansicht schaltet auf die Perspektive (Auto Perspective)",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Perspektive && gedrehtUm(v, n, 0.0F, -blGrad(50.0F), "B1 Vorne"); });
    f.beginn(start(true));
    add(tu("MBNAV: freie orthographische Ansicht", [] {
        const float w[3] = {20.0F, 60.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Orthographisch, w, false);
    }));
    f.beginn(tu("MBNAV: (User Orthographic)", [] {}));
    add(zieh("Blender: Mitteltaste in der freien Orthoansicht", neben(0.0F, 0.0F), 50.0F, 0.0F, kMitte));
    f.pruefung("Blender B1: die freie orthographische Ansicht bleibt beim Kreisen orthographisch",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Orthographisch && gedrehtUm(v, n, 0.0F, -blGrad(50.0F), "B1 User Ortho"); });
    {
        // Alt WAEHREND des Kreisens rastet auf die Achsenansicht in der Naehe (15 Grad): 20 Bildpunkte = 8 Grad neben Vorne
        auto z = std::make_shared<ImVec2>();
        f.beginn(start(true));
        add(Schritt{"Ziehe Blender: Mitteltaste, dann Alt", [=](int b) {
                        ImGuiIO& io = ImGui::GetIO();
                        if (b == 0) {
                            *z = neben(0.0F, 0.0F)();
                            setzeMaus(*z);
                            return false;
                        }
                        if (b == 2) { io.AddMouseButtonEvent(kMitte, true); }
                        if (b >= 3 && b <= 12) { setzeMaus(ImVec2{z->x + 2.0F * static_cast<float>(b - 2), z->y}); }
                        if (b == 13) { io.AddKeyEvent(ImGuiMod_Alt, true); }
                        if (b == 16) { io.AddMouseButtonEvent(kMitte, false); }
                        if (b == 17) { io.AddKeyEvent(ImGuiMod_Alt, false); }
                        return b >= 21;
                    }});
        f.pruefung("Blender B7: Alt waehrend des Kreisens rastet auf die nahe Achsenansicht (Vorne, orthographisch)", [](const Lage&, const Lage& n) {
            melde("B7 Einrasten: Art %d, Blick %.3f %.3f %.3f", static_cast<int>(n.art), static_cast<double>(n.v[0]), static_cast<double>(n.v[1]), static_cast<double>(n.v[2]));
            return n.art == AnsichtArt::Vorne;
        });
    }
    {
        auto ortVor = std::make_shared<ImVec2>();
        add(start(true));
        add(tu("MBNAV: Orbit Around Selection an, Hand waehlen", [] {
            g_app->settings.modellBlOrbitAuswahl = true;
            t().auswahl = {fernsterTeil()};
            g_app->modelDirty = true;
        }));
        add(pause(0.4));
        add(tu("MBNAV: Schirmort der Auswahl merken", [=] {
            float m[3];
            if (!modellAuswahlMitte(m) || !modellAufSchirm(ansicht(), m, *ortVor)) { meldeFehler("MBNAV B22: keine Auswahl"); }
        }));
        add(zieh("Blender: Mitteltaste mit Auswahl", neben(0.0F, 120.0F), 60.0F, 0.0F, kMitte));
        add(pause(0.4));
        add(pruefe("MBNAV Blender B22: mit Orbit Around Selection kreist die Mitteltaste um die Mitte der Auswahl", [=] {
            float m[3];
            ImVec2 o{0.0F, 0.0F};
            if (!modellAuswahlMitte(m) || !modellAufSchirm(ansicht(), m, o)) { return false; }
            melde("B22: Mitte der Auswahl im Bild %.1f, %.1f -> %.1f, %.1f", static_cast<double>(ortVor->x), static_cast<double>(ortVor->y), static_cast<double>(o.x), static_cast<double>(o.y));
            return std::hypot(o.x - ortVor->x, o.y - ortVor->y) < 2.0F;
        }));
        add(tu("MBNAV: Orbit Around Selection aus (Vorgabe)", [] { g_app->settings.modellBlOrbitAuswahl = false; }));
    }
    {
        auto unter = std::make_shared<std::array<float, 3>>();
        f.beginn(start(true));
        add(tu("MBNAV: Zoom to Mouse Position an, Punkt unter der Maus merken", [=] {
            g_app->settings.modellBlZoomMaus = true;
            weltVon(lageVon(), neben(150.0F, 100.0F)(), unter->data());
        }));
        add(radAn("Blender: Rad neben der Mitte, Zoom to Mouse Position", neben(150.0F, 100.0F), 1.0F));
        f.pruefung("Blender B22: mit Zoom to Mouse Position bleibt der Punkt unter der Maus stehen", [=](const Lage& v, const Lage& n) {
            const ImVec2 o = schirmVon(n, unter->data());
            const ImVec2 m = mitteVon();
            return gezoomtUm(v, n, 1.0F / 1.2F, "B22 Zoom zur Maus") && std::fabs(o.x - m.x - 150.0F) < 1.5F && std::fabs(o.y - m.y - 100.0F) < 1.5F;
        });
        add(tu("MBNAV: Zoom to Mouse Position aus (Vorgabe)", [] { g_app->settings.modellBlZoomMaus = false; }));
    }
    {
        auto unter = std::make_shared<std::array<float, 3>>();
        f.beginn(start(true, AnsichtArt::Vorne));
        add(tu("MBNAV: Mitte des Bereichs merken", [=] { weltVon(lageVon(), neben(150.0F, 100.0F)(), unter->data()); }));
        add(tasteKombi(ImGuiMod_Shift | ImGuiKey_B, "Umschalt+B"));
        add(zieh("Blender: Bereich aufziehen", neben(50.0F, 20.0F), 200.0F, 160.0F, 0));
        f.pruefung("Blender B25: Umschalt+B - der aufgezogene Bereich fuellt die Ansicht, danach ist der Modus zu Ende (view3d.zoom_border)", [=](const Lage& v, const Lage& n) {
            const ModellAnsicht& a = ansicht();
            const float soll = std::max(200.0F / a.groesse[0], 160.0F / a.groesse[1]);
            melde("B25: Faktor %.4f (Soll %.4f), Mitte %.2f daneben, Modus %d", static_cast<double>(n.halb / v.halb), static_cast<double>(soll),
                  static_cast<double>(abstandPunkte(n.p, unter->data())), t().navModus);
            return std::fabs(n.halb / v.halb - soll) < 0.01F * soll && abstandPunkte(n.p, unter->data()) < 0.02F * v.halb && t().navModus == 0;
        });
    }
    // --- Umschalt+Mitte: schieben, Strg+Mitte: zoomen ---------------------------------------------------------------------------
    f.beginn(start(true));
    add(zieh("Blender: Umschalt+Mitteltaste", neben(0.0F, 0.0F), 120.0F, 60.0F, kMitte, ImGuiMod_Shift));
    f.pruefung("Blender B2: Umschalt+Mitteltaste schiebt 1:1 (view3d.move)",
               [](const Lage& v, const Lage& n) { return verschobenUm(v, n, 120.0F, 60.0F, "B2") && blickNeben(n, v.w[0], v.w[1]) < 0.01F; });
    for (int ortho = 0; ortho < 2; ++ortho) {
        f.beginn(start(true, ortho != 0 ? AnsichtArt::Vorne : AnsichtArt::Perspektive));
        add(zieh("Blender: Strg+Mitteltaste nach oben", neben(40.0F, 0.0F), 0.0F, -100.0F, kMitte, ImGuiMod_Ctrl));
        f.pruefung(std::string("Blender B3: Strg+Mitteltaste zoomt wie view3d.zoom (Dolly-Formel, ") + (ortho != 0 ? "Achsenansicht" : "Perspektive") + ")",
                   [=](const Lage& v, const Lage& n) {
                       const float halbe = ansicht().groesse[1] * 0.5F;
                       return (ortho == 0 || n.art == AnsichtArt::Vorne) && gezoomtUm(v, n, dollyFaktor(halbe, halbe - 100.0F), "B3") && abstandPunkte(v.p, n.p) < 0.01F * v.halb;
                   });
    }
    // --- Alt+Mitte: ziehen rastet, klicken zentriert ----------------------------------------------------------------------------
    {
        static const struct {
            float dx;
            float dy;
            const char* name;
            AnsichtArt art;
        } kZug[4] = {{60.0F, 0.0F, "rechts (Ost)", AnsichtArt::Links},
                     {-60.0F, 0.0F, "links (West)", AnsichtArt::Rechts},
                     {0.0F, -60.0F, "oben (Nord)", AnsichtArt::Unten},
                     {0.0F, 60.0F, "unten (Sued)", AnsichtArt::Oben}};
        for (const auto& z : kZug) {
            f.beginn(start(true, AnsichtArt::Vorne));
            add(zieh(std::string("Blender: Alt+Mitteltaste nach ") + z.name, neben(0.0F, 0.0F), z.dx, z.dy, kMitte, ImGuiMod_Alt));
            f.pruefung(std::string("Blender B5: Alt+Mitteltaste aus Vorne nach ") + z.name + " ziehen rastet auf die Nachbaransicht (view_axis relativ)",
                       [art = z.art, name = z.name](const Lage& v, const Lage& n) {
                           melde("B5 %s: Art %d -> %d, Blick %.2f %.2f %.2f", name, static_cast<int>(v.art), static_cast<int>(n.art), static_cast<double>(n.v[0]),
                                 static_cast<double>(n.v[1]), static_cast<double>(n.v[2]));
                           return n.art == art;
                       },
                       0.5);
        }
    }
    {
        auto unter = std::make_shared<std::array<float, 3>>();
        f.beginn(start(true));
        add(tu("MBNAV: Punkt unter der Maus merken (neben der Figur)", [=] {
            const ModellAnsicht& a = ansicht();
            weltVon(lageVon(), neben(-a.groesse[0] * 0.38F, 80.0F)(), unter->data());
        }));
        add(klickMit("Blender: Alt+Mitteltaste klicken", [] { return neben(-ansicht().groesse[0] * 0.38F, 80.0F)(); }, kMitte, ImGuiMod_Alt));
        f.pruefung("Blender B6: Alt+Mitteltaste klicken legt den Punkt unter der Maus in die Bildmitte (view_center_pick)", [=](const Lage& v, const Lage& n) {
            melde("B6: der Punkt unter der Maus liegt %.3f Einheiten neben der neuen Bildmitte", static_cast<double>(abstandPunkte(n.p, unter->data())));
            return abstandPunkte(n.p, unter->data()) < 0.01F * v.halb && std::fabs(n.halb / v.halb - 1.0F) < 1e-3F;
        }, 0.6);
    }
    // --- Rad, Ziffernblock +/-, Strg+= / Strg+- -----------------------------------------------------------------------------------
    for (int ortho = 0; ortho < 2; ++ortho) {
        f.beginn(start(true, ortho != 0 ? AnsichtArt::Vorne : AnsichtArt::Perspektive));
        add(radAn("Blender: Rad heran, neben der Mitte", neben(150.0F, 100.0F), 1.0F));
        f.pruefung(std::string("Blender B8: eine Raste heran teilt den Abstand durch 1,2, der Drehpunkt bleibt (nicht zur Maus, ") + (ortho != 0 ? "Achsenansicht" : "Perspektive") + ")",
                   [](const Lage& v, const Lage& n) { return gezoomtUm(v, n, 1.0F / 1.2F, "B8 heran") && abstandPunkte(v.p, n.p) < 0.01F * v.halb; });
        add(radAn("Blender: Rad zwei Rasten hinaus", neben(150.0F, 100.0F), -2.0F));
        f.pruefung("Blender B8: zwei Rasten hinaus - mal 1,2 gegenueber dem Anfang", [](const Lage& v, const Lage& n) { return gezoomtUm(v, n, 1.2F, "B8 hinaus"); });
    }
    {
        static const struct {
            ImGuiKeyChord chord;
            const char* name;
            float faktor;
        } kZoom[4] = {{ImGuiKey_KeypadAdd, "Ziffernblock +", 1.0F / 1.2F},
                      {ImGuiKey_KeypadSubtract, "Ziffernblock -", 1.2F},
                      {ImGuiMod_Ctrl | ImGuiKey_Equal, "Strg+=", 1.0F / 1.2F},
                      {ImGuiMod_Ctrl | ImGuiKey_Minus, "Strg+-", 1.2F}};
        for (const auto& z : kZoom) {
            f.beginn(start(true));
            add(tasteKombi(z.chord, z.name));
            f.pruefung(std::string("Blender B9: ") + z.name + " zoomt wie eine Raste des Rads",
                       [faktor = z.faktor, name = z.name](const Lage& v, const Lage& n) { return gezoomtUm(v, n, faktor, name); });
        }
    }
    // --- Ziffernblock -------------------------------------------------------------------------------------------------------------
    {
        auto weich = std::make_shared<bool>(false);
        f.beginn(start(true));
        add(tu("MBNAV: schraeg schauen", [] {
            g_app->modelOrbit.turn(g_app->modelCam, -20.0F, 20.0F);
            g_app->modelDirty = true;
        }));
        f.beginn(tu("MBNAV: (schraeg)", [] {}));
        add(tasteUndWeich(ImGuiKey_Keypad1, "Ziffernblock 1", weich));
        f.pruefung("Blender B10: Ziffernblock 1 dreht WEICH auf Vorne (orthographisch), Mitte und Groesse bleiben", [=](const Lage& v, const Lage& n) {
            melde("B10: nach der Taste laeuft ein weicher Wechsel: %d, am Ende Art %d, Mitte %.3f gewandert, Groesse %.4f", *weich ? 1 : 0,
                  static_cast<int>(n.art), static_cast<double>(abstandPunkte(v.p, n.p)), static_cast<double>(n.halb / v.halb));
            return *weich && n.art == AnsichtArt::Vorne && abstandPunkte(v.p, n.p) < 0.01F * v.halb && std::fabs(n.halb / v.halb - 1.0F) < 1e-3F;
        }, 0.5);
        add(tasteKombi(ImGuiKey_Keypad5, "Ziffernblock 5"));
        f.pruefung("Blender B11: Ziffernblock 5 aus Vorne - Perspektive von vorne", [](const Lage&, const Lage& n) {
            return n.art == AnsichtArt::Perspektive && blickNeben(n, 0.0F, 90.0F) < 0.05F;
        });
    }
    f.beginn(start(true));
    add(tu("MBNAV: schraeg schauen", [] {
        g_app->modelOrbit.turn(g_app->modelCam, -20.0F, 20.0F);
        g_app->modelDirty = true;
    }));
    f.beginn(tu("MBNAV: (schraeg)", [] {}));
    add(tasteKombi(ImGuiKey_Keypad5, "Ziffernblock 5"));
    f.pruefung("Blender B11: Ziffernblock 5 aus der Perspektive - orthographisch in DERSELBEN Richtung (view_persportho), Mitte und Groesse bleiben",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Orthographisch && gedrehtUm(v, n, 0.0F, 0.0F, "B11") && std::fabs(n.halb / v.halb - 1.0F) < 1e-3F; });
    add(tasteKombi(ImGuiKey_Keypad5, "Ziffernblock 5"));
    f.pruefung("Blender B11: noch einmal 5 - wieder die Perspektive in derselben Richtung",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Perspektive && gedrehtUm(v, n, 0.0F, 0.0F, "B11 zurueck") && std::fabs(n.halb / v.halb - 1.0F) < 1e-3F; });
    {
        static const struct {
            ImGuiKeyChord chord;
            const char* name;
            float nick;
            float gier;
        } kKreis[4] = {{ImGuiKey_Keypad4, "Ziffernblock 4 (Orbit Left: die Kamera geht nach links)", 0.0F, -15.0F},
                       {ImGuiKey_Keypad6, "Ziffernblock 6 (Orbit Right)", 0.0F, 15.0F},
                       {ImGuiKey_Keypad8, "Ziffernblock 8 (Orbit Up: die Kamera steigt)", 15.0F, 0.0F},
                       {ImGuiKey_Keypad2, "Ziffernblock 2 (Orbit Down)", -15.0F, 0.0F}};
        for (const auto& k : kKreis) {
            f.beginn(start(true));
            add(tasteKombi(k.chord, k.name));
            f.pruefung(std::string("Blender B12: ") + k.name + " - 15 Grad um den Drehpunkt",
                       [nick = k.nick, gier = k.gier, name = k.name](const Lage& v, const Lage& n) { return gedrehtUm(v, n, nick, gier, name); }, 0.5);
        }
    }
    f.beginn(start(true, AnsichtArt::Vorne));
    add(tasteKombi(ImGuiKey_Keypad4, "Ziffernblock 4"));
    f.pruefung("Blender B12: Ziffernblock 4 aus Vorne - Perspektive (Auto Perspective)",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Perspektive && gedrehtUm(v, n, 0.0F, -15.0F, "B12 Vorne"); }, 0.5);
    f.beginn(start(true));
    add(tu("MBNAV: freie orthographische Ansicht", [] {
        const float w[3] = {20.0F, 60.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Orthographisch, w, false);
    }));
    f.beginn(tu("MBNAV: (User Orthographic)", [] {}));
    add(tasteKombi(ImGuiKey_Keypad4, "Ziffernblock 4"));
    f.pruefung("Blender B12: Ziffernblock 4 in der freien orthographischen Ansicht - sie bleibt orthographisch",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Orthographisch && gedrehtUm(v, n, 0.0F, -15.0F, "B12 User Ortho"); }, 0.5);
    f.beginn(start(true));
    for (int i = 0; i < 7; ++i) {
        add(tasteKombi(ImGuiKey_Keypad8, "Ziffernblock 8"));
        add(pause(0.3));
    }
    f.pruefung("Blender B12: siebenmal Ziffernblock 8 geht ueber den Pol (Nick 105, kein Kappen)", [](const Lage& v, const Lage& n) { return gedrehtUm(v, n, 105.0F, 0.0F, "B12 Pol"); });
    {
        static const struct {
            ImGuiKeyChord chord;
            const char* name;
            float dx;
            float dy;
        } kPan[4] = {{ImGuiMod_Ctrl | ImGuiKey_Keypad4, "Strg+Ziffernblock 4 (Pan Left)", 32.0F, 0.0F},
                     {ImGuiMod_Ctrl | ImGuiKey_Keypad6, "Strg+Ziffernblock 6 (Pan Right)", -32.0F, 0.0F},
                     {ImGuiMod_Ctrl | ImGuiKey_Keypad8, "Strg+Ziffernblock 8 (Pan Up)", 0.0F, 25.0F},
                     {ImGuiMod_Ctrl | ImGuiKey_Keypad2, "Strg+Ziffernblock 2 (Pan Down)", 0.0F, -25.0F}};
        for (const auto& k : kPan) {
            f.beginn(start(true));
            add(tasteKombi(k.chord, k.name));
            f.pruefung(std::string("Blender B13: ") + k.name + " schiebt das Bild um 32 bzw. 25 Bildpunkte (view3d.view_pan)",
                       [dx = k.dx, dy = k.dy, name = k.name](const Lage& v, const Lage& n) { return verschobenUm(v, n, dx, dy, name); });
        }
    }
    f.beginn(start(true));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Keypad4, "Umschalt+Ziffernblock 4"));
    f.pruefung("Blender B14: Umschalt+Ziffernblock 4 rollt um 15 Grad - die Szene dreht im Bild gegen den Uhrzeigersinn (Roll +15)", [](const Lage& v, const Lage& n) {
        melde("B14 links: Roll %.2f -> %.2f", static_cast<double>(v.w[2]), static_cast<double>(n.w[2]));
        return std::fabs(winkelWeg(v.w[2], n.w[2]) - 15.0F) < 0.05F && blickNeben(n, v.w[0], v.w[1]) < 0.05F;
    });
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Keypad6, "Umschalt+Ziffernblock 6"));
    add(pause(0.4));
    add(tasteKombi(ImGuiMod_Shift | ImGuiKey_Keypad6, "Umschalt+Ziffernblock 6"));
    f.pruefung("Blender B14: zweimal Umschalt+Ziffernblock 6 rollt zurueck und 15 Grad im Uhrzeigersinn (Roll -15)", [](const Lage& v, const Lage& n) {
        melde("B14 rechts: Roll %.2f -> %.2f", static_cast<double>(v.w[2]), static_cast<double>(n.w[2]));
        return std::fabs(winkelWeg(v.w[2], n.w[2]) + 15.0F) < 0.05F;
    });
    // --- Auswahl einpassen, alles zeigen ------------------------------------------------------------------------------------------
    f.beginn(start(true));
    add(tu("MBNAV: wegzoomen und schieben", [] {
        g_app->modelOrbit.zoom(g_app->modelCam, -6.0F);
        g_app->modelOrbit.pan(g_app->modelCam, 150.0F, 60.0F);
        g_app->modelDirty = true;
    }));
    f.beginn(tu("MBNAV: (verstellt)", [] {}));
    add(tasteKombi(ImGuiKey_KeypadDecimal, "Ziffernblock ,"));
    f.pruefung("Blender B16: Ziffernblock , ohne Auswahl aendert nichts (view3d.view_selected)",
               [](const Lage& v, const Lage& n) { return abstandPunkte(v.p, n.p) < 1e-3F * v.halb && std::fabs(n.halb / v.halb - 1.0F) < 1e-4F; }, 0.5);
    for (int was = 0; was < 3; ++was) {
        static const char* const kName[3] = {"Ziffernblock , mit gewaehlter Hand (view_selected)", "Pos1 (view_all)", "Umschalt+C (view_all, center)"};
        f.beginn(start(true));
        add(tu("MBNAV: schraeg, wegzoomen, schieben", [=] {
            g_app->modelOrbit.turn(g_app->modelCam, -30.0F, 15.0F);
            g_app->modelOrbit.zoom(g_app->modelCam, -6.0F);
            g_app->modelOrbit.pan(g_app->modelCam, 150.0F, 60.0F);
            if (was == 0) { t().auswahl = {fernsterTeil()}; }
            g_app->modelDirty = true;
        }));
        f.beginn(tu("MBNAV: (verstellt)", [] {}));
        add(was == 0 ? tasteKombi(ImGuiKey_KeypadDecimal, "Ziffernblock ,") : (was == 1 ? tasteKombi(ImGuiKey_Home, "Pos1") : tasteKombi(ImGuiMod_Shift | ImGuiKey_C, "Umschalt+C")));
        f.pruefung(std::string("Blender B16/B17: ") + kName[was] + " - Mitte des Kastens in der Bildmitte, die groesste Kastenseite mal 0,7 ist die halbe kleinere Bildseite, die Richtung bleibt",
                   [=](const Lage& v, const Lage& n) {
                       float mn[3];
                       float mx[3];
                       if (!kasten(was == 0, mn, mx)) { return false; }
                       const float mitte[3] = {(mn[0] + mx[0]) * 0.5F, (mn[1] + mx[1]) * 0.5F, (mn[2] + mx[2]) * 0.5F};
                       const float seite = std::max({mx[0] - mn[0], mx[1] - mn[1], mx[2] - mn[2]});
                       melde("%s: Kasten %.2f x %.2f x %.2f, Mitte %.3f daneben, halbe kleinere Bildseite %.3f (Soll %.3f)", kName[was], static_cast<double>(mx[0] - mn[0]),
                             static_cast<double>(mx[1] - mn[1]), static_cast<double>(mx[2] - mn[2]), static_cast<double>(abstandPunkte(n.p, mitte)),
                             static_cast<double>(halbKlein(n)), static_cast<double>(seite * 0.7F));
                       return abstandPunkte(n.p, mitte) < 0.01F * seite && std::fabs(halbKlein(n) / (seite * 0.7F) - 1.0F) < 0.01F && blickNeben(n, v.w[0], v.w[1]) < 0.05F;
                   },
                   0.6);
    }
    // --- Kuchenmenue, Vierer-Ansicht ----------------------------------------------------------------------------------------------
    add(start(true));
    add(mausDann("in die Ansicht", neben(-100.0F, 40.0F)));
    add(pause(0.2));
    add(tasteKombi(ImGuiKey_GraveAccent, "`"));
    add(pause(0.4));
    add(pruefe("MBNAV Blender B19: ` oeffnet das Kuchenmenue View am Mauszeiger", [] { return popupOffen(); }));
    add(klick(tr(Str::MbAnsichtOben)));
    add(pause(0.5));
    add(pruefe("MBNAV Blender B19: der Eintrag Top des Kuchenmenues schaut von oben", [] { return ansicht().art == AnsichtArt::Oben && !popupOffen(); }));
    add(popupsZu());
    add(start(true));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_Q, "Strg+Alt+Q"));
    add(pause(0.5));
    add(pruefe("MBNAV Blender B20: Strg+Alt+Q - Vierer-Ansicht wie Blender (oben links Top, oben rechts die Perspektive, unten links Front, unten rechts Right)", [] {
        if (!t().vierAnsichten) { return false; }
        const int lo = ansichtIn(false, false);
        const int ro = ansichtIn(false, true);
        const int lu = ansichtIn(true, false);
        const int ru = ansichtIn(true, true);
        if (lo < 0 || ro < 0 || lu < 0 || ru < 0) { return false; }
        melde("B20: oben links %d, oben rechts %d, unten links %d, unten rechts %d (Art)", static_cast<int>(ansicht(lo).art), static_cast<int>(ansicht(ro).art),
              static_cast<int>(ansicht(lu).art), static_cast<int>(ansicht(ru).art));
        return ansicht(lo).art == AnsichtArt::Oben && ansicht(ro).art == AnsichtArt::Perspektive && ansicht(lu).art == AnsichtArt::Vorne && ansicht(ru).art == AnsichtArt::Rechts;
    }));
    add(tasteKombi(ImGuiMod_Ctrl | ImGuiMod_Alt | ImGuiKey_Q, "Strg+Alt+Q"));
    add(pause(0.3));
    add(pruefe("MBNAV Blender B20: noch einmal Strg+Alt+Q - wieder eine Ansicht", [] { return !t().vierAnsichten; }));
    // --- Gizmo ----------------------------------------------------------------------------------------------------------------------
    f.beginn(start(true));
    add(zieh("Blender: Gizmo Zoom nach oben ziehen", gizmoOrt(6), 0.0F, -80.0F, 0));
    f.pruefung("Blender B23: der Zoom-Knopf des Gizmos zoomt wie view3d.zoom aus der Bildmitte (Dolly-Formel)", [](const Lage& v, const Lage& n) {
        const float halbe = ansicht().groesse[1] * 0.5F;
        return gezoomtUm(v, n, dollyFaktor(halbe, halbe - 80.0F), "B23 Zoom") && abstandPunkte(v.p, n.p) < 0.01F * v.halb;
    });
    f.beginn(start(true));
    add(zieh("Blender: Gizmo Verschieben ziehen", gizmoOrt(7), -60.0F, 30.0F, 0));
    f.pruefung("Blender B23: der Verschieben-Knopf des Gizmos schiebt 1:1 (view3d.move)", [](const Lage& v, const Lage& n) { return verschobenUm(v, n, -60.0F, 30.0F, "B23 Verschieben"); });
    f.beginn(start(true));
    add(tu("MBNAV: schraeg schauen", [] {
        g_app->modelOrbit.turn(g_app->modelCam, -20.0F, 20.0F);
        g_app->modelDirty = true;
    }));
    f.beginn(tu("MBNAV: (schraeg)", [] {}));
    add(klickAn("Blender: Gizmo Persp/Ortho", gizmoOrt(8)));
    f.pruefung("Blender B23: der Knopf Persp/Ortho des Gizmos schaltet in derselben Richtung um (view_persportho)",
               [](const Lage& v, const Lage& n) { return n.art == AnsichtArt::Orthographisch && gedrehtUm(v, n, 0.0F, 0.0F, "B23 Persp/Ortho"); });
    add(start(true));
    add(pause(0.3));
    add(klickAn("Blender: Gizmo -Y bei gerader Sicht von vorne (Perspektive)", gizmoOrt(3)));
    add(pause(0.5));
    add(pruefe("MBNAV Blender B23: schaut man genau entlang einer Achse, waehlt der Klick in die Mitte des Gizmos die Gegenseite (Back)", [] {
        const Lage n = lageVon();
        melde("B23 Mitte: Art %d, Blick %.2f %.2f %.2f", static_cast<int>(n.art), static_cast<double>(n.v[0]), static_cast<double>(n.v[1]), static_cast<double>(n.v[2]));
        return n.art == AnsichtArt::Hinten;
    }));
}

// Die Tasten der Navigation, die im Model-Tab schon etwas anderes tun (Bereichstasten gehen in ihrem Bereich vor): als Liste
// ins Protokoll - fuer den Bericht statt stiller Umbelegung.
Schritt kollisionen() {
    return tu("MBNAV: Tastenkollisionen auflisten", [] {
        for (int bl = 0; bl < 2; ++bl) {
            const int zahl = bl != 0 ? modellBlenderTasteZahl() : modellTasteZahl();
            for (int i = 0; i < zahl; ++i) {
                const ImGuiKeyChord c = bl != 0 ? modellBlenderTasteChord(i) : modellTasteChord(i);
                const char* name = bl != 0 ? modellBlenderTasteText(i) : modellTasteText(i);
                for (int j = i + 1; j < zahl; ++j) {
                    if ((bl != 0 ? modellBlenderTasteChord(j) : modellTasteChord(j)) == c) {
                        melde("Kollision %s: %s und %s auf derselben Taste", bl != 0 ? "Blender" : "Max", name, bl != 0 ? modellBlenderTasteText(j) : modellTasteText(j));
                    }
                }
                for (int k = 0; k < modellMeshAktionZahl(); ++k) {
                    if (modellMeshAktionChord(k, bl != 0) == c && c != 0) { melde("Kollision %s (Bereich Modellieren geht vor): %s - %s", bl != 0 ? "Blender" : "Max", name, modellMeshAktionText(k)); }
                }
                for (int k = 0; k < modellKurvenAktionZahl(); ++k) {
                    if (modellKurvenAktionChord(k, bl != 0) == c && c != 0) { melde("Kollision %s (Bereich Animation geht vor): %s - %s", bl != 0 ? "Blender" : "Max", name, modellKurvenAktionText(k)); }
                }
            }
        }
    });
}

// Die Bilder des Handbuchs (Seite "Model-Tab: Ansehen und Pruefen", Abschnitt Navigation): h_mbn_max, h_mbn_orbit, h_mbn_blender.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { add(std::move(x)); }
    auto teiler = std::make_shared<float>(0.0F);
    add(tu("MBNAV Handbuch: breite Anordnung, Max, BOTH_STAND1, Startansicht", [=] {
        *teiler = g_app->settings.splitModel;
        g_app->settings.splitModel = 0.85F;
        g_app->settings.modellAnzeigeUmzugGesehen = true;
        g_app->settings.modellNavigation = true;
        modellBedienungSetzen(0);
        animWaehlen(mb1::animNach("BOTH_STAND1") >= 0 ? mb1::animNach("BOTH_STAND1") : 0, false);
    }));
    add(pause(0.5));
    add(tu("MBNAV Handbuch: Startansicht", [] {
        t().vierAnsichten = false;
        t().aktiv = 3;
        modellStartansicht(3);
        t().fokus = true;
    }));
    add(pause(0.5));
    add(klickAn("MBNAV Handbuch: [Ansicht]", mbkopf::feldOrt(3, 1)));
    add(pause(0.4));
    add(klick(tr(Str::MkBlNavigation)));
    add(pause(0.7));
    add(hilfe2::schrittFoto("h_mbn_max"));
    add(popupsZu());
    add(pause(0.3));
    add(tu("MBNAV Handbuch: Orbit-Modus, schraeg von vorne", [] {
        const float w[3] = {18.0F, 62.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        t().fokus = true;
        if (t().navModus != 3) { modellNavModusSetzen(3); }
        setzeMaus(neben(120.0F, -60.0F)());
    }));
    add(pause(0.6));
    add(hilfe2::schrittFoto("h_mbn_orbit"));
    add(tu("MBNAV Handbuch: Blender, Startansicht", [] {
        modellNavModusSetzen(0);
        modellBedienungSetzen(1);
        modellStartansicht(3);
        const float w[3] = {14.0F, 66.0F, 0.0F};
        modellBlickSetzen(3, AnsichtArt::Perspektive, w, false);
        t().fokus = true;
        setzeMaus(neben(-260.0F, 40.0F)());
    }));
    add(pause(0.5));
    add(tasteKombi(ImGuiKey_GraveAccent, "`"));
    add(pause(0.6));
    add(hilfe2::schrittFoto("h_mbn_blender"));
    add(popupsZu());
    add(tu("MBNAV Handbuch: zurueck", [=] {
        g_app->settings.splitModel = *teiler;
        modellBedienungSetzen(0);
        modellStartansicht(3);
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    add(pause(0.2));
    return s;
}

std::vector<Schritt> navSchritte(const std::string& n) {
    std::vector<Schritt> s;
    s.push_back(tu("MBNAV: Skript fuer den Test", [] {
        openScriptFromMemory("//Generated by BehavEd\nwait ( 1000.000 );\n", "mbnav.txt");
        g_app->settings.modellAnzeigeUmzugGesehen = true;
    }));
    for (Schritt& x : mb1::laden("models/players/stormtrooper/model.glm")) { s.push_back(std::move(x)); }
    s.push_back(pause(0.6));
    s.push_back(kollisionen());
    if (n != "navblender") { maxSchritte(s); }
    if (n != "navmax") { blenderSchritte(s); }
    s.push_back(tu("MBNAV: zurueck", [] {
        modellBedienungSetzen(0);
        t().vierAnsichten = false;
        t().aktiv = 3;
        t().auswahl.clear();
        modellStartansicht(3);
        g_app->settings.modellNavigation = true;
        g_app->settings.modellKompass = true;
        g_app->settings.modellOrbitArt = 2;
        g_app->settings.modellZoomMausPersp = true;
        g_app->settings.modellBlOrbitAuswahl = false;
        g_app->settings.modellBlZoomMaus = false;
        g_app->leftMode = 1;
        g_app->doc.markSaved();
    }));
    s.push_back(pause(0.2));
    return s;
}

std::vector<Schritt> schritte(const std::string& n) {
    if (n == "nav" || n == "navmax" || n == "navblender") { return navSchritte(n); }
    return {};
}

}  // namespace mbnav
