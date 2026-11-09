// bildregister.h - ein Bild je Name, wie das Spiel es haelt
//
// MR8 Punkt 1. Die Engine laedt jedes Bild nur EINMAL je Name: R_FindImageFile fragt zuerst R_FindImageFile_NoLoad
// (rd-vanilla tr_image.cpp:936-971 OpenJK, :920-965 Movie Duels 9), und das sucht in AllocatedImages nach
// GenerateImageMappingName (klein, ohne Endung, '\' -> '/'). Gibt es das Bild schon, kommt genau dieses image_t zurueck -
// mit dem mipmap, allowPicmip, allowTC und glWrapClampMode, mit denen es der ERSTE geladen hat (R_CreateImage :983-1053:
// Upload32 mit allowTC des Ersten, glTexParameter WRAP des Ersten). Ein spaeterer Shader mit anderen Werten bekommt es
// trotzdem; die Engine warnt nur fuer mipmap, allowPicmip und glWrapClampMode ("WARNING: reused image %s with mixed ...
// parm", :955-962; Movie Duels nur mit g_DebugSaberCombat, :941-953), fuer allowTC gar nicht.
//
// Wer zuerst laedt, entscheidet also. Das Register spielt die Aufrufe von R_FindImageFile in der Reihenfolge nach, in der
// die Shader registriert werden (R_FindShader, tr_shader.cpp:3433-3510), und merkt sich je Bild die Werte des Ersten.
#ifndef BHED_BILDREGISTER_H
#define BHED_BILDREGISTER_H

#include "bhed/shaderscript.h"

#include <cstddef>
#include <functional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace bhed {

struct BspGeometry;

// Die Werte, mit denen ein Bild hochgeladen wurde (image_t: mipmap, allowPicmip, wrapClampMode; allowTC nur in Upload32).
struct BildWahl {
    bool mip = true;          // mipmap
    bool picmip = true;       // allowPicmip
    bool tc = true;           // allowTC (komprimiert nur mit r_ext_compress_textures, Upload32 tr_image.cpp:634-689)
    bool klammern = false;    // GL_CLAMP (mit clampToEdge GL_CLAMP_TO_EDGE) statt GL_REPEAT
    std::string shader;       // der Shader, der das Bild zuerst geladen hat
};

class BildRegister {
public:
    // Das Skript zu einem Shadernamen (klein, ohne Endung) oder nullptr, wenn es keins gibt.
    using Skripte = std::function<const ShaderInfo*(const std::string&)>;
    // FS_ReadFile: false, wenn es die Datei nicht gibt.
    using Leser = std::function<bool(const std::string&, std::string&)>;
    // MR9 Punkt 2: findet R_FindImageFile das Bild? (leer: jedes Bild gilt als vorhanden)
    using Vorhanden = std::function<bool(const std::string&)>;

    // R_FindShader(name, ..., mipRawImage): beim ersten Mal je Name (COM_StripExtension, klein) die Bilder in der
    // Reihenfolge, in der ParseShader sie laedt - Stufe fuer Stufe (map/clampMap/animMap, ParseStage :1259-1335), die
    // Himmelsseiten an der Stelle von skyParms (ParseSkyParms :1970, rt lf bk ft up dn, mipmap/picmip ja, GL_CLAMP).
    // Ohne Skript das Bild des Namens selbst (:3502: mipmap/picmip = mipRawImage, allowTC ja, mipRawImage ? GL_REPEAT
    // : GL_CLAMP). Ein Shader, den das Register schon kennt, laedt nichts neu (sh_hashTable; mit anderer Lightmap
    // wird er neu gelesen, findet aber nur schon geladene Bilder).
    void shader(const std::string& name, const Skripte& skripte, bool mipRawImage = true);
    // R_FindImageFile(bild, mipmap, allowPicmip, allowTC, glWrapClampMode): das erste Laden bestimmt; jedes weitere mit
    // anderem mipmap/allowPicmip/glWrapClampMode schreibt die Warnung der Engine.
    void bild(const std::string& bild, const BildWahl& wahl);

