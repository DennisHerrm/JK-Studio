// texturbrowser.h - Karten-Editor: der Texturbrowser ohne Oberflaeche - die
// aktiven Shader, welche davon gezeigt werden, wo jede Textur steht, Rollen,
// Fokus, Treffer und der Verzeichnisbaum.
//
// Portiert aus NetRadiant-Custom 20260114: radiant/texwindow.cpp
// (TextureGroups_compare/addShader/addDirectory, TextureBrowser::
// getTextureWH/evaluateHeight/setOriginY/clampOriginY/getOriginY/
// heightChanged/updateScroll, TextureLayout::nextPos, Texture_filtered,
// Texture_IsShown, TextureBrowser_ShowDirectory, TextureCategoryLoadShader,
// TexturePath_loadTexture, TextureBrowser_ShowDirectoryOfShader,
// TextureBrowser_trackRecentFolders, TextureBrowser_SetHideUnused,
// TextureBrowser_Focus, Texture_At, TextureBrowser::draw (Lagen und Rahmen),
// TextureGroups_constructTreeModel, TextureGroups_constructTreeView,
// TextureBrowser_showAll, TextureBrowser_exportTitle, TextureBrowser_SetStatus,
// TextureScaleImport/Export) und plugins/shaders/shaders.cpp (CShader,
// g_ActiveShaders, Try_Shader_ForName, FreeShaders, IsInUse/SetInUse).
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
//
// Alle Lagen in Geraetepunkten wie NRC (m_width = Breite * devicePixelRatio)
// und in NRCs GL-Koordinaten: y waechst nach oben, die erste Zeile beginnt
// bei -4, der sichtbare Bereich reicht von originy - hoehe bis originy.
// Die Tags (ShaderPlug-Tagdatei) kommen mit Stufe 2.
#ifndef BHED_KARTE_TEXTURBROWSER_H
#define BHED_KARTE_TEXTURBROWSER_H

#include "bhed/karte/texturen.h"

#include <cstddef>
#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

// CShader (shaders.cpp:820-1110): ein Shader, den Radiant angefordert hat.
struct AktiverShader {
    std::string name;              // m_Name (wie angefordert)
    bool standard = true;          // IsDefault(): ohne Shaderdatei (CreateDefault)
    std::string datei;             // getShaderFileName
    std::string texturName;        // m_template.m_textureName
    std::size_t breite = 64;       // getTexture()->width
    std::size_t hoehe = 64;        // getTexture()->height
    bool bildFehlt = false;        // getTexture() ist notex.png / shadernotex.png
    bool imGebrauch = false;       // IsInUse (FaceShader/Patch instanceAttach)
};

// Ohne Gross/klein (shader_less_t = string_less_nocase).
struct ShaderKleiner {
    bool operator()(const std::string& a, const std::string& b) const;
};

// g_ActiveShaders: jeder angeforderte Shader bleibt, bis die Shader neu
// geladen werden (FreeShaders); sortiert ohne Gross/klein.
class AktiveShader {
public:
    using Liste = std::map<std::string, AktiverShader, ShaderKleiner>;
    // Try_Shader_ForName / Shader_ForName: vorhandener Eintrag oder neu aus
    // der Definition (sonst CreateDefault: der Name ist das Bild).
    AktiverShader& holen(const std::string& name, const Texturinventar* inventar);
    void leeren();                                          // FreeShaders
    [[nodiscard]] const Liste& alle() const { return m_liste; }
    [[nodiscard]] const AktiverShader* finden(const std::string& name) const;
    // SetInUse fuer alle: genau die Namen in imGebrauch sind benutzt.
    // Liefert true, wenn sich etwas geaendert hat (g_ActiveShadersChangedNotify).
    bool gebrauchSetzen(const std::set<std::string, ShaderKleiner>& imGebrauch);
    // Zaehlt jede Aenderung (g_ActiveShadersChangedNotify).
    [[nodiscard]] std::size_t stand() const { return m_stand; }

private:
    Liste m_liste;
    std::size_t m_stand = 0;
};

// Was TextureBrowser::draw an einer Textur zeichnet.
struct TexturLage {
    const AktiverShader* shader = nullptr;
    int x = 0;   // TextureLayout::nextPos (GL-Koordinaten)
    int y = 0;
    int b = 0;   // getTextureWH
    int h = 0;
    enum class Rand { Keiner, Gewaehlt, ImGebrauch, Shader };
    Rand rand = Rand::Keiner;   // rot (2 breit), hellgruen, weiss
    bool strichel = false;      // !IsDefault: schwarze Strichel 0xF000
};

