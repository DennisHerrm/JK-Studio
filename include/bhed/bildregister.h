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

    // Die Werte des Ersten, nullptr fuer ein Bild, das noch niemand geladen hat.
    [[nodiscard]] const BildWahl* wahl(const std::string& bild) const;
    [[nodiscard]] bool kenntShader(const std::string& name) const;
    [[nodiscard]] const std::vector<std::string>& warnungen() const noexcept { return m_warnungen; }
    [[nodiscard]] std::size_t bilder() const noexcept { return m_bilder.size(); }
    void leeren();

private:
    std::unordered_set<std::string> m_shader;
    std::unordered_map<std::string, BildWahl> m_bilder;
    std::vector<std::string> m_warnungen;
};

// Die Shader der Karte in der Reihenfolge, in der RE_LoadWorldMap (tr_bsp.cpp:1420-1427) sie registriert: zuerst
// R_LoadFogs je Nebel (:1088), dann R_LoadSurfaces je Flaeche der Datei (ShaderForShaderNum, :389/477/547/622) - auch
// unsichtbare. Jeder Name einmal, beim ersten Auftreten.
[[nodiscard]] std::vector<std::string> weltShaderReihenfolge(const BspGeometry& geo);

}  // namespace bhed

#endif  // BHED_BILDREGISTER_H
