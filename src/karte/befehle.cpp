// befehle.cpp - Karten-Editor: Befehlstabelle und Tastenkuerzel wie NRC/Qt.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/commands.cpp,
// radiant/mainframe.cpp, radiant/filterbar.cpp, radiant/gtkmisc.cpp; die
// Tastenlogik nach Qt 5 (qkeysequence.cpp, qwindowskeymapper.cpp,
// qshortcutmap.cpp), wie NRC sie ueber QAction-Kuerzel benutzt.
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/befehle.h"

#include <algorithm>
#include <cctype>
#include <tuple>
#include <utility>

namespace bhed::karte::befehle {

using i18n::Str;

namespace {

const Befehl kBefehle[] = {
#include "befehle_liste.inc"
};

#include "menues.inc"

// qkeysequence.cpp keyname[]: der erste Eintrag ist der Name fuer toString,
// die "More consistent namings" (Insert, Delete, Escape, Page Up ...) werden
// beim Lesen ebenfalls erkannt.
struct KeyName {
    std::uint32_t key;
    const char* name;
};
const KeyName kKeyNames[] = {
    {0x20, "Space"},
    {kKeyEscape, "Esc"},
    {kKeyTab, "Tab"},
    {0x01000002, "Backtab"},
    {kKeyBackspace, "Backspace"},
    {kKeyReturn, "Return"},
    {kKeyEnter, "Enter"},
    {kKeyInsert, "Ins"},
    {kKeyDelete, "Del"},
    {0x01000008, "Pause"},
    {0x01000009, "Print"},
    {0x0100000a, "SysReq"},
    {kKeyHome, "Home"},
    {kKeyEnd, "End"},
    {kKeyLeft, "Left"},
    {kKeyUp, "Up"},
    {kKeyRight, "Right"},
    {kKeyDown, "Down"},
    {kKeyPageUp, "PgUp"},
    {kKeyPageDown, "PgDown"},
    {0x01000024, "CapsLock"},
    {0x01000025, "NumLock"},
    {0x01000026, "ScrollLock"},
    {0x01000055, "Menu"},
    {0x01000058, "Help"},
    // More consistent namings
    {0x01000009, "Print Screen"},
    {kKeyPageUp, "Page Up"},
    {kKeyPageDown, "Page Down"},
    {0x01000024, "Caps Lock"},
    {0x01000025, "Num Lock"},
    {0x01000025, "Number Lock"},
    {0x01000026, "Scroll Lock"},
    {kKeyInsert, "Insert"},
    {kKeyDelete, "Delete"},
    {kKeyEscape, "Escape"},
    {0x0100000a, "System Request"},
};

bool gleichOhneFall(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) {
            return false;
        }
    }
    return true;
}

// ModsTbl (qwindowskeymapper.cpp)
const std::uint32_t kModsTbl[9] = {
    0, kShift, kCtrl, kCtrl | kShift, kAlt, kAlt | kShift, kAlt | kCtrl, kAlt | kShift | kCtrl, 0,
};

}  // namespace

// QKeySequencePrivate::decodeString (PortableText)
Taste tasteAusText(std::string_view text) {
    while (!text.empty() && text.front() == ' ') { text.remove_prefix(1); }
    while (!text.empty() && text.back() == ' ') { text.remove_suffix(1); }
    if (text.empty()) { return 0; }
    std::uint32_t mods = 0;
    // Zusatztasten vorn: "ctrl+", "shift+", "alt+", "meta+", "num+".
    for (;;) {
        bool weiter = false;
        for (const auto& [name, bit] : {std::pair<const char*, std::uint32_t>{"ctrl+", kCtrl}, {"shift+", kShift},
                                        {"alt+", kAlt}, {"meta+", kMeta}, {"num+", kNum}}) {
            const std::size_t n = std::char_traits<char>::length(name);
            // "Num++" heisst Ziffernblock-Plus, "Shift++" Umschalt+Plus: das
            // letzte Zeichen bleibt immer die Taste.
            if (text.size() > n && gleichOhneFall(text.substr(0, n), name)) {
                mods |= bit;
                text.remove_prefix(n);
                weiter = true;
                break;
            }
        }
        if (!weiter) { break; }
    }
    if (text.empty()) { return 0; }
    std::uint32_t key = 0;
    if (text.size() == 1) {
        key = static_cast<unsigned char>(std::toupper(static_cast<unsigned char>(text[0])));
    } else if ((text[0] == 'F' || text[0] == 'f') && text.size() <= 3 &&
               std::all_of(text.begin() + 1, text.end(), [](char c) { return c >= '0' && c <= '9'; })) {
        const int n = std::stoi(std::string(text.substr(1)));
        if (n < 1 || n > 35) { return 0; }
        key = kKeyF1 + static_cast<std::uint32_t>(n - 1);
    } else {
        for (const KeyName& k : kKeyNames) {
            if (gleichOhneFall(text, k.name)) {
                key = k.key;
                break;
            }
        }
    }
    if (key == 0) { return 0; }
    return key | mods;
}

