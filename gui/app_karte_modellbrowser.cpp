// app_karte_modellbrowser.cpp - Karten-Editor: der Modell-Browser (Seite
// Models des Gruppendialogs) und die Einstellungsseite "Model Browser".
//
// Portiert aus NetRadiant-Custom 20260114: radiant/modelwindow.cpp:527-668
// (ModelBrowser: m_originZ, setOriginZ, validate, updateScroll, testSelect,
// trackingDelta: rechte Taste rollt (Umschalt x 4), linke dreht das aktuelle
// Modell), 758-952 (ModelBrowser_render: Hintergrund 0.25, hellere Quadrate
// +0.05, ein Licht aus Blickrichtung mit Umgebung und Streuung 0.4, die
// Modelle mit ihren Shadern, die Dateinamen), 955-1078 (ModelBrowserGLWidget:
// Geraetepunkte, Doppelklick "insertModel" mit misc_model vor der Kamera,
// Loslassen links ohne Weg < 16 "entityAssignModel", rechts ohne Weg die Lage
// neu, Mausrad 64), 1080-1123 (TreeView_onRowActivated), 1216-1252
// (ModelBrowser_constructTree), 1254-1324 (ModelBrowser_constructWindow:
// QSplitter { 100, 500 }, links QToolBar "Reload Model Folders Tree View"
// und QTreeView ohne Kopf, rechts die Ansicht und QScrollBar), 1345-1386
// (Einstellungen "Model View Size" 16..8192, "List of *folderToLoad/depth*",
// ModelBrowserFolders, ModelBrowserCellSize, ColorModBroBackground).
//
// Die Ordner listen die Endungen, die der Editor laden kann (md3, ase, glm);
// NRC listet alle Modelltypen seiner Lader (dazu die von assimp).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "karte_gpu.h"

#include "bhed/karte/eklasse.h"
#include "bhed/karte/modellbrowser.h"
#include "bhed/karte/raster.h"

#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <map>
#include <set>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

constexpr int kLeisteH = 25;       // QToolBar: Knopf 21 + 2 x (Rand 1 + Polster 1)
constexpr int kKnopf = 21;         // PM_SmallIconSize 16 + Rahmen
constexpr int kZeile = 16;         // QTreeView: Segoe UI 9 pt
constexpr int kEinzug = 20;        // PM_TreeViewIndentation
constexpr int kRollbalken = 7;     // dark.qss QScrollBar
constexpr int kGpuKennung = 300;   // Renderziel der Ansicht

struct Zustand {
    karte::ModellOrdner baum;                     // m_modelFS
    bool baumAlt = true;                          // ModelBrowser_constructTree
    const void* baumSpiel = nullptr;
    std::set<std::string> offen;                  // aufgeklappte Ordner (Pfad mit '/')
    std::string zeile;                            // gewaehlte Zeile (Pfad)
    std::string ordnerPfad;                       // m_currentFolderPath
    std::vector<std::string> dateien;             // m_currentFolder->m_files
    bool ordnerDa = false;                        // m_currentFolder != nullptr
    std::vector<karte::Matrix4> lagen;            // localToParent je Modell
    std::vector<karte::Kasten> grenzen;           // localAABB je Modell
    int ursprungZ = 0;                            // m_originZ (<= 0)
    bool ursprungUngueltig = true;                // m_originInvalid
    int aktuell = -1;                             // m_currentModelId
    int breite = 0;                               // m_width (Geraetepunkte)
    int hoehe = 0;
    // Maus (FreezePointer, trackingDelta)
    bool zug = false;
    int taste = -1;
    int weg = 0;                                  // m_move_amount
    float letzteX = 0;
    float letzteY = 0;
};
Zustand& z() {
    static Zustand s;
    return s;
}

