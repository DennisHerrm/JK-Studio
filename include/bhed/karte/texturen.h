// texturen.h - Karten-Editor: Dateisystem des Spiels und Textur-Inventar fuer
// den spaeteren Texturbrowser, Texturgroessen fuer die Texturprojektion.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): plugins/vfspk3/vfs.cpp
// (InitDirectory: Ordner, dann .pk3 absteigend, sp_/mp_-Vorrang;
// GetDirList/GetFileList; forEachArchive), radiant/qe3.cpp (QE_InitVFS:
// Mod vor Basis), plugins/shaders/shaders.cpp (Shaders_Load mit
// shaderlist.txt, BuildShaderList, ParseShaderFile, ShaderTemplate::
// parseQuake3, Try_Shader_ForName), radiant/texwindow.cpp
// (TextureGroups_addShader/addDirectory, TextureGroups_compare,
// TextureBrowser_ShowDirectory, texture_name_ignore, Texture_IsShown),
// radiant/image.cpp (QERApp_LoadImage: Bildmodule alphabetisch - jpg, png,
// tga) und radiant/texwindow.cpp:1650 (notex.png / shadernotex.png, 64x64).
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
// Archive werden mit den vorhandenen Lesern gelesen (bhed/pk3.h), Bilder mit
// bhed/image.h. Geschrieben wird nie - NRC legt ohne shaderlist.txt eine aus
// dem Gamepack im Spielordner an (shaderlist_findOrInstall); das passiert hier
// nicht, statt dessen kann die Datei als "standardShaderliste" mitgegeben
// werden, sonst gilt NRCs Rueckfall "No shaderlist.txt found: loading all
// shaders".
#ifndef BHED_KARTE_TEXTUREN_H
#define BHED_KARTE_TEXTUREN_H

#include "bhed/karte/pinsel.h"
#include "bhed/karte/protokoll.h"
#include "bhed/pk3.h"

#include <array>
#include <cstddef>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace bhed::karte {

// Das virtuelle Dateisystem (Quake3FileSystem).
class Dateisystem {
public:
    // Die Meldungen von InitDirectory/InitPakFile (vfs.cpp:98, 227, 290) in
    // ihrer Reihenfolge, fuer die Konsole.
    Protokoll protokoll;
    // QE_InitVFS ruft initDirectory fuer Mod-Ordner, dann Basis-Ordner.
    void ordnerHinzufuegen(const std::string& ordner, const std::string& spielmodus);   // InitDirectory
    [[nodiscard]] bool dateiLesen(const std::string& pfad, std::string& inhalt) const;   // openFile (erstes Archiv)
    [[nodiscard]] bool textLesen(const std::string& pfad, std::string& inhalt) const;    // openTextFile
    // findFile: die Wurzel des ersten Archivs mit der Datei (Ordner mit '/'
    // am Ende oder Pfad der .pk3; "" = nirgends), istOrdner: ein Verzeichnis.
    [[nodiscard]] std::string finden(const std::string& pfad, bool* istOrdner = nullptr) const;
    // GetDirList / GetFileList: Namen relativ zu basis (mit '/' am Ende),
    // eindeutig ohne Gross/klein, in Archivreihenfolge.
    [[nodiscard]] std::vector<std::string> verzeichnisse(const std::string& basis, std::size_t tiefe = 1) const;
    [[nodiscard]] std::vector<std::string> dateien(const std::string& basis, const std::string& endung,
                                                   std::size_t tiefe = 1) const;
    // forEachArchive: Namen der Archive, auf Wunsch umgekehrt.
    [[nodiscard]] std::vector<std::string> archive(bool umgekehrt) const;
    // Archive::forEachFile( VisitorFunc( v, eFiles, tiefe ), basis ) auf einem
    // Archiv: jede Datei unter basis bis zur Tiefe, mit vollem Pfad.
    void dateienImArchiv(const std::string& archiv, const std::string& basis, std::size_t tiefe,
                         const std::function<void(const std::string&)>& besuch) const;
    // Archive::openTextFile auf einem bestimmten Archiv.
    [[nodiscard]] bool textAusArchiv(const std::string& archiv, const std::string& pfad, std::string& inhalt) const;
    [[nodiscard]] std::size_t anzahl() const { return m_archive.size(); }

private:
    struct Archiv {
        std::string name;    // Ordner mit '/' am Ende oder Pfad der .pk3
        bool istPak = false;
        Pk3 pak;
    };
    bool lesenAus(const Archiv& a, const std::string& pfad, std::string& inhalt) const;
    // alle Dateipfade eines Archivs (relativ, mit '/'), fuer Listen
    void pfadeSammeln(const Archiv& a, const std::string& basis, std::size_t tiefe, bool verzeichnisse,
                      std::vector<std::string>& out) const;
    std::vector<Archiv> m_archive;
};

// QER_* (include/ishaders.h:28-41)
enum ShaderFlags : unsigned int {
    QER_TRANS = 1 << 0,
    QER_NOCARVE = 1 << 1,
    QER_NODRAW = 1 << 2,
    QER_NONSOLID = 1 << 3,
    QER_LIQUID = 1 << 4,
    QER_FOG = 1 << 5,
    QER_ALPHATEST = 1 << 6,
    QER_CULL = 1 << 7,
    QER_AREAPORTAL = 1 << 8,
    QER_CLIP = 1 << 9,
    QER_BOTCLIP = 1 << 10,
    QER_SKY = 1 << 11,
};

