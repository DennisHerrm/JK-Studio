// app_karte_ebenen.cpp - Karten-Editor: der Ebenen-Browser (Seite Layers
// des Gruppendialogs), das Menue "Move to layer" (Umschalt+Q), Expand
// Selection To Layers (Strg+Q) und die Texteingabe "Layer Name".
//
// Portiert aus NetRadiant-Custom 20260114: radiant/layerswindow.cpp:192-564
// (LayersBrowser, Column::name/current/select/visible, item_construct mit den
// Tooltips, LayersBrowser_constructTree, QTreeWidget_layers: Ziehen und
// Ablegen (InternalMove) und die Auswahlsymbole nach den Zusatztasten,
// layer_new mit QInputDialog::getText "Layer Name" / "Enter layer name:" /
// "UnnamedpLayer", context_menu, context_moveto_menu, LayersSetVisible,
// itemClicked, LayersBrowser_constructWindow: QTreeWidget ohne Kopf, Name
// gedehnt, abwechselnde Zeilenfarben, Symbole PM_SmallIconSize * 1.3 = 20,
// darunter rechtsbuendig New Layer / Show All Layers / Hide All Layers),
// Qt 5.15 qtreewidget.cpp (dropEvent: nicht auf sich selbst oder ein Kind,
// die Zeile nach dem Herausnehmen), qabstractitemview.cpp (position:
// Rand qBound( 2, hoehe / 5.5, 12 ), startDragDistance 10),
// qinputdialog.cpp (QVBoxLayout: Beschriftung, QLineEdit, OK/Cancel).
//
// Masse am Foto dlg_View_Layers_Browser_0.png (doppelte Groesse): Baum mit
// Rahmen 1 ueber die ganze Seite, Zeilen 22 hoch, Haken 14 bei 3, die Spalte
// current 28 breit, Name mit Einzug 20 und Text 3 dahinter, Auswahlsymbol 52
// und Auge 24 vor dem rechten Rand; unten 6 Abstand und drei Werkzeugknoepfe
// 21 x 21, Abstand 6, rechtsbuendig.
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
#include "platform.h"

#include "bhed/karte/ebenen.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <string>
#include <vector>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

constexpr float kZeileH = 22.0F;
constexpr float kEinzug = 20.0F;
constexpr float kAktuellB = 28.0F;
constexpr float kSymbol = 20.0F;
constexpr float kKnopf = 21.0F;

float g_sk = 1.0F;
float px(float l) { return std::round(l * g_sk); }

// Die Sichtbarkeit je Ebene im Browser (item->data( Column::visible )),
// neu bei jeder Karte (LayersBrowser_constructTree).
void sichtbarkeitAbgleichen() {
    Editor& ed = e();
    Editor::EbenenBrowser& b = ed.ebenenBro;
    if (b.stand != ed.ladeZaehler || b.sichtbar.size() != ed.karte.schichten.size()) {
        if (b.stand != ed.ladeZaehler) {
            b.sichtbar.assign(ed.karte.schichten.size(), 1);
            b.gewaehlt = -1;
        } else {
            b.sichtbar.resize(ed.karte.schichten.size(), 1);
        }
        b.stand = ed.ladeZaehler;
    }
}

// Was der Baum neben der Karte haelt, mit den neuen Nummern
void umnummern(const karte::EbenenNummern& n) {
    Editor::EbenenBrowser& b = e().ebenenBro;
    if (n.empty()) { return; }
    std::vector<char> neu(e().karte.schichten.size(), 1);
    for (std::size_t i = 0; i < n.size() && i < b.sichtbar.size(); ++i) {
        if (n[i] >= 0 && static_cast<std::size_t>(n[i]) < neu.size()) { neu[static_cast<std::size_t>(n[i])] = b.sichtbar[i]; }
    }
    b.sichtbar = std::move(neu);
    if (b.gewaehlt >= 0) { b.gewaehlt = static_cast<std::size_t>(b.gewaehlt) < n.size() ? n[static_cast<std::size_t>(b.gewaehlt)] : -1; }
}

void sichtGeaendert() {
    ++e().sichtStand;
    auswahlGeaendert();
}

