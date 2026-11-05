// modell_assimp.cpp - rc609 KEMD: Modelle ueber assimp im Karten-Editor (.obj, .fbx, .gltf, .glb)
//
// Portiert aus NetRadiant-Custom 20260114, plugins/assmodel: loadPicoModel (model.cpp:665-693, dieselben
// assimp-Schritte und dieselbe Drehung Y oben -> Z oben), PicoSurface::CopyPicoSurface (model.cpp:217-296,
// Shadername aus Material oder Diffuse-Textur) und AssIOSystem (plugin.cpp:94-160, Dateien aus dem Spiel-VFS).
// q3map2 laedt misc_model genauso (tools/quake3/q3map2/model.cpp: LoadModel, AssModelMesh) - darum zeigt der
// Editor dieselben Flaechen und Shader, die der Bau einbackt. rc610 MODOBJ: alle Leser von assimp (wie NRCs
// libassimp-6.dll und der q3map2), die Endungen wie Radiant_RegisterModules (plugin.cpp:304-319).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   This program is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free Software
//   Foundation; either version 2 of the License, or (at your option) any later
//   version (LICENSE).
#include "bhed/karte/modell.h"

#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/MemoryIOWrapper.h>
#include <assimp/material.h>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

#include <algorithm>
#include <cctype>
#include <cstring>
#include <vector>