// QKeySequencePrivate::encodeString (PortableText): Meta+Ctrl+Alt+Shift+Num+Taste
std::string tasteAlsText(Taste t) {
    if (t == 0) { return std::string(); }
    std::string s;
    const auto dazu = [&s](const char* n) {
        if (!s.empty()) { s += '+'; }
        s += n;
    };
    if ((t & kMeta) != 0U) { dazu("Meta"); }
    if ((t & kCtrl) != 0U) { dazu("Ctrl"); }
    if ((t & kAlt) != 0U) { dazu("Alt"); }
    if ((t & kShift) != 0U) { dazu("Shift"); }
    if ((t & kNum) != 0U) { dazu("Num"); }
    const std::uint32_t key = t & ~kModMaske;
    std::string name;
    if (key != 0 && key < kKeyEscape && key != 0x20) {
        name.assign(1, static_cast<char>(key));
    } else if (key >= kKeyF1 && key < kKeyF1 + 35) {
        name = "F" + std::to_string(key - kKeyF1 + 1);
    } else {
        for (const KeyName& k : kKeyNames) {
            if (k.key == key) {
                name = k.name;
                break;
            }
        }
    }
    if (!s.empty()) { s += '+'; }
    return s + name;
}

// QWindowsKeyMapper::possibleKeys (Qt 5.15, qwindowskeymapper.cpp)
std::vector<Taste> moeglicheTasten(const TastenBelegung& kb, std::uint32_t keyMods, bool erweitertesReturn) {
    std::vector<Taste> result;
    if (!kb.vorhanden) { return result; }
    const std::uint32_t baseKey = kb.qtKey[0];
    if (baseKey == kKeyReturn && erweitertesReturn) {
        result.push_back(kKeyEnter + keyMods);
        return result;
    }
    result.push_back(baseKey + keyMods);   // The base key is _always_ valid, of course
    for (int i = 1; i < 9; ++i) {
        const std::uint32_t neededMods = kModsTbl[i];
        const std::uint32_t key = kb.qtKey[static_cast<std::size_t>(i)];
        if (key != 0U && key != baseKey && ((keyMods & neededMods) == neededMods)) {
            const std::uint32_t missingMods = keyMods & ~neededMods;
            const std::uint32_t matchedKey = key + missingMods;
            const auto it = std::find_if(result.begin(), result.end(),
                                         [key](std::uint32_t k) { return (k & ~kModMaske) == key; });
            // QTBUG-67200: Use the match with the least modifiers (prefer
            // Shift+9 over Alt + Shift + 9) resulting in more missing modifiers.
            if (it == result.end()) {
                result.push_back(matchedKey);
            } else if (missingMods > (*it & kModMaske)) {
                *it = matchedKey;
            }
        }
    }
    return result;
}

const std::vector<Befehl>& alle() {
    static const std::vector<Befehl> v(std::begin(kBefehle), std::end(kBefehle));
    return v;
}

const std::vector<std::string>& stufe2() {
    static const std::vector<std::string> v = {
#include "befehle_stufe2.inc"
    };
    return v;
}

const Befehl* finden(std::string_view name) {
    for (const Befehl& b : alle()) {
        if (name == b.name) { return &b; }
    }
    return nullptr;
}

bool bedingungGilt(Bedingung b, Anordnung a, bool gruppenDialogEingebaut) {
    const bool regular = a == Anordnung::Regular || a == Anordnung::RegularLeft;
    switch (b) {
    case kImmer: return true;
    case kNurFloating: return a == Anordnung::Floating;
    case kNichtRegular: return !regular;
    case kTexturen: return !regular || gruppenDialogEingebaut;
    case kNichtSplit: return a != Anordnung::Split;
    case kNurSplit: return a == Anordnung::Split;
    }
    return true;
}

const std::vector<MenueEintrag>& menues() {
    static const std::vector<MenueEintrag> v(std::begin(kMenues), std::end(kMenues));
    return v;
}

const std::vector<MenueEintrag>& texturMenues() {
    static const std::vector<MenueEintrag> v(std::begin(kTexturMenues), std::end(kTexturMenues));
    return v;
}