// LayersSetVisible: die Ebenen (mit Unterebenen) zeigen oder verstecken
void sichtbarSetzen(const std::vector<int>& ebenen, bool sichtbar) {
    Editor& ed = e();
    for (const int i : ebenen) {
        if (i >= 0 && static_cast<std::size_t>(i) < ed.ebenenBro.sichtbar.size()) { ed.ebenenBro.sichtbar[static_cast<std::size_t>(i)] = sichtbar ? 1 : 0; }
    }
    karte::ebenenSichtbar(ed.karte, ebenen, sichtbar);
    sichtGeaendert();
}

void alleSichtbar(bool sichtbar) {   // layers_setVisible_all
    std::vector<int> alle;
    for (std::size_t i = 0; i < e().karte.schichten.size(); ++i) { alle.push_back(static_cast<int>(i)); }
    sichtbarSetzen(alle, sichtbar);
}

// layer_new: QInputDialog "Layer Name"
void neueEbeneFragen(int eltern) {
    qtEingabe(trK(Str::KeLayName), trK(Str::KeLayEnter), "UnnamedpLayer", [eltern](const std::string& text) {
        const std::string name = karte::ebenenName(text);
        if (name.empty()) { return; }
        int neu = -1;
        umnummern(karte::ebeneNeu(e().karte, eltern, name, &neu));
        e().ebenenBro.sichtbar.resize(e().karte.schichten.size(), 1);
        e().ebenenBro.sichtbar[static_cast<std::size_t>(neu)] = 1;
    });
}

// Die Zeilen in Baumreihenfolge (Tiefensuche = Reihenfolge der Karte)
struct Zeile {
    int ebene;
    int tiefe;
    bool hatKinder;
};

std::vector<Zeile> zeilen() {
    const karte::Karte& k = e().karte;
    std::vector<Zeile> z;
    for (std::size_t i = 0; i < k.schichten.size(); ++i) {
        const bool kinder = i + 1 < k.schichten.size() && k.schichten[i + 1].eltern == static_cast<int>(i);
        z.push_back(Zeile{static_cast<int>(i), karte::ebenenTiefe(k, static_cast<int>(i)), kinder});
    }
    return z;
}

// Wo abgelegt wird (QAbstractItemViewPrivate::position)
enum class Ablage { Ueber, Unter, Auf, Leer };

}  // namespace

