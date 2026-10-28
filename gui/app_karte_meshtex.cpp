// app_karte_meshtex.cpp - Karten-Editor, Stufe 4 Punkt 3: die Oberflaeche des
// Plugins MeshTex (die Rechnung: src/karte/meshtex.cpp).
//
// Portiert aus NetRadiant-Custom 20260114, contrib/meshtex: MainMenu.cpp (die
// Befehle; CommandMeshVisitor mit UndoableCommand( Befehl ) und "No valid
// patch meshes selected."), MeshVisitor.cpp (je gewaehltem Patch ein
// MeshEntity; Kaesten "Mesh Info" / "Warning" / "Error"), GenericDialog.cpp
// (nicht modal; OK = Apply und verstecken, Apply, Cancel = verstecken),
// GetInfoDialog.cpp, SetScaleDialog.cpp, GeneralFunctionDialog.cpp
// (die Werte, die Abhaengigkeiten der Felder: RegisterWidgetDependence).
//
// Masse an den Fotos dlg_Plugins_MeshTex_*_0.png (Massstab 2): Get Info
// 357 x 126 (QFormLayout nach den Qt-Regeln), Set S/T Scale 418 x 344 und
// General Function 912 x 298 (verschachtelte QGroupBox / QGridLayout: die
// Lagen am Foto gemessen, siehe die Zahlen unten). Knoepfe OK / Cancel /
// Apply rechtsbuendig (QDialogButtonBox, WinLayout).
//
// Abweichung: Meldet ein Befehl fuer mehrere Netze mehrere Kaesten, zeigt das
// Programm einen Kasten mit allen Texten (NRC: einen nach dem anderen).
//
//   Copyright (C) 2012 Joel Baxter
//
//   This file is part of MeshTex. MeshTex is free software: you can
//   redistribute it and/or modify it under the terms of the GNU General Public
//   License as published by the Free Software Foundation, either version 2 of
//   the License, or (at your option) any later version. MeshTex is distributed
//   in the hope that it will be useful, but WITHOUT ANY WARRANTY; without even
//   the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//   PURPOSE. See the GNU General Public License for more details (LICENSE).
#include "karteneditor.h"

#include "app_internal.h"
#include "bhed/karte/patch.h"
#include "bhed/karte/rueckgaengig.h"
#include "karte_qtdialog.h"

#include <algorithm>
#include <cmath>
#include <memory>

