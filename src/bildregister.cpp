// bildregister.cpp - siehe bhed/bildregister.h
#include "bhed/bildregister.h"

#include "bhed/bspgeo.h"
#include "bhed/image.h"

#include <cctype>

namespace bhed {
namespace {

// COM_StripExtension (q_shared.cpp) und der Vergleich in IsShader (Q_stricmp): ohne Endung, gross/klein egal.
std::string shaderSchluessel(const std::string& name) {
    std::string s = name;
    const std::size_t punkt = s.find_last_of('.');
    if (punkt != std::string::npos && s.find('/', punkt) == std::string::npos && s.find('\\', punkt) == std::string::npos) {
        s.resize(punkt);
    }
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

}  // namespace

void BildRegister::bild(const std::string& bild, const BildWahl& wahl) {
    // ParseStage: "$whiteimage", "*white" und "$lightmap" gehen nie an R_FindImageFile (tr_shader.cpp:1236-1255)
    if (bild.empty() || bild[0] == '$' || bild[0] == '*') { return; }
    const std::string name = image::mappingName(bild);
    const auto it = m_bilder.find(name);
    if (it == m_bilder.end()) {
        m_bilder.emplace(name, wahl);
        return;
    }
    // R_FindImageFile_NoLoad (tr_image.cpp:953-962): dieselben Warnungen, dieselbe Reihenfolge
    const BildWahl& erst = it->second;
    if (erst.mip != wahl.mip) { m_warnungen.push_back("WARNING: reused image " + name + " with mixed mipmap parm"); }
    if (erst.picmip != wahl.picmip) { m_warnungen.push_back("WARNING: reused image " + name + " with mixed allowPicmip parm"); }
    if (erst.klammern != wahl.klammern) { m_warnungen.push_back("WARNING: reused image " + name + " with mixed glWrapClampMode parm"); }
}

void BildRegister::shader(const std::string& name, const Skripte& skripte, bool mipRawImage) {
    if (name.empty()) { return; }
    const std::string schl = shaderSchluessel(name);
    if (!m_shader.insert(schl).second) { return; }
    const ShaderInfo* si = skripte ? skripte(schl) : nullptr;
    if (si == nullptr) {
        // R_FindShader ohne Skript (tr_shader.cpp:3502)
        BildWahl w;
        w.mip = mipRawImage;
        w.picmip = mipRawImage;
        w.tc = true;
        w.klammern = !mipRawImage;
        w.shader = schl;
        bild(name, w);
        return;
    }
    const auto himmel = [&] {
        if (si->skyBox.empty()) { return; }
        // ParseSkyParms (tr_shader.cpp:1967-1970): R_FindImageFile(pathname, qtrue, qtrue, !shader.noTC, GL_CLAMP)
        BildWahl w;
        w.tc = !si->himmelKeinTc;
        w.klammern = true;
        w.shader = schl;
        for (const char* suf : {"rt", "lf", "bk", "ft", "up", "dn"}) { bild(si->skyBox + "_" + suf, w); }
    };
    for (std::size_t i = 0; i < si->stages.size(); ++i) {
        if (i == si->himmelVorStufe) { himmel(); }
        const ShaderInfo::Stage& st = si->stages[i];
        if (st.lightmap || st.weiss || !st.video.empty()) { continue; }
        BildWahl w;
        w.mip = !st.keinMip;
        w.picmip = !st.keinPicmip;
        w.tc = !st.keinTc;
        w.klammern = st.clamp;   // clampmap / clampanimMap
        w.shader = schl;
        if (st.animImages.empty()) {
            bild(st.image, w);
        } else {
            for (const std::string& b : st.animImages) { bild(b, w); }   // animMap: jedes Bild nacheinander (:1323-1335)
        }
    }
    if (si->himmelVorStufe >= si->stages.size()) { himmel(); }
}

const BildWahl* BildRegister::wahl(const std::string& bild) const {
    const auto it = m_bilder.find(image::mappingName(bild));
    return it == m_bilder.end() ? nullptr : &it->second;
}

bool BildRegister::kenntShader(const std::string& name) const { return m_shader.count(shaderSchluessel(name)) != 0U; }

void BildRegister::leeren() {
    m_shader.clear();
    m_bilder.clear();
    m_warnungen.clear();
}

std::vector<std::string> weltShaderReihenfolge(const BspGeometry& geo) {
    std::vector<std::string> out;
    std::unordered_set<std::string> schon;
    const auto dazu = [&](const std::string& n) {
        if (!n.empty() && schon.insert(shaderSchluessel(n)).second) { out.push_back(n); }
    };
    for (const BspGeometry::Nebel& n : geo.nebel) { dazu(n.shader); }   // R_LoadFogs
    for (const BspSurface& s : geo.surfaces) {                          // R_LoadSurfaces
        if (s.type == BspSurface::Type::Bad || s.shader < 0 || static_cast<std::size_t>(s.shader) >= geo.shaders.size()) { continue; }
        dazu(geo.shaders[static_cast<std::size_t>(s.shader)].name);
    }
    return out;
}

}  // namespace bhed