const std::vector<Knopf>& texturKnoepfe() {
    static const std::vector<Knopf> v(std::begin(kTexturKnoepfe), std::end(kTexturKnoepfe));
    return v;
}

const std::vector<Knopf>& werkzeugleiste() {
    static const std::vector<Knopf> v(std::begin(kWerkzeuge), std::end(kWerkzeuge));
    return v;
}

const std::vector<Knopf>& filterleiste() {
    static const std::vector<Knopf> v(std::begin(kFilter), std::end(kFilter));
    return v;
}

// create_plugin_toolbar (plugintoolbar.cpp:91-120): die Knoepfe der Plugins.
// Fuer Jedi Academy / Jedi Outcast bringt nur bobToolz welche mit
// (contrib/bobtoolz/bobToolz-GTK.cpp:184-279; ufoaiplug: ToolbarNoButtons).
// eSpace = Trenner; Befehlsname plugin_construct_command_name
// (pluginmanager.cpp:43-70): "bobToolz::" + Text ohne Leerzeichen, Woerter
// gross. Die Plugins kommen in Stufe 3.
const std::vector<Knopf>& pluginleiste() {
    using i18n::Str;
    const auto z = [](Str s) { return std::array<Str, 5>{s, Str::Count, Str::Count, Str::Count, Str::Count}; };
    const std::array<Str, 5> leer{Str::Count, Str::Count, Str::Count, Str::Count, Str::Count};
    static const std::vector<Knopf> v = {
        {"", "", "", false, kImmer, "", true, leer},
        {"bobToolz::PolygonBuilder", "Polygons", "bobtoolz_poly", false, kImmer, "", false, z(Str::KePlPolygons)},
        {"", "", "", false, kImmer, "", true, leer},
        {"bobToolz::TreePlanter", "Tree Planter", "bobtoolz_treeplanter", true, kImmer, "", false, z(Str::KePlTreePlanter)},
        {"bobToolz::PlotSplines", "Plot Splines", "bobtoolz_trainpathplot", false, kImmer, "", false, z(Str::KePlPlotSplines)},
        {"bobToolz::DropEntity", "Drop Entity", "bobtoolz_dropent", false, kImmer, "", false, z(Str::KePlDropEntity)},
        {"", "", "", false, kImmer, "", true, leer},
        {"bobToolz::MergePatches", "Merge Patches", "bobtoolz_merge", false, kImmer, "", false, z(Str::KePlMergePatches)},
        {"bobToolz::SplitPatch", "Split Patch", "bobtoolz_split", false, kImmer, "", false, z(Str::KePlSplitPatch)},
        {"bobToolz::SplitPatchRows", "Split Patch Rows", "bobtoolz_splitrow", false, kImmer, "", false, z(Str::KePlSplitPatchRows)},
        {"bobToolz::SplitPatchColumns", "Split Patch Columns", "bobtoolz_splitcol", false, kImmer, "", false,
         z(Str::KePlSplitPatchColumns)},
        {"", "", "", false, kImmer, "", true, leer},
        {"bobToolz::FlipTerrain", "Flip Terrain (Turn Edge)", "bobtoolz_turnedge", false, kImmer, "", false, z(Str::KePlFlipTerrain)},
    };
    return v;
}

const std::vector<PluginEintrag>& pluginEintraege() {
    static const std::vector<PluginEintrag> v(std::begin(kPluginEintraege), std::end(kPluginEintraege));
    return v;
}

const std::vector<std::string>& standardVersteckt() {
    static const std::vector<std::string> v(std::begin(kVersteckt), std::end(kVersteckt));
    return v;
}

// toolbar_action_set_tooltip (gtkmisc.cpp:305-321)
std::string knopfTooltip(const std::vector<std::string>& zeilen, const std::string& taste) {
    std::string out;
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        if (i > 0) { out += '\n'; }
        out += zeilen[i];
        if (i == 0 && !taste.empty()) { out += " (" + taste + ")"; }
    }
    return out;
}

// --- Belegung -----------------------------------------------------------------

namespace {

// rc612 NUMPAD: die Vorgaben eines Befehls aus seiner NRC-Taste. raster: eine
// Ziffer ohne Zusatztaste bekommt "Num+<Ziffer>" als Zweittaste, und eine
// NRC-Vorgabe "Num+<Ziffer>" entfaellt (die Taste gehoert dann dem Raster).
std::pair<Taste, Taste> vorgaben(Taste nrc, bool raster) {
    if (!raster) { return {nrc, 0}; }
    const Taste k = nrc & ~kModMaske;
    const bool ziffer = k >= '0' && k <= '9';
    if (ziffer && (nrc & kModMaske) == 0U) { return {nrc, kNum | k}; }
    if (ziffer && (nrc & kModMaske) == kNum) { return {0, 0}; }
    return {nrc, 0};
}

// Beide Tasten eines Befehls als Text ("1 / Num+1"); dieselbe Taste nur einmal.
std::string beideAlsText(Taste erste, Taste zweite) {
    std::string s = tasteAlsText(erste);
    if (zweite != 0 && zweite != erste) { s += (s.empty() ? "" : " / ") + tasteAlsText(zweite); }
    return s;
}

}  // namespace

