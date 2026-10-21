// texturbrowser.cpp - Karten-Editor: der Texturbrowser ohne Oberflaeche.
// Portiert aus NetRadiant-Custom 20260114 (Quellen in texturbrowser.h).
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
#include "bhed/karte/texturbrowser.h"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace bhed::karte {

namespace {

int vergleichOhneFall(const std::string& a, const std::string& b) {   // string_compare_nocase
    const std::size_t n = std::min(a.size(), b.size());
    for (std::size_t i = 0; i < n; ++i) {
        const int ca = std::tolower(static_cast<unsigned char>(a[i]));
        const int cb = std::tolower(static_cast<unsigned char>(b[i]));
        if (ca != cb) { return ca < cb ? -1 : 1; }
    }
    return a.size() < b.size() ? -1 : (a.size() > b.size() ? 1 : 0);
}

bool praefixOhneFall(const std::string& s, const std::string& p) {   // shader_equal_prefix / string_equal_prefix_nocase
    if (p.size() > s.size()) { return false; }
    for (std::size_t i = 0; i < p.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(s[i])) != std::tolower(static_cast<unsigned char>(p[i]))) { return false; }
    }
    return true;
}

bool gleichOhneFall(const std::string& a, const std::string& b) { return a.size() == b.size() && praefixOhneFall(a, b); }   // shader_equal

bool enthaeltOhneFall(const std::string& s, const std::string& teil) {   // string_in_string_nocase
    if (teil.empty()) { return true; }
    for (std::size_t i = 0; i + teil.size() <= s.size(); ++i) {
        if (praefixOhneFall(s.substr(i), teil)) { return true; }
    }
    return false;
}

std::string ohneEndung(const std::string& name) {   // PathExtensionless
    const std::size_t s = name.find_last_of('/');
    const std::size_t p = name.find_last_of('.');
    if (p == std::string::npos || (s != std::string::npos && p < s)) { return name; }
    return name.substr(0, p);
}

// TextureGroups_compare (texwindow.cpp:95-104): ohne Gross/klein, bei
// Gleichheit nach Bytes (fuer Dateisysteme mit Gross/klein).
struct GruppenKleiner {
    bool operator()(const std::string& a, const std::string& b) const {
        const int c = vergleichOhneFall(a, b);
        if (c != 0) { return c < 0; }
        return a < b;
    }
};

// TextureGroups_addShader (texwindow.cpp:120-128)
void gruppeAusShader(std::set<std::string, GruppenKleiner>& gruppen, const std::string& name) {
    if (!praefixOhneFall(name, kTexturPraefix)) { return; }   // path_make_relative
    const std::string textur = name.substr(std::strlen(kTexturPraefix));
    const std::size_t schraeg = textur.find('/');   // path_remove_directory
    if (schraeg != std::string::npos && schraeg + 1 < textur.size()) { gruppen.insert(textur.substr(0, schraeg)); }
}

}  // namespace

bool ShaderKleiner::operator()(const std::string& a, const std::string& b) const { return vergleichOhneFall(a, b) < 0; }

// --- plugins/shaders/shaders.cpp --------------------------------------------------

// Try_Shader_ForName (shaders.cpp:1488-1516) mit CShader::realise
// (shaders.cpp:946-962): das Editorbild oder notex.png / shadernotex.png.
AktiverShader& AktiveShader::holen(const std::string& name, const Texturinventar* inventar) {
    const auto it = m_liste.find(name);
    if (it != m_liste.end()) { return it->second; }
    AktiverShader s;
    s.name = name;
    const ShaderEintrag* def = inventar != nullptr ? inventar->shaderFinden(name) : nullptr;
    if (def != nullptr) {
        s.standard = def->istStandard();
        s.datei = def->datei;
        s.texturName = def->texturName;
    } else {   // ShaderTemplate::CreateDefault (g_enableDefaultShaders)
        s.standard = true;
        s.texturName = name;
    }
    if (inventar != nullptr) {
        const Texturinventar::BildMass m = inventar->bildMass(name);
        s.breite = m.breite;
        s.hoehe = m.hoehe;
        s.bildFehlt = !m.gefunden;
    } else {
        s.bildFehlt = true;
    }
    ++m_stand;   // g_ActiveShadersChangedNotify
    return m_liste.emplace(name, std::move(s)).first->second;
}