namespace bhed::gui::karteneditor {

namespace {

namespace mt = karte::meshtex;
using i18n::Str;

Editor& e() { return editor(); }

// MeshVisitor: die gewaehlten Patches in Auswahlreihenfolge
std::vector<std::shared_ptr<karte::Patch>> gewaehltePatches() {
    std::vector<std::shared_ptr<karte::Patch>> v;
    for (const karte::AuswahlElement& a : e().karte.auswahl.elemente()) {
        const auto p = a.primitiv.lock();
        if (p && p->art() == karte::Primitiv::Art::Patch) { v.push_back(std::static_pointer_cast<karte::Patch>(p)); }
    }
    return v;
}

// Die Kaesten von GenericPluginUI (Info "Mesh Info", Warning, Error)
void kaesten(const mt::Meldungen& m) {
    if (m.empty()) { return; }
    int art = 0;
    for (const mt::Meldung& x : m) { art = std::max(art, x.art); }
    art = std::clamp(art, 0, 2);   // Stufe 8d (/analyze C6385): titel hat drei Eintraege
    std::string text;
    for (const mt::Meldung& x : m) {
        if (x.art != art) { continue; }
        if (!text.empty()) { text += "\n\n"; }
        text += x.text;
    }
    static const char* const titel[3] = {"Mesh Info", "Warning", "Error"};
    qtMeldung(titel[art], text, art == 2 ? MeldungArt::Fehler : (art == 1 ? MeldungArt::Warnung : MeldungArt::Info), false, nullptr);
}

// visit(): je Patch ein MeshEntity, nur gueltige zaehlen
int besuchen(const std::function<void(mt::MeshEntity&)>& f, mt::Meldungen& m) {
    int n = 0;
    for (const auto& p : gewaehltePatches()) {
        const auto g = e().karte.kontext.groesse(p->shader());
        mt::MeshEntity me(*p, static_cast<int>(g.first), static_cast<int>(g.second), m);
        if (me.IsValid()) {
            f(me);
            ++n;
        }
    }
    return n;
}

void voreinstellung(const mt::Voreinstellung& v) {   // MainMenu::CommandMeshVisitor
    Editor& ed = e();
    mt::Meldungen m;
    int n = 0;
    {
        karte::Befehl b(ed.karte.undo, v.befehl);   // UndoableCommand( commandString )
        n = besuchen([&v](mt::MeshEntity& me) { (me.*v.funktion)(v.achsen); }, m);
    }
    if (n == 0) { m.push_back({1, mt::kKeineNetze}); }   // WarningReportDialog
    kaesten(m);
    ++ed.sichtStand;
}

mt::SliceDesignation schnitt(const MeshTexSchnitt& s) { return mt::SliceDesignation{s.max, s.nummer}; }
mt::RefSliceDescriptor referenz(const MeshTexReferenz& r) { return mt::RefSliceDescriptor{schnitt(r.schnitt), r.nurGesamt}; }

// SetScaleDialog::Apply
bool setScaleAnwenden() {
    Editor& ed = e();
    MeshTexZustand& z = ed.meshtex;
    if (gewaehltePatches().empty()) {
        kaesten({{1, mt::kKeineNetze}});
        return false;
    }
    if (!z.sSetzen && !z.tSetzen) { return true; }
    const mt::SliceDesignation alignCol = schnitt(z.nullSpalte);
    const mt::SliceDesignation alignRow = schnitt(z.nullZeile);
    const mt::RefSliceDescriptor refRow = referenz(z.refZeile);
    const mt::RefSliceDescriptor refCol = referenz(z.refSpalte);
    mt::Meldungen m;
    {
        karte::Befehl b(ed.karte.undo, "SetScale");   // UndoableCommand( _triggerCommand )
        (void)besuchen(
            [&](mt::MeshEntity& me) {
                if (z.sSetzen) {
                    me.SetScale(mt::ROW_SLICE_TYPE, &alignCol, z.refZeile.an ? &refRow : nullptr, z.sArt == 0,
                                static_cast<float>(z.sArt == 0 ? z.sSkala : z.sKacheln));
                }
                if (z.tSetzen) {
                    me.SetScale(mt::COL_SLICE_TYPE, &alignRow, z.refSpalte.an ? &refCol : nullptr, z.tArt == 0,
                                static_cast<float>(z.tArt == 0 ? z.tSkala : z.tKacheln));
                }
            },
            m);
    }
    kaesten(m);
    ++ed.sichtStand;
    return true;
}

// GetInfoDialog::Apply
bool getInfoAnwenden() {
    Editor& ed = e();
    MeshTexZustand& z = ed.meshtex;
    const auto netze = gewaehltePatches();
    if (netze.empty()) {
        kaesten({{1, mt::kKeineNetze}});
        return false;
    }
    if (z.uebertragen && netze.size() != 1) {
        kaesten({{2, mt::kNurEinNetz}});
        return false;
    }
    const int zeile = z.infoZeile;
    const int spalte = z.infoSpalte;
    const mt::TexInfoCallback zeileRueck = [&z](float skala, float kacheln) {   // SetScaleDialog::PopulateSWidgets
        z.sSkala = skala;
        z.sKacheln = kacheln;
    };
    const mt::TexInfoCallback spalteRueck = [&z](float skala, float kacheln) {   // PopulateTWidgets
        z.tSkala = skala;
        z.tKacheln = kacheln;
    };
    mt::Meldungen m;
    (void)besuchen(
        [&](mt::MeshEntity& me) {
            me.GetInfo(z.infoZeileAn ? &zeile : nullptr, z.infoSpalteAn ? &spalte : nullptr, z.infoZeileAn && z.uebertragen ? &zeileRueck : nullptr,
                       z.infoSpalteAn && z.uebertragen ? &spalteRueck : nullptr);
        },
        m);
    kaesten(m);
    if (z.uebertragen && z.setScaleOffen) { ImGui::SetWindowFocus("###mtsetscale"); }   // _setScaleDialog->Raise()
    return true;
}

// GeneralFunctionDialog::Apply
bool genFuncAnwenden() {
    Editor& ed = e();
    MeshTexZustand& z = ed.meshtex;
    if (gewaehltePatches().empty()) {
        kaesten({{1, mt::kKeineNetze}});
        return false;
    }
    if (!z.gfS && !z.gfT) { return true; }
    const auto faktoren = [](const std::array<double, 6>& f) {
        return mt::GeneralFunctionFactors{static_cast<float>(f[0]), static_cast<float>(f[1]), static_cast<float>(f[2]),
                                          static_cast<float>(f[3]), static_cast<float>(f[4]), static_cast<float>(f[5])};
    };
    const mt::GeneralFunctionFactors s = faktoren(z.sFaktoren);
    const mt::GeneralFunctionFactors t = faktoren(z.tFaktoren);
    const mt::SliceDesignation alignRow = schnitt(z.gfNullZeile);
    const mt::SliceDesignation alignCol = schnitt(z.gfNullSpalte);
    const mt::RefSliceDescriptor refRow = referenz(z.gfRefZeile);
    const mt::RefSliceDescriptor refCol = referenz(z.gfRefSpalte);
    mt::Meldungen m;
    {
        karte::Befehl b(ed.karte.undo, "GeneralFunction");
        (void)besuchen(
            [&](mt::MeshEntity& me) {
                me.GeneralFunction(z.gfS ? &s : nullptr, z.gfT ? &t : nullptr, &alignRow, &alignCol, z.gfRefZeile.an ? &refRow : nullptr,
                                   z.gfRefSpalte.an ? &refCol : nullptr, z.flaechenWerte);
            },
            m);
    }
    kaesten(m);
    ++ed.sichtStand;
    return true;
}

// FinalizeCallback: 0 OK (Apply, bei Erfolg verstecken), 1 Cancel (verstecken), 2 Apply
void knoepfeAuswerten(int k, bool& offen, const std::function<bool()>& anwenden) {
    if (k < 0) { return; }
    bool erfolg = true;
    if (k != 1) { erfolg = anwenden(); }
    if (erfolg && k != 2) { offen = false; }
}

std::vector<std::string> okCancelApply() { return {trK(Str::KePrefOk), trK(Str::KeCancel), trK(Str::KeMtApply)}; }

// "Zero" col/row bzw. die Zeile einer Referenz: Radioknopf + SpinBox( 0, 30 ), "Max" rechts
void schnittZeile(MeshTexSchnitt& s, const std::string& id, float xRadio, float xSpin, float xMax, float y, bool aktiv) {
    int wahl = s.max ? 1 : 0;
    (void)qtd::radio("", id + "r", wahl, 0, xRadio, y, 22, aktiv);
    (void)qtd::ganzFeld("", id + "n", s.nummer, 0, 30, 1, -1.0F, xSpin, 38, y, aktiv && wahl == 0);
    (void)qtd::radio(trK(Str::KeMtMax), id + "m", wahl, 1, xMax, y, 22, aktiv);
    s.max = wahl == 1;
}

void getInfoZeichnen() {
    MeshTexZustand& z = e().meshtex;
    if (!z.getInfoOffen) { return; }
    const std::string c1 = trK(Str::KeMtSRow);
    const std::string c2 = trK(Str::KeMtTCol);
    const std::string c3 = trK(Str::KeMtTransfer);
    const auto kn = okCancelApply();
    const float labelB = std::max(qtd::hakenHinweis(c1), qtd::hakenHinweis(c2));
    const float feldB = std::max(qtd::spinHinweis(0, 30, 0), qtd::knoepfeHinweis(kn));
    const float x1 = 11.0F + labelB + 6.0F;
    const float innenB = std::max(x1 + feldB, 11.0F + qtd::hakenHinweis(c3)) + 11.0F;
    const float innenH = 11 + 22 + 6 + 22 + 6 + 20 + 6 + 22 + 11;   // 126
    if (!qtd::beginnen(std::string(trK(Str::KeMtGetInfo)) + "###mtgetinfo", z.getInfoOeffnen, innenB, innenH, false)) {
        if (qtd::geschlossen()) { z.getInfoOffen = false; }
        return;
    }
    qtd::merke("MtGetInfo", 0, 0, innenB, innenH);
    (void)qtd::haken(c1, "mtinfozeile", z.infoZeileAn, 11, 11, 22);
    (void)qtd::ganzFeld("", "mtinfozeilen", z.infoZeile, 0, 30, 1, -1.0F, x1, innenB - 11.0F - x1, 11, z.infoZeileAn);
    (void)qtd::haken(c2, "mtinfospalte", z.infoSpalteAn, 11, 39, 22);
    (void)qtd::ganzFeld("", "mtinfospalten", z.infoSpalte, 0, 30, 1, -1.0F, x1, innenB - 11.0F - x1, 39, z.infoSpalteAn);
    (void)qtd::haken(c3, "mtuebertragen", z.uebertragen, 11, 67);
    const int k = qtd::knoepfe(kn, "mtinfook", innenB - 11.0F, 93, 0, 1);
    qtd::beenden(false);
    knoepfeAuswerten(k, z.getInfoOffen, getInfoAnwenden);
}

// Eine Haelfte von Set S/T Scale (die QGroupBox "Set S" bzw. "Set T"), dx = 0 oder 201
void setScaleHaelfte(bool s, float dx) {
    MeshTexZustand& z = e().meshtex;
    bool& an = s ? z.sSetzen : z.tSetzen;
    int& art = s ? z.sArt : z.tArt;
    double& skala = s ? z.sSkala : z.tSkala;
    double& kacheln = s ? z.sKacheln : z.tKacheln;
    MeshTexSchnitt& null = s ? z.nullSpalte : z.nullZeile;
    MeshTexReferenz& ref = s ? z.refZeile : z.refSpalte;
    const std::string k = s ? "mts" : "mtt";
    (void)qtd::haken(trK(s ? Str::KeMtSetS : Str::KeMtSetT), k + "an", an, 13 + dx, 12, 14);
    qtd::rahmen(12 + dx, 29, 196, 276);
    qtd::text(trK(Str::KeMtScaling), 24 + dx, 41, 14, an);
    qtd::rahmen(24 + dx, 58, 170, 72);
    (void)qtd::radio(trK(Str::KeMtNatural), k + "nat", art, 0, 35 + dx, 69, 22, an);
    (void)qtd::zahlFeld("", k + "skala", skala, -999, 999, 3, 1, -1.0F, 111 + dx, 70, 69, an && art == 0);
    (void)qtd::radio(trK(Str::KeMtTiles), k + "kach", art, 1, 35 + dx, 97, 22, an);
    (void)qtd::zahlFeld("", k + "kacheln", kacheln, -999, 999, 3, 1, -1.0F, 111 + dx, 70, 97, an && art == 1);
    qtd::text(trK(s ? Str::KeMtZeroCol : Str::KeMtZeroRow), 24 + dx, 137, 14, an);
    qtd::rahmen(24 + dx, 154, 170, 44);
    schnittZeile(null, k + "null", 35 + dx, 55 + dx, 138 + dx, 165, an);
    (void)qtd::haken(trK(s ? Str::KeMtRefRow : Str::KeMtRefCol), k + "ref", ref.an, 25 + dx, 205, 14, an);
    qtd::rahmen(24 + dx, 222, 170, 71);
    schnittZeile(ref.schnitt, k + "refs", 35 + dx, 55 + dx, 138 + dx, 234, an && ref.an);
    (void)qtd::haken(trK(Str::KeMtTotal), k + "gesamt", ref.nurGesamt, 36 + dx, 262, 20, an && ref.an);
}

void setScaleZeichnen() {
    MeshTexZustand& z = e().meshtex;
    if (!z.setScaleOffen) { return; }
    const float innenB = 418;
    const float innenH = 344;
    if (!qtd::beginnen(std::string(trK(Str::KeMtSetScale)) + "###mtsetscale", z.setScaleOeffnen, innenB, innenH, false)) {
        if (qtd::geschlossen()) { z.setScaleOffen = false; }
        return;
    }
    qtd::merke("MtSetScale", 0, 0, innenB, innenH);
    setScaleHaelfte(true, 0);
    setScaleHaelfte(false, 201);
    const int k = qtd::knoepfe(okCancelApply(), "mtscaleok", innenB - 11.0F, 311, 0, 1);
    qtd::beenden(false);
    knoepfeAuswerten(k, z.setScaleOffen, setScaleAnwenden);
}

void genFuncZeichnen() {
    MeshTexZustand& z = e().meshtex;
    if (!z.genFuncOffen) { return; }
    const float innenB = 912;
    const float innenH = 298;
    if (!qtd::beginnen(std::string(trK(Str::KeMtGenFunc)) + "###mtgenfunc", z.genFuncOeffnen, innenB, innenH, false)) {
        if (qtd::geschlossen()) { z.genFuncOffen = false; }
        return;
    }
    qtd::merke("MtGenFunc", 0, 0, innenB, innenH);
    int wert = z.flaechenWerte ? 0 : 1;
    (void)qtd::radio(trK(Str::KeMtSurface), "mtflaeche", wert, 0, 245, 11, 22);
    (void)qtd::radio(trK(Str::KeMtControl), "mtkontroll", wert, 1, 462, 11, 22);
    z.flaechenWerte = wert == 0;
    qtd::hLinie(11, 36, 890);
    // die Zeilen S = ... und T = ... (QHBoxLayout: QLabel, DoubleSpinBox( -999, 999, 0, 3, .01 ) je Faktor);
    // die zweite Beschriftung ist auch in der T-Zeile " * old_S  + " (GeneralFunctionDialog.cpp)
    static const float spinX[6] = {66, 207, 363, 514, 674, 830};
    static const char* const namen[6] = {nullptr, " * old_S  + ", " * row_dist  + ", " * col_dist  + ", " * row_num  + ", " * col_num  + "};
    for (int zeile = 0; zeile < 2; ++zeile) {
        const float y = zeile == 0 ? 47.0F : 74.0F;
        bool& an = zeile == 0 ? z.gfS : z.gfT;
        auto& f = zeile == 0 ? z.sFaktoren : z.tFaktoren;
        const std::string k = zeile == 0 ? "mtgfs" : "mtgft";
        (void)qtd::haken("", k + "an", an, 12, y, 22);
        for (int i = 0; i < 6; ++i) {
            const std::string name = i == 0 ? std::string(zeile == 0 ? " S =  " : " T =  ") : std::string(namen[i]);
            qtd::text(name, spinX[i] - 6.0F - qtd::textBreite(name), y, 22, an);
            (void)qtd::zahlFeld("", k + std::to_string(i), f[static_cast<std::size_t>(i)], -999, 999, 3, 0.01, -1.0F, spinX[i], 70, y, an);
        }
    }
    // "Zero" col / "Zero" row (Zeile 4), die Referenzen (Zeile 5)
    for (int g = 0; g < 2; ++g) {
        const float x = g == 0 ? 246.0F : 463.0F;
        const float b = g == 0 ? 209.0F : 204.0F;
        const float xr = g == 0 ? 257.0F : 474.0F;
        const float xs = g == 0 ? 277.0F : 494.0F;
        const float xm = g == 0 ? 400.0F : 612.0F;
        const std::string k = g == 0 ? "mtgfc" : "mtgfr";
        qtd::text(trK(g == 0 ? Str::KeMtZeroCol : Str::KeMtZeroRow), x, 103, 14);
        qtd::rahmen(x, 120, b, 43);
        schnittZeile(g == 0 ? z.gfNullSpalte : z.gfNullZeile, k + "null", xr, xs, xm, 130, true);
        MeshTexReferenz& ref = g == 0 ? z.gfRefZeile : z.gfRefSpalte;
        (void)qtd::haken(trK(g == 0 ? Str::KeMtRefRowDist : Str::KeMtRefColDist), k + "ref", ref.an, x + 1.0F, 171, 14);
        qtd::rahmen(x, 188, b, 69);
        schnittZeile(ref.schnitt, k + "refs", xr, xs, xm, 199, ref.an);
        (void)qtd::haken(trK(Str::KeMtTotal), k + "gesamt", ref.nurGesamt, xr + 1.0F, 228, 20, ref.an);
    }
    const int k = qtd::knoepfe(okCancelApply(), "mtgfok", innenB - 11.0F, 265, 0, 1);
    qtd::beenden(false);
    knoepfeAuswerten(k, z.genFuncOffen, genFuncAnwenden);
}

}  // namespace

std::vector<PluginUmsetzung> meshTexUmsetzungen() {
    std::vector<PluginUmsetzung> v;
    const auto zeigen = [](bool MeshTexZustand::*offen, bool MeshTexZustand::*oeffnen) {   // GenericDialog::Show
        return [offen, oeffnen] {
            MeshTexZustand& z = e().meshtex;
            z.*offen = true;
            z.*oeffnen = true;
        };
    };
    v.push_back({"meshTex::SetScale", zeigen(&MeshTexZustand::setScaleOffen, &MeshTexZustand::setScaleOeffnen), nullptr});
    v.push_back({"meshTex::GetInfo", zeigen(&MeshTexZustand::getInfoOffen, &MeshTexZustand::getInfoOeffnen), nullptr});
    v.push_back({"meshTex::GeneralFunction", zeigen(&MeshTexZustand::genFuncOffen, &MeshTexZustand::genFuncOeffnen), nullptr});
    for (const mt::Voreinstellung& vo : mt::voreinstellungen()) {
        const mt::Voreinstellung kopie = vo;
        v.push_back({std::string("meshTex::") + vo.befehl, [kopie] {
                         e().meshtex.letzterBefehl = kopie.befehl;
                         voreinstellung(kopie);
                     },
                     nullptr});
    }
    v.push_back({"meshTex::Help", [] { qtMeldung("Help", mt::kHilfe, MeldungArt::Info, false, nullptr); }, nullptr});
    v.push_back({"meshTex::About", [] { qtMeldung("About", mt::kAbout, MeldungArt::Info, false, nullptr); }, nullptr});
    return v;
}

void meshTexDialogeZeichnen() {
    setScaleZeichnen();
    getInfoZeichnen();
    genFuncZeichnen();
}

}  // namespace bhed::gui::karteneditor
