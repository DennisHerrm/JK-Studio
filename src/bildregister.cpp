// bildregister.cpp - siehe bhed/bildregister.h
#include "bhed/bildregister.h"

#include "bhed/bspgeo.h"
#include "bhed/efx/gp2.h"
#include "bhed/image.h"

#include <array>
#include <cctype>
#include <cstddef>
#include <cstdint>
#include <cstring>

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

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Little-Endian aus der Datei; false, wenn die Stelle hinter dem Ende liegt.
bool int32An(const std::string& d, std::size_t at, std::int32_t& out) {
    if (at > d.size() || d.size() - at < 4U) { return false; }
    std::uint32_t v = 0;
    for (int i = 3; i >= 0; --i) { v = (v << 8U) | static_cast<unsigned char>(d[at + static_cast<std::size_t>(i)]); }
    std::memcpy(&out, &v, 4);
    return true;
}

// char name[64] (MAX_QPATH) bis zur ersten Null.
std::string name64An(const std::string& d, std::size_t at) {
    if (at > d.size() || d.size() - at < 64U) { return {}; }
    const char* p = d.data() + at;
    std::size_t n = 0;
    while (n < 64U && p[n] != '\0') { ++n; }
    return std::string(p, n);
}

// R_LoadMD3 (tr_model.cpp:784-930): MD3_VERSION 15, mindestens ein Bild, dann je Flaeche (ofsSurfaces, ofsEnd weiter) jeder
// md3Shader_t (name[64], shaderIndex) - R_FindShader in dieser Reihenfolge.
std::vector<std::string> md3Shader(const std::string& d) {
    std::vector<std::string> out;
    std::int32_t version = 0;
    std::int32_t bilder = 0;
    std::int32_t flaechen = 0;
    std::int32_t ofs = 0;
    if (!int32An(d, 4, version) || version != 15 || !int32An(d, 76, bilder) || bilder < 1 || !int32An(d, 84, flaechen) ||
        !int32An(d, 100, ofs) || ofs < 0) {
        return out;
    }
    std::size_t at = static_cast<std::size_t>(ofs);
    for (std::int32_t i = 0; i < flaechen; ++i) {
        std::int32_t zahl = 0;
        std::int32_t ofsShader = 0;
        std::int32_t ofsEnde = 0;
        if (!int32An(d, at + 76, zahl) || !int32An(d, at + 92, ofsShader) || !int32An(d, at + 104, ofsEnde) || ofsEnde <= 0 ||
            ofsShader < 0) {
            break;
        }
        for (std::int32_t j = 0; j < zahl; ++j) {
            out.push_back(name64An(d, at + static_cast<std::size_t>(ofsShader) + static_cast<std::size_t>(j) * 68U));
        }
        at += static_cast<std::size_t>(ofsEnde);
    }
    return out;
}

// R_LoadMDXM (tr_ghoul2.cpp:3600-3751): MDXM_VERSION 6, je mdxmSurfHierarchy_t (name[64], flags, shader[64], shaderIndex,
// parentIndex, numChildren, childIndexes[numChildren]) der Shader; "[nomaterial]" wird leer (:3738-3741).
std::vector<std::string> glmShader(const std::string& d) {
    std::vector<std::string> out;
    std::int32_t version = 0;
    std::int32_t flaechen = 0;
    std::int32_t ofs = 0;
    if (!int32An(d, 4, version) || version != 6 || !int32An(d, 152, flaechen) || !int32An(d, 156, ofs) || ofs < 0) { return out; }
    std::size_t at = static_cast<std::size_t>(ofs);
    for (std::int32_t i = 0; i < flaechen; ++i) {
        std::int32_t kinder = 0;
        if (!int32An(d, at + 140, kinder) || kinder < 0) { break; }
        std::string sh = name64An(d, at + 68);
        if (!sh.empty() && sh[0] == '[') { sh.clear(); }
        out.push_back(std::move(sh));
        at += 144U + static_cast<std::size_t>(kinder) * 4U;
    }
    return out;
}

