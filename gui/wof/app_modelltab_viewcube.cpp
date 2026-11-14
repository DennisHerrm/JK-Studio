// SPDX-License-Identifier: BSD-3-Clause
// Copyright (c) 2026, WhiteoutDex Contributors
// Copyright (c) 2026, Fernando Sahmkow
//
// gui/wof/app_modelltab_viewcube.cpp - MBKOPF (rc612): ViewCube (Bedienmodus wie 3ds Max) und Navigations-Gizmo (wie
// Blender) in den Ansichten des Model-Tabs
//
// Der ViewCube ist eine Portierung von WhiteoutDex src/native_sources/WhiteoutDexRenderer/max_viewcube.h (Stand 09.11.2026,
// "A view cube that behaves like 3ds Max's"), das seinerseits WhiteoutFlakes tools/common/imgui_viewcube.h folgt: 26
// Klickziele (6 Flaechen, 12 Kanten, 8 Ecken; das Ziel unter der Maus leuchtet, ein Klick dreht in kurzer Animation
// dorthin), Ziehen am Wuerfel kreist, darunter der Kompassring (N/O/S/W: Ziehen dreht um die Hochachse, ein Buchstabe
// schaut von dort), der Home-Knopf (Haus) oben links - Home fliegt -, mit Blick gerade auf eine Flaeche vier Pfeile zu
// den Nachbarflaechen und zwei Rollpfeile (90 Grad um die Blickachse), Rechtsklick: Home, aktuelle Ansicht als Home
// setzen, orthographisch; halb durchsichtig, bis die Maus darueber ist. Nur ImDrawList.
// Gegenueber der Vorlage: je Ansicht ein Wuerfel (dort einer), Masse mit der Schriftgroesse, Beschriftung in der
// Programmsprache, die Kamera ist die des Model-Tabs (Quake-Winkel; die Perspektive bleibt Perspektive, in einer
// Orthoansicht gibt eine Flaeche ihre Achsenansicht und alles andere "Orthographisch"), genau senkrecht statt 88,8 Grad,
// die Unterseite wie die Ansicht [Unten], der Ring folgt der Maus auch von unten gesehen, Norden einstellbar, Tooltips.
// Das Gizmo wie Blender (Navigate Gizmo: Achsenkugeln X/Y/Z mit Gegenkugeln, Klick = Achsenansicht, erneuter Klick =
// Gegenseite, Ziehen = Kreisen, darunter Zoom, Verschieben, Perspektive/Orthographisch) ist eigene Zeichnung, keine
// Grafik von Autodesk oder Blender; die Achsenkugeln zeichnet seit MBRASTER modellGizmoKugelnZeichnen in
// gui/app_modelltab_ansicht.cpp nach Blender gizmo_axis_draw (GPL-2.0-or-later, dort vermerkt).
// TEST1411 L (shank 14.11.: "der viewcube zeigt das W durch und wenn ich da dann drauf klicke schwingt die Kamera ... So ist es nicht in
// 3ds max"): die Kompassbuchstaben wieder in der Tiefe wie die Vorlage und 3ds Max - hinter dem Wuerfel verdeckt er sie beim Zeichnen
// und beim Treffen (MBRASTER hatte sie obenauf gelegt).
// Achsen: eine Figur schaut in ihrer .glm nach -Y (REST M9; der Selbsttest kopfrot misst es am Skelett) - Vorne ist die
// Flaeche -Y, Links -X, Oben +Z wie die Ansichten (ansichtAchsen). Norden liegt wie in 3ds Max hinter der Flaeche Vorne
// (Welt +Y): wer der Figur ins Gesicht schaut, schaut nach Norden; S steht unter Vorne.
//
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
// following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
//    disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
//    products derived from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES,
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// AI-Derived Works Notice (WhiteoutFlakes LICENSE-AI.md): For the purposes of the BSD-3-Clause License, any code generated
// by automated systems, machine learning models, or AI tools that is derived from, based on, or generated using this
// software as input shall be considered a derivative work. Redistributions of such AI-derived works must retain the
// original copyright notice, license conditions, and disclaimer as required by the BSD-3-Clause License.
// (Diese Datei ist ein solches Werk: von einer KI nach dem Lesen der genannten Quellen geschrieben.)
#include "../app_internal.h"
#include "../ui_menue.h"   // KLEIN2: ui::menue/popup - Menues bleiben im Fenster

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>

