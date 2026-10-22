// app_karte_konsole.cpp - Karten-Editor: die Konsole.
//
// Wie NRCs Konsole (radiant/console.cpp): ein schreibgeschuetztes
// QPlainTextEdit, in das Sys_Print die Meldungen Stueck fuer Stueck
// schreibt - Ausgabe in der Textfarbe, Warnungen (255, 127, 0), Fehler
// (255, 0, 0). Eine Zeile endet erst am '\n', darum stehen z. B.
// " ++Camera Move Speed: " (Ausgabe) und die Zahl (Warnung) auf einer Zeile.
// Nach einem '\n' rollt sie ans Ende (ensureCursorVisible). Umbruch an der
// Breite (QPlainTextEdit::WidgetWidth, WrapAtWordBoundaryOrAnywhere),
// Auswahl mit der Maus (TextSelectableByMouse: ziehen, Doppelklick Wort,
// dreifach die Zeile), kein Tastaturfokus (Qt::NoFocus), Kontextmenue
// createStandardContextMenu ("Copy", "Select All") plus "Copy All" und
// "Clear" (QPlainTextEdit_console).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/console.cpp
// (QPlainTextEdit_console, Console_constructWindow, Sys_Print) und Qt 5.15
// (qwidgettextcontrol.cpp createStandardContextMenu, mouse*Event).
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

#include "bhed/diag.h"

#include <algorithm>
#include <cctype>
#include <cmath>