void AktiveShader::leeren() {   // FreeShaders (shaders.cpp:1146-1155)
    m_liste.clear();
    ++m_stand;
}

const AktiverShader* AktiveShader::finden(const std::string& name) const {
    const auto it = m_liste.find(name);
    return it != m_liste.end() ? &it->second : nullptr;
}

// OpenGLShader::incrementUsed / decrementUsed (renderstate.cpp:666-675) ->
// CShader::SetInUse (shaders.cpp:918-921)
bool AktiveShader::gebrauchSetzen(const std::set<std::string, ShaderKleiner>& imGebrauch) {
    bool geaendert = false;
    for (auto& [n, s] : m_liste) {
        const bool an = imGebrauch.count(n) != 0U;
        if (s.imGebrauch != an) {
            s.imGebrauch = an;
            geaendert = true;
        }
    }
    if (geaendert) { ++m_stand; }
    return geaendert;
}

// --- radiant/texwindow.cpp ----------------------------------------------------------

std::string shaderTexturName(const std::string& name) {   // shader_get_textureName (shaderlib.h)
    const std::size_t n = std::strlen(kTexturPraefix);
    return name.size() >= n ? name.substr(n) : std::string();
}

std::string pfadDateiname(const std::string& name) {   // path_get_filename_start
    const std::size_t s = name.find_last_of('/');
    return s == std::string::npos ? name : name.substr(s + 1);
}

int texturSkalaAusIndex(int index) {   // TextureScaleImport (texwindow.cpp:1988-2007)
    static const int werte[5] = {10, 25, 50, 100, 200};
    return index >= 0 && index < 5 ? werte[index] : -1;
}

int texturSkalaIndex(int skala) {   // TextureScaleExport (texwindow.cpp:2010-2029)
    static const int werte[5] = {10, 25, 50, 100, 200};
    for (int i = 0; i < 5; ++i) {
        if (werte[i] == skala) { return i; }
    }
    return -1;
}

// TextureLayout::nextPos (texwindow.cpp:413-435)
std::pair<int, int> TexturAnordnung::naechste(const TexturBrowser& tb, int b, int h) {
    if (x + b > tb.breite - 8 && zeile != 0) {   // go to the next row unless the texture is the first on the row
        x = 8;
        y -= zeile + tb.schriftHoehe + 1;
        zeile = 0;
    }
    const int px = x;
    const int py = y;
    if (zeile < h) { zeile = h; }   // Is our texture larger than the row?
    x += std::max(b, 96) + 8;       // never go less than 96, or the names get all crunched up
    return {px, py};
}

// TextureBrowser::getTextureWH (texwindow.cpp:300-330)
std::pair<int, int> TexturBrowser::texturMass(const AktiverShader& s) const {
    const auto skala = static_cast<std::size_t>(std::max(0, e.skala));
    int b = static_cast<int>(std::max(std::size_t(1), s.breite * skala / 100));
    int h = static_cast<int>(std::max(std::size_t(1), s.hoehe * skala / 100));
    if (e.festeGroesse) {
        if (b >= h) {   // Texture is square, or wider than it is tall
            if (b > e.einheitsGroesse) {
                h = e.einheitsGroesse * h / b;
                b = e.einheitsGroesse;
            } else if (b < e.einheitsMin) {
                h = e.einheitsMin * h / b;
                b = e.einheitsMin;
            }
        } else {   // Texture taller than it is wide
            if (h > e.einheitsGroesse) {
                b = e.einheitsGroesse * b / h;
                h = e.einheitsGroesse;
            } else if (h < e.einheitsMin) {
                b = e.einheitsMin * b / h;
                h = e.einheitsMin;
            }
        }
    }
    return {b, h};
}

// Texture_filtered (texwindow.cpp:438-454)
bool TexturBrowser::gefiltert(const std::string& name) const {
    if (filter.empty()) { return false; }
    if (e.vonAnfang) {
        if (praefixOhneFall(name, filter)) { return false; }
    } else if (enthaeltOhneFall(name, filter)) {
        return false;
    }
    return true;
}

