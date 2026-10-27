// app_karte_spielen.cpp - Karten-Editor, Stufe 8b Punkt 3: "Im Editor spielen"
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// View > "Play in editor" (F6, eine Taste ohne NRC-Belegung): ohne Kompilieren
// durch die offene Karte laufen - mit dem vorhandenen Pmove-Port
// (navSpielerPmove, src/navlauf.cpp) und der Kollision direkt aus den Brushes
// und Patches (kollisionAusKarte, src/karte/spielkollision.cpp: wie q3map2 und
// CM_LoadMap). Start an der Kamera (die Augen dort, der Kasten darunter) oder an
// info_player_start (origin + 9 wie SelectSpawnPoint, angle). Regelwerk nach dem
// Spiel des Editors (JKA, Movie Duels: gleiche Rechnung; Jedi Outcast: die
// Stellen von codeJK2). Steuerung wie im Spiel: W/A/S/D, Leertaste springt,
// C oder Strg duckt, Umschalt geht, die Maus schaut (sensitivity 5 x m_yaw/
// m_pitch 0.022 = 0,11 Grad je Punkt), N schaltet Noclip, Esc beendet und
// stellt die Kamera wieder her. Pmove laeuft wie im Spiel mit der Bildzeit
// (Stuecke von hoechstens 66 ms).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors (Pmove,
//   Quake III Arena / Jedi Academy, OpenJK). GNU General Public License,
//   version 2 (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/auswahlsystem.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <string>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

constexpr float kMausGrad = 5.0F * 0.022F;   // sensitivity 5 * m_yaw / m_pitch

ImVec2 kameraMitte() {
    const KameraFenster& f = e().kamFenster;
    return ImVec2{std::floor(f.bildX + static_cast<float>(f.breite) * 0.5F), std::floor(f.bildY + static_cast<float>(f.hoehe) * 0.5F)};
}

std::vector<std::string> surfaceparmsVon(const std::string& shader) {
    const Editor& ed = e();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    if (!sp) { return {}; }
    const karte::ShaderEintrag* s = sp->inventar.shaderFinden(shader);
    return s != nullptr ? s->surfaceparms : std::vector<std::string>{};
}

bool istJk2() {
    const Editor& ed = e();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    return sp && sp->spiel == kRegelwerkJediOutcast;
}

void kameraNachfuehren() {
    Editor& ed = e();
    SpielenZustand& z = ed.spielen;
    const float auge = navSpielerAugenhoehe(z.s);
    ed.kamera.setzeUrsprung(karte::Vektor3f(z.s.ort[0], z.s.ort[1], z.s.ort[2] + auge));
    ed.kamera.setzeWinkel(karte::Vektor3f(-z.s.nick, z.s.gier, 0.0F));   // NRC: Nick nach oben positiv
    ++ed.sichtStand;
}

}  // namespace

bool spielenAktiv() { return e().spielen.aktiv; }

void spielenStarten() {
    Editor& ed = e();
    SpielenZustand& z = ed.spielen;
    if (z.aktiv) { return; }
    freelookBeenden();
    const bool jk2 = istJk2();
    auto k = std::make_shared<KollisionsKarte>();
    z.bericht = karte::kollisionAusKarte(ed.karte, surfaceparmsVon, !jk2, *k);
    z.kollision = k;
    z.kameraUrsprung = ed.kamera.ursprung;
    z.kameraWinkel = ed.kamera.winkel;
    z.s = NavSpieler{};
    z.s.jk2 = jk2;
    z.amSpawn = false;
    if (z.startAnSpawn) {
        for (const auto& ent : ed.karte.entitaeten()) {
            if (ent->klassenname() != "info_player_start") { continue; }
            const karte::Vektor3f o = karte::entitaetUrsprung(*ent);
            z.s.ort = {o[0], o[1], o[2] + 9.0F};   // SelectSpawnPoint: origin[2] += 9
            z.s.gier = static_cast<float>(std::atof(ent->wert("angle").c_str()));
            z.amSpawn = true;
            break;
        }
    }
    if (!z.amSpawn) {
        z.s.ort = {ed.kamera.ursprung[0], ed.kamera.ursprung[1], ed.kamera.ursprung[2] - navSpielerAugenhoehe(z.s)};
        z.s.gier = ed.kamera.winkel[1];
        z.s.nick = -ed.kamera.winkel[0];
    }
    z.restMs = 0.0;
    z.bilder = 0;
    z.eingabe = NavSpielerEingabe{};
    z.aktiv = true;
    if (!z.testEingabe) { zeigerSetzen(kameraMitte()); }
    char t[256];
    std::snprintf(t, sizeof(t), trK(Str::KeSpGestartet), z.bericht.pinsel, z.bericht.patches, jk2 ? "Jedi Outcast" : "Jedi Academy");
    ausgabe(t);
    kameraNachfuehren();
}

void spielenBeenden() {
    Editor& ed = e();
    SpielenZustand& z = ed.spielen;
    if (!z.aktiv) { return; }
    z.aktiv = false;
    ed.kamera.setzeUrsprung(z.kameraUrsprung);
    ed.kamera.setzeWinkel(z.kameraWinkel);
    ++ed.sichtStand;
    ausgabe(trK(Str::KeSpBeendet));
}