std::string bereinigt(std::string s) {
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

int schriftHoehe() { return static_cast<int>(std::lround(ImGui::GetFontSize())); }   // GlobalOpenGL().m_font->getPixelHeight()
int schriftUnterlaenge() { return static_cast<int>(std::lround(ImGui::GetFontSize() * 0.2F)); }   // getPixelDescent

karte::ModellZellen zellen() {   // constructCellPos
    return karte::ModellZellen(z().breite, e().modBroE.zelle, schriftHoehe(), schriftUnterlaenge());
}

void lagenSetzen() {   // forEachModelInstance( models_set_transforms() )
    Zustand& s = z();
    const karte::ModellZellen zz = zellen();
    s.lagen.assign(s.dateien.size(), karte::kEinheitsMatrix);
    for (std::size_t i = 0; i < s.dateien.size(); ++i) {
        if (i < s.grenzen.size() && karte::kastenGueltig(s.grenzen[i])) { s.lagen[i] = karte::modellInZelle(s.grenzen[i], zz, static_cast<int>(i)); }
    }
}

int gesamtHoehe() { return zellen().gesamtHoehe(z().hoehe, static_cast<int>(z().dateien.size())); }

void pruefen() {   // validate
    Zustand& s = z();
    if (s.ursprungUngueltig) {
        s.ursprungUngueltig = false;
        const int tiefste = std::min(s.hoehe - gesamtHoehe(), 0);
        s.ursprungZ = std::max(tiefste, std::min(s.ursprungZ, 0));
    }
}

void ursprungSetzen(int z0) {   // setOriginZ
    z().ursprungZ = z0;
    z().ursprungUngueltig = true;
    pruefen();
}

// Die Ansicht des Modells i (ModelNode::setModel: ohne Skin und _remap)
karte::ModellAnsicht modellAnsicht(std::size_t i) { return kartenModellPfad(z().ordnerPfad + z().dateien[i]); }

// TreeView_onRowActivated: die Modelle des Ordners laden
void ordnerAktivieren(const std::string& pfad, const karte::ModellOrdner& o) {
    Zustand& s = z();
    s.ordnerDa = true;
    s.ordnerPfad = pfad;
    s.dateien.assign(o.dateien.begin(), o.dateien.end());
    s.grenzen.clear();
    s.aktuell = -1;
    s.ursprungZ = 0;
    s.ursprungUngueltig = true;
    for (std::size_t i = 0; i < s.dateien.size(); ++i) { s.grenzen.push_back(modellAnsicht(i).modell->grenzen); }
    lagenSetzen();
}

// Die Zeilen des Baums: (Pfad mit '/', Name, Tiefe, Ordner)
struct Zeile {
    std::string pfad;
    std::string name;
    int tiefe;
    const karte::ModellOrdner* ordner;
};

void zeilenSammeln(const karte::ModellOrdner& o, const std::string& vorn, int tiefe, std::vector<Zeile>& v) {
    for (const auto& [name, kind] : o.ordner) {   // ModelBrowser_constructTreeModel
        const std::string pfad = vorn + name + "/";
        v.push_back(Zeile{pfad, name, tiefe, &kind});
        if (z().offen.count(pfad) != 0U) { zeilenSammeln(kind, pfad, tiefe + 1, v); }
    }
}

std::vector<Zeile> zeilen() {
    std::vector<Zeile> v;
    const karte::ModellOrdner& w = z().baum;
    if (!w.dateien.empty()) { v.push_back(Zeile{"", "", 0, &w}); }   // models in the root: add blank item for access
    zeilenSammeln(w, "", 0, v);
    return v;
}

// mouseDoubleClick: misc_model vor der Kamera ("insertModel")
void modellEinfuegen() {
    Editor& ed = e();
    Zustand& s = z();
    if (!s.ordnerDa || s.aktuell < 0) { return; }
    karte::Karte& k = ed.karte;
    k.undo.start();
    auto ent = k.neueEntitaet("misc_model", false);   // findOrInsert( "misc_model", false )
    karte::Vektor3f p = ed.kamera.ursprung - ed.kamera.vpn * 128.0F;   // Camera_getOrigin - Camera_getViewVector * 128
    karte::einrasten(p, ed.raster.einrastGroesse());
    ent->setzen("origin", karte::zahlText(p[0]) + ' ' + karte::zahlText(p[1]) + ' ' + karte::zahlText(p[2]));
    k.entitaetEinfuegen(ent);
    k.auswahl.leeren();
    k.auswahl.waehlen(ent);
    const karte::EntitaetsKlasse* kl = ent->klasse();
    ent->setzen(kl != nullptr ? kl->miscmodelKey : std::string("model"), s.ordnerPfad + s.dateien[static_cast<std::size_t>(s.aktuell)]);
    k.undo.finish("insertModel");
    auswahlGeaendert();
}

// mouseReleaseEvent links ohne Weg: das Modell den gewaehlten Entitaeten ("entityAssignModel")
void modellZuweisen() {
    Editor& ed = e();
    Zustand& s = z();
    const std::string pfad = s.ordnerPfad + s.dateien[static_cast<std::size_t>(s.aktuell)];
    karte::Karte& k = ed.karte;
    k.undo.start();
    for (const auto& w : k.auswahl.elemente()) {   // foreachSelected: Node_getEntity( instance.path().top() )
        const auto ent = w.entitaet.lock();
        if (!ent || !w.primitiv.expired()) { continue; }
        const karte::EntitaetsKlasse* kl = ent->klasse();
        ent->setzen(kl != nullptr ? kl->miscmodelKey : std::string("model"), pfad);
    }
    k.undo.finish("entityAssignModel");
}

// ModelBrowser_render
void ansichtZeichnen(ImVec2 a, ImVec2 c) {
    Editor& ed = e();
    Zustand& s = z();
    const int w = std::max(1, static_cast<int>(std::lround(c.x - a.x)));
    const int h = std::max(1, static_cast<int>(std::lround(c.y - a.y)));
    if (w != s.breite || h != s.hoehe) {   // resizeGL
        s.breite = w;
        s.hoehe = h;
        s.ursprungUngueltig = true;
        lagenSetzen();
    }
    pruefen();
    const auto& bg = ed.modBroE.hintergrund;
    const float hinten[4] = {bg[0], bg[1], bg[2], 0.0F};
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // m_projection, m_modelview (XZ-Ebene, y = Tiefe)
    karte::Matrix4 proj = karte::kEinheitsMatrix;
    proj[0] = 1.0F / (static_cast<float>(w) / 2.0F);
    proj[5] = 1.0F / (static_cast<float>(h) / 2.0F);
    proj[10] = 1.0F / 9999.0F;
    proj[14] = -1;
    karte::Matrix4 mv = karte::kEinheitsMatrix;
    mv[12] = -static_cast<float>(w) / 2.0F;
    mv[13] = static_cast<float>(h) / 2.0F - static_cast<float>(s.ursprungZ);
    mv[14] = 9999;
    mv[0] = 1;
    mv[1] = 0;
    mv[2] = 0;
    mv[4] = 0;
    mv[5] = 0;
    mv[6] = 1;
    mv[8] = 0;
    mv[9] = 1;
    mv[10] = 0;
    const karte::Matrix4 mvp = karte::multipliziert(proj, mv);
    if (kartegpu::verfuegbar() && kartegpu::beginnen(kGpuKennung, w, h, 8, hinten, true)) {
        if (s.ordnerDa) {
            const karte::ModellZellen zz = zellen();
            {   // brighter background squares
                std::vector<kartegpu::Punkt3> q;
                const auto farbe = karte::farbeAus(bg[0] + .05F, bg[1] + .05F, bg[2] + .05F);
                for (std::size_t i = 0; i < s.dateien.size(); ++i) {
                    const karte::Vektor3f o = zz.ursprung(static_cast<int>(i));
                    const float r = static_cast<float>(zz.zelle());
                    const float p[4][3] = {{o[0] - r, 0, o[2] + r}, {o[0] - r, 0, o[2] - r}, {o[0] + r, 0, o[2] - r}, {o[0] + r, 0, o[2] + r}};
                    for (const int j : {0, 1, 2, 0, 2, 3}) {
                        q.push_back(kartegpu::Punkt3{{p[j][0], p[j][1], p[j][2]}, {0, 1, 0}, {0, 0}, farbe});
                    }
                }
                kartegpu::Zustand3 zq;
                zq.matrix = &mvp[0];
                zq.tiefe = kartegpu::TiefenTest::Aus;
                zq.tiefeSchreiben = false;
                zq.wegschneiden = false;
                kartegpu::dreiecke(q, zq);
            }
            // die Modelle (ModelRenderer mit globalstate: Licht aus Blickrichtung)
            for (const bool durch : {false, true}) {
                for (std::size_t i = 0; i < s.dateien.size(); ++i) {
                    const karte::ModellAnsicht an = modellAnsicht(i);
                    if (!an.modell || i >= s.lagen.size()) { continue; }
                    for (std::size_t fi = 0; fi < an.modell->flaechen.size(); ++fi) {
                        const karte::ModellFlaeche& f = an.modell->flaechen[fi];
                        const std::string& sh = fi < an.shader.size() ? an.shader[fi] : f.shader;
                        if (sh.empty()) { continue; }
                        kartegpu::Zustand3 zf;
                        zf.matrix = &mvp[0];
                        zf.vpn[0] = 0;
                        zf.vpn[1] = 1;   // -m_view.getViewDir()
                        zf.vpn[2] = 0;
                        bool d = false;
                        if (!shaderZustand(sh, zf, d) || d != durch) { continue; }
                        std::vector<kartegpu::Punkt3> tri;
                        for (const std::uint32_t idx : f.indizes) {
                            const karte::ModellPunkt& p = f.punkte[idx];
                            const karte::Vektor3f o = karte::transformierterPunkt(s.lagen[i], p.ort);
                            karte::Vektor3f n = karte::transformierteRichtung(s.lagen[i], p.normale);
                            const float l = static_cast<float>(karte::laenge(n));
                            if (l > 0) { n = n * (1.0F / l); }
                            tri.push_back(kartegpu::Punkt3{{o[0], o[1], o[2]}, {n[0], n[1], n[2]}, {p.st[0], p.st[1]}, 0xFFFFFFFFU});
                        }
                        kartegpu::dreiecke(tri, zf);
                    }
                }
            }
        }
        kartegpu::beenden();
        dl->AddImage(reinterpret_cast<ImTextureID>(kartegpu::textur(kGpuKennung)), a, c);
    } else {
        dl->AddRectFilled(a, c, karte::farbeAus(bg[0], bg[1], bg[2]));
    }
    // render model file names (glRasterPos + drawString, nur sichtbare)
    if (s.ordnerDa) {
        const karte::ModellZellen zz = zellen();
        dl->PushClipRect(a, c, true);
        for (std::size_t i = 0; i < s.dateien.size(); ++i) {
            const karte::Vektor3f t = zz.textOrt(static_cast<int>(i));
            const float x = t[0];
            const float y = -t[2] + static_cast<float>(s.ursprungZ);   // Fensterpunkte von oben
            if (y < 0 || y > static_cast<float>(h)) { continue; }   // m_view.TestPoint
            dl->AddText(ImVec2{a.x + x, a.y + y - static_cast<float>(schriftHoehe())}, IM_COL32(255, 255, 255, 255), s.dateien[i].c_str());
        }
        dl->PopClipRect();
    }
}

void ansichtMaus(ImVec2 a, ImVec2 c) {
    Editor& ed = e();
    Zustand& s = z();
    ImGuiIO& io = ImGui::GetIO();
    ImGui::SetCursorScreenPos(a);
    ImGui::InvisibleButton("##modbroansicht", ImVec2{c.x - a.x, c.y - a.y}, ImGuiButtonFlags_MouseButtonLeft | ImGuiButtonFlags_MouseButtonRight);
    const bool ueber = ImGui::IsItemHovered();
    const float mx = io.MousePos.x - a.x;
    const float my = io.MousePos.y - a.y;
    if (ueber && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {   // MousePresses::Left2x
        modellEinfuegen();
        s.zug = false;
        return;
    }
    for (const int t : {0, 1}) {
        if (ueber && ImGui::IsMouseClicked(t)) {   // tracking_MouseDown
            s.zug = true;
            s.taste = t;
            s.weg = 0;
            s.letzteX = io.MousePos.x;
            s.letzteY = io.MousePos.y;
            if (t == 0) {   // testSelect( x, z - m_originZ )
                s.aktuell = zellen().treffer(static_cast<int>(mx), static_cast<int>(my) - s.ursprungZ);
                if (s.aktuell >= static_cast<int>(s.dateien.size())) { s.aktuell = -1; }
            }
        }
    }
    if (s.zug) {
        const int dx = static_cast<int>(std::lround(io.MousePos.x - s.letzteX));
        const int dy = static_cast<int>(std::lround(io.MousePos.y - s.letzteY));
        if (dx != 0 || dy != 0) {   // trackingDelta
            s.letzteX = io.MousePos.x;
            s.letzteY = io.MousePos.y;
            s.weg += std::abs(dx) + std::abs(dy);
            if (ImGui::IsMouseDown(1) && dy != 0) {   // scroll view
                const int skala = io.KeyShift ? 4 : 1;
                ursprungSetzen(s.ursprungZ + dy * skala);
            } else if (ImGui::IsMouseDown(0) && s.aktuell >= 0 && s.aktuell < static_cast<int>(s.lagen.size())) {   // rotate selected model
                s.lagen[static_cast<std::size_t>(s.aktuell)] = karte::modellDrehen(s.lagen[static_cast<std::size_t>(s.aktuell)], dx, dy, zellen(), s.aktuell);
            }
        }
        if (ImGui::IsMouseReleased(s.taste)) {   // mouseReleaseEvent
            s.zug = false;
            if (s.taste == 0 && s.weg < 16 && s.ordnerDa && s.aktuell >= 0) {
                modellZuweisen();
            } else if (s.taste == 1 && s.weg < 16 && s.ordnerDa) {
                lagenSetzen();
            }
        }
    }
    if (ueber && io.MouseWheel != 0.0F) {   // wheelEvent: std::copysign( 64, delta )
        ursprungSetzen(s.ursprungZ + static_cast<int>(std::copysign(64.0F, io.MouseWheel)));
    }
    (void)ed;
}

// QScrollBar: Bereich 0 .. totalHeight - height, Seite = height
void rollbalken(ImVec2 a, ImVec2 c) {
    Zustand& s = z();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRectFilled(a, c, ImGui::GetColorU32(ImGuiCol_ScrollbarBg));
    const float gesamt = static_cast<float>(gesamtHoehe());
    const float hoehe = c.y - a.y;
    if (gesamt <= static_cast<float>(s.hoehe) || hoehe <= 0) { return; }
    const float griffH = std::max(hoehe * static_cast<float>(s.hoehe) / gesamt, 8.0F);
    const float lauf = hoehe - griffH;
    const float max = gesamt - static_cast<float>(s.hoehe);
    const float y = a.y + lauf * (static_cast<float>(-s.ursprungZ) / max);
    ImGui::SetCursorScreenPos(ImVec2{a.x, y});
    ImGui::InvisibleButton("##modbrogriff", ImVec2{c.x - a.x, griffH});
    if (ImGui::IsItemActive() && ImGui::GetIO().MouseDelta.y != 0.0F) {
        ursprungSetzen(s.ursprungZ - static_cast<int>(std::lround(ImGui::GetIO().MouseDelta.y * max / lauf)));
    }
    dl->AddRectFilled(ImVec2{a.x, y}, ImVec2{c.x, y + griffH},
                      ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ScrollbarGrabActive
                                                               : ImGui::IsItemHovered() ? ImGuiCol_ScrollbarGrabHovered : ImGuiCol_ScrollbarGrab));
}

}  // namespace