// Texture_IsShown (texwindow.cpp:464-505); m_searchedTags (Tags) mit Stufe 2.
bool TexturBrowser::gezeigt(const AktiverShader& s) const {
    if (e.ohneNotex && s.bildFehlt) { return false; }   // filter notex / shadernotex images
    if (!praefixOhneFall(s.name, kTexturPraefix)) { return false; }
    if (!e.shaderZeigen && !s.standard) { return false; }
    if (!e.texturenZeigen && s.standard) { return false; }
    if (unbenutzteVerstecken && !s.imGebrauch) { return false; }
    if (e.commonOhneTexturen && s.standard && !s.imGebrauch && praefixOhneFall(shaderTexturName(s.name), commonVerzeichnis)) {
        return false;
    }
    if (!praefixOhneFall(shaderTexturName(s.name), verzeichnis)) { return false; }
    return !gefiltert(pfadDateiname(s.name));
}

std::vector<TexturLage> TexturBrowser::anordnen(const AktiveShader& aktiv) const {
    std::vector<TexturLage> v;
    TexturAnordnung lage;
    for (const auto& [n, s] : aktiv.alle()) {
        (void)n;
        if (!gezeigt(s)) { continue; }
        const auto [b, h] = texturMass(s);
        const auto [x, y] = lage.naechste(*this, b, h);
        TexturLage l;
        l.shader = &s;
        l.x = x;
        l.y = y;
        l.b = b;
        l.h = h;
        v.push_back(l);
    }
    return v;
}

// TextureBrowser::draw (texwindow.cpp:919-1067): "Is this texture visible?"
// und die Rahmen (gewaehlt rot, benutzt hellgruen, Shader weiss + Strichel).
std::vector<TexturLage> TexturBrowser::sichtbare(const AktiveShader& aktiv) {
    (void)gesamtHoehe(aktiv);   // evaluateHeight
    const int o = ursprungY(aktiv);
    std::vector<TexturLage> v;
    for (TexturLage l : anordnen(aktiv)) {
        if (!(l.y - l.h - schriftHoehe < o && l.y > o - hoehe)) { continue; }
        if (gleichOhneFall(shader, l.shader->name)) {
            l.rand = TexturLage::Rand::Gewaehlt;
        } else if (!unbenutzteVerstecken && l.shader->imGebrauch) {
            l.rand = TexturLage::Rand::ImGebrauch;
        } else if (!l.shader->standard) {
            l.rand = TexturLage::Rand::Shader;
        }
        l.strichel = !l.shader->standard;
        v.push_back(l);
    }
    return v;
}

void TexturBrowser::standPruefen(const AktiveShader& aktiv) {   // TextureBrowser_activeShadersChanged
    if (aktiv.stand() != m_stand) {
        m_stand = aktiv.stand();
        hoeheGeaendert();
    }
}

// TextureBrowser::evaluateHeight (texwindow.cpp:507-525)
int TexturBrowser::gesamtHoehe(const AktiveShader& aktiv) {
    standPruefen(aktiv);
    if (m_hoeheGeaendert) {
        m_hoeheGeaendert = false;
        m_gesamtHoehe = 0;
        TexturAnordnung lage;
        for (const auto& [n, s] : aktiv.alle()) {
            (void)n;
            if (!gezeigt(s)) { continue; }
            const auto [b, h] = texturMass(s);
            (void)lage.naechste(*this, b, h);
            m_gesamtHoehe = std::max(m_gesamtHoehe, std::abs(lage.y) + schriftHoehe + h + 4);
        }
    }
    return m_gesamtHoehe;
}

void TexturBrowser::klemmen(const AktiveShader& aktiv) {   // clampOriginY
    const int minimum = std::min(hoehe - gesamtHoehe(aktiv), 0);
    m_ursprungY = std::clamp(m_ursprungY, minimum, 0);
}

int TexturBrowser::ursprungY(const AktiveShader& aktiv) {   // getOriginY
    standPruefen(aktiv);
    if (m_ursprungUngueltig) {
        m_ursprungUngueltig = false;
        klemmen(aktiv);
    }
    return m_ursprungY;
}