// CommaParse (tr_skin.cpp:45-140): Leerraum, // und /* */ ueberspringen, "..." am Stueck, sonst bis Leerraum oder Komma.
std::string kommaWort(const std::string& t, std::size_t& p) {
    for (;;) {
        while (p < t.size() && static_cast<unsigned char>(t[p]) <= ' ') { ++p; }
        if (p + 1 < t.size() && t[p] == '/' && t[p + 1] == '/') {
            while (p < t.size() && t[p] != '\n') { ++p; }
        } else if (p + 1 < t.size() && t[p] == '/' && t[p + 1] == '*') {
            while (p < t.size() && !(t[p] == '*' && p + 1 < t.size() && t[p + 1] == '/')) { ++p; }
            if (p < t.size()) { p += 2; }
        } else {
            break;
        }
    }
    std::string w;
    if (p >= t.size()) { return w; }
    if (t[p] == '"') {
        ++p;
        while (p < t.size() && t[p] != '"') { w += t[p++]; }
        if (p < t.size()) { ++p; }
        return w;
    }
    while (p < t.size() && static_cast<unsigned char>(t[p]) > ' ' && t[p] != ',') { w += t[p++]; }
    return w;
}

}  // namespace

void BildRegister::modell(const std::string& pfad, const Leser& lies, const Skripte& skripte) {
    if (pfad.empty() || !lies) { return; }
    const bool md3 = klein(pfad).find(".md3") != std::string::npos;   // strstr(name, ".md3") (tr_model.cpp:634)
    for (int stufe = md3 ? 2 : 0; stufe >= 0; --stufe) {
        std::string datei = pfad;
        if (stufe != 0) {
            const std::size_t punkt = datei.find_last_of('.');
            if (punkt != std::string::npos) { datei.resize(punkt); }
            datei += "_" + std::to_string(stufe) + ".md3";
        }
        std::string d;
        if (!lies(datei, d) || d.size() < 4U) { continue; }
        std::vector<std::string> namen;
        if (d.compare(0, 4, "IDP3") == 0) {
            namen = md3Shader(d);
        } else if (d.compare(0, 4, "2LGM") == 0) {
            namen = glmShader(d);
        }
        for (const std::string& n : namen) { shader(n, skripte); }
    }
}

void BildRegister::haut(const std::string& name, const Leser& lies, const Skripte& skripte) {
    if (name.empty() || !lies) { return; }
    // RE_RegisterIndividualSkin: false, wenn die Datei fehlt (dann laedt RE_RegisterSkin die weiteren Teile nicht)
    const auto eine = [&](const std::string& datei) {
        std::string t;
        if (!lies(datei, t)) { return false; }
        std::size_t p = 0;
        int zahl = 0;
        while (p < t.size()) {
            const std::string flaeche = klein(kommaWort(t, p));
            if (flaeche.empty()) { break; }
            if (p < t.size() && t[p] == ',') { ++p; }
            if (flaeche.compare(0, 4, "tag_") == 0) { continue; }
            const std::string sh = kommaWort(t, p);
            if (flaeche.size() >= 4U && flaeche.compare(flaeche.size() - 4, 4, "_off") == 0 && sh == "*off") { continue; }
            if (zahl >= 128) { break; }   // skin_t::surfaces[128] (tr_local.h:517)
            shader(sh, skripte);
            ++zahl;
        }
        return true;
    };
    // RE_SplitSkins (tr_skin.cpp:162-200): "ordner/|kopf|rumpf|beine"
    const std::size_t a = name.find('|');
    const std::size_t b = a == std::string::npos ? a : name.find('|', a + 1);
    const std::size_t c = b == std::string::npos ? b : name.find('|', b + 1);
    if (c == std::string::npos) {
        (void)eine(name);
        return;
    }
    const std::string ordner = name.substr(0, a);
    const std::string kopf = ordner + name.substr(a + 1, b - a - 1) + ".skin";
    const std::string rumpf = ordner + name.substr(b + 1, c - b - 1) + ".skin";
    const std::string beine = ordner + name.substr(c + 1) + ".skin";
    if (!eine(kopf)) { return; }
    if (kopf != rumpf && !eine(rumpf)) { return; }
    if (kopf != beine && rumpf != beine) { (void)eine(beine); }
}

