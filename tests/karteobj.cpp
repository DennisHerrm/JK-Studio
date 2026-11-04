// karteobj.cpp - rc609 KEMD: .obj im Karten-Editor wie in q3map2 (ctest karte_obj)
//
// Ein Spielordner unter TEMP mit models/kemd/kiste.obj und .mtl: zwei Materialien, eines mit Diffuse-Textur
// (Shader = Textur neben dem Modell), eines mit einem Spielshader als Namen ("textures/kemd/wand", die Textur
// zaehlt dann nicht). Der Editor laedt es ueber assimpLesen aus dem VFS; q3map2 -bhedmodels schreibt dasselbe
// Modell so, wie InsertModel es einbackt (LoadModel/AssModelMesh). Verglichen werden je Flaeche Shader, Ecken
// (Lage, st) und Dreiecke. Aufruf: karteobj <bhed_q3map2.exe>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "bhed/karte/modell.h"
#include "bhed/karte/texturen.h"

namespace {
int g_fehler = 0;
void expect(const std::string& was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was.c_str());
    if (!ok) { ++g_fehler; }
}
namespace fs = std::filesystem;
std::string text(const fs::path& p) {
    const std::u8string u = p.generic_u8string();
    return std::string(u.begin(), u.end());
}
void schreiben(const fs::path& p, const std::string& s) {
    std::error_code ec;
    fs::create_directories(p.parent_path(), ec);
    std::ofstream(p, std::ios::binary).write(s.data(), static_cast<std::streamsize>(s.size()));
}