Belegung::Belegung() {
    // g_shortcuts ist eine std::map: nach Namen sortiert (CopiedString <).
    std::vector<const Befehl*> sortiert;
    for (const Befehl& b : alle()) { sortiert.push_back(&b); }
    std::sort(sortiert.begin(), sortiert.end(),
              [](const Befehl* a, const Befehl* b) { return std::string_view(a->name) < std::string_view(b->name); });
    for (const Befehl* b : sortiert) {
        m_namen.emplace_back(b->name);
        m_nrc.push_back(tasteAusText(b->taste));
        const auto [t, z] = vorgaben(m_nrc.back(), m_zbRaster);
        m_tasten.push_back(t);
        m_standard.push_back(t);
        m_zweit.push_back(z);
        m_zweitStandard.push_back(z);
    }
}

void Belegung::ziffernblockRaster(bool raster) {
    m_zbRaster = raster;
    const std::size_t n = m_namen.size();
    // Eigene Belegungen bleiben; was der Vorgabe folgte, folgt der neuen.
    std::vector<char> eigenErste(n);
    std::vector<char> eigenZweite(n);
    const std::vector<Taste> altErste = m_standard;
    const std::vector<Taste> altZweite = m_zweitStandard;
    for (std::size_t i = 0; i < n; ++i) {
        eigenErste[i] = m_tasten[i] != m_standard[i] ? 1 : 0;
        eigenZweite[i] = m_zweit[i] != m_zweitStandard[i] ? 1 : 0;
        std::tie(m_standard[i], m_zweitStandard[i]) = vorgaben(m_nrc[i], raster);
    }
    // Eine Vorgabe, die mit dem Schalter neu auf eine Taste kommt, weicht der
    // eigenen Belegung eines anderen Befehls: sonst waere die Taste
    // mehrdeutig und loeste nichts mehr aus (QShortcutMap::find).
    const auto eigene = [&](Taste t, std::size_t ausser) {
        if (t == 0) { return false; }
        for (std::size_t j = 0; j < n; ++j) {
            if (j == ausser) { continue; }
            if ((eigenErste[j] != 0 && m_tasten[j] == t) || (eigenZweite[j] != 0 && m_zweit[j] == t)) { return true; }
        }
        return false;
    };
    for (std::size_t i = 0; i < n; ++i) {
        if (eigenErste[i] == 0 && m_standard[i] != altErste[i]) { m_tasten[i] = eigene(m_standard[i], i) ? 0 : m_standard[i]; }
        if (eigenZweite[i] == 0 && m_zweitStandard[i] != altZweite[i]) {
            m_zweit[i] = eigene(m_zweitStandard[i], i) ? 0 : m_zweitStandard[i];
        }
    }
}

bool Belegung::vorgabeWieNrc(std::string_view name) const {
    const std::ptrdiff_t i = index(name);
    return i < 0 || (m_standard[static_cast<std::size_t>(i)] == m_nrc[static_cast<std::size_t>(i)] &&
                     m_zweitStandard[static_cast<std::size_t>(i)] == 0);
}

std::ptrdiff_t Belegung::index(std::string_view name) const {
    const auto it = std::lower_bound(m_namen.begin(), m_namen.end(), name,
                                     [](const std::string& a, std::string_view b) { return std::string_view(a) < b; });
    if (it == m_namen.end() || *it != name) { return -1; }
    return it - m_namen.begin();
}

// ReadCommandMap (commands.cpp:455-485)
std::vector<std::string> Belegung::laden(const std::vector<std::pair<std::string, std::string>>& eigene) {
    std::vector<std::string> meldungen;
    for (const auto& [eintrag, text] : eigene) {
        // rc612 NUMPAD: "<Befehl>#Num" ist die Zweittaste des Befehls
        const bool zweit = eintrag.size() > kZweitEndung.size() &&
                           std::string_view(eintrag).substr(eintrag.size() - kZweitEndung.size()) == kZweitEndung;
        const std::string name = zweit ? eintrag.substr(0, eintrag.size() - kZweitEndung.size()) : eintrag;
        const std::ptrdiff_t i = index(name);
        if (i < 0) { continue; }
        std::vector<Taste>& ziel = zweit ? m_zweit : m_tasten;
        if (text.empty()) {
            ziel[static_cast<std::size_t>(i)] = 0;
            continue;
        }
        const Taste t = tasteAusText(text);
        if (t != 0) {
            ziel[static_cast<std::size_t>(i)] = t;
        } else {
            meldungen.push_back("WARNING: failed to parse user command \"" + eintrag + "\": unknown key \"" + text + "\"");
        }
    }
    return meldungen;
}

