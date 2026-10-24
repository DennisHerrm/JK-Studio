// karte_vektoricons.cpp - die Symbole der Werkzeugleisten des Karten-Editors
// im Strichstil des Programms (gui/uiicons.cpp).
//
// shank (21.10.2026): "es soll moderner aussehen von den buttons her als
// Radiant aber dieselbe Positionen haben". Also NICHT NRCs Bitmaps, sondern
// eigene Vektorzeichnungen auf dem 16er-Raster mit gleichmaessiger
// Linienstaerke - in den Farben des Programms. Jedes Symbol behaelt die
// Bedeutung des NRC-Symbols gleichen Namens (setup/data/tools/bitmaps/<name>
// .svg), damit man es wiedererkennt: CSG Subtract als Kasten mit
// ausgeschnittener Ecke, Clipper als Schnittlinie mit zwei Punkten,
// Vertices als Wuerfel mit Eckpunkten und so weiter.
#include "karteneditor.h"

#include "app_internal.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <initializer_list>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

constexpr float kPi = 3.14159265358979F;

struct Stift {
    ImDrawList* dl;
    ImVec2 o;
    float s;      // Bildpunkte je Rastereinheit (Groesse / 16)
    ImU32 c;
    float w;      // Linienstaerke
    [[nodiscard]] ImVec2 p(float x, float y) const { return ImVec2{o.x + x * s, o.y + y * s}; }
    void linie(float x0, float y0, float x1, float y1) const { dl->AddLine(p(x0, y0), p(x1, y1), c, w); }
    void zug(std::initializer_list<ImVec2> pts, bool zu = false) const {
        std::vector<ImVec2> v;
        for (const ImVec2& q : pts) { v.push_back(p(q.x, q.y)); }
        dl->AddPolyline(v.data(), static_cast<int>(v.size()), c, zu ? ImDrawFlags_Closed : ImDrawFlags_None, w);
    }
    void flaeche(std::initializer_list<ImVec2> pts, ImU32 farbe) const {
        std::vector<ImVec2> v;
        for (const ImVec2& q : pts) { v.push_back(p(q.x, q.y)); }
        dl->AddConvexPolyFilled(v.data(), static_cast<int>(v.size()), farbe);
    }
    void rahmen(float x0, float y0, float x1, float y1, float rund = 1.2F) const {
        dl->AddRect(p(x0, y0), p(x1, y1), c, rund * s, 0, w);
    }
    void kasten(float x0, float y0, float x1, float y1, ImU32 farbe, float rund = 1.0F) const {
        dl->AddRectFilled(p(x0, y0), p(x1, y1), farbe, rund * s);
    }
    void gestrichelt(float x0, float y0, float x1, float y1, float strich = 1.6F) const {
        const float dx = x1 - x0;
        const float dy = y1 - y0;
        const float l = std::sqrt(dx * dx + dy * dy);
        if (l <= 0.0F) { return; }
        for (float t = 0.0F; t < l; t += strich * 2.0F) {
            const float e = std::min(l, t + strich);
            linie(x0 + dx * t / l, y0 + dy * t / l, x0 + dx * e / l, y0 + dy * e / l);
        }
    }
    void strichRahmen(float x0, float y0, float x1, float y1) const {
        gestrichelt(x0, y0, x1, y0);
        gestrichelt(x1, y0, x1, y1);
        gestrichelt(x1, y1, x0, y1);
        gestrichelt(x0, y1, x0, y0);
    }
    void kreis(float x, float y, float r) const { dl->AddCircle(p(x, y), r * s, c, 0, w); }
    void punkt(float x, float y, float r) const { dl->AddCircleFilled(p(x, y), r * s, c); }
    void punkt(float x, float y, float r, ImU32 farbe) const { dl->AddCircleFilled(p(x, y), r * s, farbe); }
    void bogen(float x, float y, float r, float a0, float a1) const {
        dl->PathArcTo(p(x, y), r * s, a0, a1);
        dl->PathStroke(c, ImDrawFlags_None, w);
    }
    void spitze(float x, float y, float dx, float dy, float l = 2.4F) const {
        const float n = std::sqrt(dx * dx + dy * dy);
        if (n <= 0.0F) { return; }
        dx /= n;
        dy /= n;
        zug({ImVec2{x - dx * l - dy * l * 0.8F, y - dy * l + dx * l * 0.8F}, ImVec2{x, y},
             ImVec2{x - dx * l + dy * l * 0.8F, y - dy * l - dx * l * 0.8F}});
    }
    // Bogenpfeil um (x, y) von a0 nach a1 (Bogenmass), Spitze am Ende.
    void bogenPfeil(float x, float y, float r, float a0, float a1) const {
        bogen(x, y, r, a0, a1);
        const float ex = x + std::cos(a1) * r;
        const float ey = y + std::sin(a1) * r;
        const float richtung = a1 > a0 ? 1.0F : -1.0F;
        spitze(ex, ey, -std::sin(a1) * richtung, std::cos(a1) * richtung, 2.2F);
    }
    // Wuerfel in Schraegansicht (Drahtgitter), Ecken fuer Hervorhebungen.
    void wuerfel(float x0, float y0, float g, float t) const {
        rahmen(x0, y0 + t, x0 + g, y0 + t + g, 0.2F);
        zug({ImVec2{x0, y0 + t}, ImVec2{x0 + t, y0}, ImVec2{x0 + t + g, y0}, ImVec2{x0 + g, y0 + t}});
        zug({ImVec2{x0 + t + g, y0}, ImVec2{x0 + t + g, y0 + g}, ImVec2{x0 + g, y0 + t + g}});
    }
};

ImU32 mitAlpha(ImU32 c, float a) {
    ImVec4 v = ImGui::ColorConvertU32ToFloat4(c);
    v.w *= a;
    return ImGui::ColorConvertFloat4ToU32(v);
}

bool ist(const char* a, const char* b) { return std::strcmp(a, b) == 0; }

}  // namespace

