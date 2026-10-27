// app_karte_logik.cpp - Karten-Editor, Stufe 8b Punkt 4: das Fenster
// "Logic view" (eigene Funktion, nicht in NetRadiant-Custom).
//
// Der Graph der Entity-Verknuepfungen (src/karte/logik.cpp): je Entity ein
// Kasten mit Klasse, Namen und den Skript-Schluesseln, je Verknuepfung ein
// Pfeil mit dem Schluessel (target, target2, NPC_target ...); fehlende Ziele
// rot als "? name". Klick auf einen Kasten waehlt die Entity und zentriert die
// Ansichten (CenterXYView, CameraFocusOnSelected); Doppelklick auf eine
// Skriptzeile oeffnet das ICARUS-Skript (scripts/<wert>.txt, .icarus, .ibi aus
// dem Spiel des Editors bzw. den Spielordnern des Programms) im Skript-Editor
// des Programms; fehlt es, sagt ein Kasten wo gesucht wurde. Ziehen mit der
// rechten oder mittleren Maustaste verschiebt, das Rad zoomt.
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/logik.h"

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace bhed::gui::karteneditor {

namespace {

using i18n::Str;

Editor& e() { return editor(); }

void grafNeu(LogikZustand& z) {
    const Editor& ed = e();
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    z.graf = karte::logikGraph(ed.karte, sp ? &sp->klassen : nullptr);
    z.stand = ed.karte.undo.abschluesse();
    z.ladeStand = ed.ladeZaehler;
}

// Bildpunkte je Graph-Punkt: Zoom mal Schriftmassstab (18 Graph-Punkte = eine Zeile)
ImVec2 bild(const LogikZustand& z, const ImVec2& ursprung, float x, float y) {
    return ImVec2{ursprung.x + (x + z.verschiebung[0]) * z.mass, ursprung.y + (y + z.verschiebung[1]) * z.mass};
}

}  // namespace

void logikZeigen() {
    LogikZustand& z = e().logik;
    z.offen = true;
    z.einpassen = true;
    grafNeu(z);
}

bool logikSkriptOeffnen(const std::string& wert) {
    Editor& ed = e();
    std::string pfad = wert;
    for (char& c : pfad) {
        if (c == '\\') { c = '/'; }
    }
    std::string daten;
    std::string gefunden;
    const auto& sp = ed.karteSpiel ? ed.karteSpiel : ed.spiel;
    for (const char* ext : {".txt", ".icarus", ".ibi", ".IBI"}) {
        const std::string name = "scripts/" + pfad + ext;
        if ((sp && sp->fs.dateiLesen(name, daten)) || readFromArchives(name, daten)) {
            gefunden = name;
            break;
        }
    }
    ed.logik.letztesSkript = gefunden;
    if (gefunden.empty()) {
        char t[600];
        std::snprintf(t, sizeof(t), trK(Str::KeLgSkriptFehlt), pfad.c_str(), pfad.c_str());
        qtMeldung(trK(Str::KeLgTitel), t, MeldungArt::Warnung, false, nullptr);
        return false;
    }
    if (ed.logik.nurPruefen) { return true; }   // Selbsttest: nicht in den Skript-Editor wechseln
    const std::string dat = daten;
    const std::string nm = gefunden;
    withUnsaved([dat, nm] {
        g_app->leftMode = 0;   // der Skript-Editor des Programms
        oeffneSkriptAusSpeicher(dat, nm);
    });
    return true;
}

void logikKnotenWaehlen(int nr) {
    Editor& ed = e();
    LogikZustand& z = ed.logik;
    if (nr < 0 || nr >= static_cast<int>(z.graf.knoten.size())) { return; }
    const auto ent = z.graf.knoten[static_cast<std::size_t>(nr)].entitaet.lock();
    z.gewaehlt = nr;
    if (!ent) { return; }
    ed.karte.auswahl.leeren();
    if (ent->istBehaelter() && !ent->kinder().empty()) {
        for (const auto& p : ent->kinder()) { ed.karte.auswahl.waehlen(ent, p); }
    } else {
        ed.karte.auswahl.waehlen(ent);
    }
    auswahlGeaendert();
    (void)ausfuehren("CenterXYView");
    (void)ausfuehren("CameraFocusOnSelected");
    ++ed.sichtStand;
}

void logikZeichnen() {
    Editor& ed = e();
    LogikZustand& z = ed.logik;
    if (!z.offen) { return; }
    if (z.stand != ed.karte.undo.abschluesse() || z.ladeStand != ed.ladeZaehler) { grafNeu(z); }
    const float fs = ImGui::GetFontSize();
    ImGui::SetNextWindowSize(ImVec2{58.0F * fs, 34.0F * fs}, ImGuiCond_FirstUseEver);
    const std::string titel = std::string(trK(Str::KeLgTitel)) + "###kartelogik";
    if (!ImGui::Begin(titel.c_str(), &z.offen, ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoScrollWithMouse)) {
        ImGui::End();
        return;
    }
    char kopf[256];
    std::snprintf(kopf, sizeof(kopf), trK(Str::KeLgKopf), static_cast<int>(z.graf.knoten.size()) - (z.graf.fehlend > 0 ? 0 : 0),
                  static_cast<int>(z.graf.kanten.size()), z.graf.fehlend);
    ImGui::TextUnformatted(kopf);
    ImGui::SameLine();
    ImGui::TextDisabled("(%s)", trK(Str::KeLgNichtNrc));
    const ImVec2 ursprung = ImGui::GetCursorScreenPos();
    const ImVec2 groesse = ImGui::GetContentRegionAvail();
    if (z.einpassen && groesse.x > 10.0F && groesse.y > 10.0F) {
        // beim Oeffnen: der ganze Graph ins Fenster (hoechstens Zoom 1)
        z.einpassen = false;
        float bx = 1.0F;
        float by = 1.0F;
        for (const karte::LogikKnoten& n : z.graf.knoten) {
            bx = std::max(bx, n.x + n.breite);
            by = std::max(by, n.y + n.hoehe);
        }
        const float basis = ImGui::GetFontSize() / 14.0F;
        z.zoom = std::clamp(std::min((groesse.x - 20.0F) / (bx * basis), (groesse.y - 20.0F) / (by * basis)), 0.15F, 1.0F);
        z.verschiebung[0] = 10.0F / (z.zoom * basis);
        z.verschiebung[1] = 10.0F / (z.zoom * basis);
    }
    z.mass = z.zoom * ImGui::GetFontSize() / 14.0F;
    ImGui::InvisibleButton("##logikflaeche", ImVec2{std::max(1.0F, groesse.x), std::max(1.0F, groesse.y)},
                           ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight | ImGuiButtonFlags_MouseButtonMiddle);
    const bool ueber = ImGui::IsItemHovered();
    ImGuiIO& io = ImGui::GetIO();
    if (ImGui::IsItemActive() && (ImGui::IsMouseDragging(ImGuiMouseButton_Right) || ImGui::IsMouseDragging(ImGuiMouseButton_Middle))) {
        z.verschiebung[0] += io.MouseDelta.x / z.mass;
        z.verschiebung[1] += io.MouseDelta.y / z.mass;
    }
    if (ueber && io.MouseWheel != 0.0F) {
        const float alt = z.mass;
        z.zoom = std::clamp(z.zoom * (io.MouseWheel > 0 ? 1.15F : 1.0F / 1.15F), 0.15F, 3.0F);
        z.mass = z.zoom * ImGui::GetFontSize() / 14.0F;
        // um den Zeiger zoomen
        const float mx = (io.MousePos.x - ursprung.x) / alt - z.verschiebung[0];
        const float my = (io.MousePos.y - ursprung.y) / alt - z.verschiebung[1];
        z.verschiebung[0] = (io.MousePos.x - ursprung.x) / z.mass - mx;
        z.verschiebung[1] = (io.MousePos.y - ursprung.y) / z.mass - my;
    }
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->PushClipRect(ursprung, ImVec2{ursprung.x + groesse.x, ursprung.y + groesse.y}, true);
    dl->AddRectFilled(ursprung, ImVec2{ursprung.x + groesse.x, ursprung.y + groesse.y}, ImGui::GetColorU32(ImGuiCol_FrameBg));
    const float zeile = 18.0F * z.mass;
    // Schrift mit dem Zoom (14 Punkte je Zeile von 18), damit Kaesten und Text zusammen wachsen
    ImFont* schrift = ImGui::GetFont();
    const float schriftGr = 14.0F * z.mass;
    // Kanten
    for (const karte::LogikKante& k : z.graf.kanten) {
        const karte::LogikKnoten& a = z.graf.knoten[static_cast<std::size_t>(k.von)];
        const karte::LogikKnoten& b = z.graf.knoten[static_cast<std::size_t>(k.zu)];
        const ImVec2 p0 = bild(z, ursprung, a.x + a.breite, a.y + 9.0F);
        const ImVec2 p1 = bild(z, ursprung, b.x, b.y + 9.0F);
        const bool fehlt = b.entitaet.expired();
        const ImU32 farbe = fehlt ? IM_COL32(230, 80, 80, 255) : (k.von == z.gewaehlt || k.zu == z.gewaehlt ? IM_COL32(255, 210, 90, 255)
                                                                                                             : IM_COL32(140, 170, 220, 220));
        const float d = std::max(30.0F * z.mass, std::fabs(p1.x - p0.x) * 0.4F);
        dl->AddBezierCubic(p0, ImVec2{p0.x + d, p0.y}, ImVec2{p1.x - d, p1.y}, p1, farbe, 1.5F);
        dl->AddTriangleFilled(p1, ImVec2{p1.x - 7 * z.mass, p1.y - 4 * z.mass}, ImVec2{p1.x - 7 * z.mass, p1.y + 4 * z.mass}, farbe);
        if (z.zoom > 0.45F) {
            const ImVec2 m{(p0.x + p1.x) * 0.5F, (p0.y + p1.y) * 0.5F - schriftGr};
            dl->AddText(schrift, schriftGr, m, farbe, k.schluessel.c_str());
        }
    }
    // Knoten
    int unterMaus = -1;
    int skriptUnterMaus = -1;
    for (std::size_t i = 0; i < z.graf.knoten.size(); ++i) {
        const karte::LogikKnoten& n = z.graf.knoten[i];
        const ImVec2 a = bild(z, ursprung, n.x, n.y);
        const ImVec2 b = bild(z, ursprung, n.x + n.breite, n.y + n.hoehe);
        if (b.x < ursprung.x || a.x > ursprung.x + groesse.x || b.y < ursprung.y || a.y > ursprung.y + groesse.y) { continue; }
        const bool fehlt = n.entitaet.expired();
        const bool gew = static_cast<int>(i) == z.gewaehlt;
        dl->AddRectFilled(a, b, fehlt ? IM_COL32(90, 30, 30, 255) : (gew ? IM_COL32(80, 70, 30, 255) : IM_COL32(45, 50, 60, 255)), 3.0F);
        dl->AddRect(a, b, gew ? IM_COL32(255, 210, 90, 255) : IM_COL32(120, 130, 150, 255), 3.0F);
        if (z.zoom > 0.3F) {
            float y = a.y + 2.0F * z.mass;
            dl->AddText(schrift, schriftGr, ImVec2{a.x + 4 * z.mass, y}, IM_COL32(255, 255, 255, 255), n.klasse.c_str());
            y += zeile;
            if (!n.name.empty()) {
                dl->AddText(schrift, schriftGr, ImVec2{a.x + 4 * z.mass, y}, IM_COL32(170, 220, 170, 255), n.name.c_str());
                y += zeile;
            }
            for (std::size_t s = 0; s < n.skripte.size(); ++s) {
                const std::string t = n.skripte[s].first + ": " + n.skripte[s].second;
                const bool ueberZeile = ueber && io.MousePos.x >= a.x && io.MousePos.x <= b.x && io.MousePos.y >= y && io.MousePos.y < y + zeile;
                dl->AddText(schrift, schriftGr, ImVec2{a.x + 4 * z.mass, y}, ueberZeile ? IM_COL32(255, 230, 120, 255) : IM_COL32(150, 190, 255, 255),
                            t.c_str());
                if (ueberZeile) { skriptUnterMaus = static_cast<int>(s); }
                y += zeile;
            }
        }
        if (ueber && io.MousePos.x >= a.x && io.MousePos.x <= b.x && io.MousePos.y >= a.y && io.MousePos.y <= b.y) {
            unterMaus = static_cast<int>(i);
        }
        // fuer den Selbsttest: die Lage jedes Kastens (Bildpunkte)
        if (z.lagenMerken) {
            if (z.lagen.size() < z.graf.knoten.size()) { z.lagen.resize(z.graf.knoten.size()); }
            z.lagen[i] = {a.x, a.y, b.x, b.y};
        }
    }
    dl->PopClipRect();
    if (unterMaus >= 0) {
        const karte::LogikKnoten& n = z.graf.knoten[static_cast<std::size_t>(unterMaus)];
        if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left) && skriptUnterMaus >= 0) {
            (void)logikSkriptOeffnen(n.skripte[static_cast<std::size_t>(skriptUnterMaus)].second);
        } else if (ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
            logikKnotenWaehlen(unterMaus);
        }
        if (const auto ent = n.entitaet.lock()) {
            std::string tip;
            for (const auto& [s, w] : ent->schluessel()) { tip += s + " = " + w + "\n"; }
            if (!n.skripte.empty()) { tip += trK(Str::KeLgDoppelklick); }
            ImGui::SetTooltip("%s", tip.c_str());
        } else {
            ImGui::SetTooltip("%s", trK(Str::KeLgFehlt));
        }
    }
    ImGui::End();
}

}  // namespace bhed::gui::karteneditor
