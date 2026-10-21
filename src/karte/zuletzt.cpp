// zuletzt.cpp - Karten-Editor: zuletzt geoeffnete Karten.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/mru.cpp.
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
#include "bhed/karte/zuletzt.h"

#include <algorithm>

namespace bhed::karte {

// MRU_AddFile (mru.cpp:95-125)
void ZuletztListe::hinzufuegen(const std::string& pfad) {
    // check if file is already in our list (strcmp: Gross/klein zaehlt)
    const auto it = std::find(m_pfade.begin(), m_pfade.end(), pfad);
    if (it != m_pfade.end()) {
        // reorder menu
        m_pfade.erase(it);
        m_pfade.insert(m_pfade.begin(), pfad);
        return;
    }
    // move items down
    m_pfade.insert(m_pfade.begin(), pfad);
    if (m_pfade.size() > kMax) { m_pfade.pop_back(); }
    m_geleert = false;
}

// MRU_Activate (mru.cpp:144-170)
std::string ZuletztListe::aktivieren(std::size_t index, bool lesbar) {
    if (index >= m_pfade.size()) { return std::string(); }
    const std::string text = m_pfade[index];
    if (lesbar) {   //\todo Test 'map load succeeds' instead of 'file is readable'.
        hinzufuegen(text);
        return text;
    }
    m_pfade.erase(m_pfade.begin() + static_cast<std::ptrdiff_t>(index));
    if (m_pfade.empty()) { m_geleert = true; }   // "Recent Files", ausgegraut
    return std::string();
}

// MRU_Init (mru.cpp:127-131) und die Preferences "Count"/"File0".."File8"
void ZuletztListe::setzen(const std::vector<std::string>& pfade) {
    m_pfade.clear();
    for (const std::string& p : pfade) {
        if (m_pfade.size() == kMax) { break; }
        m_pfade.push_back(p);
    }
    m_geleert = false;
}

// MRU_updateWidget + EscapedMnemonic (mru.cpp:48-87)
std::string ZuletztListe::beschriftung(std::size_t index) const {
    if (index >= m_pfade.size()) {
        if (index == 0) { return m_geleert ? "Recent Files" : "&1"; }
        return "&" + std::to_string(index + 1);
    }
    std::string m = "&" + std::to_string(index + 1) + "- ";
    for (const char c : m_pfade[index]) {
        if (c == '&') { m.push_back('&'); }
        m.push_back(c);
    }
    return m;
}

std::string ohneMnemonik(const std::string& text) {
    std::string out;
    out.reserve(text.size());
    for (std::size_t i = 0; i < text.size(); ++i) {
        if (text[i] == '&' && i + 1 < text.size()) {
            ++i;   // "&&" -> "&", "&x" -> "x"
        }
        out.push_back(text[i]);
    }
    return out;
}

}  // namespace bhed::karte