// Ein Eintrag des Verzeichnisbaums (QStandardItem): pfad leer = Gruppe.
struct TexturGruppe {
    std::string name;   // DisplayRole
    std::string pfad;   // ToolTipRole (zum Laden)
    std::vector<TexturGruppe> kinder;
};

// TextureGroups_constructTreeModel: Verzeichnisse mit gleichem Anfang bis zum
// ersten '_' unter einer Gruppe, wenn das naechste damit beginnt.
[[nodiscard]] std::vector<TexturGruppe> texturGruppenBaum(const std::vector<std::string>& gruppen);

// Die Einstellungen des Texturbrowsers (TextureBrowser_Construct:
// registerPreference) mit NRCs Vorgaben.
struct TexturBrowserEinstellungen {
    int skala = 50;                  // TextureScale (m_textureScale, 10/25/50/100/200)
    int einheitsGroesse = 160;       // UniformTextureSize (m_uniformTextureSize)
    int einheitsMin = 48;            // UniformTextureMinSize
    int radSchritt = 64;             // WheelMouseInc (m_mouseWheelScrollIncrement)
    bool rollbalken = true;          // TextureScrollbar (m_showTextureScrollbar)
    int startShader = 0;             // LoadShaders (m_startupShaders: 0 None, 1 Common)
    bool shaderZeigen = true;        // ShowShaders
    bool texturenZeigen = true;      // ShowTextures
    bool nurShaderliste = false;     // ShowShaderlistOnly (g_TextureBrowser_shaderlistOnly)
    bool festeGroesse = true;        // FixedSize (g_TextureBrowser_fixedSize)
    bool ohneNotex = false;          // FilterNotex (g_TextureBrowser_filterNotex)
    bool transparenz = false;        // EnableAlpha (g_TextureBrowser_enableAlpha)
    // TagsShowGui (m_tags) steht nur in TagZustand::gui (Stufe 6: ein zweites
    // Feld hier ueberschrieb beim Speichern den Wert von TagsToggleGui).
    bool vonAnfang = false;          // SearchFromStart (g_TextureBrowser_filter_searchFromStart)
    bool commonOhneTexturen = true;  // HideNonShadersInCommon (m_hideNonShadersInCommon)
};

// TextureScaleImport / TextureScaleExport: Klappfeld-Index <-> Prozent.
[[nodiscard]] int texturSkalaAusIndex(int index);   // -1 = unbekannt
[[nodiscard]] int texturSkalaIndex(int skala);      // -1 = keiner der fuenf Werte

// shader_get_textureName: der Name ohne "textures/".
[[nodiscard]] std::string shaderTexturName(const std::string& name);
// path_get_filename_start: nach dem letzten '/'.
[[nodiscard]] std::string pfadDateiname(const std::string& name);

class TexturBrowser {
public:
    TexturBrowserEinstellungen e;
    int breite = 0;                 // m_width (Geraetepunkte)
    int hoehe = 0;                  // m_height
    int schriftHoehe = 16;          // TextureBrowser_fontHeight (GlobalOpenGL().m_font)
    std::string shader = kTexturPraefix;   // m_shader (texdef_name_default)
    std::string verzeichnis;        // g_TextureBrowser_currentDirectory
    std::string filter;             // m_filter_string
    bool unbenutzteVerstecken = false;   // m_hideUnused
    std::vector<std::string> zuletzt;    // m_recent_folders (hoechstens 25)
    std::string commonVerzeichnis = "common/";   // TextureBrowser_getCommonShadersDir
    bool tagSuche = false;                       // m_searchedTags: nur m_found_shaders zeigen
    std::set<std::string> gefundeneShader;       // m_found_shaders

    // getTextureWH
    [[nodiscard]] std::pair<int, int> texturMass(const AktiverShader& s) const;
    // Texture_filtered (Name ohne Verzeichnis)
    [[nodiscard]] bool gefiltert(const std::string& name) const;
    // Texture_IsShown
    [[nodiscard]] bool gezeigt(const AktiverShader& s) const;
    // Alle gezeigten Texturen in der Reihenfolge der aktiven Shader mit ihrer
    // Lage (TextureLayout ueber QERApp_ActiveShaders_Iterator).
    [[nodiscard]] std::vector<TexturLage> anordnen(const AktiveShader& aktiv) const;
    // TextureBrowser::draw: nur die sichtbaren, mit Rahmen und Strichel.
    [[nodiscard]] std::vector<TexturLage> sichtbare(const AktiveShader& aktiv);