    // MR9 Punkt 1: was ausser der Welt Shader registriert, in der Reihenfolge der Engine (alle mit mipRawImage qtrue).
    // RE_RegisterModel (rd-vanilla tr_model.cpp:630-700): .md3 mit den Stufen rueckwaerts (Name_2.md3, Name_1.md3, dann der
    // Name; MD3_MAX_LODS 3), je Flaeche jeder Shader (R_LoadMD3 :923-926); .glm je Flaeche der Hierarchie (R_LoadMDXM
    // tr_ghoul2.cpp:3713-3751, "[nomaterial]" leer). Ein schon geladenes Modell registriert seine Shader wieder (:217-232).
    // true, wenn es die Datei `pfad` gibt (FS_ReadFile; jede Datei wird je Register nur einmal gelesen).
    bool modell(const std::string& pfad, const Leser& lies, const Skripte& skripte);
    // RE_RegisterSkin (tr_skin.cpp:348-428): "ordner/|kopf|rumpf|beine" sind drei .skin-Dateien (RE_SplitSkins), jede nur
    // einmal und nur, solange die vorige da war; je Zeile "flaeche,shader" R_FindShader (RE_RegisterIndividualSkin :262-330:
    // ohne tag_*, ohne "_off,*off", hoechstens 128 Flaechen je Haut).
    void haut(const std::string& name, const Leser& lies, const Skripte& skripte);
    // CFxScheduler::RegisterEffect (cgame/FxScheduler.cpp:418-560): effects/<Name ohne Endung>.efx einmal je Name; je
    // Primitiv-Gruppe (particle, line, ... flash) die Eigenschaften in der Reihenfolge der Datei (ParsePrimitive
    // FxTemplate.cpp:1754-1830): shader(s) -> RegisterShader, model(s) -> RegisterModel, impactfx/deathfx/emitfx/playfx ->
    // RegisterEffect sofort (ParseFX :1484-1510).
    void effekt(const std::string& pfad, const Leser& lies, const Skripte& skripte);

    // Die Werte des Ersten, nullptr fuer ein Bild, das noch niemand geladen hat.
    [[nodiscard]] const BildWahl* wahl(const std::string& bild) const;
    [[nodiscard]] bool kenntShader(const std::string& name) const;
    // MR9 Punkt 2: ParseStage bricht ab, wenn R_FindImageFile ein Bild von map/clampMap/animMap nicht findet
    // (tr_shader.cpp:1263-1268, 1288-1293, 1329-1334): die Stufen danach (und skyParms danach) laden nichts, ParseShader
    // scheitert und der Shader wird defaultShader (R_FindShader :3489-3493) - ShaderForShaderNum gibt der Flaeche dann
    // tr.defaultShader (tr_bsp.cpp:352), Modelle bekommen Shader 0 (R_LoadMD3 :926-931), RE_RegisterShader gibt 0 (:3614).
    // Ohne Skript ebenso, wenn das Bild des Namens fehlt (:3502-3509).
    [[nodiscard]] bool fehlerhaft(const std::string& name) const;
    void setzeVorhanden(Vorhanden v) { m_vorhanden = std::move(v); }
    [[nodiscard]] const std::vector<std::string>& warnungen() const noexcept { return m_warnungen; }
    [[nodiscard]] std::size_t bilder() const noexcept { return m_bilder.size(); }
    void leeren();
    // REST4 Punkt 2: R_InitShaders (RE_SVModelInit, R_Init beim Neustart des Renderers) - die Shadertabelle beginnt neu, die
    // Bilder bleiben (R_InitImages raeumt AllocatedImages nicht, tr_image.cpp:1481-1489); ein Shader wird wieder gelesen und
    // findet seine Bilder mit den Werten des Ersten.
    void shaderVergessen() {
        m_shader.clear();
        m_fehlerhaft.clear();
    }

private:
    std::unordered_set<std::string> m_shader;
    std::unordered_set<std::string> m_effekte;   // mEffectIDs
    std::unordered_set<std::string> m_fehlerhaft;   // MR9 Punkt 2: defaultShader
    Vorhanden m_vorhanden;
    std::unordered_map<std::string, BildWahl> m_bilder;
    std::vector<std::string> m_warnungen;
    // Messrunde 09.11.: je Modelldatei (klein) ob es sie gibt und ihre Shader in der Folge der Datei
    struct ModellDatei {
        bool da = false;
        std::vector<std::string> shader;
    };
    std::unordered_map<std::string, ModellDatei> m_modellDateien;
};

// Die Shader der Karte in der Reihenfolge, in der RE_LoadWorldMap (tr_bsp.cpp:1420-1427) sie registriert: zuerst
// R_LoadFogs je Nebel (:1088), dann R_LoadSurfaces je Flaeche der Datei (ShaderForShaderNum, :389/477/547/622) - auch
// unsichtbare. Jeder Name einmal, beim ersten Auftreten.
[[nodiscard]] std::vector<std::string> weltShaderReihenfolge(const BspGeometry& geo);

}  // namespace bhed

#endif  // BHED_BILDREGISTER_H