// ShaderTemplate (Quake3-Teil)
struct ShaderEintrag {
    std::string name;          // "textures/..." ohne Endung
    std::string texturName;    // m_textureName: qer_editorimage oder der Name
    std::string datei;         // Shaderdatei ("" = ohne Shader: nur ein Bild)
    unsigned int flags = 0;    // QER_*
    float trans = 1.0f;        // m_fTrans
    int alphaFunc = 0;         // IShader::EAlphaFunc (eAlways = 0)
    float alphaRef = 0.0f;
    int cull = 0;              // IShader::ECull (eCullNone = 0, eCullBack = 1)
    std::string himmel;        // m_skyBox
    // Stufe 8b Punkt 3: die surfaceparm-Namen (klein) in der Reihenfolge der
    // Datei - fuer die Inhalte wie q3map2 (ApplySurfaceParm, Im Editor spielen)
    std::vector<std::string> surfaceparms;
    [[nodiscard]] bool istStandard() const { return datei.empty(); }   // IsDefault
};

// Das Inventar: Shaderdateien, Shader, Verzeichnisse und Bilder.
class Texturinventar final : public TexturGroessen {
public:
    // Shaders_Load: shaderlist.txt aus allen Archiven (umgekehrt), sonst die
    // standardShaderliste (wie von NRC installiert), sonst alle shaders/*.shader.
    void laden(const Dateisystem& fs, const std::string& standardShaderliste = std::string());

    [[nodiscard]] const std::vector<std::string>& shaderDateien() const { return m_shaderDateien; }   // l_shaderfiles
    [[nodiscard]] std::vector<std::string> meldungen() const { return m_meldungen; }
    // Shaders_Load / LoadShaderFile / ParseShaderFile (shaders.cpp:1403-1670):
    // "Found shaderlist.txt in", "No shaderlist.txt found: loading all
    // shaders", "Parsing shaderfile", Fehler - in NRCs Reihenfolge und Art.
    Protokoll protokoll;
    // g_shaderDefinitions: alle Shader aus den Dateien, Namen ohne Gross/klein sortiert
    [[nodiscard]] std::vector<const ShaderEintrag*> shader() const;
    [[nodiscard]] const ShaderEintrag* shaderFinden(const std::string& name) const;

    // TextureGroups_constructTreeView: Verzeichnisse unter textures/ (aus den
    // Archiven, wenn nichtNurShaderliste) und erste Verzeichnisstufe der
    // Shadernamen; sortiert wie TextureGroups_compare.
    [[nodiscard]] std::vector<std::string> verzeichnisse(bool nurShaderliste = false) const;
    // TextureBrowser_ShowDirectory + Texture_IsShown: Shader mit dem Praefix
    // textures/<dir>/ und die Bilder direkt in textures/<dir>/.
    [[nodiscard]] std::vector<ShaderEintrag> verzeichnis(const std::string& dir) const;

    // TexturGroessen: Groesse des Editorbilds (jpg, png, tga in dieser
    // Reihenfolge), sonst 64x64 (notex/shadernotex). Ergebnis zwischengespeichert.
    [[nodiscard]] std::pair<std::size_t, std::size_t> groesse(const std::string& shader) const override;
    // Dasselbe mit der Angabe, ob das Bild gefunden wurde (sonst notex.png
    // bzw. shadernotex.png: CShader::realise, m_notfound).
    struct BildMass {
        std::size_t breite = 64;
        std::size_t hoehe = 64;
        bool gefunden = false;
    };
    [[nodiscard]] BildMass bildMass(const std::string& shader) const;

private:
    void shaderDateiParsen(const std::string& text, const std::string& datei);
    bool bildGroesse(const std::string& texturName, std::size_t& w, std::size_t& h) const;

    const Dateisystem* m_fs = nullptr;
    std::vector<std::string> m_shaderDateien;
    std::vector<std::string> m_meldungen;
    struct OhneFall {
        bool operator()(const std::string& a, const std::string& b) const;
    };
    std::map<std::string, ShaderEintrag, OhneFall> m_shader;   // g_shaderDefinitions
    mutable std::mutex m_sperre;
    mutable std::map<std::string, BildMass, OhneFall> m_groessen;
};

// texture_name_ignore (texwindow.cpp:589-607)
bool texturNameIgnorieren(const std::string& name);
// shader_valid (shaderlib.h:46-55): kein Leerraum, kein Rueckstrich, nur ASCII.
bool shaderGueltig(const std::string& s);

// ResampleGamma (textures.cpp:143-155): die Gammatabelle der Texturen
[[nodiscard]] std::array<unsigned char, 256> gammaTabelle(float gamma);
// LoadTextureRGBA (textures.cpp:206): GL_TEXTURE_BASE_LEVEL =
// min( g_Textures_mipLevel, (int)log2( max( w, h ) ) )
[[nodiscard]] int mipBasis(int mipStufe, int breite, int hoehe);
// ETexturesMode (textures.cpp:44-52) und die Reihenfolge der Klappliste
// (TextureModeImport/Export: Nearest, Nearest Mipmap, Linear, Bilinear,
// Bilinear Mipmap, Trilinear)
[[nodiscard]] int texturModusAusListe(int index);
[[nodiscard]] int texturModusZuListe(int modus);

}  // namespace bhed::karte
#endif