// q3map2 -bhedmodels (BhedModelsMain): je Modell die Flaechen
struct Q3Flaeche {
    std::string shader;
    std::vector<float> xyz;
    std::vector<float> st;
    std::vector<std::uint32_t> idx;
};
bool bhmdLesen(const std::string& d, std::vector<Q3Flaeche>& out) {
    std::size_t o = 0;
    const auto u32 = [&](std::uint32_t& v) {
        if (o + 4 > d.size()) { return false; }
        std::memcpy(&v, d.data() + o, 4);
        o += 4;
        return true;
    };
    const auto str = [&](std::string& s) {
        std::uint32_t n = 0;
        if (!u32(n) || o + n > d.size()) { return false; }
        s.assign(d.data() + o, n);
        o += n;
        return true;
    };
    std::uint32_t v = 0;
    std::uint32_t modelle = 0;
    if (d.compare(0, 4, "BHMD") != 0) { return false; }
    o = 4;
    if (!u32(v) || v != 2 || !u32(modelle) || modelle != 1) { return false; }
    std::string name;
    std::uint32_t frame = 0;
    std::uint32_t geladen = 0;
    std::uint32_t netze = 0;
    if (!str(name) || !u32(frame) || !u32(geladen) || !u32(netze) || geladen != 1) { return false; }
    for (std::uint32_t n = 0; n < netze; ++n) {
        Q3Flaeche f;
        std::uint32_t ecken = 0;
        std::uint32_t dreiecke = 0;
        if (!str(f.shader) || !u32(ecken) || !u32(dreiecke)) { return false; }
        f.xyz.resize(ecken * 3);
        f.st.resize(ecken * 2);
        f.idx.resize(dreiecke * 3);
        const std::size_t b = (f.xyz.size() + f.st.size()) * 4 + f.idx.size() * 4;
        if (o + b > d.size()) { return false; }
        std::memcpy(f.xyz.data(), d.data() + o, f.xyz.size() * 4);
        o += f.xyz.size() * 4;
        std::memcpy(f.st.data(), d.data() + o, f.st.size() * 4);
        o += f.st.size() * 4;
        std::memcpy(f.idx.data(), d.data() + o, f.idx.size() * 4);
        o += f.idx.size() * 4;
        out.push_back(std::move(f));
    }
    return true;
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) {
        std::printf("Aufruf: karteobj <bhed_q3map2.exe>\n");
        return 2;
    }
    const fs::path tmp = fs::temp_directory_path() / "bhed_karteobj";
    std::error_code ec;
    fs::remove_all(tmp, ec);
    const fs::path base = tmp / "base";
    // zwei Materialien: kiste (Diffuse kiste.tga neben dem Modell), textures/kemd/wand (Spielshader als Name)
    schreiben(base / "models" / "kemd" / "kiste.mtl",
              "newmtl kiste\nKd 1 1 1\nmap_Kd kiste.tga\n\nnewmtl textures/kemd/wand\nKd 1 1 1\nmap_Kd egal.tga\n");
    schreiben(base / "models" / "kemd" / "kiste.obj",
              "mtllib kiste.mtl\n"
              "v -16 0 -16\nv 16 0 -16\nv 16 0 16\nv -16 0 16\nv -16 32 -16\nv 16 32 -16\nv 16 32 16\nv -16 32 16\n"
              "vt 0 0\nvt 1 0\nvt 1 1\nvt 0 1\n"
              "usemtl kiste\nf 1/1 2/2 6/3 5/4\nf 2/1 3/2 7/3 6/4\nf 3/1 4/2 8/3 7/4\n"
              "usemtl textures/kemd/wand\nf 4/1 1/2 5/3 8/4\nf 5/1 6/2 7/3 8/4\n");
    // der Editor: Spiel-VFS und assimpLesen (app_karte_modelle.cpp)
    bhed::karte::Dateisystem vfs;
    vfs.ordnerHinzufuegen(text(base), "");
    bhed::karte::Modell m;
    std::string fehler;
    const bool ok = bhed::karte::modellArtBekannt("models/kemd/kiste.obj") &&
                    bhed::karte::assimpLesen("models/kemd/kiste.obj",
                                             [&vfs](const std::string& p, std::string& b) { return vfs.dateiLesen(p, b); }, m, &fehler);
    expect("Editor laedt models/kemd/kiste.obj (" + fehler + ")", ok && m.flaechen.size() == 2);
    // q3map2: -bhedmodels mit demselben Spielordner
    schreiben(tmp / "liste.txt", "models/kemd/kiste.obj\t0\n");
    const std::string befehl = "\"\"" + std::string(argv[1]) + "\" -game ja -fs_basepath \"" + text(tmp) + "\" -bhedmodels \"" +
                               text(tmp / "liste.txt") + "\" \"" + text(tmp / "aus.bhmd") + "\" > \"" + text(tmp / "q3.txt") + "\" 2>&1\"";
    const int rc = std::system(befehl.c_str());
    std::string d;
    {
        std::ifstream f(tmp / "aus.bhmd", std::ios::binary);
        d.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
    }
    std::vector<Q3Flaeche> q3;
    expect("q3map2 -bhedmodels laedt das Modell (Rueckgabe " + std::to_string(rc) + ")", rc == 0 && bhmdLesen(d, q3));
    expect("gleich viele Flaechen (Editor " + std::to_string(m.flaechen.size()) + ", q3map2 " + std::to_string(q3.size()) + ")",
           q3.size() == m.flaechen.size() && !q3.empty());
    for (std::size_t i = 0; i < q3.size() && i < m.flaechen.size(); ++i) {
        const bhed::karte::ModellFlaeche& f = m.flaechen[i];
        const Q3Flaeche& q = q3[i];
        expect("Flaeche " + std::to_string(i) + ": Shader " + f.shader + " = " + q.shader, f.shader == q.shader);
        bool ecken = f.punkte.size() * 3 == q.xyz.size();
        float weit = 0;
        for (std::size_t k = 0; ecken && k < f.punkte.size(); ++k) {
            const auto& p = f.punkte[k];
            for (int a = 0; a < 3; ++a) { weit = std::max(weit, std::fabs(p.ort[a] - q.xyz[k * 3 + static_cast<std::size_t>(a)])); }
            weit = std::max({weit, std::fabs(p.st[0] - q.st[k * 2]), std::fabs(p.st[1] - q.st[k * 2 + 1])});
        }
        char t[160];
        std::snprintf(t, sizeof t, "Flaeche %zu: %zu Ecken, Lage und st gleich (groesste Abweichung %g)", i, f.punkte.size(), static_cast<double>(weit));
        expect(t, ecken && weit == 0.0F);
        expect("Flaeche " + std::to_string(i) + ": Dreiecke gleich", f.indizes == q.idx);
        // assimp GenFaceNormals verdoppelte Ecken ohne ihre st (Lesen hinter dem Feld, von Lauf zu Lauf anders): die
        // Textur des Wuerfels hat nur 0 und 1
        bool st01 = true;
        for (const auto& p : f.punkte) {
            for (int a = 0; a < 2; ++a) { st01 = st01 && (p.st[a] == 0.0F || p.st[a] == 1.0F); }
        }
        expect("Flaeche " + std::to_string(i) + ": st nur 0 oder 1 (keine Werte hinter dem Feld)", st01);
    }
    expect("Shader: Textur neben dem Modell", !m.flaechen.empty() && m.flaechen[0].shader == "models/kemd/kiste");
    expect("Shader: Spielshader als Materialname", m.flaechen.size() > 1 && m.flaechen[1].shader == "textures/kemd/wand");
    fs::remove_all(tmp, ec);
    std::printf("karte_obj: %d Fehler\n", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