void ebenenSeiteZeichnen(int breite, int hoehe) {
    Editor& ed = e();
    Editor::EbenenBrowser& b = ed.ebenenBro;
    sichtbarkeitAbgleichen();
    g_sk = massstab();
    ImGuiIO& io = ImGui::GetIO();
    const ImVec2 o = ImGui::GetCursorScreenPos();
    const float baumH = std::max(0.0F, static_cast<float>(hoehe) - 6.0F - kKnopf);
    const ImVec2 b0 = o;
    const ImVec2 b1{o.x + px(static_cast<float>(breite)), o.y + px(baumH)};
    ImDrawList* dl = ImGui::GetWindowDrawList();
    dl->AddRect(b0, b1, ImGui::GetColorU32(ImGuiCol_Border));
    // der Baum (QTreeWidget): Inhalt 1 innen, rollt senkrecht
    ImGui::SetCursorScreenPos(ImVec2{b0.x + px(1.0F), b0.y + px(1.0F)});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, px(7.0F));
    ImGui::BeginChild("##ebenenbaum", ImVec2{b1.x - b0.x - px(2.0F), b1.y - b0.y - px(2.0F)}, ImGuiChildFlags_None);
    ImDrawList* zl = ImGui::GetWindowDrawList();
    const float innenB = ImGui::GetContentRegionAvail().x / g_sk;
    const std::vector<Zeile> z = zeilen();
    const float zh = px(kZeileH);
    const ImVec2 start = ImGui::GetCursorScreenPos();
    const ImU32 text = ImGui::GetColorU32(ImGuiCol_Text);
    // die Auswahlsymbole nach den Zusatztasten (set_select_icons)
    const UiIcon waehlSymbol = io.KeyCtrl && !io.KeyAlt && !io.KeyShift   ? UiIcon::Plus
                               : io.KeyAlt && !io.KeyCtrl && !io.KeyShift ? UiIcon::ZoomMinus
                               : io.KeyShift && !io.KeyCtrl && !io.KeyAlt ? UiIcon::Vor
                                                                          : UiIcon::Haken;
    int ueberZeile = -1;
    for (std::size_t i = 0; i < z.size(); ++i) {
        const Zeile& r = z[i];
        const karte::Schicht& s = ed.karte.schichten[static_cast<std::size_t>(r.ebene)];
        const ImVec2 a{start.x, start.y + zh * static_cast<float>(i)};
        const ImVec2 c{start.x + px(innenB), a.y + zh};
        ImGui::SetCursorScreenPos(a);
        ImGui::InvisibleButton(("##ebene" + std::to_string(r.ebene)).c_str(), ImVec2{c.x - a.x, zh}, ImGuiButtonFlags_MouseButtonLeft);
        const bool hover = ImGui::IsItemHovered();
        if (hover) { ueberZeile = static_cast<int>(i); }
        if (i % 2 == 1) { zl->AddRectFilled(a, c, ImGui::GetColorU32(ImGuiCol_TableRowBgAlt)); }   // setAlternatingRowColors
        if (b.gewaehlt == r.ebene) { zl->AddRectFilled(a, c, ImGui::GetColorU32(ImGuiCol_Header)); }
        // Spalte current: Hintergrund in der Ebenenfarbe, Haken 14 bei 3
        if (s.farbe[0] != 0 || s.farbe[1] != 0 || s.farbe[2] != 0) {
            zl->AddRectFilled(a, ImVec2{a.x + px(kAktuellB), c.y}, IM_COL32(s.farbe[0], s.farbe[1], s.farbe[2], 255));
        }
        const ImVec2 h0{a.x + px(3.0F), a.y + std::floor((zh - px(14.0F)) * 0.5F)};
        zl->AddRectFilled(h0, ImVec2{h0.x + px(14.0F), h0.y + px(14.0F)}, ImGui::GetColorU32(ImGuiCol_FrameBg));
        zl->AddRect(h0, ImVec2{h0.x + px(14.0F), h0.y + px(14.0F)}, ImGui::GetColorU32(ImGuiCol_Border));
        if (ed.karte.aktuelleSchicht == static_cast<std::size_t>(r.ebene)) {
            uiIcon(zl, UiIcon::Haken, ImVec2{h0.x + px(1.0F), h0.y + px(1.0F)}, px(12.0F), text);
        }
        // Spalte name: Einzug (rootIsDecorated), Pfeil fuer Ebenen mit Kindern
        const float nx = kAktuellB + kEinzug * static_cast<float>(r.tiefe);
        if (r.hatKinder) {
            const float mx = a.x + px(nx + kEinzug * 0.5F);
            const float my = std::floor(a.y + zh * 0.5F);
            zl->AddTriangleFilled(ImVec2{mx - px(4.0F), my - px(2.0F)}, ImVec2{mx + px(4.0F), my - px(2.0F)}, ImVec2{mx, my + px(2.0F)}, text);
        }
        zl->AddText(ImVec2{a.x + px(nx + kEinzug + 3.0F), std::floor(a.y + (zh - ImGui::GetTextLineHeight()) * 0.5F)}, text, s.name.c_str());
        // Spalten select und visible
        const ImVec2 wahl{a.x + px(innenB - 52.0F), a.y + std::floor((zh - px(kSymbol)) * 0.5F)};
        const ImVec2 auge{a.x + px(innenB - 24.0F), wahl.y};
        uiIcon(zl, waehlSymbol, wahl, px(kSymbol), ImGui::GetColorU32(uiIconFarbe(waehlSymbol)));
        const bool sicht = static_cast<std::size_t>(r.ebene) < b.sichtbar.size() && b.sichtbar[static_cast<std::size_t>(r.ebene)] != 0;
        uiIcon(zl, sicht ? UiIcon::Sichtbar : UiIcon::Versteckt, auge, px(kSymbol), text);
        // Tooltips je Spalte (item_construct)
        if (hover && !b.ziehen) {
            const float mx = (io.MousePos.x - a.x) / g_sk;
            if (mx < kAktuellB) {
                ImGui::SetTooltip("%s", trK(Str::KeLaySetCurrent));
            } else if (mx >= innenB - 52.0F && mx < innenB - 24.0F) {
                ImGui::SetTooltip("%s", trK(Str::KeLaySelectTip));
            } else if (mx >= innenB - 24.0F) {
                ImGui::SetTooltip("%s", trK(Str::KeLayVisibleTip));
            }
        }
        if (ImGui::IsItemActivated()) {   // mousePressEvent: die Zeile wird gewaehlt
            b.gewaehlt = r.ebene;
            b.druck = r.ebene;
            b.druckX = io.MousePos.x;
            b.druckY = io.MousePos.y;
        }
    }
    const float inhaltH = zh * static_cast<float>(z.size());
    ImGui::SetCursorScreenPos(ImVec2{start.x, start.y + inhaltH});
    ImGui::Dummy(ImVec2{0.0F, 0.0F});
    const ImVec2 cmin = ImGui::GetWindowPos();
    const ImVec2 cmax{cmin.x + ImGui::GetWindowWidth(), cmin.y + ImGui::GetWindowHeight()};
    const bool imBaum = ImGui::IsMouseHoveringRect(cmin, cmax);
    // Ziehen: ab startDragDistance (10)
    if (b.druck >= 0 && ImGui::IsMouseDown(ImGuiMouseButton_Left) && !b.ziehen) {
        const float dx = io.MousePos.x - b.druckX;
        const float dy = io.MousePos.y - b.druckY;
        if (std::sqrt(dx * dx + dy * dy) >= px(10.0F)) { b.ziehen = true; }
    }
    // die Ablage unter der Maus (position)
    Ablage ablage = Ablage::Leer;
    int ziel = -1;
    if (imBaum) {
        const int zi = static_cast<int>(std::floor((io.MousePos.y - start.y) / zh));
        if (zi >= 0 && zi < static_cast<int>(z.size())) {
            ziel = z[static_cast<std::size_t>(zi)].ebene;
            const float top = start.y + zh * static_cast<float>(zi);
            const float rand = std::clamp(std::round(kZeileH / 5.5F), 2.0F, 12.0F) * g_sk;
            if (io.MousePos.y - top < rand) {
                ablage = Ablage::Ueber;
            } else if (top + zh - 1.0F - io.MousePos.y < rand) {
                ablage = Ablage::Unter;
            } else {
                ablage = Ablage::Auf;
            }
        }
    }
    if (b.ziehen && imBaum) {   // der Ablagestrich
        const ImU32 f = ImGui::GetColorU32(ImGuiCol_DragDropTarget);
        if (ziel >= 0) {
            std::size_t zi = 0;
            while (zi < z.size() && z[zi].ebene != ziel) { ++zi; }
            const float top = start.y + zh * static_cast<float>(zi);
            if (ablage == Ablage::Ueber) { zl->AddLine(ImVec2{start.x, top}, ImVec2{start.x + px(innenB), top}, f, px(1.0F)); }
            if (ablage == Ablage::Unter) { zl->AddLine(ImVec2{start.x, top + zh - 1.0F}, ImVec2{start.x + px(innenB), top + zh - 1.0F}, f, px(1.0F)); }
            if (ablage == Ablage::Auf) { zl->AddRect(ImVec2{start.x, top}, ImVec2{start.x + px(innenB), top + zh}, f); }
        }
    }
    if (ImGui::IsMouseReleased(ImGuiMouseButton_Left) && b.druck >= 0) {
        const int gezogen = b.druck;
        b.druck = -1;
        if (b.ziehen) {
            b.ziehen = false;
            // QTreeWidget::dropEvent: nicht auf sich selbst oder ein Kind
            const std::vector<int> selbst = karte::ebenenUnterbaum(ed.karte, gezogen);
            if (imBaum && std::find(selbst.begin(), selbst.end(), ziel) == selbst.end() && !(ablage == Ablage::Auf && ziel == gezogen)) {
                int eltern = -1;
                int zeile = -1;   // -1: ans Ende
                if (ablage == Ablage::Auf) {
                    eltern = ziel;
                } else if (ablage == Ablage::Ueber || ablage == Ablage::Unter) {
                    eltern = ed.karte.schichten[static_cast<std::size_t>(ziel)].eltern;
                    const std::vector<int> geschw = karte::ebenenKinder(ed.karte, eltern);
                    const int r = static_cast<int>(std::find(geschw.begin(), geschw.end(), ziel) - geschw.begin());
                    zeile = ablage == Ablage::Ueber ? r : r + 1;
                    // die Zeile nach dem Herausnehmen (QPersistentModelIndex dropRow)
                    const int alt = static_cast<int>(std::find(geschw.begin(), geschw.end(), gezogen) - geschw.begin());
                    if (alt < static_cast<int>(geschw.size()) && alt < zeile) { --zeile; }
                }
                if (zeile < 0) { zeile = static_cast<int>(karte::ebenenKinder(ed.karte, eltern).size()); }
                umnummern(karte::ebeneVerschieben(ed.karte, gezogen, eltern, zeile));
            } else if (imBaum && ablage == Ablage::Leer) {   // OnViewport: ans Ende der Wurzel
                umnummern(karte::ebeneVerschieben(ed.karte, gezogen, -1, static_cast<int>(karte::ebenenKinder(ed.karte, -1).size())));
            }
        } else if (ueberZeile >= 0 && z[static_cast<std::size_t>(ueberZeile)].ebene == gezogen) {   // itemClicked
            const float mx = (io.MousePos.x - start.x) / g_sk;
            if (mx >= innenB - 52.0F && mx < innenB - 24.0F) {   // Column::select
                if (!io.KeyShift && !(io.KeyCtrl && io.KeyAlt)) {
                    if (!io.KeyCtrl && !io.KeyAlt) { ed.karte.auswahl.leeren(); }   // setSelectedAll( false )
                    karte::ebenenWaehlen(ed.karte, karte::ebenenUnterbaum(ed.karte, gezogen), !io.KeyAlt);
                    auswahlGeaendert();
                } else if (io.KeyShift && !io.KeyCtrl && !io.KeyAlt) {   // Move to layer
                    karte::auswahlAufEbene(ed.karte, gezogen);
                    auswahlGeaendert();
                }
            } else if (mx >= innenB - 24.0F) {   // Column::visible
                const bool jetzt = static_cast<std::size_t>(gezogen) < b.sichtbar.size() && b.sichtbar[static_cast<std::size_t>(gezogen)] != 0;
                sichtbarSetzen(karte::ebenenUnterbaum(ed.karte, gezogen), !jetzt);
            } else if (mx < kAktuellB) {   // Column::current
                ed.karte.aktuelleSchicht = static_cast<std::size_t>(gezogen);
            }
        }
    }
    // Kontextmenue (customContextMenuRequested, beim Loslassen)
    if (imBaum && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) {
        b.menueEbene = ueberZeile >= 0 ? z[static_cast<std::size_t>(ueberZeile)].ebene : -1;
        ImGui::OpenPopup("##ebenenmenue");
    }
    if (qtPopupAnfang("##ebenenmenue")) {
        const int ebene = b.menueEbene;
        const bool da = ebene >= 0 && static_cast<std::size_t>(ebene) < ed.karte.schichten.size();
        const auto punkt = [](const std::string& id, bool aktiv) {
            QtPunkt p;
            p.id = id.c_str();
            p.aktiv = aktiv;
            return qtPunkt(p);
        };
        if (punkt(std::string(trK(Str::KeLayMoveSel)) + "##ebenezuordnen", da)) {
            karte::auswahlAufEbene(ed.karte, ebene);
        }
        if (punkt(std::string(trK(Str::KeLayNew)) + "##ebeneneu", true)) { neueEbeneFragen(da ? ebene : -1); }
        const bool einzige = da && ed.karte.schichten[static_cast<std::size_t>(ebene)].eltern < 0 && karte::ebenenKinder(ed.karte, -1).size() == 1;
        if (punkt(std::string(trK(Str::KeLayDelete)) + "##ebeneloeschen", da && !einzige)) {
            umnummern(karte::ebeneLoeschen(ed.karte, ebene));
            sichtGeaendert();
        }
        if (punkt(std::string(trK(Str::KeLayRename)) + "##ebeneumbenennen", da)) {
            qtEingabe(trK(Str::KeLayName), trK(Str::KeLayEnter), ed.karte.schichten[static_cast<std::size_t>(ebene)].name, [ebene](const std::string& t) {
                const std::string name = karte::ebenenName(t);
                if (!name.empty()) { karte::ebeneUmbenennen(e().karte, ebene, name); }
            });
        }
        if (punkt(std::string(trK(Str::KeLayColor)) + "##ebenefarbe", da)) {
            const auto& f = ed.karte.schichten[static_cast<std::size_t>(ebene)].farbe;
            farbDialogOeffnen(karte::Vektor3f(static_cast<float>(f[0]) / 255.0F, static_cast<float>(f[1]) / 255.0F, static_cast<float>(f[2]) / 255.0F),
                              [ebene](const karte::Vektor3f& c) {
                                  karte::ebeneFarbe(e().karte, ebene,
                                                    {static_cast<int>(c[0] * 255.0F), static_cast<int>(c[1] * 255.0F), static_cast<int>(c[2] * 255.0F)});
                              });
        }
        qtPopupEnde();
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(3);
    // unten rechts: New Layer, Show All Layers, Hide All Layers (QToolButton)
    const float ky = baumH + 6.0F;
    const UiIcon symbole[3] = {UiIcon::Plus, UiIcon::Sichtbar, UiIcon::Versteckt};
    const Str tipps[3] = {Str::KeLayNew, Str::KeLayShowAll, Str::KeLayHideAll};
    for (int i = 0; i < 3; ++i) {
        const float kx = static_cast<float>(breite) - kKnopf * static_cast<float>(3 - i) - 6.0F * static_cast<float>(2 - i);
        const ImVec2 k0{o.x + px(kx), o.y + px(ky)};
        const ImVec2 k1{k0.x + px(kKnopf), k0.y + px(kKnopf)};
        ImGui::SetCursorScreenPos(k0);
        const bool geklickt = ImGui::InvisibleButton((std::string("##ebenenknopf") + std::to_string(i)).c_str(), ImVec2{k1.x - k0.x, k1.y - k0.y});
        const bool h = ImGui::IsItemHovered();
        dl->AddRectFilled(k0, k1, ImGui::GetColorU32(h ? (ImGui::IsItemActive() ? ImGuiCol_ButtonActive : ImGuiCol_ButtonHovered) : ImGuiCol_Button),
                          ImGui::GetStyle().FrameRounding);
        const float g = px(16.0F);
        uiIcon(dl, symbole[i], ImVec2{std::floor(k0.x + (k1.x - k0.x - g) * 0.5F), std::floor(k0.y + (k1.y - k0.y - g) * 0.5F)}, g,
               ImGui::GetColorU32(uiIconFarbe(symbole[i])));
        if (h) { ImGui::SetTooltip("%s", trK(tipps[i])); }
        if (geklickt) {
            if (i == 0) { neueEbeneFragen(-1); }   // layer_new( nullptr, nullptr, ... )
            if (i == 1) { alleSichtbar(true); }
            if (i == 2) { alleSichtbar(false); }
        }
    }
    ImGui::SetCursorScreenPos(ImVec2{o.x, o.y + px(static_cast<float>(hoehe))});
    ImGui::Dummy(ImVec2{0.0F, 0.0F});
}