namespace bhed::gui {
namespace {

ModellTab& mt() { return g_app->mtab; }

constexpr float kPi = 3.14159265F;

struct V3 {
    float x = 0.0F;
    float y = 0.0F;
    float z = 0.0F;
};
V3 operator+(V3 a, V3 b) { return {a.x + b.x, a.y + b.y, a.z + b.z}; }
V3 operator*(V3 a, float s) { return {a.x * s, a.y * s, a.z * s}; }
float dot(V3 a, V3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
V3 kreuz(V3 a, V3 b) { return {a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x}; }

// Was ein ViewCube bzw. Gizmo je Ansicht zwischen den Bildern behaelt (max_viewcube.h State: was beim Druecken unter der
// Maus lag - darauf wirkt der Klick, auch wenn die Hand zuckt -, und der Zug), dazu die Lage fuer den Selbsttest.
struct Lage {
    ImVec2 mitte{};
    float skala = 0.0F;
    V3 r{};
    V3 h{};
    V3 v{};
    bool blender = false;
    bool da = false;
    ImVec2 druck{};
    // ViewCube
    enum class Zug { Keiner, Wuerfel, Ring } zug = Zug::Keiner;
    bool zugBewegt = false;
    enum class Ziel { Keins, Home, Rollen, Buchstabe, Pfeil, Zone } ziel = Ziel::Keins;
    int zielIndex = -1;
    V3 zielRichtung{};
    float ringWinkel = 0.0F;   // Winkel der Maus um die Ringmitte im letzten Bild
    bool kreist = false;       // MBNAV: der Zug am Wuerfel bzw. Gizmo hat schon gedreht
    ImVec2 geschoben{};        // MBNAV: Weg, um den der Verschieben-Knopf des Gizmos in diesem Zug schon geschoben hat
    float alpha = 0.0F;
    ImVec2 home{};
    bool kompass = false;
    int buchstabeVerdeckt = 0;   // TEST1411 L: Bit k = Buchstabe k (N O S W) liegt hinter dem Wuerfel
    std::array<ImVec2, 4> buchstabe{};   // N O S W
    ImVec2 ringPunkt{};
    bool pfeile = false;
    std::array<ImVec2, 4> pfeil{};       // oben, unten, links, rechts
    std::array<ImVec2, 2> rollPfeil{};   // rechts oben, links oben
    // Gizmo
    std::array<ImVec2, 6> kugel{};   // +X -X +Y -Y +Z -Z
    float kugelR = 0.0F;
    std::array<ImVec2, 4> knopf{};   // von oben wie Blender: Zoom, Verschieben, Kamera, Persp/Ortho
    float knopfR = 0.0F;
    int ziehArt = 0;   // 0 Kreisen, 1 Zoom, 2 Verschieben, 3 nichts (Kamera-Knopf)
};
std::array<Lage, 4> g_lage;

ImVec2 projiziere(const Lage& l, V3 p) { return {l.mitte.x + dot(p, l.r) * l.skala, l.mitte.y - dot(p, l.h) * l.skala}; }

bool imViereck(ImVec2 p, const ImVec2 q[4]) {   // max_viewcube.h vc::PointInQuad
    bool pos = false;
    bool neg = false;
    for (int i = 0; i < 4; ++i) {
        const ImVec2& a = q[i];
        const ImVec2& b = q[(i + 1) & 3];
        const float s = (b.x - a.x) * (p.y - a.y) - (b.y - a.y) * (p.x - a.x);
        pos = pos || s > 0.0F;
        neg = neg || s < 0.0F;
    }
    return !(pos && neg);
}

bool imDreieck(ImVec2 p, ImVec2 a, ImVec2 b, ImVec2 c) {   // vc::PointInTri
    const auto s = [](ImVec2 p0, ImVec2 p1, ImVec2 p2) { return (p1.x - p0.x) * (p2.y - p0.y) - (p1.y - p0.y) * (p2.x - p0.x); };
    const float d1 = s(p, a, b);
    const float d2 = s(p, b, c);
    const float d3 = s(p, c, a);
    return !((d1 < 0.0F || d2 < 0.0F || d3 < 0.0F) && (d1 > 0.0F || d2 > 0.0F || d3 > 0.0F));
}

ImU32 mitAlpha(ImU32 c, float a) {   // vc::WithAlpha
    const auto alpha = static_cast<ImU32>(static_cast<float>((c >> IM_COL32_A_SHIFT) & 0xFFU) * a);
    return (c & ~IM_COL32_A_MASK) | (alpha << IM_COL32_A_SHIFT);
}

V3 achse(int k, float s) {
    V3 e;
    (k == 0 ? e.x : (k == 1 ? e.y : e.z)) = s;
    return e;
}

// Ziehen: Kreisen wie Alt+Mitteltaste (3ds Max) bzw. die Mitteltaste von Blender - die Szene folgt der Maus; Achsenansichten
// werden in 3ds Max "Orthographisch", in Blender (Auto Perspective) perspektivisch (MBNAV: gui/app_modelltab_nav.cpp).
void kreisen(int n, ImVec2 d, bool beginn) {
    mt().ansichten[n].animStart = -1.0;
    modellNavKreisenPx(n, d, beginn);
}

// --- 3ds Max: ViewCube (Portierung von max_viewcube.h) --------------------------------------------------------------

// Die sechs Flaechen: Normale, die Richtung, die auf der Beschriftung oben ist, Name und Achsenansicht.
struct Flaeche {
    V3 n;
    V3 hoch;
    Str name;
    AnsichtArt art;
};
const Flaeche kFlaechen[6] = {
    {{0.0F, -1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, Str::MbAnsichtVorne, AnsichtArt::Vorne},  {{0.0F, 1.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, Str::MbAnsichtHinten, AnsichtArt::Hinten},
    {{-1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, Str::MbAnsichtLinks, AnsichtArt::Links},  {{1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 1.0F}, Str::MbAnsichtRechts, AnsichtArt::Rechts},
    {{0.0F, 0.0F, 1.0F}, {0.0F, 1.0F, 0.0F}, Str::MbAnsichtOben, AnsichtArt::Oben},     {{0.0F, 0.0F, -1.0F}, {0.0F, 1.0F, 0.0F}, Str::MbAnsichtUnten, AnsichtArt::Unten},
};

// Eine Richtung des Wuerfels mit Anteilen aus {-1, 0, 1}: eine Flaeche, Kante oder Ecke (vc::Zone).
struct Zone {
    int x = 0;
    int y = 0;
    int z = 0;
    [[nodiscard]] bool da() const { return x != 0 || y != 0 || z != 0; }
    bool operator==(const Zone& o) const { return x == o.x && y == o.y && z == o.z; }
};

Zone zoneVon(V3 v) {
    const auto c = [](float f) { return f > 0.5F ? 1 : (f < -0.5F ? -1 : 0); };
    return {c(v.x), c(v.y), c(v.z)};
}

// Eine Flaeche 3 x 3 geteilt: die Mitte ist die Flaeche, die Streifen ihre Kanten, die Quadrate ihre Ecken.
constexpr float kRandBreite = 0.22F;
struct Teil {
    ImVec2 q[4];
    Zone zone;
};
void flaechenTeile(const Lage& l, const Flaeche& f, std::array<Teil, 9>& teile) {
    const V3 n = f.n;
    const V3 a{n.y, n.z, n.x};   // zu einer Achsennormalen senkrecht
    const V3 b = kreuz(n, a);
    const float schnitt[4] = {-0.5F, -0.5F + kRandBreite, 0.5F - kRandBreite, 0.5F};
    std::size_t k = 0;
    for (int iu = 0; iu < 3; ++iu) {
        for (int iv = 0; iv < 3; ++iv) {
            const V3 c = n * 0.5F;
            Teil& p = teile[k++];
            p.q[0] = projiziere(l, c + a * schnitt[iu] + b * schnitt[iv]);
            p.q[1] = projiziere(l, c + a * schnitt[iu + 1] + b * schnitt[iv]);
            p.q[2] = projiziere(l, c + a * schnitt[iu + 1] + b * schnitt[iv + 1]);
            p.q[3] = projiziere(l, c + a * schnitt[iu] + b * schnitt[iv + 1]);
            p.zone = zoneVon(n + a * static_cast<float>(iu - 1) + b * static_cast<float>(iv - 1));
        }
    }
}

// Die Winkel (pitch, yaw, roll 0), mit denen man aus Richtung d - vom Wuerfel zur Kamera - schaut (vc::ViewAngles).
void blickWinkel(V3 d, float w[3]) {
    const float len = std::sqrt(dot(d, d));
    w[0] = std::asin(std::clamp(d.z / len, -1.0F, 1.0F)) * 180.0F / kPi;
    w[1] = std::atan2(-d.y, -d.x) * 180.0F / kPi;
    w[2] = 0.0F;
}

bool winkelGleich(float a, float b) { return std::fabs(std::remainder(a - b, 360.0F)) < 0.01F; }

// Eine Orthoansicht auf die Winkel w drehen: trifft sie eine Achsenansicht, dann diese (das Feld zeigt [Vorne] ...),
// sonst die freie Ansicht "Orthographisch".
void orthoBlick(int n, const float w[3]) {
    for (const Flaeche& f : kFlaechen) {
        ModellAnsicht probe;
        probe.art = f.art;
        float pw[3];
        modellAnsichtWinkel(probe, pw);
        if (winkelGleich(pw[0], w[0]) && winkelGleich(pw[1], w[1]) && winkelGleich(w[2], 0.0F)) {
            modellBlickSetzen(n, f.art, nullptr, true);
            return;
        }
    }
    modellBlickSetzen(n, AnsichtArt::Orthographisch, w, true);
}

// Klick auf eine Zone oder einen Pfeil: der ViewCube dreht die Ansicht, die Projektion bleibt (3ds Max; die Vorlage
// kennt eine Kamera mit Schalter orthographisch). Eine Flaeche schaut wie ihre Achsenansicht - Oben und Unten mit Vorne
// am unteren Bildrand -, eine Voreinstellung steht aufrecht (Rollwinkel 0).
void zoneKlick(int n, Zone z) {
    const bool persp = mt().ansichten[n].art == AnsichtArt::Perspektive;
    float w[3];
    for (const Flaeche& f : kFlaechen) {
        if (!(zoneVon(f.n) == z)) { continue; }
        if (!persp) {
            modellBlickSetzen(n, f.art, nullptr, true);
            return;
        }
        ModellAnsicht probe;
        probe.art = f.art;
        modellAnsichtWinkel(probe, w);
        modellBlickSetzen(n, AnsichtArt::Perspektive, w, true);
        return;
    }
    blickWinkel(V3{static_cast<float>(z.x), static_cast<float>(z.y), static_cast<float>(z.z)}, w);
    if (persp) {
        modellBlickSetzen(n, AnsichtArt::Perspektive, w, true);
    } else {
        orthoBlick(n, w);
    }
}

// Klick auf einen Buchstaben des Kompasses: von dort schauen, die Neigung bleibt (turnTo(y, camera.GetPitch())).
void buchstabeKlick(int n, V3 d) {
    const ModellAnsicht& a = mt().ansichten[n];
    float jetzt[3];
    modellAnsichtWinkel(a, jetzt);
    float w[3];
    blickWinkel(d, w);
    w[0] = jetzt[0];
    if (a.art == AnsichtArt::Perspektive) {
        modellBlickSetzen(n, AnsichtArt::Perspektive, w, true);
    } else {
        orthoBlick(n, w);
    }
}

// Um die Blickachse rollen (Rollpfeile) bzw. um die Hochachse drehen (Zug am Ring): nur dieser eine Winkel aendert sich.
void winkelAendern(int n, int welcher, float grad, bool weich) {
    const ModellAnsicht& a = mt().ansichten[n];
    float w[3];
    modellAnsichtWinkel(a, w);
    w[welcher] += grad;
    modellBlickSetzen(n, a.art == AnsichtArt::Perspektive ? AnsichtArt::Perspektive : AnsichtArt::Orthographisch, w, weich);
}

// Die Beschriftung auf der Flaeche: jede Glyphe als Viereck aus dem Schriftatlas durch `projiziere`, sie dreht und
// kippt mit der Flaeche (vc::FaceLabel), UTF-8 fuer die Programmsprache.
void beschriftung(ImDrawList* dl, const Lage& l, V3 n, V3 hoch, const char* text, ImU32 farbe) {
    ImFontBaked* baked = ImGui::GetFontBaked();
    if (baked == nullptr || text == nullptr) { return; }
    const V3 rt = kreuz(hoch, n);
    float breite = 0.0F;
    float y0 = 1e9F;
    float y1 = -1e9F;
    for (const char* s = text; *s != '\0';) {
        unsigned int c = 0;
        s += ImTextCharFromUtf8(&c, s, nullptr);
        const ImFontGlyph* g = baked->FindGlyph(static_cast<ImWchar>(c));
        if (g == nullptr) { continue; }
        breite += g->AdvanceX;
        y0 = std::min(y0, g->Y0);
        y1 = std::max(y1, g->Y1);
    }
    if (breite <= 0.0F || y1 <= y0) { return; }
    const float proPx = std::min(0.66F / breite, 0.30F / (y1 - y0));
    const float mitteY = (y0 + y1) * 0.5F;
    const V3 m = n * 0.5F;
    float stift = -breite * 0.5F;
    const ImTextureRef tex = ImGui::GetIO().Fonts->TexRef;
    for (const char* s = text; *s != '\0';) {
        unsigned int c = 0;
        s += ImTextCharFromUtf8(&c, s, nullptr);
        const ImFontGlyph* g = baked->FindGlyph(static_cast<ImWchar>(c));
        if (g == nullptr) { continue; }
        const auto ecke = [&](float px, float py) { return projiziere(l, m + rt * ((stift + px) * proPx) + hoch * ((mitteY - py) * proPx)); };
        dl->AddImageQuad(tex, ecke(g->X0, g->Y0), ecke(g->X1, g->Y0), ecke(g->X1, g->Y1), ecke(g->X0, g->Y1), ImVec2{g->U0, g->V0},
                         ImVec2{g->U1, g->V0}, ImVec2{g->U1, g->V1}, ImVec2{g->U0, g->V1}, farbe);
        stift += g->AdvanceX;
    }
}

void viewCube(int n, ModellAnsicht& a, Lage& l) {
    ImDrawList* dl = ImGui::GetWindowDrawList();
    const ImGuiIO& io = ImGui::GetIO();
    // --- Anordnung: die Masse der Vorlage (Wuerfel 78, Rand fuer Pfeile und Ring 26, Home 18 Bildpunkte bei 16er
    // Schrift) mit der Schrift skaliert, in kleinen Ansichten kleiner ---
    const float m = std::min(ImGui::GetFontSize() / 16.0F, std::min(a.groesse[0], a.groesse[1]) * 0.5F / 140.0F);
    if (m < 0.4F) { return; }
    const float kante = 78.0F * m;
    const float rand = 26.0F * m;
    const float boxB = kante + 2.0F * rand;
    const float boxH = boxB + 10.0F * m;
    const ImVec2 box{a.lo[0] + a.groesse[0] - boxB - 6.0F * m, a.lo[1] + 6.0F * m};
    a.wuerfel[0] = box.x;
    a.wuerfel[1] = box.y;
    a.wuerfel[2] = box.x + boxB;
    a.wuerfel[3] = box.y + boxH;
    ImGui::SetCursorScreenPos(box);
    ImGui::InvisibleButton("##mkwuerfel", ImVec2{boxB, boxH});
    const bool ueber = ImGui::IsItemHovered();
    const bool gedrueckt = ImGui::IsItemActivated();
    const bool gehalten = ImGui::IsItemActive();
    const bool losgelassen = ImGui::IsItemDeactivated();
    const ImVec2 maus = io.MousePos;
    if (gedrueckt) {
        mt().aktiv = n;
        mt().klickDrinBild = ImGui::GetFrameCount();
    }
    l.alpha = (ueber || gehalten || a.animStart >= 0.0) ? 1.0F : 0.55F;   // halb durchsichtig, bis die Maus darueber ist
    const float alpha = l.alpha;
    // --- Projektion (orthographisch): die Achsen der Ansicht, der Wuerfel dreht mit dem Rollwinkel ---
    l.mitte = ImVec2{box.x + boxB * 0.5F, box.y + rand + kante * 0.5F};
    l.skala = kante * 0.5F / 0.92F;
    const V3 auge = l.v * -1.0F;
    // --- Home (Haus) ---
    const ImVec2 h0{box.x + 3.0F * m, box.y + 3.0F * m};
    const ImVec2 h1{h0.x + 18.0F * m, h0.y + 18.0F * m};
    l.home = ImVec2{(h0.x + h1.x) * 0.5F, (h0.y + h1.y) * 0.5F};
    const bool aufHome = ueber && maus.x >= h0.x && maus.x <= h1.x && maus.y >= h0.y && maus.y <= h1.y;
    // --- der Kompassring in der Ebene unter dem Wuerfel ---
    constexpr int kRingN = 72;
    constexpr float kRingR = 0.98F;
    constexpr float kRingZ = -0.62F;
    const bool kompass = g_app->settings.modellKompass;
    l.kompass = kompass;
    // Norden: Welt +Y (hinter der Flaeche Vorne, wie 3ds Max), um die Einstellung im Uhrzeigersinn von oben gedreht
    const float nordWinkel = static_cast<float>(g_app->settings.modellKompassNord) * kPi / 180.0F;
    const V3 nord{std::sin(nordWinkel), std::cos(nordWinkel), 0.0F};
    const V3 ost{std::cos(nordWinkel), -std::sin(nordWinkel), 0.0F};
    const V3 unten{0.0F, 0.0F, kRingZ};
    const auto ringOrt = [&](float winkel, float radius) { return (nord * std::cos(winkel) + ost * std::sin(winkel)) * radius + unten; };
    std::array<ImVec2, kRingN> ring{};
    std::array<bool, kRingN> ringNah{};   // das Stueck i (Punkt i bis i+1) liegt vor dem Wuerfel
    for (int i = 0; kompass && i < kRingN; ++i) {
        const auto k = static_cast<std::size_t>(i);
        ring[k] = projiziere(l, ringOrt(2.0F * kPi * static_cast<float>(i) / kRingN, kRingR));
        ringNah[k] = dot(ringOrt(2.0F * kPi * (static_cast<float>(i) + 0.5F) / kRingN, kRingR), auge) > 0.0F;
    }
    const ImVec2 ringMitte = projiziere(l, unten);
    // Abstand der Maus zum Ring, getrennt fuer die Haelfte vor dem Wuerfel und die dahinter (die der Wuerfel verdeckt)
    float ringDistNah = 1e9F;
    float ringDistFern = 1e9F;
    for (int i = 0; kompass && i < kRingN; ++i) {
        const ImVec2 p0 = ring[static_cast<std::size_t>(i)];
        const ImVec2 p1 = ring[static_cast<std::size_t>((i + 1) % kRingN)];
        const ImVec2 ab{p1.x - p0.x, p1.y - p0.y};
        const float l2 = ab.x * ab.x + ab.y * ab.y;
        const float s = l2 > 0.0F ? std::clamp(((maus.x - p0.x) * ab.x + (maus.y - p0.y) * ab.y) / l2, 0.0F, 1.0F) : 0.0F;
        float& d = ringNah[static_cast<std::size_t>(i)] ? ringDistNah : ringDistFern;
        d = std::min(d, std::hypot(p0.x + ab.x * s - maus.x, p0.y + ab.y * s - maus.y));
    }
    {   // fuer den Selbsttest: der Punkt des Rings zwischen zwei Buchstaben, der der Kamera am naechsten ist
        float beste = -1e9F;
        for (int i = 0; i < 4; ++i) {
            const V3 p = ringOrt(kPi * (0.25F + 0.5F * static_cast<float>(i)), kRingR);
            if (dot(p, auge) > beste) {
                beste = dot(p, auge);
                l.ringPunkt = projiziere(l, p);
            }
        }
    }
    // --- sichtbare Flaechen, von hinten nach vorn ---
    struct Sicht {
        int flaeche;
        float tiefe;
    };
    std::array<Sicht, 6> sicht{};
    int zahl = 0;
    for (int i = 0; i < 6; ++i) {
        const float d = dot(kFlaechen[i].n, auge);
        if (d > 1e-3F) { sicht[static_cast<std::size_t>(zahl++)] = {i, d}; }
    }
    std::sort(sicht.begin(), sicht.begin() + zahl, [](const Sicht& x, const Sicht& y) { return x.tiefe < y.tiefe; });
    // --- was unter der Maus liegt, von vorn nach hinten wie in Max: Ring und Buchstaben vor dem Wuerfel, dann der
    // Wuerfel, dann was hinter ihm liegt (nur wo er es nicht verdeckt) ---
    Zone wuerfelZone;
    if (ueber && !aufHome) {
        for (int s = zahl - 1; s >= 0 && !wuerfelZone.da(); --s) {
            std::array<Teil, 9> teile{};
            flaechenTeile(l, kFlaechen[sicht[static_cast<std::size_t>(s)].flaeche], teile);
            for (const Teil& p : teile) {
                if (imViereck(maus, p.q)) {
                    wuerfelZone = p.zone;
                    break;
                }
            }
        }
    }
    const bool ueberWuerfel = wuerfelZone.da();
    const float griff = 6.0F * m;
    const bool ringHover = kompass && ueber && !aufHome && (ringDistNah < griff || (!ueberWuerfel && ringDistFern < griff));
    // --- gerade auf eine Flaeche: Pfeile zu den Nachbarflaechen, knapp ausserhalb der Flaeche ---
    bool vonVorn = false;
    for (const Flaeche& f : kFlaechen) { vonVorn = vonVorn || dot(f.n, auge) > 0.9995F; }
    l.pfeile = vonVorn;
    struct Pfeil {
        ImVec2 a;
        ImVec2 b;
        ImVec2 c;
        V3 nach;
    };
    std::array<Pfeil, 4> pfeile{};
    int hoverPfeil = -1;
    const float halb = 0.5F * l.skala;
    if (vonVorn) {
        const float r = halb + 9.0F * m;
        const float w = 7.0F * m;
        const float h = 8.0F * m;
        const ImVec2 richtung[4] = {ImVec2{0.0F, -1.0F}, ImVec2{0.0F, 1.0F}, ImVec2{-1.0F, 0.0F}, ImVec2{1.0F, 0.0F}};
        const V3 nach[4] = {l.h, l.h * -1.0F, l.r * -1.0F, l.r};
        for (int i = 0; i < 4; ++i) {
            const ImVec2 d = richtung[i];
            const ImVec2 o{-d.y, d.x};
            const ImVec2 spitze{l.mitte.x + d.x * (r + h), l.mitte.y + d.y * (r + h)};
            const ImVec2 b0{l.mitte.x + d.x * r + o.x * w, l.mitte.y + d.y * r + o.y * w};
            const ImVec2 b1{l.mitte.x + d.x * r - o.x * w, l.mitte.y + d.y * r - o.y * w};
            pfeile[static_cast<std::size_t>(i)] = {spitze, b0, b1, nach[i]};
            l.pfeil[static_cast<std::size_t>(i)] = ImVec2{(spitze.x + b0.x + b1.x) / 3.0F, (spitze.y + b0.y + b1.y) / 3.0F};
            if (ueber && imDreieck(maus, spitze, b0, b1)) { hoverPfeil = i; }
        }
    }
    // Rollpfeile ueber den oberen Ecken: rechts im, links gegen den Uhrzeigersinn
    int hoverRoll = -1;
    if (vonVorn) {
        const float o = halb + 10.0F * m;
        l.rollPfeil[0] = ImVec2{l.mitte.x + o, l.mitte.y - o};
        l.rollPfeil[1] = ImVec2{l.mitte.x - o, l.mitte.y - o};
        for (int i = 0; i < 2; ++i) {
            const ImVec2 c = l.rollPfeil[static_cast<std::size_t>(i)];
            if (ueber && std::hypot(maus.x - c.x, maus.y - c.y) < 9.0F * m) { hoverRoll = i; }
        }
    }
    // Buchstaben des Kompasses: Klick schaut von dort
    const char* const buchstaben[4] = {tr(Str::MkKompassN), tr(Str::MkKompassO), tr(Str::MkKompassS), tr(Str::MkKompassW)};
    int hoverBuchstabe = -1;
    std::array<bool, 4> buchstabeNah{};   // TEST1411 L: der Buchstabe liegt vor dem Wuerfel (sonst verdeckt ihn der Wuerfel)
    for (int i = 0; kompass && i < 4; ++i) {
        const auto k = static_cast<std::size_t>(i);
        const V3 p = ringOrt(kPi * 0.5F * static_cast<float>(i), kRingR + 0.16F);
        l.buchstabe[k] = projiziere(l, p);
        buchstabeNah[k] = dot(p, auge) > 0.0F;
        const ImVec2 ts = ImGui::CalcTextSize(buchstaben[i]);
        // TEST1411 L: hinter dem Wuerfel nur dort zu treffen, wo der Wuerfel ihn nicht verdeckt - die sichtbare Flaeche geht vor
        if (ueber && !aufHome && (buchstabeNah[k] || !ueberWuerfel) && std::fabs(maus.x - l.buchstabe[k].x) <= ts.x * 0.5F + 3.0F &&
            std::fabs(maus.y - l.buchstabe[k].y) <= ts.y * 0.5F + 2.0F) {
            hoverBuchstabe = i;
        }
    }
    l.buchstabeVerdeckt = 0;
    for (std::size_t k = 0; k < 4; ++k) { l.buchstabeVerdeckt |= buchstabeNah[k] ? 0 : (1 << k); }
    // der Teil des Wuerfels unter der Maus, wenn nichts vor ihm liegt
    Zone hoverZone;
    if (l.zug == Lage::Zug::Keiner && !(ringHover && ringDistNah < griff) && hoverBuchstabe < 0) {
        hoverZone = wuerfelZone;
    }
    // --- zeichnen: Ring und Buchstaben in zwei Durchgaengen - was hinter dem Wuerfel liegt vor ihm, damit er es verdeckt ---
    const ImU32 kBlau = IM_COL32(120, 170, 235, 255);
    const auto kompassZeichnen = [&](bool nah) {
        if (!kompass) { return; }
        const bool hell = ringHover || l.zug == Lage::Zug::Ring;
        const ImU32 farbe = mitAlpha(hell ? kBlau : IM_COL32(150, 156, 166, 230), alpha);
        for (int i = 0; i < kRingN; ++i) {
            if (ringNah[static_cast<std::size_t>(i)] != nah) { continue; }
            dl->AddLine(ring[static_cast<std::size_t>(i)], ring[static_cast<std::size_t>((i + 1) % kRingN)], farbe, (ringHover ? 5.0F : 4.0F) * m);
        }
    };
    // TEST1411 L: die Buchstaben wie der Ring in zwei Durchgaengen - die hinter dem Wuerfel vor ihm (er verdeckt sie, wie 3ds Max und
    // die Vorlage), die vor ihm danach; mit dunklem Rand, damit sie auf jedem Grund zu lesen sind
    const auto buchstabenZeichnen = [&](bool nah) {
        if (!kompass) { return; }
        const ImU32 rand = mitAlpha(IM_COL32(24, 26, 32, 255), alpha);
        for (int i = 0; i < 4; ++i) {
            const auto k = static_cast<std::size_t>(i);
            if (buchstabeNah[k] != nah) { continue; }
            const ImVec2 ts = ImGui::CalcTextSize(buchstaben[i]);
            const ImVec2 o{std::round(l.buchstabe[k].x - ts.x * 0.5F), std::round(l.buchstabe[k].y - ts.y * 0.5F)};
            for (int dy = -1; dy <= 1; ++dy) {
                for (int dx = -1; dx <= 1; ++dx) {
                    if (dx != 0 || dy != 0) { dl->AddText(ImVec2{o.x + static_cast<float>(dx), o.y + static_cast<float>(dy)}, rand, buchstaben[i]); }
                }
            }
            dl->AddText(o, mitAlpha(i == hoverBuchstabe ? kBlau : IM_COL32(225, 228, 235, 255), alpha), buchstaben[i]);
        }
    };
    kompassZeichnen(false);
    buchstabenZeichnen(false);
    for (int s = 0; s < zahl; ++s) {
        const Flaeche& f = kFlaechen[sicht[static_cast<std::size_t>(s)].flaeche];
        std::array<Teil, 9> teile{};
        flaechenTeile(l, f, teile);
        for (const Teil& p : teile) {
            const bool hell = hoverZone.da() && p.zone == hoverZone;
            dl->AddConvexPolyFilled(p.q, 4, mitAlpha(hell ? IM_COL32(110, 168, 240, 255) : IM_COL32(214, 218, 224, 255), alpha));
        }
        const ImVec2 umriss[4] = {teile[0].q[0], teile[6].q[1], teile[8].q[2], teile[2].q[3]};
        dl->AddPolyline(umriss, 4, mitAlpha(IM_COL32(70, 76, 86, 255), alpha), ImDrawFlags_Closed, 1.2F * m);
        beschriftung(dl, l, f.n, f.hoch, tr(f.name), mitAlpha(IM_COL32(40, 44, 52, 255), alpha));
    }
    kompassZeichnen(true);
    buchstabenZeichnen(true);
    for (int i = 0; vonVorn && i < 4; ++i) {
        const Pfeil& p = pfeile[static_cast<std::size_t>(i)];
        dl->AddTriangleFilled(p.a, p.b, p.c, mitAlpha(i == hoverPfeil ? kBlau : IM_COL32(205, 210, 218, 235), alpha));
    }
    for (int i = 0; vonVorn && i < 2; ++i) {
        // ein Viertelbogen mit Pfeilspitze in der Richtung, in die er rollt
        const ImU32 farbe = mitAlpha(i == hoverRoll ? kBlau : IM_COL32(205, 210, 218, 235), alpha);
        const ImVec2 c = l.rollPfeil[static_cast<std::size_t>(i)];
        const float a0 = i == 0 ? -2.6F : -0.55F;
        const float a1 = i == 0 ? -0.55F : -2.6F;
        dl->PathArcTo(c, 7.0F * m, std::min(a0, a1), std::max(a0, a1), 10);
        dl->PathStroke(farbe, ImDrawFlags_None, 2.0F * m);
        const ImVec2 spitze{c.x + 7.0F * m * std::cos(a1), c.y + 7.0F * m * std::sin(a1)};
        const float quer = a1 + kPi * 0.5F * (i == 0 ? 1.0F : -1.0F);
        dl->AddTriangleFilled(ImVec2{spitze.x + 4.0F * m * std::cos(quer), spitze.y + 4.0F * m * std::sin(quer)},
                              ImVec2{spitze.x + 3.0F * m * std::cos(a1), spitze.y + 3.0F * m * std::sin(a1)},
                              ImVec2{spitze.x - 3.0F * m * std::cos(a1), spitze.y - 3.0F * m * std::sin(a1)}, farbe);
    }
    {   // das Haus: Dach, Wand, Tuer
        // TEST1411 M: das Haus in der Farbe der Beschriftung zum Grund, auch ohne Maus kaum blasser (auf hellem Grund war es kaum zu sehen)
        const ImU32 farbe = aufHome ? mitAlpha(kBlau, alpha) : modellUeberGrund(std::max(alpha, 0.85F), false);
        const float x0 = h0.x + 2.0F * m;
        const float x1 = h1.x - 2.0F * m;
        const float y0 = h0.y + 2.0F * m;
        const float y1 = h1.y - 2.0F * m;
        const float xm = (x0 + x1) * 0.5F;
        const float ym = y0 + (y1 - y0) * 0.45F;
        dl->AddTriangleFilled(ImVec2{x0 - m, ym}, ImVec2{xm, y0}, ImVec2{x1 + m, ym}, farbe);
        dl->AddRectFilled(ImVec2{x0 + 2.0F * m, ym}, ImVec2{x1 - 2.0F * m, y1}, farbe);
        dl->AddRectFilled(ImVec2{xm - 2.0F * m, y1 - 6.0F * m}, ImVec2{xm + 2.0F * m, y1}, mitAlpha(IM_COL32(40, 44, 52, 255), alpha));
    }
    if (ueber) {
        const Str tip = aufHome ? Str::MkHomeHint
                                : (hoverRoll >= 0 ? Str::MkRollHint
                                                  : (hoverBuchstabe >= 0 || ringHover ? Str::MkKompassHint : (hoverPfeil >= 0 ? Str::MkPfeilHint : Str::MkWuerfelHint)));
        ImGui::SetItemTooltip("%s", tr(tip));
    }
    // --- Eingabe ---
    if (gedrueckt) {
        l.druck = maus;
        l.zugBewegt = false;
        l.kreist = false;
        l.zug = (ringHover && hoverBuchstabe < 0) ? Lage::Zug::Ring
                                                 : (aufHome || hoverPfeil >= 0 || hoverRoll >= 0 || hoverBuchstabe >= 0 ? Lage::Zug::Keiner : Lage::Zug::Wuerfel);
        l.ringWinkel = std::atan2(maus.y - ringMitte.y, maus.x - ringMitte.x);
        l.ziel = Lage::Ziel::Keins;
        if (aufHome) {
            l.ziel = Lage::Ziel::Home;
        } else if (hoverRoll >= 0) {
            l.ziel = Lage::Ziel::Rollen;
            l.zielIndex = hoverRoll;
        } else if (hoverBuchstabe >= 0) {
            l.ziel = Lage::Ziel::Buchstabe;
            l.zielIndex = hoverBuchstabe;
        } else if (hoverPfeil >= 0) {
            l.ziel = Lage::Ziel::Pfeil;
            l.zielRichtung = pfeile[static_cast<std::size_t>(hoverPfeil)].nach;
        } else if (hoverZone.da()) {
            l.ziel = Lage::Ziel::Zone;
            l.zielRichtung = V3{static_cast<float>(hoverZone.x), static_cast<float>(hoverZone.y), static_cast<float>(hoverZone.z)};
        }
    }
    if (gehalten && l.zug != Lage::Zug::Keiner) {
        // eine Maus, die das Fenster verlassen hat, hat keinen Ort: das ist kein Zug
        if (ImGui::IsMousePosValid(&maus) && std::fabs(maus.x - l.druck.x) + std::fabs(maus.y - l.druck.y) > 3.0F) { l.zugBewegt = true; }
        if (l.zugBewegt) {
            a.animStart = -1.0;
            if (l.zug == Lage::Zug::Wuerfel) {
                if (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F) {
                    kreisen(n, io.MouseDelta, !l.kreist);
                    l.kreist = true;
                }
            } else {
                // der Ring folgt der Maus: ihr Winkel um die Ringmitte, von unten gesehen andersherum
                const float winkel = std::atan2(maus.y - ringMitte.y, maus.x - ringMitte.x);
                const float grad = std::remainder(winkel - l.ringWinkel, 2.0F * kPi) * 180.0F / kPi * (l.v.z > 0.0F ? -1.0F : 1.0F);
                l.ringWinkel = winkel;
                if (grad != 0.0F) { winkelAendern(n, 1, grad, false); }
            }
        }
    }
    if (losgelassen) {
        if (!l.zugBewegt) {
            switch (l.ziel) {
                case Lage::Ziel::Home: modellHomeFlug(n); break;
                case Lage::Ziel::Rollen: winkelAendern(n, 2, l.zielIndex == 0 ? -90.0F : 90.0F, true); break;
                case Lage::Ziel::Buchstabe: buchstabeKlick(n, ringOrt(kPi * 0.5F * static_cast<float>(l.zielIndex), 1.0F) + unten * -1.0F); break;
                case Lage::Ziel::Pfeil:
                case Lage::Ziel::Zone: zoneKlick(n, zoneVon(l.zielRichtung)); break;
                case Lage::Ziel::Keins: break;
            }
        }
        l.ziel = Lage::Ziel::Keins;
        l.zug = Lage::Zug::Keiner;
        l.zugBewegt = false;
    }
    // --- Rechtsklick-Menue wie das von Max: Home, aktuelle Ansicht als Home setzen, orthographisch ---
    if (ueber && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { ImGui::OpenPopup("##mkwuerfelmenue"); }
    if (ui::popup("##mkwuerfelmenue")) {
        if (ImGui::MenuItem(tr(Str::MkStartansicht))) { modellHomeFlug(n); }
        ImGui::SetItemTooltip("%s", tr(Str::MkHomeHint));
        if (ImGui::MenuItem(tr(Str::MkHomeSetzen))) { modellHomeSetzen(n); }
        ImGui::SetItemTooltip("%s", tr(Str::MkHomeSetzenHint));   // KLEIN: sagt, dass es fuer die Figur bleibt
        // KLEIN (rc612, Entscheidung A12): ein gesetztes Home bleibt je Figur - hier wird es wieder die Startansicht (blass ohne Home)
        if (ImGui::MenuItem(tr(Str::MkHomeZurueck), nullptr, false, mt().homeDa)) { modellHomeZuruecksetzen(); }
        ImGui::SetItemTooltip("%s", tr(Str::MkHomeZurueckHint));
        const bool ortho = a.art != AnsichtArt::Perspektive;
        if (ImGui::MenuItem(tr(Str::MkOrtho), nullptr, ortho)) {
            float w[3];
            modellAnsichtWinkel(a, w);
            modellBlickSetzen(n, ortho ? AnsichtArt::Perspektive : AnsichtArt::Orthographisch, w, false);
        }
        ImGui::EndPopup();
    }
}

// --- Blender: Navigations-Gizmo --------------------------------------------------------------------------------------

AnsichtArt kugelArt(int i) {   // Klick auf die Kugel +X schaut von +X zur Mitte (Rechts), -Y von vorn (Vorne) ...
    static const AnsichtArt kArt[6] = {AnsichtArt::Rechts, AnsichtArt::Links, AnsichtArt::Hinten, AnsichtArt::Vorne, AnsichtArt::Oben, AnsichtArt::Unten};
    return kArt[i];
}

AnsichtArt gegenArt(AnsichtArt a) {
    switch (a) {
        case AnsichtArt::Rechts: return AnsichtArt::Links;
        case AnsichtArt::Links: return AnsichtArt::Rechts;
        case AnsichtArt::Hinten: return AnsichtArt::Vorne;
        case AnsichtArt::Vorne: return AnsichtArt::Hinten;
        case AnsichtArt::Oben: return AnsichtArt::Unten;
        case AnsichtArt::Unten: return AnsichtArt::Oben;
        default: break;
    }
    return a;
}

void knopfSymbol(ImDrawList* dl, int art, ImVec2 m, float r, ImU32 f, bool ortho) {
    if (art == 0) {   // Lupe
        dl->AddCircle(ImVec2{m.x - r * 0.15F, m.y - r * 0.15F}, r * 0.45F, f, 16, 1.6F);
        dl->AddLine(ImVec2{m.x + r * 0.17F, m.y + r * 0.17F}, ImVec2{m.x + r * 0.6F, m.y + r * 0.6F}, f, 2.2F);
    } else if (art == 1) {   // vier Pfeile
        for (int k = 0; k < 4; ++k) {
            const float c = std::cos(static_cast<float>(k) * kPi * 0.5F);
            const float s = std::sin(static_cast<float>(k) * kPi * 0.5F);
            const ImVec2 spitze{m.x + c * r * 0.7F, m.y + s * r * 0.7F};
            dl->AddLine(m, spitze, f, 1.6F);
            dl->AddTriangleFilled(spitze, ImVec2{spitze.x - c * r * 0.25F - s * r * 0.18F, spitze.y - s * r * 0.25F + c * r * 0.18F},
                                  ImVec2{spitze.x - c * r * 0.25F + s * r * 0.18F, spitze.y - s * r * 0.25F - c * r * 0.18F}, f);
        }
    } else if (art == 2) {   // Kamera: Gehaeuse, Objektiv, zwei Spulen
        dl->AddRectFilled(ImVec2{m.x - r * 0.58F, m.y - r * 0.12F}, ImVec2{m.x + r * 0.18F, m.y + r * 0.46F}, f, r * 0.08F);
        dl->AddTriangleFilled(ImVec2{m.x + r * 0.18F, m.y + r * 0.17F}, ImVec2{m.x + r * 0.62F, m.y - r * 0.12F}, ImVec2{m.x + r * 0.62F, m.y + r * 0.46F}, f);
        dl->AddCircleFilled(ImVec2{m.x - r * 0.36F, m.y - r * 0.34F}, r * 0.2F, f, 12);
        dl->AddCircleFilled(ImVec2{m.x + r * 0.02F, m.y - r * 0.36F}, r * 0.16F, f, 12);
    } else if (ortho) {   // Raster: orthographisch
        dl->AddRect(ImVec2{m.x - r * 0.5F, m.y - r * 0.5F}, ImVec2{m.x + r * 0.5F, m.y + r * 0.5F}, f, 0.0F, 0, 1.4F);
        dl->AddLine(ImVec2{m.x, m.y - r * 0.5F}, ImVec2{m.x, m.y + r * 0.5F}, f, 1.2F);
        dl->AddLine(ImVec2{m.x - r * 0.5F, m.y}, ImVec2{m.x + r * 0.5F, m.y}, f, 1.2F);
    } else {   // Trapez: perspektivisch
        const ImVec2 q[4] = {ImVec2{m.x - r * 0.3F, m.y - r * 0.45F}, ImVec2{m.x + r * 0.3F, m.y - r * 0.45F}, ImVec2{m.x + r * 0.6F, m.y + r * 0.45F},
                             ImVec2{m.x - r * 0.6F, m.y + r * 0.45F}};
        dl->AddPolyline(q, 4, f, ImDrawFlags_Closed, 1.4F);
        dl->AddLine(ImVec2{m.x, m.y - r * 0.45F}, ImVec2{m.x, m.y + r * 0.45F}, f, 1.2F);
    }
}

void gizmo(int n, ModellAnsicht& a, Lage& l, ImVec2 o, float groesse) {
    ModellTab& t = mt();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    ImGuiIO& io = ImGui::GetIO();
    const float radius = groesse * 0.5F;
    l.mitte = ImVec2{o.x - radius - 2.0F, o.y + radius + 2.0F};
    l.kugelR = radius * 0.22F;
    l.skala = radius - l.kugelR;
    l.knopfR = radius * 0.30F;
    // KLEIN (rc612): vier Knoepfe wie Blenders Navigations-Gizmo (VIEW3D_GGT_navigate: Zoom, Move, Camera, Persp/Ortho); bis rc611
    // fehlte der Kamera-Knopf. Der Model-Tab hat keine Szenenkamera: der Knopf ist blass, sagt es im Tooltip und in der Statuszeile
    // und laesst die Ansicht, wie sie ist (in Blender bricht view3d.view_camera ohne Kamera ebenfalls ab).
    for (int i = 0; i < 4; ++i) { l.knopf[static_cast<std::size_t>(i)] = ImVec2{l.mitte.x, l.mitte.y + radius + l.knopfR * (1.4F + 2.4F * static_cast<float>(i))}; }
    const ImVec2 box0{l.mitte.x - radius - 2.0F, o.y};
    const ImVec2 box1{o.x, l.knopf[3].y + l.knopfR + 2.0F};
    a.wuerfel[0] = box0.x;
    a.wuerfel[1] = box0.y;
    a.wuerfel[2] = box1.x;
    a.wuerfel[3] = box1.y;
    ImGui::SetCursorScreenPos(box0);
    ImGui::InvisibleButton("##mkgizmo", ImVec2{box1.x - box0.x, box1.y - box0.y});
    const bool ueber = ImGui::IsItemHovered();
    // welches Teil unter der Maus: Kugeln (die vorderste), Knoepfe, sonst der Kreis (Ziehen = Kreisen)
    struct Teil {
        int i;
        float tiefe;
    };
    std::array<Teil, 6> teile{};
    for (int i = 0; i < 6; ++i) {
        const V3 e = achse(i / 2, (i % 2) == 0 ? 1.0F : -1.0F);
        l.kugel[static_cast<std::size_t>(i)] = projiziere(l, e);
        teile[static_cast<std::size_t>(i)] = {i, -dot(e, l.v)};
    }
    std::sort(teile.begin(), teile.end(), [](const Teil& x, const Teil& y) { return x.tiefe < y.tiefe; });
    int unter = -1;   // 0..5 Kugel, 6 Zoom, 7 Verschieben, 8 Persp/Ortho, 9 Kreis, 10 Kamera
    static const int kKnopfTeil[4] = {6, 7, 10, 8};   // die Knoepfe von oben nach unten
    if (ueber) {
        for (const Teil& x : teile) {
            const ImVec2 p = l.kugel[static_cast<std::size_t>(x.i)];
            if (std::hypot(io.MousePos.x - p.x, io.MousePos.y - p.y) <= l.kugelR) { unter = x.i; }
        }
        for (int i = 0; i < 4 && unter < 0; ++i) {
            const ImVec2 p = l.knopf[static_cast<std::size_t>(i)];
            if (std::hypot(io.MousePos.x - p.x, io.MousePos.y - p.y) <= l.knopfR) { unter = kKnopfTeil[i]; }
        }
        if (unter < 0 && std::hypot(io.MousePos.x - l.mitte.x, io.MousePos.y - l.mitte.y) <= radius) { unter = 9; }
    }
    if (ImGui::IsItemActivated()) {
        l.druck = io.MousePos;
        l.ziehArt = unter == 6 ? 1 : (unter == 7 ? 2 : (unter == 10 ? 3 : 0));
        l.kreist = false;
        l.geschoben = ImVec2{0.0F, 0.0F};
        if (l.ziehArt == 1) { modellNavBlZoomZug(n, true, 0.0F); }
        t.aktiv = n;
        t.klickDrinBild = ImGui::GetFrameCount();
    }
    if (l.ziehArt != 3 && ImGui::IsItemActive() && ImGui::IsMouseDragging(ImGuiMouseButton_Left, 3.0F) && (io.MouseDelta.x != 0.0F || io.MouseDelta.y != 0.0F)) {
        a.animStart = -1.0;
        if (l.ziehArt == 0) {
            kreisen(n, io.MouseDelta, !l.kreist);
            l.kreist = true;
        } else if (l.ziehArt == 1) {   // Zoom ziehen: view3d.zoom, als begaenne der Zug in der Bildmitte (use_cursor_init = false)
            modellNavBlZoomZug(n, false, io.MousePos.y - l.druck.y);
        } else {   // Verschieben ziehen: view3d.move, 1:1
            // um den ganzen Weg seit dem Druecken: die Zugschwelle verschluckt so keine Bildpunkte
            const ImVec2 gesamt{io.MousePos.x - l.druck.x, io.MousePos.y - l.druck.y};
            modellNavSchiebenPx(n, ImVec2{gesamt.x - l.geschoben.x, gesamt.y - l.geschoben.y});
            l.geschoben = gesamt;
        }
        g_app->modelDirty = true;
    }
    int ausgerichtet = -1;   // die Achse, entlang der man genau schaut (Blender: len_squared_v2(matrix_offset[axis]) < 1e-6)
    for (int k = 0; k < 3; ++k) {
        const float d = dot(achse(k, 1.0F), l.v);
        if (1.0F - d * d < 1e-6F) { ausgerichtet = k; }
    }
    const bool klick = ImGui::IsItemDeactivated() && std::hypot(io.MousePos.x - l.druck.x, io.MousePos.y - l.druck.y) < 3.0F;
    if (klick && unter >= 0 && unter < 6) {
        AnsichtArt ziel = kugelArt(unter);
        // Blender gizmo_axis_test_select: schaut man genau entlang einer Achse, waehlt der Klick in die Mitte die Gegenseite
        // ("there is no point to clicking on the current axis so show the reverse") - auch in der Perspektive (MBNAV)
        if (a.art == ziel || ausgerichtet == unter / 2) { ziel = gegenArt(ziel); }
        modellBlickSetzen(n, ziel, nullptr, true);
    } else if (klick && unter == 8) {
        t.aktiv = n;
        modellPerspektiveUmschalten();
    } else if (klick && unter == 10) {   // Kameraansicht: es gibt keine Szenenkamera (wie der Eintrag im Kuchenmenue der Ansicht)
        t.meldung = tr(Str::MnBlKameraHint);
        addStatus(t.meldung);
    }
    // --- zeichnen: die Kugeln wie Blender (gizmo_axis_draw; GPL-Teil in app_modelltab_ansicht.cpp), dann die Knoepfe ---
    if (unter >= 0) { dl->AddCircleFilled(l.mitte, radius, IM_COL32(255, 255, 255, 28), 40); }
    std::array<ModellGizmoKugel, 6> kugeln{};
    for (std::size_t k = 0; k < kugeln.size(); ++k) {
        kugeln[k] = {teile[k].i, teile[k].tiefe, l.kugel[static_cast<std::size_t>(teile[k].i)]};
    }
    modellGizmoKugelnZeichnen(dl, kugeln.data(), l.mitte, l.kugelR, groesse, unter, ausgerichtet);
    for (int i = 0; i < 4; ++i) {
        const ImVec2 p = l.knopf[static_cast<std::size_t>(i)];
        const bool kamera = kKnopfTeil[i] == 10;
        dl->AddCircleFilled(p, l.knopfR, unter == kKnopfTeil[i] ? IM_COL32(90, 90, 96, 230) : IM_COL32(40, 40, 46, 190), 20);
        knopfSymbol(dl, i, p, l.knopfR, kamera ? IM_COL32(225, 225, 225, 110) : IM_COL32(225, 225, 225, 255), a.art != AnsichtArt::Perspektive);
    }
    if (ueber) {
        static const Str kTip[3] = {Str::MkZoomZiehen, Str::MkVerschiebenZiehen, Str::MbBlPersp};
        if (unter == 10) {
            ImGui::SetItemTooltip("%s\n%s", tr(Str::MnBlKamera), tr(Str::MnBlKameraHint));
        } else {
            ImGui::SetItemTooltip("%s", tr(unter >= 6 && unter <= 8 ? kTip[unter - 6] : Str::MkGizmoHint));
        }
    }
}

}  // namespace

bool modellNavigationSichtbar(int n) {
    const ModellTab& t = mt();
    if (!g_app->settings.modellNavigation || g_app->model.empty() || n < 0 || n > 3) { return false; }
    return g_app->settings.modellNavigationAlle || !t.vierAnsichten || n == t.aktiv;
}

void drawModellNavigation(int n, ModellAnsicht& a) {
    Lage& l = g_lage[static_cast<std::size_t>(n)];
    a.wuerfel[0] = a.wuerfel[1] = a.wuerfel[2] = a.wuerfel[3] = 0.0F;
    l.da = false;
    l.kompass = false;
    l.pfeile = false;
    l.alpha = 0.0F;
    if (!modellNavigationSichtbar(n)) { return; }
    l.blender = modellBlender();
    float r[3];
    float h[3];
    float v[3];
    modellAnsichtAchsenVon(a.art, r, h, v);
    l.r = {r[0], r[1], r[2]};
    l.h = {h[0], h[1], h[2]};
    l.v = {v[0], v[1], v[2]};
    if (l.blender) {
        // oben rechts unter der Kopfzeile (dort stehen Overlays und Viewport Shading)
        const float fs = ImGui::GetFontSize();
        const float groesse = std::min(fs * 5.4F, std::min(a.groesse[0], a.groesse[1]) * 0.28F);
        if (groesse < fs * 2.0F) { return; }
        gizmo(n, a, l, ImVec2{a.lo[0] + a.groesse[0] - 8.0F, std::max(a.kopfUnten, a.lo[1]) + 4.0F}, groesse);
        l.da = true;
    } else {
        viewCube(n, a, l);   // oben rechts in der Ansicht wie die Vorlage (die Kopfzeile steht links)
        l.da = a.wuerfel[2] > a.wuerfel[0];
    }
}

// Selbsttest: die Mitte einer Flaeche, Kante oder Ecke (-1/0/1 je Achse) auf dem Schirm; 9,9,9 = Home.
bool modellWuerfelOrt(int ansicht, int x, int y, int z, ImVec2& out) {
    if (ansicht < 0 || ansicht > 3) { return false; }
    const Lage& l = g_lage[static_cast<std::size_t>(ansicht)];
    if (!l.da || l.blender) { return false; }
    if (x == 9 && y == 9 && z == 9) {
        out = l.home;
        return true;
    }
    const int d[3] = {x, y, z};
    // die sichtbarste beteiligte Flaeche traegt den Punkt (+-0,5), die anderen Achsen in der Mitte des Randstreifens
    int best = -1;
    float bt = 0.0F;
    for (int k = 0; k < 3; ++k) {
        if (d[k] == 0) { continue; }
        const float t = -dot(achse(k, static_cast<float>(d[k])), l.v);
        if (t > bt) {
            bt = t;
            best = k;
        }
    }
    if (best < 0) { return false; }
    V3 p;
    for (int k = 0; k < 3; ++k) {
        const float c = d[k] == 0 ? 0.0F : static_cast<float>(d[k]) * (k == best ? 0.5F : 0.5F - kRandBreite * 0.5F);
        p = p + achse(k, c);
    }
    out = projiziere(l, p);
    return true;
}

bool modellGizmoOrt(int ansicht, int teil, ImVec2& out) {
    if (ansicht < 0 || ansicht > 3 || teil < 0 || teil > 9) { return false; }
    const Lage& l = g_lage[static_cast<std::size_t>(ansicht)];
    if (!l.da || !l.blender) { return false; }
    static const std::size_t kKnopf[4] = {0, 1, 3, 2};   // Teil 6 Zoom, 7 Verschieben, 8 Persp/Ortho (der unterste), 9 Kamera (der dritte)
    out = teil < 6 ? l.kugel[static_cast<std::size_t>(teil)] : l.knopf[kKnopf[teil - 6]];
    return true;
}

bool modellKompassOrt(int ansicht, int teil, ImVec2& out) {
    if (ansicht < 0 || ansicht > 3 || teil < 0 || teil > 4) { return false; }
    const Lage& l = g_lage[static_cast<std::size_t>(ansicht)];
    if (!l.da || l.blender || !l.kompass) { return false; }
    out = teil < 4 ? l.buchstabe[static_cast<std::size_t>(teil)] : l.ringPunkt;
    return true;
}

bool modellKompassVerdeckt(int ansicht, int teil) {
    if (ansicht < 0 || ansicht > 3 || teil < 0 || teil > 3) { return false; }
    return (g_lage[static_cast<std::size_t>(ansicht)].buchstabeVerdeckt & (1 << teil)) != 0;
}

bool modellWuerfelPfeilOrt(int ansicht, int teil, ImVec2& out) {
    if (ansicht < 0 || ansicht > 3 || teil < 0 || teil > 5) { return false; }
    const Lage& l = g_lage[static_cast<std::size_t>(ansicht)];
    if (!l.da || l.blender || !l.pfeile) { return false; }
    out = teil < 4 ? l.pfeil[static_cast<std::size_t>(teil)] : l.rollPfeil[static_cast<std::size_t>(teil - 4)];
    return true;
}

float modellWuerfelAlpha(int ansicht) {
    if (ansicht < 0 || ansicht > 3) { return 0.0F; }
    const Lage& l = g_lage[static_cast<std::size_t>(ansicht)];
    return l.da && !l.blender ? l.alpha : 0.0F;
}

}  // namespace bhed::gui