std::vector<std::pair<std::string, std::string>> Belegung::abweichende() const {
    std::vector<std::pair<std::string, std::string>> v;
    for (std::size_t i = 0; i < m_namen.size(); ++i) {
        if (m_tasten[i] != m_standard[i]) { v.emplace_back(m_namen[i], tasteAlsText(m_tasten[i])); }
        if (m_zweit[i] != m_zweitStandard[i]) { v.emplace_back(m_namen[i] + std::string(kZweitEndung), tasteAlsText(m_zweit[i])); }
    }
    return v;
}

Taste Belegung::taste(std::string_view name) const {
    const std::ptrdiff_t i = index(name);
    return i < 0 ? 0 : m_tasten[static_cast<std::size_t>(i)];
}

Taste Belegung::standard(std::string_view name) const {
    const std::ptrdiff_t i = index(name);
    return i < 0 ? 0 : m_standard[static_cast<std::size_t>(i)];
}

Taste Belegung::zweitTaste(std::string_view name) const {
    const std::ptrdiff_t i = index(name);
    return i < 0 ? 0 : m_zweit[static_cast<std::size_t>(i)];
}

Taste Belegung::zweitStandard(std::string_view name) const {
    const std::ptrdiff_t i = index(name);
    return i < 0 ? 0 : m_zweitStandard[static_cast<std::size_t>(i)];
}

std::string Belegung::text(std::string_view name) const { return beideAlsText(taste(name), zweitTaste(name)); }

std::string Belegung::standardText(std::string_view name) const { return beideAlsText(standard(name), zweitStandard(name)); }

bool Belegung::geaendert(std::string_view name) const {
    return taste(name) != standard(name) || zweitTaste(name) != zweitStandard(name);
}

void Belegung::setzen(std::string_view name, Taste t) {
    const std::ptrdiff_t i = index(name);
    if (i >= 0) { m_tasten[static_cast<std::size_t>(i)] = t; }
}

void Belegung::leeren(std::string_view name) {
    const std::ptrdiff_t i = index(name);
    if (i < 0) { return; }
    m_tasten[static_cast<std::size_t>(i)] = 0;
    m_zweit[static_cast<std::size_t>(i)] = 0;
}

void Belegung::loesen(std::string_view name, Taste t) {
    const std::ptrdiff_t i = index(name);
    if (i < 0) { return; }
    if (m_tasten[static_cast<std::size_t>(i)] == t) { m_tasten[static_cast<std::size_t>(i)] = 0; }
    if (m_zweit[static_cast<std::size_t>(i)] == t) { m_zweit[static_cast<std::size_t>(i)] = 0; }
}

void Belegung::zuruecksetzen(std::string_view name) {
    const std::ptrdiff_t i = index(name);
    if (i < 0) { return; }
    m_tasten[static_cast<std::size_t>(i)] = m_standard[static_cast<std::size_t>(i)];
    m_zweit[static_cast<std::size_t>(i)] = m_zweitStandard[static_cast<std::size_t>(i)];
}

void Belegung::alleZuruecksetzen() {
    m_tasten = m_standard;
    m_zweit = m_zweitStandard;
}

std::string Belegung::belegtVon(Taste t, std::string_view ausser) const {
    if (t == 0) { return std::string(); }
    for (std::size_t i = 0; i < m_namen.size(); ++i) {
        if ((m_tasten[i] == t || m_zweit[i] == t) && m_namen[i] != ausser) { return m_namen[i]; }
    }
    return std::string();
}

std::vector<std::string> Belegung::treffer(const std::vector<Taste>& moegliche) const {
    std::vector<std::string> v;
    for (const Taste t : moegliche) {
        for (std::size_t i = 0; i < m_namen.size(); ++i) {
            if ((m_tasten[i] == t || m_zweit[i] == t) && std::find(v.begin(), v.end(), m_namen[i]) == v.end()) {
                v.push_back(m_namen[i]);
            }
        }
    }
    return v;
}

}  // namespace bhed::karte::befehle