void spielenSchritt(int msec) {
    Editor& ed = e();
    SpielenZustand& z = ed.spielen;
    if (!z.aktiv || !z.kollision) { return; }
    const std::shared_ptr<const KollisionsKarte> k = z.kollision;
    const NavSpurFn spur = [k](const NavVec& a, const NavVec& mi, const NavVec& ma, const NavVec& b, int, std::uint32_t m) {
        const KollisionsSpur t = kollisionsSpur(*k, a.data(), mi.data(), ma.data(), b.data(), m, 0);
        NavSpur r;
        r.allsolid = t.allsolid;
        r.startsolid = t.startsolid;
        r.fraction = t.fraction;
        r.endpos = t.endpos;
        r.normal = t.normale;
        r.contents = t.inhalt;
        r.entity = t.fraction < 1.0F ? kNavWeltEntity : kNavKeineEntity;
        return r;
    };
    // Pmove: "if ( msec > 66 ) msec = 66" je PmoveSingle
    while (msec > 0) {
        const int stueck = std::min(msec, 66);
        navSpielerPmove(z.s, spur, z.eingabe, stueck);
        msec -= stueck;
        ++z.bilder;
    }
    kameraNachfuehren();
}

void spielenBild(const ImVec2& o, const ImVec2& groesse) {
    Editor& ed = e();
    SpielenZustand& z = ed.spielen;
    ImGuiIO& io = ImGui::GetIO();
    // Esc und N immer von der Tastatur (auch wenn der Selbsttest die Bewegung einspielt)
    if (ImGui::IsKeyPressed(ImGuiKey_N, false)) { z.s.noclip = !z.s.noclip; }
    if (ImGui::IsKeyPressed(ImGuiKey_Escape, false) || (!z.testEingabe && io.AppFocusLost)) {
        spielenBeenden();
        return;
    }
    if (!z.testEingabe) {
        // ucmd wie CL_KeyMove: forwardmove/rightmove 127 (laufen) bzw. 64 (Umschalt: gehen), upmove 127 / -127
        const signed char tempo = io.KeyShift ? static_cast<signed char>(64) : static_cast<signed char>(127);
        int vor = 0;
        int seit = 0;
        if (ImGui::IsKeyDown(ImGuiKey_W)) { vor += tempo; }
        if (ImGui::IsKeyDown(ImGuiKey_S)) { vor -= tempo; }
        if (ImGui::IsKeyDown(ImGuiKey_D)) { seit += tempo; }
        if (ImGui::IsKeyDown(ImGuiKey_A)) { seit -= tempo; }
        int hoch = 0;
        if (ImGui::IsKeyDown(ImGuiKey_Space)) { hoch += 127; }
        if (ImGui::IsKeyDown(ImGuiKey_C) || io.KeyCtrl) { hoch -= 127; }
        z.eingabe.vor = static_cast<signed char>(std::clamp(vor, -127, 127));
        z.eingabe.seit = static_cast<signed char>(std::clamp(seit, -127, 127));
        z.eingabe.hoch = static_cast<signed char>(std::clamp(hoch, -127, 127));
        // Maus: wie im Spiel um die Mitte (FreezePointer)
        ImVec2 p;
        const ImVec2 m = kameraMitte();
        if (zeigerHolen(p)) {
            const float dx = p.x - m.x;
            const float dy = p.y - m.y;
            if (dx != 0.0F || dy != 0.0F) {
                z.s.gier -= dx * kMausGrad;
                z.s.nick = std::clamp(z.s.nick + dy * kMausGrad, -89.0F, 89.0F);
                zeigerSetzen(m);
            }
        }
        ImGui::SetMouseCursor(ImGuiMouseCursor_None);
        z.restMs += static_cast<double>(io.DeltaTime) * 1000.0;
        const int ms = static_cast<int>(z.restMs);
        z.restMs -= ms;
        if (ms > 0) { spielenSchritt(ms); }
    }
    // Anzeige oben links
    ImDrawList* dl = ImGui::GetWindowDrawList();
    char t[320];
    std::snprintf(t, sizeof(t), trK(Str::KeSpHud), z.s.ort[0], z.s.ort[1], z.s.ort[2],
                  std::sqrt(z.s.tempo[0] * z.s.tempo[0] + z.s.tempo[1] * z.s.tempo[1]), z.s.noclip ? " noclip" : "", z.s.amBoden ? "" : " (air)");
    dl->AddRectFilled(ImVec2{o.x + 4, o.y + 4}, ImVec2{o.x + 8 + ImGui::CalcTextSize(t).x, o.y + 8 + ImGui::GetTextLineHeight()}, IM_COL32(0, 0, 0, 160));
    dl->AddText(ImVec2{o.x + 6, o.y + 6}, IM_COL32(255, 230, 120, 255), t);
    const char* hilfe = trK(Str::KeSpHilfe);
    dl->AddText(ImVec2{o.x + 6, o.y + groesse.y - ImGui::GetTextLineHeight() - 6}, IM_COL32(220, 220, 220, 255), hilfe);
    // Fadenkreuz
    const ImVec2 mm{o.x + groesse.x * 0.5F, o.y + groesse.y * 0.5F};
    dl->AddLine(ImVec2{mm.x - 6, mm.y}, ImVec2{mm.x + 6, mm.y}, IM_COL32(255, 255, 255, 200));
    dl->AddLine(ImVec2{mm.x, mm.y - 6}, ImVec2{mm.x, mm.y + 6}, IM_COL32(255, 255, 255, 200));
}

}  // namespace bhed::gui::karteneditor