// --- Befehle (layerswindow.cpp:578-582) ---------------------------------------------------

void ebenenBefehl(const std::string& name) {
    Editor& ed = e();
    if (name == "ExpandSelectionToLayers") {   // Scene_ExpandSelectionToLayers
        karte::auswahlAufEbenenErweitern(ed.karte);
        auswahlGeaendert();
    } else if (name == "LayersMenu") {   // context_moveto_menu
        if (!ed.ebenenBro.menueOffen) { ed.ebenenBro.menueOeffnen = true; }
    }
}

void zeichneEbenenMenue() {   // context_moveto_menu: QMenu an QCursor::pos()
    Editor& ed = e();
    Editor::EbenenBrowser& b = ed.ebenenBro;
    if (b.menueOeffnen) {
        b.menueOeffnen = false;
        ImGui::OpenPopup("##ebenenzuordnenmenue");
    }
    b.menueOffen = qtPopupAnfang("##ebenenzuordnenmenue");
    if (!b.menueOffen) { return; }
    const bool gewaehlt = ed.karte.auswahl.anzahl() != 0;
    for (std::size_t i = 0; i < ed.karte.schichten.size(); ++i) {   // Layers::forEach
        const std::string id = "-> " + ed.karte.schichten[i].name + "##ebenezu" + std::to_string(i);
        QtPunkt p;
        p.id = id.c_str();
        p.aktiv = gewaehlt;
        if (qtPunkt(p)) { karte::auswahlAufEbene(ed.karte, static_cast<int>(i)); }
    }
    qtPopupEnde();
}

