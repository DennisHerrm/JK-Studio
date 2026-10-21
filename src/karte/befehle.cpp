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

Belegung::Belegung() {
    // g_shortcuts ist eine std::map: nach Namen sortiert (CopiedString <).
    std::vector<const Befehl*> sortiert;
    for (const Befehl& b : alle()) { sortiert.push_back(&b); }
    std::sort(sortiert.begin(), sortiert.end(),
              [](const Befehl* a, const Befehl* b) { return std::string_view(a->name) < std::string_view(b->name); });
    for (const Befehl* b : sortiert) {
        m_namen.emplace_back(b->name);
        const Taste t = tasteAusText(b->taste);
        m_tasten.push_back(t);
        m_standard.push_back(t);
    }
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
    for (const auto& [name, text] : eigene) {
        const std::ptrdiff_t i = index(name);
        if (i < 0) { continue; }
        if (text.empty()) {
            m_tasten[static_cast<std::size_t>(i)] = 0;
            continue;
        }
        const Taste t = tasteAusText(text);
        if (t != 0) {
            m_tasten[static_cast<std::size_t>(i)] = t;
        } else {
            meldungen.push_back("WARNING: failed to parse user command \"" + name + "\": unknown key \"" + text + "\"");
        }
    }
    return meldungen;
}

std::vector<std::pair<std::string, std::string>> Belegung::abweichende() const {
    std::vector<std::pair<std::string, std::string>> v;
    for (std::size_t i = 0; i < m_namen.size(); ++i) {
        if (m_tasten[i] != m_standard[i]) { v.emplace_back(m_namen[i], tasteAlsText(m_tasten[i])); }
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

void Belegung::setzen(std::string_view name, Taste t) {
    const std::ptrdiff_t i = index(name);
    if (i >= 0) { m_tasten[static_cast<std::size_t>(i)] = t; }
}

void Belegung::zuruecksetzen(std::string_view name) {
    const std::ptrdiff_t i = index(name);
    if (i >= 0) { m_tasten[static_cast<std::size_t>(i)] = m_standard[static_cast<std::size_t>(i)]; }
}

void Belegung::alleZuruecksetzen() { m_tasten = m_standard; }

std::string Belegung::belegtVon(Taste t, std::string_view ausser) const {
    if (t == 0) { return std::string(); }
    for (std::size_t i = 0; i < m_namen.size(); ++i) {
        if (m_tasten[i] == t && m_namen[i] != ausser) { return m_namen[i]; }
    }
    return std::string();
}

std::vector<std::string> Belegung::treffer(const std::vector<Taste>& moegliche) const {
    std::vector<std::string> v;
    for (const Taste t : moegliche) {
        for (std::size_t i = 0; i < m_namen.size(); ++i) {
            if (m_tasten[i] == t && std::find(v.begin(), v.end(), m_namen[i]) == v.end()) { v.push_back(m_namen[i]); }
        }
    }
    return v;
}

}  // namespace bhed::karte::befehle