namespace bhed::gui::karteneditor {

namespace {

Editor& e() { return editor(); }

constexpr int kRand = 1 + 4;   // Rahmen 1 + documentMargin 4
constexpr int kRollbalken = 7;  // dark.qss QScrollBar

// Eine Stelle im Text (QTextCursor): Zeile und Byte in KonsolenZeile::text
struct Stelle {
    std::size_t zeile = 0;
    std::size_t spalte = 0;
};
bool vor(const Stelle& a, const Stelle& b) { return a.zeile != b.zeile ? a.zeile < b.zeile : a.spalte < b.spalte; }

// Eine sichtbare Reihe nach dem Umbruch
struct Reihe {
    std::size_t zeile = 0;
    std::size_t von = 0;
    std::size_t bis = 0;
};

struct Ansicht {
    Stelle anker;
    Stelle ende;
    bool ziehen = false;
    std::vector<Reihe> reihen;
    float breite = -1.0F;
    std::size_t stand = static_cast<std::size_t>(-1);
    double letzterKlick = -1.0;
    int klicks = 0;
};
Ansicht g_a;

bool hatAuswahl() { return vor(g_a.anker, g_a.ende) || vor(g_a.ende, g_a.anker); }

// UTF-8: Laenge des Zeichens ab i und seine Breite
float zeichenBreite(const std::string& t, std::size_t i, std::size_t& laenge) {
    unsigned int c = 0;
    const int n = ImTextCharFromUtf8(&c, t.c_str() + i, t.c_str() + t.size());
    laenge = n > 0 ? static_cast<std::size_t>(n) : 1U;
    return ImGui::GetFontBaked()->GetCharAdvance(static_cast<ImWchar>(c));
}

// QTextLayout mit WrapAtWordBoundaryOrAnywhere: an Leerzeichen umbrechen,
// ein zu langes Wort an beliebiger Stelle; Leerzeichen am Reihenende zaehlen nicht.
void umbrechen(float breite) {
    const Editor& ed = e();
    g_a.reihen.clear();
    for (std::size_t z = 0; z < ed.konsole.size(); ++z) {
        const std::string& t = ed.konsole[z].text;
        std::size_t start = 0;
        if (t.empty()) {
            g_a.reihen.push_back({z, 0, 0});
            continue;
        }
        while (start < t.size()) {
            float x = 0.0F;
            std::size_t i = start;
            std::size_t letzterBruch = std::string::npos;   // Stelle nach einem Leerzeichen
            while (i < t.size()) {
                std::size_t l = 1;
                const float w = zeichenBreite(t, i, l);
                if (t[i] == ' ') {   // Leerzeichen passen immer (am Reihenende unsichtbar)
                    x += w;
                    i += l;
                    letzterBruch = i;
                    continue;
                }
                if (x + w > breite && i > start) { break; }
                x += w;
                i += l;
            }
            if (i < t.size() && letzterBruch != std::string::npos && letzterBruch > start) { i = letzterBruch; }
            g_a.reihen.push_back({z, start, i});
            start = i;
        }
    }
}

float breiteBis(const std::string& t, std::size_t von, std::size_t bis) {
    float x = 0.0F;
    for (std::size_t i = von; i < bis && i < t.size();) {
        std::size_t l = 1;
        x += zeichenBreite(t, i, l);
        i += l;
    }
    return x;
}

// Stelle unter dem Zeiger (QTextLayout::xToCursor, CursorBetweenCharacters)
Stelle stelleBei(ImVec2 p, ImVec2 o, float zeileH) {
    const Editor& ed = e();
    Stelle s;
    if (g_a.reihen.empty()) { return s; }
    const int r = std::clamp(static_cast<int>(std::floor((p.y - o.y) / zeileH)), 0, static_cast<int>(g_a.reihen.size()) - 1);
    const Reihe& re = g_a.reihen[static_cast<std::size_t>(r)];
    const std::string& t = ed.konsole[re.zeile].text;
    s.zeile = re.zeile;
    s.spalte = re.von;
    if (p.y < o.y) { return s; }
    if (p.y >= o.y + zeileH * static_cast<float>(g_a.reihen.size())) {
        s.spalte = re.bis;
        return s;
    }
    float x = o.x;
    for (std::size_t i = re.von; i < re.bis;) {
        std::size_t l = 1;
        const float w = zeichenBreite(t, i, l);
        if (p.x < x + w * 0.5F) { return s; }
        x += w;
        i += l;
        s.spalte = i;
    }
    return s;
}

std::string auswahlText() {
    const Editor& ed = e();
    Stelle a = g_a.anker;
    Stelle b = g_a.ende;
    if (vor(b, a)) { std::swap(a, b); }
    std::string s;
    for (std::size_t z = a.zeile; z <= b.zeile && z < ed.konsole.size(); ++z) {
        const std::string& t = ed.konsole[z].text;
        const std::size_t von = z == a.zeile ? std::min(a.spalte, t.size()) : 0;
        const std::size_t bis = z == b.zeile ? std::min(b.spalte, t.size()) : t.size();
        if (bis > von) { s += t.substr(von, bis - von); }
        if (z != b.zeile) { s += '\n'; }   // QTextCursor::selectedText: U+2029, copy() macht daraus '\n'
    }
    return s;
}

std::string alles() {
    std::string s;
    const Editor& ed = e();
    for (std::size_t z = 0; z < ed.konsole.size(); ++z) {
        s += ed.konsole[z].text;
        if (z + 1 < ed.konsole.size() || !ed.konsole[z].offen) { s += '\n'; }   // toPlainText: die Zeilenenden, nicht das letzte
    }
    if (!s.empty() && s.back() == '\n') { s.pop_back(); }
    return s;
}

bool wortZeichen(char c) { return std::isalnum(static_cast<unsigned char>(c)) != 0 || c == '_' || (static_cast<unsigned char>(c) & 0x80U) != 0; }

ImU32 farbeVon(KonsolenZeile::Art a) {
    switch (a) {
    case KonsolenZeile::Art::Warnung: return IM_COL32(255, 127, 0, 255);   // QColor( 255, 127, 0 )
    case KonsolenZeile::Art::Fehler: return IM_COL32(255, 0, 0, 255);      // QColor( 255, 0, 0 )
    case KonsolenZeile::Art::Ausgabe: break;                               // clearForeground
    }
    return ImGui::GetColorU32(ImGuiCol_Text);
}

}  // namespace

// Sys_Print: Text an das Ende, '\n' beendet die Zeile
void konsoleSchreiben(KonsolenZeile::Art art, const std::string& text) {
    Editor& ed = e();
    std::size_t i = 0;
    while (i <= text.size()) {
        const std::size_t nl = text.find('\n', i);
        const std::size_t ende = nl == std::string::npos ? text.size() : nl;
        if (ende > i || nl != std::string::npos) {
            if (ed.konsole.empty() || !ed.konsole.back().offen) {
                KonsolenZeile z;
                z.art = art;
                ed.konsole.push_back(std::move(z));
            }
            KonsolenZeile& z = ed.konsole.back();
            if (ende > i) {
                const std::size_t von = z.text.size();
                z.text.append(text, i, ende - i);
                if (!z.teile.empty() && z.teile.back().art == art) {
                    z.teile.back().laenge += ende - i;
                } else {
                    z.teile.push_back({art, von, ende - i});
                }
            }
            if (nl != std::string::npos) {
                z.offen = false;
                ed.konsoleRollen = true;   // ensureCursorVisible
                const char* vorsatz = z.art == KonsolenZeile::Art::Fehler ? "Karten-Editor Fehler: "
                                      : z.art == KonsolenZeile::Art::Warnung ? "Karten-Editor Warnung: "
                                                                            : "Karten-Editor: ";
                diag::detail(vorsatz + z.text);
            }
        }
        if (nl == std::string::npos) { break; }
        i = nl + 1;
    }
    ++ed.konsoleStand;
}

void ausgabe(const std::string& text) { konsoleSchreiben(KonsolenZeile::Art::Ausgabe, text + "\n"); }
void warnung(const std::string& text) { konsoleSchreiben(KonsolenZeile::Art::Warnung, text + "\n"); }
void fehler(const std::string& text) { konsoleSchreiben(KonsolenZeile::Art::Fehler, text + "\n"); }

void konsoleProtokoll(const karte::Protokoll& p) {
    for (const karte::ProtokollTeil& t : p) {
        const KonsolenZeile::Art a = t.art == karte::ProtokollTeil::Art::Fehler    ? KonsolenZeile::Art::Fehler
                                     : t.art == karte::ProtokollTeil::Art::Warnung ? KonsolenZeile::Art::Warnung
                                                                                   : KonsolenZeile::Art::Ausgabe;
        konsoleSchreiben(a, t.text);
    }
}

void konsoleLeeren() {   // QPlainTextEdit::clear
    Editor& ed = e();
    ed.konsole.clear();
    ++ed.konsoleStand;
    g_a.anker = g_a.ende = Stelle{};
}

std::string konsoleAuswahl() { return auswahlText(); }

void konsoleZeichnen() {
    Editor& ed = editor();
    dialogSchriftAn();   // QPlainTextEdit: Grundschrift (Tahoma 8 pt)
    const float s = massstab();
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2{std::round(static_cast<float>(kRand) * s), std::round(static_cast<float>(kRand) * s)});
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarSize, std::round(static_cast<float>(kRollbalken) * s));
    ImGui::PushStyleVar(ImGuiStyleVar_ScrollbarRounding, 0.0F);
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2{0.0F, 0.0F});
    ImGui::BeginChild("##konsoleText", ImVec2{-FLT_MIN, -FLT_MIN}, ImGuiChildFlags_Borders, ImGuiWindowFlags_None);
    const float zeileH = ImGui::GetTextLineHeight();   // fm.lineSpacing()
    const float breite = std::max(1.0F, ImGui::GetContentRegionAvail().x);
    if (g_a.stand != ed.konsoleStand || std::fabs(g_a.breite - breite) > 0.5F) {
        umbrechen(breite);
        g_a.stand = ed.konsoleStand;
        g_a.breite = breite;
    }
    const ImVec2 o = ImGui::GetCursorScreenPos();
    ImDrawList* dl = ImGui::GetWindowDrawList();
    // Auswahl, geordnet
    Stelle a = g_a.anker;
    Stelle b = g_a.ende;
    if (vor(b, a)) { std::swap(a, b); }
    const bool auswahl = hatAuswahl();
    ImGuiListClipper clip;
    clip.Begin(static_cast<int>(g_a.reihen.size()), zeileH);
    while (clip.Step()) {
        for (int r = clip.DisplayStart; r < clip.DisplayEnd; ++r) {
            const Reihe& re = g_a.reihen[static_cast<std::size_t>(r)];
            const KonsolenZeile& z = ed.konsole[re.zeile];
            const ImVec2 p{o.x, o.y + zeileH * static_cast<float>(r)};
            if (auswahl && re.zeile >= a.zeile && re.zeile <= b.zeile) {
                const std::size_t sv = re.zeile == a.zeile ? std::max(a.spalte, re.von) : re.von;
                std::size_t sb = re.zeile == b.zeile ? std::min(b.spalte, re.bis) : re.bis;
                const bool bisZumEnde = re.zeile != b.zeile && re.bis == z.text.size();
                if (sb > sv || bisZumEnde) {
                    if (sb < sv) { sb = sv; }
                    const float x0 = p.x + breiteBis(z.text, re.von, sv);
                    float x1 = p.x + breiteBis(z.text, re.von, sb);
                    if (bisZumEnde) { x1 += ImGui::GetFontSize() * 0.3F; }   // das Zeilenende
                    dl->AddRectFilled(ImVec2{x0, p.y}, ImVec2{x1, p.y + zeileH}, ImGui::GetColorU32(ImGuiCol_TextSelectedBg));
                }
            }
            float x = p.x;
            for (const KonsolenZeile::Teil& t : z.teile) {
                const std::size_t tv = std::max(t.von, re.von);
                const std::size_t tb = std::min(t.von + t.laenge, re.bis);
                if (tb <= tv) { continue; }
                const float w0 = breiteBis(z.text, re.von, tv);
                dl->AddText(ImVec2{p.x + w0, p.y}, farbeVon(t.art), z.text.c_str() + tv, z.text.c_str() + tb);
                x = p.x + breiteBis(z.text, re.von, tb);
            }
            (void)x;
        }
    }
    ImGui::SetCursorScreenPos(o);
    ImGui::Dummy(ImVec2{breite, zeileH * static_cast<float>(g_a.reihen.size())});
    // Maus (TextSelectableByMouse): kein Fokus, kein Tastaturbefehl
    const ImGuiIO& io = ImGui::GetIO();
    const bool ueber = ImGui::IsWindowHovered();
    if (ueber && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
        const Stelle st = stelleBei(io.MousePos, o, zeileH);
        const double jetzt = ImGui::GetTime();
        g_a.klicks = (jetzt - g_a.letzterKlick < static_cast<double>(io.MouseDoubleClickTime)) ? g_a.klicks + 1 : 1;
        g_a.letzterKlick = jetzt;
        if (g_a.klicks == 2 && st.zeile < ed.konsole.size()) {   // Doppelklick: das Wort
            const std::string& t = ed.konsole[st.zeile].text;
            std::size_t v = std::min(st.spalte, t.size());
            std::size_t w = v;
            while (v > 0 && wortZeichen(t[v - 1])) { --v; }
            while (w < t.size() && wortZeichen(t[w])) { ++w; }
            g_a.anker = {st.zeile, v};
            g_a.ende = {st.zeile, w};
            g_a.ziehen = false;
        } else if (g_a.klicks >= 3 && st.zeile < ed.konsole.size()) {   // dreifach: die Zeile (BlockUnderCursor)
            g_a.anker = {st.zeile, 0};
            g_a.ende = {st.zeile, ed.konsole[st.zeile].text.size()};
            g_a.ziehen = false;
        } else {
            if (!io.KeyShift) { g_a.anker = st; }   // Umschalt: Auswahl erweitern
            g_a.ende = st;
            g_a.ziehen = true;
        }
    }
    if (g_a.ziehen) {
        if (ImGui::IsMouseDown(ImGuiMouseButton_Left)) {
            g_a.ende = stelleBei(io.MousePos, o, zeileH);
            // ausserhalb rollen (QAbstractScrollArea autoScroll)
            const ImVec2 wp = ImGui::GetWindowPos();
            if (io.MousePos.y < wp.y) { ImGui::SetScrollY(ImGui::GetScrollY() - zeileH); }
            if (io.MousePos.y > wp.y + ImGui::GetWindowHeight()) { ImGui::SetScrollY(ImGui::GetScrollY() + zeileH); }
        } else {
            g_a.ziehen = false;
        }
    }
    // Kontextmenue beim Loslassen (WM_CONTEXTMENU)
    if (ueber && ImGui::IsMouseReleased(ImGuiMouseButton_Right)) { ImGui::OpenPopup("##konsolemenue"); }
    // QMenu wie die uebrigen Menues (app_karte_qtmenue.cpp)
    if (qtPopupAnfang("##konsolemenue")) {
        const auto punkt = [](const std::string& id, const char* nrc, const char* taste, bool aktiv) {
            QtPunkt p;
            p.id = id.c_str();
            p.nrc = nrc;
            p.taste = taste;
            p.aktiv = aktiv;
            return qtPunkt(p);
        };
        if (punkt(std::string(trK(Str::KeKoCopy)) + "##konsolecopy", "&Copy", "Ctrl+C", hatAuswahl())) {
            ImGui::SetClipboardText(auswahlText().c_str());   // copy()
        }
        qtTrenner();
        if (punkt(std::string(trK(Str::KeKoSelectAll)) + "##konsoleselectall", "Select All", "Ctrl+A", !ed.konsole.empty())) {
            g_a.anker = Stelle{};   // selectAll()
            g_a.ende = ed.konsole.empty() ? Stelle{} : Stelle{ed.konsole.size() - 1, ed.konsole.back().text.size()};
        }
        if (punkt(std::string(trK(Str::KeKoCopyAll)) + "##konsolecopyall", "Copy All", nullptr, true)) {
            ImGui::SetClipboardText(alles().c_str());   // clipboard()->setText( toPlainText() )
        }
        if (punkt(std::string(trK(Str::KeKoClear)) + "##konsoleclear", "Clear", nullptr, true)) { konsoleLeeren(); }
        qtPopupEnde();
    }
    // Sys_Print: nach einem '\n' ans Ende
    if (ed.konsoleRollen) {
        ed.konsoleRollen = false;
        ImGui::SetScrollHereY(1.0F);
    }
    ImGui::EndChild();
    ImGui::PopStyleVar(4);
    ImGui::PopStyleColor();
    dialogSchriftAus();
}

}  // namespace bhed::gui::karteneditor