void modellBrowserBaumBauen() {   // ModelBrowser_constructTree
    Editor& ed = e();
    Zustand& s = z();
    s.baum = ed.karteSpiel ? karte::modellBaum(ed.karteSpiel->fs, ed.modBroE.ordner, {"md3", "ase", "glm"}) : karte::ModellOrdner{};
    s.baumAlt = false;
    s.baumSpiel = ed.karteSpiel.get();
    s.ordnerDa = false;   // ModelGraph_clear
    s.dateien.clear();
    s.lagen.clear();
    s.grenzen.clear();
    s.aktuell = -1;
}

void modellBrowserLagenNeu() {   // CellSizeImport
    lagenSetzen();
    z().ursprungUngueltig = true;
}

std::string modellBrowserOrdner() { return z().ordnerDa ? z().ordnerPfad : std::string(); }
std::vector<std::string> modellBrowserDateien() { return z().dateien; }
int modellBrowserAktuell() { return z().aktuell; }

void modellBrowserZeichnen(int breite, int hoehe) {
    Editor& ed = e();
    Zustand& s = z();
    if (s.baumAlt || s.baumSpiel != ed.karteSpiel.get()) { modellBrowserBaumBauen(); }
    texturBilderAbholen();   // die Bilder der Modelle kommen auch ohne Kamera an
    const float sc = massstab();
    karte::Teiler& t = ed.modTeiler;
    if (t.kinder.size() != 2) {   // setStretchFactor( 0, 0 ) / ( 1, 1 ), addSplitter( { 100, 500 } )
        t = karte::Teiler{};
        karte::Teiler::Kind links;
        links.dehnfaktor = 0;
        karte::Teiler::Kind rechts;
        rechts.dehnfaktor = 1;
        t.kinder = {links, rechts};
        t.setzeGroessen({100, 500});
    }
    const ImVec2 o = ImGui::GetWindowPos();
    const auto bp = [&](int x, int y) {
        return ImVec2{o.x + std::round(static_cast<float>(x) * sc), o.y + std::round(static_cast<float>(y) * sc)};
    };
    const TeilerInhalt linksZeichnen = [&](int x, int y, int b, int h) {
        if (b <= 0 || h <= 0) { return; }
        merkeFeld("Modelle:Links", x, y, b, h);
        ImDrawList* dl = ImGui::GetWindowDrawList();
        dl->AddRectFilled(bp(x, y), bp(x + b, y + kLeisteH), ImGui::GetColorU32(ImGuiCol_MenuBarBg));
        {   // "Reload Model Folders Tree View" (refresh_modelstree.png)
            const ImVec2 k0 = bp(x + 2, y + 2);
            const ImVec2 k1 = bp(x + 2 + kKnopf, y + 2 + kKnopf);
            ImGui::SetCursorScreenPos(k0);
            const bool geklickt = ImGui::InvisibleButton("##modbroneu", ImVec2{k1.x - k0.x, k1.y - k0.y});
            if (ImGui::IsItemHovered()) {
                dl->AddRectFilled(k0, k1, ImGui::GetColorU32(ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered),
                                  ImGui::GetStyle().FrameRounding + 2.0F);
                ImGui::SetTooltip("%s", trK(Str::KeModReload));
            }
            const float g = std::round(16.0F * sc);
            (void)karteIcon(dl, "refresh_modelstree", ImVec2{std::floor(k0.x + (k1.x - k0.x - g) * 0.5F), std::floor(k0.y + (k1.y - k0.y - g) * 0.5F)}, g,
                            ImGui::GetColorU32(karteIconFarbe("refresh_modelstree")));
            if (geklickt) { modellBrowserBaumBauen(); }
        }
        // QTreeView ohne Kopf
        const ImVec2 ta = bp(x, y + kLeisteH);
        const ImVec2 tc = bp(x + b, y + h);
        ImGui::SetCursorScreenPos(ta);
        ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, std::round(static_cast<float>(kRollbalken) * sc));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{std::round(sc), std::round(sc)});
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
        ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
        ImGui::BeginChild("##modbrobaum", ImVec2{tc.x - ta.x, tc.y - ta.y}, ImGuiChildFlags_Borders, ImGuiWindowFlags_HorizontalScrollbar);
        ImDrawList* bl = ImGui::GetWindowDrawList();
        const float zh = std::round(static_cast<float>(kZeile) * sc);
        for (const Zeile& r : zeilen()) {
            const ImVec2 p = ImGui::GetCursorScreenPos();
            const float einzug = static_cast<float>(kEinzug * r.tiefe) * sc;
            const float textX = einzug + static_cast<float>(kEinzug + 3) * sc;
            ImGui::PushStyleColor(ImGuiCol_Header, ImGui::GetStyleColorVec4(ImGuiCol_HeaderActive));
            const bool klick = ImGui::Selectable(("##mb" + r.pfad).c_str(), s.zeile == r.pfad, ImGuiSelectableFlags_AllowDoubleClick,
                                                 ImVec2{std::max(ImGui::GetContentRegionAvail().x, textX + ImGui::CalcTextSize(r.name.c_str()).x + 4.0F * sc), zh});
            ImGui::PopStyleColor();
            const bool kinder = !r.ordner->ordner.empty() && !r.pfad.empty();
            if (klick) {
                s.zeile = r.pfad;
                const float mxx = ImGui::GetIO().MousePos.x - p.x;
                if (kinder && mxx >= einzug && mxx < einzug + static_cast<float>(kEinzug) * sc) {   // das Dreieck klappt auf/zu
                    if (s.offen.count(r.pfad) != 0U) {
                        s.offen.erase(r.pfad);
                    } else {
                        s.offen.insert(r.pfad);
                    }
                } else if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {   // activated
                    ordnerAktivieren(r.pfad, *r.ordner);
                }
            }
            if (kinder) {   // PE_IndicatorBranch
                ImGui::RenderArrow(bl, ImVec2{p.x + einzug + 5.0F * sc, p.y + (zh - ImGui::GetFontSize()) * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text),
                                   s.offen.count(r.pfad) != 0U ? ImGuiDir_Down : ImGuiDir_Right, 0.7F);
            }
            bl->AddText(ImVec2{p.x + textX, p.y + (zh - ImGui::GetFontSize()) * 0.5F}, ImGui::GetColorU32(ImGuiCol_Text), r.name.c_str());
        }
        // Return/Enter: activated
        if (ImGui::IsWindowFocused() && (ImGui::IsKeyPressed(ImGuiKey_Enter, false) || ImGui::IsKeyPressed(ImGuiKey_KeypadEnter, false))) {
            for (const Zeile& r : zeilen()) {
                if (r.pfad == s.zeile) { ordnerAktivieren(r.pfad, *r.ordner); }
            }
        }
        ImGui::EndChild();
        ImGui::PopStyleColor();
        ImGui::PopStyleVar(3);
    };
    const TeilerInhalt rechtsZeichnen = [&](int x, int y, int b, int h) {
        if (b <= 0 || h <= 0) { return; }
        merkeFeld("Modelle:Ansicht", x, y, std::max(0, b - kRollbalken), h);
        const ImVec2 a = bp(x, y);
        const ImVec2 c = bp(x + std::max(0, b - kRollbalken), y + h);
        ansichtZeichnen(a, c);
        ansichtMaus(a, c);
        rollbalken(bp(x + b - kRollbalken, y), bp(x + b, y + h));
    };
    teilerImFenster("modbro", t, false, breite, hoehe, {linksZeichnen, rechtsZeichnen});
}

}  // namespace bhed::gui::karteneditor