// Die Farbe je Symbol nach seiner Bedeutung (wie uiIconFarbe): Brush-
// Werkzeuge bernstein, Auswahl cyan, Komponenten violett, Manipulatoren
// gruen, Fenster blau, Filter nach dem, was sie filtern. Im hellen
// Farbschema dunkler.
ImVec4 karteIconFarbe(const char* n) {
    ImVec4 c{0.80F, 0.82F, 0.86F, 1.0F};
    const auto setze = [&c](float r, float g, float b) { c = ImVec4{r, g, b, 1.0F}; };
    if (std::strncmp(n, "brush_", 6) == 0 || std::strncmp(n, "selection_csg", 13) == 0 || ist(n, "selection_makeroom") ||
        std::strncmp(n, "csgtool_", 8) == 0 ||
        ist(n, "f-structural") || ist(n, "status_brush")) {
        setze(0.98F, 0.70F, 0.30F);
    } else if (std::strncmp(n, "selection_select", 16) == 0 || ist(n, "f-region") || ist(n, "select_mouseresize")) {
        setze(0.36F, 0.82F, 0.95F);
    } else if (std::strncmp(n, "modify_", 7) == 0) {
        setze(0.78F, 0.62F, 1.00F);
    } else if (std::strncmp(n, "select_mouse", 12) == 0) {
        setze(0.45F, 0.86F, 0.50F);
    } else if (ist(n, "select_clipper")) {
        setze(0.45F, 0.70F, 1.00F);
    } else if (std::strncmp(n, "texture_lock", 12) == 0 || std::strncmp(n, "texture_vertexlock", 18) == 0) {
        setze(0.98F, 0.84F, 0.40F);
    } else if (ist(n, "file_open")) {
        setze(0.98F, 0.75F, 0.30F);
    } else if (ist(n, "file_save")) {
        setze(0.40F, 0.68F, 1.00F);
    } else if (ist(n, "f-details") || ist(n, "f-funcgroups")) {
        setze(0.85F, 0.66F, 0.42F);
    } else if (ist(n, "f-patches") || ist(n, "curve_cap") || ist(n, "status_patch")) {
        setze(0.42F, 0.85F, 0.78F);
    } else if (ist(n, "f-liquids") || ist(n, "f-sky")) {
        setze(0.45F, 0.75F, 1.00F);
    } else if (ist(n, "f-caulk")) {
        setze(1.00F, 0.55F, 0.78F);
    } else if (ist(n, "f-clip") || ist(n, "f-hint")) {
        setze(0.70F, 0.62F, 1.00F);
    } else if (ist(n, "f-entities") || ist(n, "f-pointentities") || ist(n, "entities") || ist(n, "status_entity")) {
        setze(0.36F, 0.82F, 0.95F);
    } else if (ist(n, "f-lights")) {
        setze(1.00F, 0.90F, 0.50F);
    } else if (ist(n, "f-models") || ist(n, "refresh_models")) {
        setze(0.98F, 0.86F, 0.38F);
    } else if (ist(n, "f-triggers")) {
        setze(1.00F, 0.62F, 0.30F);
    } else if (ist(n, "f-world")) {
        setze(0.42F, 0.85F, 0.60F);
    } else if (ist(n, "f-reset") || ist(n, "f-hide")) {
        setze(1.00F, 0.50F, 0.45F);
    } else if (ist(n, "select")) {
        setze(0.36F, 0.82F, 0.95F);
    } else if (std::strncmp(n, "arrow_", 6) == 0) {
        setze(0.55F, 0.78F, 1.00F);
    } else if (std::strncmp(n, "bobtoolz_", 9) == 0) {
        setze(0.62F, 0.84F, 0.48F);
    } else if (ist(n, "view_change") || ist(n, "view_cubicclipping") || ist(n, "texture_browser") || ist(n, "console") ||
               std::strncmp(n, "texbro_", 7) == 0 || std::strncmp(n, "window", 6) == 0 || ist(n, "search") ||
               ist(n, "search_from_start") || ist(n, "copy")) {
        setze(0.55F, 0.78F, 1.00F);
    }
    const ImVec4 bg = ImGui::GetStyleColorVec4(ImGuiCol_WindowBg);
    if (bg.x * 0.3F + bg.y * 0.59F + bg.z * 0.11F > 0.6F) { c = ImVec4{c.x * 0.62F, c.y * 0.62F, c.z * 0.62F, 1.0F}; }
    return c;
}