void TexturBrowser::setzeUrsprungY(int y, const AktiveShader& aktiv) {   // setOriginY
    standPruefen(aktiv);
    m_ursprungY = y;
    klemmen(aktiv);
}

TexturBrowser::Rollen TexturBrowser::rollen(const AktiveShader& aktiv) {   // updateScroll
    Rollen r;
    const int gesamt = std::max(gesamtHoehe(aktiv), hoehe);
    r.maximum = gesamt - hoehe;
    r.wert = -ursprungY(aktiv);
    r.seite = hoehe;
    return r;
}

// TextureBrowser_Focus (texwindow.cpp:771-810)
void TexturBrowser::fokus(const std::string& name, const AktiveShader& aktiv) {
    TexturAnordnung lage;
    for (const auto& [n, s] : aktiv.alle()) {
        (void)n;
        if (!gezeigt(s)) { continue; }
        const auto [b, h] = texturMass(s);
        const auto [x, y] = lage.naechste(*this, b, h);
        (void)x;
        if (gleichOhneFall(name, s.name)) {
            const int texturHoehe = h + 2 * schriftHoehe;
            int o = ursprungY(aktiv);
            if (y > o) { o = y + 4; }
            if (y - texturHoehe < o - hoehe) { o = (y - texturHoehe) + hoehe; }
            setzeUrsprungY(o, aktiv);
            return;
        }
    }
}

// Texture_At (texwindow.cpp:812-838)
const AktiverShader* TexturBrowser::texturBei(int mx, int my, const AktiveShader& aktiv) {
    my = ursprungY(aktiv) - my - 1;
    TexturAnordnung lage;
    for (const auto& [n, s] : aktiv.alle()) {
        (void)n;
        if (!gezeigt(s)) { continue; }
        const auto [b, h] = texturMass(s);
        const auto [x, y] = lage.naechste(*this, b, h);
        if (mx > x && mx - x < b && my < y && y - my < h + schriftHoehe) { return &s; }
    }
    return nullptr;
}

// TextureBrowser_ShowDirectory (texwindow.cpp:704-737) mit
// TextureCategoryLoadShader und LoadTexturesByTypeVisitor /
// TexturePath_loadTexture (Bildmodule alphabetisch: jpg, png, tga).
std::size_t TexturBrowser::verzeichnisZeigen(const std::string& dir, AktiveShader& aktiv, const Texturinventar& inventar,
                                             const Dateisystem& fs, std::vector<std::string>* warnungen) {
    verzeichnis = dir;
    // foreachShaderName: g_shaderDefinitions = die Definitionen und die ohne
    // Definition angeforderten Namen (CreateDefault).
    std::vector<std::string> namen;
    for (const ShaderEintrag* s : inventar.shader()) { namen.push_back(s->name); }
    for (const auto& [n, s] : aktiv.alle()) {
        if (s.standard && inventar.shaderFinden(n) == nullptr) { namen.push_back(n); }
    }
    std::size_t anzahl = 0;
    for (const std::string& n : namen) {
        if (praefixOhneFall(n, kTexturPraefix) && praefixOhneFall(n.substr(std::strlen(kTexturPraefix)), dir)) {
            ++anzahl;
            (void)aktiv.holen(n, &inventar);
        }
    }
    const std::string ordner = std::string(kTexturPraefix) + dir;
    for (const char* typ : {"jpg", "png", "tga"}) {
        for (const std::string& datei : fs.dateien(ordner, typ, 1)) {
            const std::string name = ordner + ohneEndung(datei);
            if (texturNameIgnorieren(name)) { continue; }
            if (!shaderGueltig(name)) {
                if (warnungen != nullptr) { warnungen->push_back("Skipping invalid texture name: [" + name + "]"); }
                continue;
            }
            (void)aktiv.holen(name, &inventar);   // if a texture is already in use to represent a shader, ignore it
        }
    }
    zuletztMerken(dir);
    unbenutzteSetzen(false);
    setzeUrsprungY(0, aktiv);
    return anzahl;
}