namespace bhed::karte {

namespace {

std::string kleinA(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string bereinigtA(std::string s) {   // PathCleaned
    for (char& c : s) {
        if (c == '\\') { c = '/'; }
    }
    return s;
}

std::string ohneEndungA(const std::string& s) {   // PathExtensionless
    const std::size_t punkt = s.find_last_of('.');
    const std::size_t schraeg = s.find_last_of("/\\");
    return punkt != std::string::npos && (schraeg == std::string::npos || punkt > schraeg) ? s.substr(0, punkt) : s;
}

bool praefixA(const std::string& s, const char* p) { return kleinA(s).rfind(p, 0) == 0; }   // string_equal_prefix_nocase

// AssIOSystem (plugin.cpp:94-160): nur Dateien aus dem VFS, keine Rueckstriche
class VfsIo final : public Assimp::IOSystem {
public:
    explicit VfsIo(const ModellDateiLeser& leser) : m_leser(leser) {}
    bool Exists(const char* datei) const override {
        if (std::strchr(datei, '\\') != nullptr) { return false; }
        std::string b;
        return m_leser(datei, b);
    }
    char getOsSeparator() const override { return '/'; }
    Assimp::IOStream* Open(const char* datei, const char* /*modus*/) override {
        if (std::strchr(datei, '\\') != nullptr) { return nullptr; }
        std::string b;
        if (!m_leser(datei, b)) { return nullptr; }
        auto* puffer = new std::uint8_t[b.size()];
        std::memcpy(puffer, b.data(), b.size());
        return new Assimp::MemoryIOStream(puffer, b.size(), true);
    }
    void Close(Assimp::IOStream* datei) override { delete datei; }

private:
    const ModellDateiLeser& m_leser;
};

// CopyPicoSurface (model.cpp:217-262): der Shadername
std::string shaderName(const aiScene* sc, const aiMesh* mesh, const std::string& wurzel) {
    const aiMaterial* material = sc->mMaterials[mesh->mMaterialIndex];
    const std::string matname = material->GetName().C_Str();
    std::string shader;
    aiString texname;
    if (material->Get(AI_MATKEY_TEXTURE_DIFFUSE(0), texname) == aiReturn_SUCCESS && texname.length != 0 &&
        !praefixA(matname, "textures/") && !praefixA(matname, "textures\\") && !praefixA(matname, "models/") && !praefixA(matname, "models\\")) {
        shader = bereinigtA(ohneEndungA(texname.C_Str()));
    } else {
        shader = bereinigtA(ohneEndungA(matname));
    }
    if (shader.find('/') == std::string::npos) {   // texture is likely in the folder, where model is
        return wurzel + shader;
    }
    if (shader[0] == '/' || (shader.size() > 1 && shader[1] == ':') || shader.find("..") != std::string::npos) {   // absolute path or with ..
        const std::string k = kleinA(shader);
        std::size_t p = k.find("/models/");
        if (p == std::string::npos) { p = k.find("/textures/"); }
        if (p != std::string::npos) { return shader.substr(p + 1); }
        const std::size_t s = shader.find_last_of('/');
        return wurzel + shader.substr(s + 1);
    }
    return shader;
}

}  // namespace

const std::vector<std::string>& modellEndungenAssimp() {
    // Radiant_RegisterModules (assmodel/plugin.cpp:304-319): GetExtensionList "*.3ds;*.obj;*.dae", je Endung ein
    // ModelLoader - ohne bsp und pk3 (der Leser geht nicht), md5anim und md5camera (kein Modell) und ogex (stuerzt ab)
    static const std::vector<std::string> liste = [] {
        Assimp::Importer importer;
        aiString endungen;
        importer.GetExtensionList(endungen);
        std::vector<std::string> v;
        const std::string text = endungen.C_Str();
        std::size_t a = 0;
        while (a < text.size()) {
            std::size_t b = text.find(';', a);
            if (b == std::string::npos) { b = text.size(); }
            const std::size_t punkt = text.find('.', a);
            if (punkt != std::string::npos && punkt < b) {
                const std::string e = kleinA(text.substr(punkt + 1, b - punkt - 1));
                if (!e.empty() && e != "bsp" && e != "pk3" && e != "md5anim" && e != "md5camera" && e != "ogex" &&
                    std::find(v.begin(), v.end(), e) == v.end()) {
                    v.push_back(e);
                }
            }
            a = b + 1;
        }
        return v;
    }();
    return liste;
}

bool modellArtAssimp(const std::string& pfad) {
    const std::size_t p = pfad.find_last_of('.');
    const std::size_t s = pfad.find_last_of('/');
    if (p == std::string::npos || (s != std::string::npos && p < s)) { return false; }
    const std::string e = kleinA(pfad.substr(p + 1));
    if (e == "md3" || e == "ase") { return false; }   // eigene Lader (MD3Model_read; .ase wie picomodel)
    const std::vector<std::string>& v = modellEndungenAssimp();
    return std::find(v.begin(), v.end(), e) != v.end();
}

bool assimpLesen(const std::string& pfad, const ModellDateiLeser& leser, Modell& m, std::string* fehler) {
    Assimp::Importer importer;
    // plugin.cpp:200-205 (die Einstellungen fuer md3/mdl spielen hier keine Rolle)
    importer.SetPropertyBool(AI_CONFIG_PP_PTV_ADD_ROOT_TRANSFORMATION, true);
    importer.SetPropertyInteger(AI_CONFIG_PP_SBP_REMOVE, aiPrimitiveType_POINT | aiPrimitiveType_LINE);
    importer.SetIOHandler(new VfsIo(leser));
    const unsigned flags = aiProcess_GenNormals | aiProcess_JoinIdenticalVertices | aiProcess_Triangulate | aiProcess_GenUVCoords |
                           aiProcess_SortByPType | aiProcess_FindDegenerates | aiProcess_FindInvalidData | aiProcess_ValidateDataStructure |
                           aiProcess_FlipUVs | aiProcess_FlipWindingOrder | aiProcess_PreTransformVertices;
    // rotate the whole scene 90 degrees around the x axis to convert assimp's Y = UP to Quakes's Z = UP
    importer.SetPropertyMatrix(AI_CONFIG_PP_PTV_ROOT_TRANSFORMATION, aiMatrix4x4(1, 0, 0, 0, 0, 0, -1, 0, 0, 1, 0, 0, 0, 0, 0, 1));
    const aiScene* sc = importer.ReadFile(pfad, flags);
    if (sc == nullptr || sc->mRootNode == nullptr) {
        if (fehler != nullptr) { *fehler = importer.GetErrorString(); }
        return false;
    }
    const std::size_t s = pfad.find_last_of('/');
    const std::string wurzel = s == std::string::npos ? std::string() : pfad.substr(0, s + 1);   // PathFilenameless
    m = Modell{};
    // PicoModel::CopyPicoModel: die Flaechen in der Reihenfolge der Knoten (nach PreTransformVertices: die Wurzel)
    const auto durchlaufen = [&](const auto& selbst, const aiNode* k) -> void {
        for (unsigned n = 0; n < k->mNumMeshes; ++n) {
            const aiMesh* mesh = sc->mMeshes[k->mMeshes[n]];
            if ((mesh->mPrimitiveTypes & aiPrimitiveType_TRIANGLE) == 0) { continue; }
            ModellFlaeche f;
            f.shader = shaderName(sc, mesh, wurzel);
            for (unsigned i = 0; i < mesh->mNumVertices; ++i) {
                ModellPunkt p;
                p.ort = Vektor3f(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z);
                if (mesh->HasNormals()) { p.normale = Vektor3f(mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z); }
                if (mesh->HasTextureCoords(0)) { p.st = Vektor2f(mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y); }
                f.punkte.push_back(p);
            }
            for (unsigned t = 0; t < mesh->mNumFaces; ++t) {
                for (unsigned i = 0; i < 3; ++i) { f.indizes.push_back(mesh->mFaces[t].mIndices[i]); }
            }
            m.flaechen.push_back(std::move(f));
        }
        for (unsigned n = 0; n < k->mNumChildren; ++n) { selbst(selbst, k->mChildren[n]); }
    };
    durchlaufen(durchlaufen, sc->mRootNode);
    modellGrenzenRechnen(m);
    return true;
}

}  // namespace bhed::karte