bool karteIcon(ImDrawList* dl, const char* n, ImVec2 pos, float groesse, unsigned int farbe) {
    const Stift z{dl, pos, groesse / 16.0F, farbe, std::max(1.0F, groesse / 16.0F * 1.3F)};
    const ImU32 schwach = mitAlpha(farbe, 0.35F);
    // --- Datei, Rueckgaengig: die Symbole des Programms ---
    if (ist(n, "file_open")) { uiIcon(dl, UiIcon::Oeffnen, pos, groesse, farbe); return true; }
    if (ist(n, "file_save")) { uiIcon(dl, UiIcon::Speichern, pos, groesse, farbe); return true; }
    if (ist(n, "undo")) { uiIcon(dl, UiIcon::Rueckgaengig, pos, groesse, farbe); return true; }
    if (ist(n, "redo")) { uiIcon(dl, UiIcon::Wiederholen, pos, groesse, farbe); return true; }
    // --- Drehen/Spiegeln: ein Dreieck an einer Achse gespiegelt ---
    if (ist(n, "brush_flip_hor")) {   // waagerecht spiegeln: senkrechte Achse
        z.gestrichelt(8.0F, 1.5F, 8.0F, 14.5F, 1.2F);
        z.zug({ImVec2{6.5F, 3.5F}, ImVec2{1.5F, 12.5F}, ImVec2{6.5F, 12.5F}}, true);
        z.flaeche({ImVec2{9.5F, 3.5F}, ImVec2{14.5F, 12.5F}, ImVec2{9.5F, 12.5F}}, schwach);
        z.zug({ImVec2{9.5F, 3.5F}, ImVec2{14.5F, 12.5F}, ImVec2{9.5F, 12.5F}}, true);
        return true;
    }
    if (ist(n, "brush_flip_vert")) {   // senkrecht spiegeln: waagerechte Achse
        z.gestrichelt(1.5F, 8.0F, 14.5F, 8.0F, 1.2F);
        z.zug({ImVec2{3.5F, 6.5F}, ImVec2{12.5F, 1.5F}, ImVec2{12.5F, 6.5F}}, true);
        z.flaeche({ImVec2{3.5F, 9.5F}, ImVec2{12.5F, 14.5F}, ImVec2{12.5F, 9.5F}}, schwach);
        z.zug({ImVec2{3.5F, 9.5F}, ImVec2{12.5F, 14.5F}, ImVec2{12.5F, 9.5F}}, true);
        return true;
    }
    if (ist(n, "brush_rotate_anti") || ist(n, "brush_rotate_clock")) {
        z.kasten(5.5F, 5.5F, 10.5F, 10.5F, schwach, 0.8F);
        z.rahmen(5.5F, 5.5F, 10.5F, 10.5F, 0.8F);
        if (ist(n, "brush_rotate_anti")) {
            z.bogenPfeil(8.0F, 8.0F, 6.2F, -0.2F * kPi, -1.35F * kPi);
        } else {
            z.bogenPfeil(8.0F, 8.0F, 6.2F, -0.8F * kPi, 0.35F * kPi);
        }
        return true;
    }
    // --- Auswahl ---
    if (ist(n, "selection_selecttouching")) {   // zwei Kaesten, die sich beruehren
        z.rahmen(1.5F, 1.5F, 9.5F, 9.5F);
        z.kasten(6.5F, 6.5F, 14.5F, 14.5F, schwach);
        z.strichRahmen(6.5F, 6.5F, 14.5F, 14.5F);
        return true;
    }
    if (ist(n, "selection_selectinside")) {   // Kasten in einem gestrichelten Rahmen
        z.strichRahmen(1.5F, 1.5F, 14.5F, 14.5F);
        z.kasten(5.0F, 5.0F, 11.0F, 11.0F, schwach);
        z.rahmen(5.0F, 5.0F, 11.0F, 11.0F);
        return true;
    }
    // --- CSG ---
    if (ist(n, "selection_csgsubtract")) {   // Kasten mit ausgeschnittener Ecke, Minus
        z.zug({ImVec2{1.5F, 1.5F}, ImVec2{14.5F, 1.5F}, ImVec2{14.5F, 7.5F}, ImVec2{7.5F, 7.5F}, ImVec2{7.5F, 14.5F},
               ImVec2{1.5F, 14.5F}}, true);
        z.strichRahmen(7.5F, 7.5F, 14.5F, 14.5F);
        z.linie(9.5F, 11.0F, 12.5F, 11.0F);
        return true;
    }
    if (ist(n, "selection_csgmerge")) {   // zwei Kaesten in einer gemeinsamen Huelle
        z.kasten(2.5F, 6.5F, 7.5F, 13.5F, schwach, 0.6F);
        z.kasten(8.5F, 2.5F, 13.5F, 9.5F, schwach, 0.6F);
        z.zug({ImVec2{1.5F, 5.5F}, ImVec2{7.5F, 1.5F}, ImVec2{14.5F, 1.5F}, ImVec2{14.5F, 10.5F}, ImVec2{8.5F, 14.5F},
               ImVec2{1.5F, 14.5F}}, true);
        return true;
    }
    // --- CSG Tool (setup/data/tools/bitmaps/csgtool_*.svg) ---
    if (ist(n, "csgtool_shrink")) {   // Kasten, vier Pfeile nach innen
        z.kasten(5.5F, 5.5F, 10.5F, 10.5F, schwach, 0.6F);
        z.rahmen(5.5F, 5.5F, 10.5F, 10.5F, 0.6F);
        z.linie(0.8F, 8.0F, 3.8F, 8.0F);
        z.spitze(3.8F, 8.0F, 1.0F, 0.0F, 1.6F);
        z.linie(15.2F, 8.0F, 12.2F, 8.0F);
        z.spitze(12.2F, 8.0F, -1.0F, 0.0F, 1.6F);
        z.linie(8.0F, 0.8F, 8.0F, 3.8F);
        z.spitze(8.0F, 3.8F, 0.0F, 1.0F, 1.6F);
        z.linie(8.0F, 15.2F, 8.0F, 12.2F);
        z.spitze(8.0F, 12.2F, 0.0F, -1.0F, 1.6F);
        return true;
    }
    if (ist(n, "csgtool_expand")) {   // gestrichelter Kasten, vier Pfeile nach aussen
        z.strichRahmen(4.5F, 4.5F, 11.5F, 11.5F);
        z.linie(4.5F, 8.0F, 1.0F, 8.0F);
        z.spitze(1.0F, 8.0F, -1.0F, 0.0F, 1.6F);
        z.linie(11.5F, 8.0F, 15.0F, 8.0F);
        z.spitze(15.0F, 8.0F, 1.0F, 0.0F, 1.6F);
        z.linie(8.0F, 4.5F, 8.0F, 1.0F);
        z.spitze(8.0F, 1.0F, 0.0F, -1.0F, 1.6F);
        z.linie(8.0F, 11.5F, 8.0F, 15.0F);
        z.spitze(8.0F, 15.0F, 0.0F, 1.0F, 1.6F);
        return true;
    }
    if (ist(n, "csgtool_diagonal")) {   // Rahmen mit Gehrungen an den Ecken
        z.kasten(1.5F, 1.5F, 14.5F, 14.5F, schwach, 0.4F);
        z.kasten(5.0F, 5.0F, 11.0F, 11.0F, ImGui::GetColorU32(ImGuiCol_WindowBg), 0.0F);
        z.rahmen(1.5F, 1.5F, 14.5F, 14.5F, 0.4F);
        z.rahmen(5.0F, 5.0F, 11.0F, 11.0F, 0.0F);
        z.linie(1.5F, 1.5F, 5.0F, 5.0F);
        z.linie(14.5F, 1.5F, 11.0F, 5.0F);
        z.linie(1.5F, 14.5F, 5.0F, 11.0F);
        z.linie(14.5F, 14.5F, 11.0F, 11.0F);
        return true;
    }
    if (ist(n, "csgtool_wrap")) {   // vier Wandstuecke im Kreis (jede Wand bis zur naechsten)
        z.kasten(1.5F, 1.5F, 11.0F, 4.5F, schwach, 0.0F);
        z.rahmen(1.5F, 1.5F, 11.0F, 4.5F, 0.0F);
        z.kasten(11.5F, 1.5F, 14.5F, 11.0F, schwach, 0.0F);
        z.rahmen(11.5F, 1.5F, 14.5F, 11.0F, 0.0F);
        z.kasten(5.0F, 11.5F, 14.5F, 14.5F, schwach, 0.0F);
        z.rahmen(5.0F, 11.5F, 14.5F, 14.5F, 0.0F);
        z.kasten(1.5F, 5.0F, 4.5F, 14.5F, schwach, 0.0F);
        z.rahmen(1.5F, 5.0F, 4.5F, 14.5F, 0.0F);
        return true;
    }
    if (ist(n, "csgtool_extrude") || ist(n, "csgtool_pull")) {
        // Pinsel mit schraeger Flaeche; davor die Platte: Extrude mit Seiten
        // senkrecht zur Flaeche, Pull mit den verlaengerten Nachbarflaechen.
        z.zug({ImVec2{5.5F, 14.5F}, ImVec2{5.5F, 9.5F}, ImVec2{9.5F, 5.5F}, ImVec2{14.5F, 5.5F}, ImVec2{14.5F, 14.5F}}, true);
        if (ist(n, "csgtool_extrude")) {   // Platte senkrecht vor der schraegen Flaeche
            z.flaeche({ImVec2{5.5F, 9.5F}, ImVec2{9.5F, 5.5F}, ImVec2{6.7F, 2.7F}, ImVec2{2.7F, 6.7F}}, schwach);
            z.zug({ImVec2{5.5F, 9.5F}, ImVec2{2.7F, 6.7F}, ImVec2{6.7F, 2.7F}, ImVec2{9.5F, 5.5F}});
        } else {   // Platte bis zu den verlaengerten Nachbarflaechen: die Ecke
            z.flaeche({ImVec2{5.5F, 9.5F}, ImVec2{9.5F, 5.5F}, ImVec2{5.5F, 5.5F}}, schwach);
            z.zug({ImVec2{5.5F, 9.5F}, ImVec2{5.5F, 5.5F}, ImVec2{9.5F, 5.5F}});
            z.gestrichelt(5.5F, 5.5F, 5.5F, 1.5F, 1.0F);
            z.gestrichelt(5.5F, 5.5F, 1.5F, 5.5F, 1.0F);
        }
        return true;
    }
    if (ist(n, "csgtool_removeinner")) {   // Kasten mit Kreuz: der innere Pinsel faellt weg
        z.rahmen(1.5F, 1.5F, 14.5F, 14.5F, 0.6F);
        z.kasten(4.5F, 4.5F, 11.5F, 11.5F, schwach, 0.4F);
        const Stift rot{dl, pos, z.s, ImGui::GetColorU32(ImVec4{1.00F, 0.45F, 0.42F, 1.0F}), z.w * 1.3F};
        rot.linie(5.5F, 5.5F, 10.5F, 10.5F);
        rot.linie(10.5F, 5.5F, 5.5F, 10.5F);
        return true;
    }
    if (ist(n, "selection_makeroom")) {   // hohler Kasten: Waende um einen Raum
        z.rahmen(1.5F, 1.5F, 14.5F, 14.5F, 0.8F);
        z.rahmen(4.5F, 4.5F, 11.5F, 11.5F, 0.4F);
        z.kasten(1.5F, 1.5F, 14.5F, 4.5F, schwach, 0.0F);
        z.kasten(1.5F, 11.5F, 14.5F, 14.5F, schwach, 0.0F);
        z.kasten(1.5F, 4.5F, 4.5F, 11.5F, schwach, 0.0F);
        z.kasten(11.5F, 4.5F, 14.5F, 11.5F, schwach, 0.0F);
        return true;
    }
    if (ist(n, "ellipsis")) {   // CSG Tool: weitere Werkzeuge
        z.punkt(3.5F, 8.0F, 1.4F);
        z.punkt(8.0F, 8.0F, 1.4F);
        z.punkt(12.5F, 8.0F, 1.4F);
        return true;
    }
    // --- Komponenten ---
    if (ist(n, "modify_vertices") || ist(n, "modify_edges") || ist(n, "modify_faces")) {
        if (ist(n, "modify_faces")) { z.flaeche({ImVec2{2.0F, 6.0F}, ImVec2{10.0F, 6.0F}, ImVec2{10.0F, 14.0F}, ImVec2{2.0F, 14.0F}}, schwach); }
        z.wuerfel(2.0F, 2.0F, 8.0F, 4.0F);
        if (ist(n, "modify_vertices")) {
            for (const ImVec2& q : {ImVec2{2.0F, 6.0F}, ImVec2{10.0F, 6.0F}, ImVec2{2.0F, 14.0F}, ImVec2{10.0F, 14.0F},
                                    ImVec2{6.0F, 2.0F}, ImVec2{14.0F, 2.0F}, ImVec2{14.0F, 10.0F}}) {
                z.punkt(q.x, q.y, 1.5F);
            }
        } else if (ist(n, "modify_edges")) {
            const Stift dick{dl, pos, z.s, farbe, z.w * 2.4F};
            dick.linie(10.0F, 6.0F, 10.0F, 14.0F);
            dick.linie(10.0F, 6.0F, 14.0F, 2.0F);
        }
        return true;
    }
    // --- Ansichten ---
    if (ist(n, "view_change")) {   // vier Felder mit Wechselpfeil
        z.rahmen(1.5F, 1.5F, 14.5F, 14.5F, 0.8F);
        z.linie(8.0F, 1.5F, 8.0F, 14.5F);
        z.linie(1.5F, 8.0F, 14.5F, 8.0F);
        z.kasten(2.5F, 2.5F, 7.0F, 7.0F, schwach, 0.0F);
        z.bogenPfeil(8.0F, 8.0F, 3.6F, -0.9F * kPi, 0.1F * kPi);
        return true;
    }
    if (ist(n, "view_cubicclipping")) {   // Wuerfel, hinten abgeschnitten
        z.wuerfel(2.0F, 2.0F, 9.0F, 4.0F);
        z.gestrichelt(13.5F, 1.0F, 13.5F, 15.0F, 1.0F);
        return true;
    }
    // --- Manipulatoren ---
    if (ist(n, "select_mouseresize")) {   // Kasten mit gezogener Ecke
        z.strichRahmen(1.5F, 1.5F, 14.5F, 14.5F);
        z.kasten(1.5F, 8.5F, 7.5F, 14.5F, schwach, 0.6F);
        z.rahmen(1.5F, 8.5F, 7.5F, 14.5F, 0.6F);
        z.linie(8.5F, 7.5F, 13.0F, 3.0F);
        z.spitze(13.0F, 3.0F, 1.0F, -1.0F);
        return true;
    }
    if (ist(n, "select_clipper")) {   // Schnittlinie durch zwei Punkte, ein Teil faellt weg
        z.flaeche({ImVec2{1.5F, 14.5F}, ImVec2{1.5F, 6.0F}, ImVec2{10.0F, 14.5F}}, schwach);
        z.rahmen(1.5F, 1.5F, 14.5F, 14.5F, 0.6F);
        z.linie(0.5F, 5.0F, 11.0F, 15.5F);
        z.punkt(3.0F, 7.5F, 1.6F);
        z.punkt(8.5F, 13.0F, 1.6F);
        return true;
    }
    if (ist(n, "select_mousetranslate")) {   // Kreuz aus vier Pfeilen
        z.linie(8.0F, 1.5F, 8.0F, 14.5F);
        z.linie(1.5F, 8.0F, 14.5F, 8.0F);
        z.spitze(8.0F, 1.5F, 0.0F, -1.0F);
        z.spitze(8.0F, 14.5F, 0.0F, 1.0F);
        z.spitze(1.5F, 8.0F, -1.0F, 0.0F);
        z.spitze(14.5F, 8.0F, 1.0F, 0.0F);
        return true;
    }
    if (ist(n, "select_mouserotate")) {   // Kreispfeil um einen Mittelpunkt
        z.punkt(8.0F, 8.0F, 1.5F);
        z.bogenPfeil(8.0F, 8.0F, 5.8F, -0.6F * kPi, 1.1F * kPi);
        return true;
    }
    if (ist(n, "select_mousescale")) {   // kleiner Kasten, grosser Kasten, Pfeil nach aussen
        z.kasten(1.5F, 8.5F, 7.5F, 14.5F, schwach, 0.6F);
        z.rahmen(1.5F, 8.5F, 7.5F, 14.5F, 0.6F);
        z.strichRahmen(1.5F, 1.5F, 14.5F, 14.5F);
        z.linie(7.5F, 8.5F, 12.5F, 3.5F);
        z.spitze(12.5F, 3.5F, 1.0F, -1.0F);
        return true;
    }
    if (ist(n, "select_mousetransform")) {   // Pfeile und Drehbogen zusammen
        z.linie(8.0F, 3.5F, 8.0F, 12.5F);
        z.linie(3.5F, 8.0F, 12.5F, 8.0F);
        z.spitze(8.0F, 3.5F, 0.0F, -1.0F, 1.8F);
        z.spitze(12.5F, 8.0F, 1.0F, 0.0F, 1.8F);
        z.bogenPfeil(8.0F, 8.0F, 6.5F, 0.6F * kPi, 1.4F * kPi);
        return true;
    }
    if (ist(n, "select_mouseuv")) {   // Texturraster mit U- und V-Pfeil
        z.rahmen(4.5F, 1.5F, 14.5F, 11.5F, 0.4F);
        z.linie(9.5F, 1.5F, 9.5F, 11.5F);
        z.linie(4.5F, 6.5F, 14.5F, 6.5F);
        z.kasten(4.5F, 1.5F, 9.5F, 6.5F, schwach, 0.0F);
        z.kasten(9.5F, 6.5F, 14.5F, 11.5F, schwach, 0.0F);
        z.linie(1.5F, 14.5F, 8.0F, 14.5F);
        z.spitze(8.0F, 14.5F, 1.0F, 0.0F, 1.8F);
        z.linie(1.5F, 14.5F, 1.5F, 8.0F);
        z.spitze(1.5F, 8.0F, 0.0F, -1.0F, 1.8F);
        return true;
    }
    // --- Patches ---
    if (ist(n, "curve_cap") || ist(n, "f-patches") || ist(n, "status_patch")) {   // gebogene Flaeche
        z.bogen(8.0F, 15.0F, 6.5F, kPi, 2.0F * kPi);
        z.bogen(8.0F, 15.0F, 3.0F, kPi, 2.0F * kPi);
        z.linie(1.5F, 15.0F, 5.0F, 15.0F);
        z.linie(11.0F, 15.0F, 14.5F, 15.0F);
        z.linie(8.0F, 8.5F, 8.0F, 12.0F);
        if (ist(n, "curve_cap")) {
            z.flaeche({ImVec2{1.5F, 3.5F}, ImVec2{14.5F, 3.5F}, ImVec2{14.5F, 5.5F}, ImVec2{1.5F, 5.5F}}, schwach);
            z.rahmen(1.5F, 3.5F, 14.5F, 5.5F, 0.4F);
        }
        return true;
    }
    // --- Texture Lock ---
    if (ist(n, "texture_lock") || ist(n, "texture_vertexlock")) {
        z.kasten(3.0F, 7.0F, 13.0F, 14.5F, schwach, 1.0F);
        z.rahmen(3.0F, 7.0F, 13.0F, 14.5F, 1.0F);
        z.bogen(8.0F, 7.0F, 3.2F, kPi, 2.0F * kPi);
        z.linie(4.8F, 7.0F, 4.8F, 6.0F);
        z.linie(11.2F, 7.0F, 11.2F, 6.0F);
        if (ist(n, "texture_vertexlock")) {
            z.punkt(13.0F, 2.5F, 1.8F);
        } else {
            z.kasten(5.0F, 9.0F, 8.0F, 11.0F, farbe, 0.0F);
            z.kasten(8.0F, 11.0F, 11.0F, 13.0F, farbe, 0.0F);
        }
        return true;
    }
    // --- Fenster ---
    if (ist(n, "entities") || ist(n, "f-entities") || ist(n, "status_entity")) {   // Entity: Kasten mit Raute
        z.rahmen(2.0F, 2.0F, 14.0F, 14.0F, 1.0F);
        z.zug({ImVec2{8.0F, 4.5F}, ImVec2{11.5F, 8.0F}, ImVec2{8.0F, 11.5F}, ImVec2{4.5F, 8.0F}}, true);
        z.punkt(8.0F, 8.0F, 1.2F);
        return true;
    }
    if (ist(n, "console")) {   // Eingabezeile ">_"
        z.rahmen(1.5F, 2.5F, 14.5F, 13.5F, 1.0F);
        z.zug({ImVec2{4.0F, 6.0F}, ImVec2{6.5F, 8.0F}, ImVec2{4.0F, 10.0F}});
        z.linie(8.0F, 10.5F, 12.0F, 10.5F);
        return true;
    }
    if (ist(n, "texture_browser")) {   // Kacheln
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                const float x0 = 1.5F + static_cast<float>(x) * 7.0F;
                const float y0 = 1.5F + static_cast<float>(y) * 7.0F;
                if ((x + y) % 2 == 0) { z.kasten(x0, y0, x0 + 6.0F, y0 + 6.0F, schwach, 0.6F); }
                z.rahmen(x0, y0, x0 + 6.0F, y0 + 6.0F, 0.6F);
            }
        }
        return true;
    }
    if (ist(n, "refresh_models") || ist(n, "texbro_refresh")) {   // Kreispfeile um einen Kasten
        z.bogenPfeil(8.0F, 8.0F, 6.0F, 0.15F * kPi, 0.95F * kPi);
        z.bogenPfeil(8.0F, 8.0F, 6.0F, 1.15F * kPi, 1.95F * kPi);
        if (ist(n, "refresh_models")) { z.rahmen(5.5F, 5.5F, 10.5F, 10.5F, 0.4F); }
        return true;
    }
    if (ist(n, "texbro_view")) { uiIcon(dl, UiIcon::Sichtbar, pos, groesse, farbe); return true; }
    // Filterfeld des Texturbrowsers (TextureBrowser_filterSetModeIcon):
    // search.png (irgendwo) / search_from_start.png (vom Anfang: Lupe mit Strich)
    if (ist(n, "search")) { uiIcon(dl, UiIcon::Suche, pos, groesse, farbe); return true; }
    if (ist(n, "search_from_start")) {
        uiIcon(dl, UiIcon::Suche, ImVec2{pos.x + groesse * 0.12F, pos.y}, groesse * 0.88F, farbe);
        z.linie(1.0F, 2.0F, 1.0F, 14.0F);
        return true;
    }
    if (ist(n, "copy")) { uiIcon(dl, UiIcon::Duplizieren, pos, groesse, farbe); return true; }   // Kontextmenue: Name kopieren
    // --- Entity Inspector (entityinspector.cpp: select.png, arrow_*.png) ---
    if (ist(n, "select")) {   // Select by key/value: gestrichelter Rahmen mit Zeiger
        z.strichRahmen(1.5F, 1.5F, 11.5F, 11.5F);
        z.flaeche({ImVec2{7.0F, 6.5F}, ImVec2{14.5F, 10.5F}, ImVec2{11.0F, 11.5F}, ImVec2{9.5F, 14.5F}}, farbe);
        return true;
    }
    if (ist(n, "arrow_left") || ist(n, "arrow_right")) {   // Select targeting entities / Select targets
        const bool links = ist(n, "arrow_left");
        z.linie(2.0F, 8.0F, 14.0F, 8.0F);
        if (links) { z.spitze(2.0F, 8.0F, -1.0F, 0.0F, 4.5F); } else { z.spitze(14.0F, 8.0F, 1.0F, 0.0F, 4.5F); }
        return true;
    }
    if (ist(n, "arrow_left_right")) {   // Select connected entities (32 x 16, mittig im Quadrat)
        const float m = 8.0F;   // Mitte des Quadrats
        z.linie(2.0F, m, 14.0F, m);
        z.spitze(2.0F, m, -1.0F, 0.0F, 2.3F);
        z.spitze(14.0F, m, 1.0F, 0.0F, 2.3F);
        return true;
    }
    if (ist(n, "eyes")) {   // AutoFocus on Selection ("ð")
        for (const float x : {5.0F, 11.0F}) {
            z.dl->AddEllipseFilled(z.p(x, 8.0F), ImVec2{2.6F * z.s, 3.6F * z.s}, IM_COL32(245, 245, 245, 255));
            z.dl->AddEllipse(z.p(x, 8.0F), ImVec2{2.6F * z.s, 3.6F * z.s}, mitAlpha(farbe, 0.6F), 0.0F, 0, z.w * 0.6F);
            z.punkt(x + 0.6F, 8.8F, 1.4F, IM_COL32(20, 20, 20, 255));
        }
        return true;
    }
    if (ist(n, "texbro_find-replace")) {   // Lupe mit Wechselpfeil
        z.kreis(6.5F, 6.5F, 4.5F);
        z.linie(9.8F, 9.8F, 14.5F, 14.5F);
        z.bogenPfeil(6.5F, 6.5F, 2.2F, -0.9F * kPi, 0.4F * kPi);
        return true;
    }
    // --- Filter ---
    if (ist(n, "f-world")) {   // Weltkugel
        z.kreis(8.0F, 8.0F, 6.5F);
        z.linie(1.5F, 8.0F, 14.5F, 8.0F);
        z.dl->AddEllipse(z.p(8.0F, 8.0F), ImVec2{3.0F * z.s, 6.5F * z.s}, z.c, 0.0F, 0, z.w);
        return true;
    }
    if (ist(n, "f-structural")) {   // fester Block: alle drei Seiten gefuellt
        z.flaeche({ImVec2{2.0F, 6.0F}, ImVec2{10.0F, 6.0F}, ImVec2{10.0F, 14.0F}, ImVec2{2.0F, 14.0F}}, mitAlpha(farbe, 0.55F));
        z.flaeche({ImVec2{2.0F, 6.0F}, ImVec2{6.0F, 2.0F}, ImVec2{14.0F, 2.0F}, ImVec2{10.0F, 6.0F}}, mitAlpha(farbe, 0.8F));
        z.flaeche({ImVec2{10.0F, 6.0F}, ImVec2{14.0F, 2.0F}, ImVec2{14.0F, 10.0F}, ImVec2{10.0F, 14.0F}}, schwach);
        z.wuerfel(2.0F, 2.0F, 8.0F, 4.0F);
        return true;
    }
    if (ist(n, "f-details")) {   // kleine Teile auf einem Block
        z.rahmen(1.5F, 9.5F, 14.5F, 14.5F, 0.4F);
        z.kasten(3.0F, 4.0F, 6.0F, 9.5F, schwach, 0.4F);
        z.rahmen(3.0F, 4.0F, 6.0F, 9.5F, 0.4F);
        z.kasten(9.0F, 2.0F, 13.0F, 9.5F, schwach, 0.4F);
        z.rahmen(9.0F, 2.0F, 13.0F, 9.5F, 0.4F);
        return true;
    }
    if (ist(n, "f-funcgroups")) {   // Bloecke in einer Klammer
        z.rahmen(4.0F, 4.0F, 8.0F, 8.0F, 0.4F);
        z.rahmen(8.5F, 8.5F, 12.0F, 12.0F, 0.4F);
        z.zug({ImVec2{3.0F, 1.5F}, ImVec2{1.5F, 1.5F}, ImVec2{1.5F, 14.5F}, ImVec2{3.0F, 14.5F}});
        z.zug({ImVec2{13.0F, 1.5F}, ImVec2{14.5F, 1.5F}, ImVec2{14.5F, 14.5F}, ImVec2{13.0F, 14.5F}});
        return true;
    }
    if (ist(n, "f-translucent")) {   // durchscheinender Kasten vor einem anderen
        z.rahmen(1.5F, 1.5F, 10.5F, 10.5F, 0.6F);
        z.kasten(5.5F, 5.5F, 14.5F, 14.5F, mitAlpha(farbe, 0.25F), 0.6F);
        z.strichRahmen(5.5F, 5.5F, 14.5F, 14.5F);
        return true;
    }
    if (ist(n, "f-liquids")) {   // Wellen
        for (int k = 0; k < 3; ++k) {
            const float y = 4.0F + static_cast<float>(k) * 4.0F;
            z.zug({ImVec2{1.5F, y}, ImVec2{4.0F, y - 1.5F}, ImVec2{6.5F, y}, ImVec2{9.0F, y + 1.5F}, ImVec2{11.5F, y},
                   ImVec2{14.5F, y - 1.5F}});
        }
        return true;
    }
    if (ist(n, "f-caulk")) {   // Kasten, schraffiert
        z.rahmen(1.5F, 1.5F, 14.5F, 14.5F, 0.6F);
        for (float k = 4.0F; k < 26.0F; k += 4.0F) {
            const float x0 = std::max(1.5F, k - 13.0F);
            const float y0 = std::min(14.5F, k);
            const float x1 = std::min(14.5F, k);
            const float y1 = std::max(1.5F, k - 13.0F);
            if (x1 > x0) { z.linie(x0, y0, x1, y1); }
        }
        return true;
    }
    if (ist(n, "f-clip")) {   // Kasten mit Kreuz (unsichtbare Wand)
        z.strichRahmen(1.5F, 1.5F, 14.5F, 14.5F);
        z.linie(4.0F, 4.0F, 12.0F, 12.0F);
        z.linie(12.0F, 4.0F, 4.0F, 12.0F);
        return true;
    }
    if (ist(n, "f-hint")) {   // schraege Ebene mit Pfeil
        z.zug({ImVec2{2.0F, 12.0F}, ImVec2{6.0F, 3.0F}, ImVec2{14.0F, 3.0F}, ImVec2{10.0F, 12.0F}}, true);
        z.linie(8.0F, 7.5F, 8.0F, 14.5F);
        z.spitze(8.0F, 14.5F, 0.0F, 1.0F, 1.8F);
        return true;
    }
    if (ist(n, "f-sky")) { uiIcon(dl, UiIcon::Himmel, pos, groesse, farbe); return true; }
    if (ist(n, "f-pointentities")) {   // Punkt mit kleinem Kasten
        z.rahmen(4.5F, 4.5F, 11.5F, 11.5F, 0.6F);
        z.punkt(8.0F, 8.0F, 1.6F);
        z.linie(8.0F, 1.0F, 8.0F, 3.5F);
        z.linie(8.0F, 12.5F, 8.0F, 15.0F);
        z.linie(1.0F, 8.0F, 3.5F, 8.0F);
        z.linie(12.5F, 8.0F, 15.0F, 8.0F);
        return true;
    }
    if (ist(n, "f-lights")) { uiIcon(dl, UiIcon::Licht, pos, groesse, farbe); return true; }
    if (ist(n, "f-models")) { uiIcon(dl, UiIcon::Figur, pos, groesse, farbe); return true; }
    if (ist(n, "f-triggers")) {   // Ausloeser: Blitz in einem gestrichelten Kasten
        z.strichRahmen(1.5F, 1.5F, 14.5F, 14.5F);
        z.zug({ImVec2{9.0F, 3.0F}, ImVec2{5.0F, 9.0F}, ImVec2{8.0F, 9.0F}, ImVec2{7.0F, 13.0F}, ImVec2{11.0F, 7.0F},
               ImVec2{8.0F, 7.0F}}, true);
        return true;
    }
    if (ist(n, "f-reset")) {   // Filter zuruecksetzen: Trichter mit Kreispfeil
        z.zug({ImVec2{1.5F, 2.5F}, ImVec2{10.5F, 2.5F}, ImVec2{7.0F, 7.5F}, ImVec2{7.0F, 12.5F}, ImVec2{5.0F, 13.5F},
               ImVec2{5.0F, 7.5F}}, true);
        z.bogenPfeil(11.5F, 11.0F, 3.0F, -0.5F * kPi, 1.0F * kPi);
        return true;
    }
    if (ist(n, "f-region")) {   // Region: gestrichelter Rahmen mit Ecken
        z.strichRahmen(3.0F, 3.0F, 13.0F, 13.0F);
        z.zug({ImVec2{1.5F, 5.0F}, ImVec2{1.5F, 1.5F}, ImVec2{5.0F, 1.5F}});
        z.zug({ImVec2{11.0F, 1.5F}, ImVec2{14.5F, 1.5F}, ImVec2{14.5F, 5.0F}});
        z.zug({ImVec2{14.5F, 11.0F}, ImVec2{14.5F, 14.5F}, ImVec2{11.0F, 14.5F}});
        z.zug({ImVec2{5.0F, 14.5F}, ImVec2{1.5F, 14.5F}, ImVec2{1.5F, 11.0F}});
        return true;
    }
    if (ist(n, "f-hide")) { uiIcon(dl, UiIcon::Versteckt, pos, groesse, farbe); return true; }
    if (ist(n, "status_brush")) {
        z.wuerfel(2.0F, 2.0F, 8.0F, 4.0F);
        return true;
    }
    // --- Plugin-Leiste (bobToolz) ---
    if (ist(n, "bobtoolz_poly")) {   // Vieleck-Saeule (Zylinder)
        z.dl->AddEllipse(z.p(8.0F, 3.5F), ImVec2{5.5F * z.s, 2.0F * z.s}, z.c, 0.0F, 0, z.w);
        z.linie(2.5F, 3.5F, 2.5F, 12.5F);
        z.linie(13.5F, 3.5F, 13.5F, 12.5F);
        z.bogen(8.0F, 12.5F, 5.5F, 0.0F, kPi);
        z.flaeche({ImVec2{2.5F, 4.0F}, ImVec2{13.5F, 4.0F}, ImVec2{13.5F, 12.5F}, ImVec2{2.5F, 12.5F}}, schwach);
        return true;
    }
    if (ist(n, "bobtoolz_treeplanter")) {   // Baum
        z.flaeche({ImVec2{8.0F, 1.5F}, ImVec2{13.5F, 10.0F}, ImVec2{2.5F, 10.0F}}, schwach);
        z.zug({ImVec2{8.0F, 1.5F}, ImVec2{13.5F, 10.0F}, ImVec2{2.5F, 10.0F}}, true);
        z.linie(8.0F, 10.0F, 8.0F, 14.5F);
        z.linie(5.0F, 14.5F, 11.0F, 14.5F);
        return true;
    }
    if (ist(n, "bobtoolz_trainpathplot")) {   // Bahn durch Punkte
        z.zug({ImVec2{1.5F, 13.0F}, ImVec2{4.0F, 7.0F}, ImVec2{8.0F, 9.0F}, ImVec2{11.0F, 4.0F}, ImVec2{14.5F, 3.0F}});
        z.punkt(1.5F, 13.0F, 1.4F);
        z.punkt(8.0F, 9.0F, 1.4F);
        z.punkt(14.5F, 3.0F, 1.4F);
        return true;
    }
    if (ist(n, "bobtoolz_dropent")) {   // Entity faellt auf den Boden
        z.rahmen(5.0F, 1.5F, 11.0F, 7.5F, 0.6F);
        z.linie(8.0F, 8.5F, 8.0F, 12.0F);
        z.spitze(8.0F, 12.0F, 0.0F, 1.0F, 1.8F);
        z.linie(1.5F, 14.5F, 14.5F, 14.5F);
        return true;
    }
    if (ist(n, "bobtoolz_merge")) {   // zwei Patches werden einer
        z.bogen(5.0F, 12.0F, 3.5F, kPi, 2.0F * kPi);
        z.bogen(11.0F, 12.0F, 3.5F, kPi, 2.0F * kPi);
        z.linie(5.0F, 4.5F, 8.0F, 7.0F);
        z.linie(11.0F, 4.5F, 8.0F, 7.0F);
        z.linie(8.0F, 1.5F, 8.0F, 7.0F);
        return true;
    }
    if (ist(n, "bobtoolz_split") || ist(n, "bobtoolz_splitrow") || ist(n, "bobtoolz_splitcol")) {   // Patch-Gitter geteilt
        z.rahmen(1.5F, 3.5F, 14.5F, 12.5F, 0.6F);
        if (!ist(n, "bobtoolz_splitrow")) { z.gestrichelt(8.0F, 1.0F, 8.0F, 15.0F, 1.2F); }
        if (!ist(n, "bobtoolz_splitcol")) { z.gestrichelt(0.5F, 8.0F, 15.5F, 8.0F, 1.2F); }
        if (ist(n, "bobtoolz_split")) { z.kasten(1.5F, 3.5F, 8.0F, 8.0F, schwach, 0.0F); }
        if (ist(n, "bobtoolz_splitrow")) { z.kasten(1.5F, 3.5F, 14.5F, 8.0F, schwach, 0.0F); }
        if (ist(n, "bobtoolz_splitcol")) { z.kasten(1.5F, 3.5F, 8.0F, 12.5F, schwach, 0.0F); }
        return true;
    }
    if (ist(n, "bobtoolz_turnedge")) {   // Viereck, Diagonale gewendet
        z.zug({ImVec2{2.0F, 8.0F}, ImVec2{8.0F, 2.0F}, ImVec2{14.0F, 8.0F}, ImVec2{8.0F, 14.0F}}, true);
        z.gestrichelt(2.0F, 8.0F, 14.0F, 8.0F, 1.2F);
        z.linie(8.0F, 2.0F, 8.0F, 14.0F);
        z.bogenPfeil(8.0F, 8.0F, 3.0F, -0.9F * kPi, -0.4F * kPi);
        return true;
    }
    // --- Fensteranordnungen (Layout-Seite der Einstellungen) ---
    if (std::strncmp(n, "window", 6) == 0) {
        z.rahmen(0.8F, 1.5F, 15.2F, 14.5F, 0.6F);
        const char art = n[6];
        if (art == '1' || art == '4') {   // Regular / Regular Left: XY+Konsole | Kamera+Texturen
            const float x = 8.0F;
            z.linie(x, 1.5F, x, 14.5F);
            const float links0 = art == '1' ? 0.8F : x;
            const float links1 = art == '1' ? x : 15.2F;
            const float rechts0 = art == '1' ? x : 0.8F;
            const float rechts1 = art == '1' ? 15.2F : x;
            z.linie(links0, 12.0F, links1, 12.0F);
            z.linie(rechts0, 9.0F, rechts1, 9.0F);
            z.kasten(rechts0 + 0.6F, 2.1F, rechts1 - 0.6F, 8.4F, schwach, 0.0F);
        } else if (art == '3') {   // Split: vier Felder
            z.linie(8.0F, 1.5F, 8.0F, 14.5F);
            z.linie(0.8F, 8.0F, 15.2F, 8.0F);
            z.kasten(1.4F, 2.1F, 7.4F, 7.4F, schwach, 0.0F);
        } else {   // Floating: einzelne Fenster
            z.rahmen(2.5F, 3.5F, 9.0F, 8.5F, 0.4F);
            z.kasten(7.0F, 7.0F, 13.5F, 12.5F, schwach, 0.4F);
            z.rahmen(7.0F, 7.0F, 13.5F, 12.5F, 0.4F);
        }
        return true;
    }
    return false;
}

}  // namespace bhed::gui::karteneditor