    // evaluateHeight (mit m_heightChanged) und totalHeight
    [[nodiscard]] int gesamtHoehe(const AktiveShader& aktiv);
    // getOriginY: klemmt nach einer Aenderung (m_originInvalid)
    [[nodiscard]] int ursprungY(const AktiveShader& aktiv);
    // setOriginY (klemmt immer)
    void setzeUrsprungY(int y, const AktiveShader& aktiv);
    // heightChanged + m_originInvalid (TextureBrowser_activeShadersChanged,
    // resizeGL, Filter, Schalter)
    void hoeheGeaendert() {
        m_hoeheGeaendert = true;
        m_ursprungUngueltig = true;
    }
    // updateScroll: Werte des QScrollBar (Maximum, Wert, Seite, Schritt 20)
    struct Rollen {
        int maximum = 0;
        int wert = 0;
        int seite = 0;
    };
    [[nodiscard]] Rollen rollen(const AktiveShader& aktiv);

    // TextureBrowser_Focus: rollt so, dass die Textur ganz zu sehen ist.
    void fokus(const std::string& name, const AktiveShader& aktiv);
    // Texture_At: mx, my in Fensterpunkten (Geraetepunkte, y nach unten).
    [[nodiscard]] const AktiverShader* texturBei(int mx, int my, const AktiveShader& aktiv);

    // TextureBrowser_ShowDirectory (ohne .wad): laedt die Shader und Bilder
    // des Verzeichnisses (dir mit '/' am Ende) in aktiv. Liefert die Zahl der
    // Shader ("Showing N shaders.") und haengt Warnungen an.
    std::size_t verzeichnisZeigen(const std::string& dir, AktiveShader& aktiv, const Texturinventar& inventar,
                                  const Dateisystem& fs, std::vector<std::string>* warnungen = nullptr);
    // TextureBrowser_ShowDirectoryOfShader: das Verzeichnis des Shaders und
    // der Fokus darauf; false, wenn der Name kein Verzeichnis hat.
    bool verzeichnisDesShaders(const std::string& name, AktiveShader& aktiv, const Texturinventar& inventar,
                               const Dateisystem& fs, std::vector<std::string>* warnungen = nullptr,
                               std::size_t* anzahl = nullptr);
    // TextureBrowser_trackRecentFolders
    void zuletztMerken(const std::string& dir);
    // TextureBrowser_SetHideUnused
    void unbenutzteSetzen(bool an);
    // TextureBrowser_showAll: Verzeichnis "" und Hide Unused umschalten.
    void alleZeigen();
    // RefreshShaders: Verzeichnis "" (die Shader laedt der Aufrufer neu).
    void neuGeladen();
    // TextureBrowser_exportTitle: "Textures: <dir>" bzw. "Textures: all"
    [[nodiscard]] std::string titel() const;

private:
    bool m_hoeheGeaendert = true;    // m_heightChanged
    bool m_ursprungUngueltig = true; // m_originInvalid
    int m_gesamtHoehe = 0;           // m_nTotalHeight
    int m_ursprungY = 0;             // m_originy
    std::size_t m_stand = static_cast<std::size_t>(-1);   // Stand der aktiven Shader beim letzten Rechnen
    void standPruefen(const AktiveShader& aktiv);
    void klemmen(const AktiveShader& aktiv);   // clampOriginY
};

// TextureLayout (texwindow.cpp:405-436)
class TexturAnordnung {
public:
    int x = 8;      // current_x
    int y = -4;     // current_y
    int zeile = 0;  // current_row
    std::pair<int, int> naechste(const TexturBrowser& tb, int b, int h);   // nextPos
};

// TextureBrowser_SetStatus: "<Name ohne textures/> W: <b> H: <h>" (name wie
// uebergeben, Masse des Shaders).
[[nodiscard]] std::string texturStatus(const std::string& name, const AktiverShader& s);

// TextureGroups_constructTreeView: Verzeichnisse unter textures/ (ohne
// "ShaderList Only") und die erste Stufe aller Shadernamen - der
// Definitionen und der ohne Definition angeforderten (CreateDefault traegt
// sie in g_shaderDefinitions ein).
[[nodiscard]] std::vector<std::string> texturGruppen(const AktiveShader& aktiv, const Texturinventar& inventar,
                                                     bool nurShaderliste);

}  // namespace bhed::karte

#endif