void BildRegister::effekt(const std::string& pfad, const Leser& lies, const Skripte& skripte) {
    if (pfad.empty() || !lies) { return; }
    std::string ohne = pfad;   // COM_StripExtension
    const std::size_t punkt = ohne.find_last_of('.');
    if (punkt != std::string::npos && ohne.find('/', punkt) == std::string::npos) { ohne.resize(punkt); }
    if (m_effekte.count(klein(ohne)) != 0U) { return; }
    std::string text;
    if (!lies("effects/" + ohne + ".efx", text)) { return; }
    const efx::gp2::ParseResult r = efx::gp2::parse(text);
    if (!r.ok()) { return; }   // CGenericParser2::Parse schlaegt fehl -> RegisterEffect gibt auf
    m_effekte.insert(klein(ohne));   // GetNewEffectTemplate traegt den Namen VOR den Primitiven ein
    static const std::array<const char*, 13> kArten = {"particle", "line",        "tail",    "sound", "cylinder",
                                                        "electricity", "emitter", "decal", "orientedparticle",
                                                        "fxrunner", "light",      "camerashake", "flash"};
    for (const efx::gp2::Group& g : r.topLevel.subGroups) {
        const std::string art = klein(g.name);
        bool bekannt = false;
        for (const char* k : kArten) { bekannt = bekannt || art == k; }
        if (!bekannt) { continue; }
        for (const efx::gp2::Property& pr : g.properties) {
            const std::string n = klein(pr.name);
            for (const std::string& v : pr.values) {
                if (v.empty()) { continue; }
                if (n == "shader" || n == "shaders") {
                    shader(v, skripte);
                } else if (n == "model" || n == "models") {
                    modell(v, lies, skripte);
                } else if (n == "impactfx" || n == "deathfx" || n == "emitfx" || n == "playfx") {
                    effekt(v, lies, skripte);
                }
            }
        }
    }
}

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
    const auto da = [this](const std::string& b) { return !m_vorhanden || m_vorhanden(b); };
    if (si == nullptr) {
        if (!da(name)) {   // R_FindShader ohne Skript und ohne Bild: defaultShader (tr_shader.cpp:3502-3509)
            m_fehlerhaft.insert(schl);
            return;
        }
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
        // animMap: jedes Bild nacheinander (:1323-1335); fehlt eins, endet ParseShader dort (MR9 Punkt 2)
        const std::vector<std::string> einzeln{st.image};
        for (const std::string& b : st.animImages.empty() ? einzeln : st.animImages) {
            if (b.empty() || b[0] == '$' || b[0] == '*') { continue; }
            if (!da(b)) {
                m_fehlerhaft.insert(schl);
                return;
            }
            bild(b, w);
        }
    }
    if (si->himmelVorStufe >= si->stages.size()) { himmel(); }
}

const BildWahl* BildRegister::wahl(const std::string& bild) const {
    const auto it = m_bilder.find(image::mappingName(bild));
    return it == m_bilder.end() ? nullptr : &it->second;
}

bool BildRegister::kenntShader(const std::string& name) const { return m_shader.count(shaderSchluessel(name)) != 0U; }

bool BildRegister::fehlerhaft(const std::string& name) const { return m_fehlerhaft.count(shaderSchluessel(name)) != 0U; }

void BildRegister::leeren() {
    m_shader.clear();
    m_effekte.clear();
    m_fehlerhaft.clear();
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