// --- QInputDialog::getText ---------------------------------------------------------------

void qtEingabe(const std::string& titel, const std::string& beschriftung, const std::string& vorgabe, std::function<void(const std::string&)> fertig) {
    Editor::Eingabe& d = e().eingabe;
    d = Editor::Eingabe{};
    d.offen = true;
    d.oeffnen = true;
    d.titel = titel;
    d.beschriftung = beschriftung;
    std::snprintf(d.puffer, sizeof(d.puffer), "%s", vorgabe.c_str());
    d.fertig = std::move(fertig);
}

void zeichneQtEingabe() {
    Editor& ed = e();
    Editor::Eingabe& d = ed.eingabe;
    if (!d.offen) { return; }
    const std::string id = d.titel + "###karteeingabe";
    if (d.oeffnen) {
        d.oeffnen = false;
        ImGui::OpenPopup(id.c_str());
    }
    g_sk = massstab();
    dialogSchriftAn();
    // QVBoxLayout (Rand 11, Abstand 6): Beschriftung 13, QLineEdit 20, Knoepfe 22;
    // Breite: die Knopfreihe (2 x 80 + 6) oder die Beschriftung
    const int lb = platform::gdiTextBreite(d.beschriftung, L"Tahoma", 11, false);
    const float innenB = 11.0F + std::max(166.0F, static_cast<float>(std::max(lb, 0))) + 11.0F;
    const float innenH = 11.0F + 13.0F + 6.0F + 20.0F + 6.0F + 22.0F + 11.0F;
    const float titelH = ImGui::GetFontSize() + ImGui::GetStyle().FramePadding.y * 2.0F;
    ImGui::SetNextWindowSize(ImVec2{px(innenB), px(innenH) + titelH}, ImGuiCond_Always);
    const ImGuiViewport* vp = ImGui::GetMainViewport();
    ImGui::SetNextWindowPos(ImVec2{vp->WorkPos.x + vp->WorkSize.x * 0.5F, vp->WorkPos.y + vp->WorkSize.y * 0.5F}, ImGuiCond_Appearing, ImVec2{0.5F, 0.5F});
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{0.0F, 0.0F});
    bool offen = true;
    const bool sichtbar = ImGui::BeginPopupModal(id.c_str(), &offen, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoScrollbar);
    ImGui::PopStyleVar();
    bool fertig = !offen;
    bool ok = false;
    if (sichtbar) {
        const ImVec2 o = ImGui::GetCursorScreenPos();
        const auto bp = [&](float x, float y) { return ImVec2{std::round(o.x + x * g_sk), std::round(o.y + y * g_sk)}; };
        ImGui::GetWindowDrawList()->AddText(bp(11.0F, 11.0F), ImGui::GetColorU32(ImGuiCol_Text), d.beschriftung.c_str());
        ImGui::SetCursorScreenPos(bp(11.0F, 30.0F));
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{px(3.0F), std::max(0.0F, (px(20.0F) - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::SetNextItemWidth(px(innenB - 22.0F));
        if (ImGui::IsWindowAppearing()) { ImGui::SetKeyboardFocusHere(); }   // selectAll beim Oeffnen
        const bool enter = ImGui::InputText("##eingabetext", d.puffer, sizeof(d.puffer), ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll);
        ImGui::PopStyleVar();
        const float knopfH = px(22.0F);
        ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2{0.0F, std::max(0.0F, (knopfH - ImGui::GetTextLineHeight()) * 0.5F)});
        ImGui::SetCursorScreenPos(bp(innenB - 11.0F - 80.0F - 6.0F - 80.0F, 56.0F));
        if (ImGui::Button((std::string(trK(Str::KePrefOk)) + "##eingabeok").c_str(), ImVec2{px(80.0F), knopfH}) || enter) {
            fertig = true;
            ok = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        ImGui::SetCursorScreenPos(bp(innenB - 11.0F - 80.0F, 56.0F));
        if (ImGui::Button((std::string(trK(Str::KeCancel)) + "##eingabeabbrechen").c_str(), ImVec2{px(80.0F), knopfH}) ||
            ImGui::IsKeyPressed(ImGuiKey_Escape, false)) {
            fertig = true;
            ed.tasteVerbrauchtBild = ImGui::GetFrameCount();
        }
        ImGui::PopStyleVar();
        ImGui::SetCursorScreenPos(bp(0.0F, innenH));
        ImGui::Dummy(ImVec2{0.0F, 0.0F});
        if (fertig) { ImGui::CloseCurrentPopup(); }
        ImGui::EndPopup();
    }
    dialogSchriftAus();
    if (!fertig) { return; }
    d.offen = false;
    if (ok && d.fertig) {
        const auto f = std::move(d.fertig);
        f(d.puffer);
    }
}

}  // namespace bhed::gui::karteneditor