// TextureBrowser_ShowDirectoryOfShader (texwindow.cpp:739-748)
bool TexturBrowser::verzeichnisDesShaders(const std::string& name, AktiveShader& aktiv, const Texturinventar& inventar,
                                          const Dateisystem& fs, std::vector<std::string>* warnungen, std::size_t* anzahl) {
    const std::size_t erstes = name.find('/');
    const std::size_t letztes = name.rfind('/');
    if (erstes == std::string::npos || letztes == erstes) { return false; }   // range.empty()
    const std::string dir = name.substr(erstes + 1, letztes - erstes);
    const std::size_t n = verzeichnisZeigen(dir, aktiv, inventar, fs, warnungen);
    if (anzahl != nullptr) { *anzahl = n; }
    fokus(name, aktiv);
    return true;
}

// TextureBrowser_trackRecentFolders (texwindow.cpp:634-647)
void TexturBrowser::zuletztMerken(const std::string& dir) {
    const auto it = std::find(zuletzt.begin(), zuletzt.end(), dir);
    if (it != zuletzt.end()) {
        std::string f = *it;
        zuletzt.erase(it);
        zuletzt.push_back(std::move(f));
        return;
    }
    if (zuletzt.size() >= 25) { zuletzt.erase(zuletzt.begin()); }
    zuletzt.push_back(dir);
}

void TexturBrowser::unbenutzteSetzen(bool an) {   // TextureBrowser_SetHideUnused
    unbenutzteVerstecken = an;
    hoeheGeaendert();
}

void TexturBrowser::alleZeigen() {   // TextureBrowser_showAll (texwindow.cpp:1940-1946)
    verzeichnis.clear();
    unbenutzteSetzen(!unbenutzteVerstecken);   // toggle to show all used on the first hit and all on the second
}

void TexturBrowser::neuGeladen() {   // RefreshShaders (texwindow.cpp:1900-1909)
    verzeichnis.clear();
    hoeheGeaendert();
}

std::string TexturBrowser::titel() const {   // TextureBrowser_exportTitle
    return std::string("Textures: ") + (verzeichnis.empty() ? std::string("all") : verzeichnis);
}

std::string texturStatus(const std::string& name, const AktiverShader& s) {   // TextureBrowser_SetStatus
    const std::string kurz = praefixOhneFall(name, kTexturPraefix) ? name.substr(std::strlen(kTexturPraefix)) : name;
    return kurz + " W: " + std::to_string(s.breite) + " H: " + std::to_string(s.hoehe);
}

// TextureGroups_constructTreeView (texwindow.cpp:1159-1172)
std::vector<std::string> texturGruppen(const AktiveShader& aktiv, const Texturinventar& inventar, bool nurShaderliste) {
    std::set<std::string, GruppenKleiner> gruppen;
    for (const std::string& g : inventar.verzeichnisse(nurShaderliste)) { gruppen.insert(g); }
    for (const auto& [n, s] : aktiv.alle()) {
        if (s.standard && inventar.shaderFinden(n) == nullptr) { gruppeAusShader(gruppen, n); }
    }
    return std::vector<std::string>(gruppen.begin(), gruppen.end());
}

// TextureGroups_constructTreeModel (texwindow.cpp:1104-1138)
std::vector<TexturGruppe> texturGruppenBaum(const std::vector<std::string>& gruppen) {
    std::vector<TexturGruppe> wurzel;
    std::size_t i = 0;
    while (i < gruppen.size()) {
        const std::string& name = gruppen[i];
        const std::size_t unterstrich = name.find('_');
        const std::string anfang = unterstrich == std::string::npos ? std::string() : name.substr(0, unterstrich + 1);
        const bool gruppe = unterstrich != std::string::npos && i + 1 < gruppen.size() &&
                            gruppen[i + 1].compare(0, anfang.size(), anfang) == 0;   // string_equal_start
        if (gruppe) {
            TexturGruppe g;
            g.name = name.substr(0, unterstrich);
            while (i < gruppen.size() && gruppen[i].compare(0, anfang.size(), anfang) == 0) {
                g.kinder.push_back(TexturGruppe{gruppen[i], gruppen[i], {}});
                ++i;
            }
            wurzel.push_back(std::move(g));
        } else {
            wurzel.push_back(TexturGruppe{name, name, {}});
            ++i;
        }
    }
    return wurzel;
}

}  // namespace bhed::karte
