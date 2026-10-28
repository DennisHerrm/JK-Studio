// karteassets.cpp - Stufe 9 Punkt 11: Asset-Browser (ctest karte_assets)
//
// Ein Spielordner unter TEMP mit einem MD3 (aus md3Paket), einem Klang (WAV) und einem Prefab
// im Import-Ordner: Sammeln je Art, Suche mit mehreren Woertern, der erste Shader des MD3, die
// Wellenform (halbe Lautstaerke -> ~0,5, Stille -> 0), der Grundriss des Prefabs.
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include "bhed/karte/assets.h"
#include "bhed/karte/geometrieimport.h"
#include "bhed/karte/modellimport.h"
#include "bhed/karte/texturen.h"
#include "bhed/sound.h"

namespace {
int g_fehler = 0;
void expect(const char* was, bool ok) {
    std::printf("  %s  %s\n", ok ? "ok  " : "FEHL", was);
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
std::string wav(const std::vector<std::int16_t>& s) {
    std::string w = "RIFF";
    const auto u32 = [&](std::uint32_t v) { w.append(reinterpret_cast<const char*>(&v), 4); };
    const auto u16 = [&](std::uint16_t v) { w.append(reinterpret_cast<const char*>(&v), 2); };
    u32(static_cast<std::uint32_t>(36 + s.size() * 2));
    w += "WAVEfmt ";
    u32(16);
    u16(1);
    u16(1);
    u32(22050);
    u32(44100);
    u16(2);
    u16(16);
    w += "data";
    u32(static_cast<std::uint32_t>(s.size() * 2));
    w.append(reinterpret_cast<const char*>(s.data()), s.size() * 2);
    return w;
}
}  // namespace

int main() {
    using namespace bhed;
    using namespace bhed::karte;
    std::error_code ec;
    const fs::path o = fs::temp_directory_path(ec) / "jkstudio_assets_test";
    fs::remove_all(o, ec);
    // ein MD3 mit Shader models/import/kiste/holz
    ImportNetz n;
    n.teile.resize(1);
    n.teile[0].material = "holz";
    n.teile[0].punkte = {{0, 0, 0}, {64, 0, 0}, {0, 64, 0}};
    n.teile[0].uv = {{0, 0}, {1, 0}, {0, 1}};
    n.teile[0].dreiecke = {{0, 1, 2}};
    const Md3Paket pk = md3Paket(n, "kiste");
    schreiben(o / "spiel" / "base" / "models" / "map_objects" / "kiste.md3", pk.dateien[0].inhalt);
    std::vector<std::int16_t> s(2205, 0);
    for (std::size_t i = 0; i < s.size() / 2; ++i) { s[i] = static_cast<std::int16_t>(16384.0 * std::sin(static_cast<double>(i) * 0.3)); }
    schreiben(o / "spiel" / "base" / "sound" / "ambience" / "wind.wav", wav(s));
    schreiben(o / "import" / "prefabs" / "saeule.map",
              "{\n\"classname\" \"worldspawn\"\n{\n( 0 0 0 ) ( 0 1 0 ) ( 0 0 1 ) a 0 0 0 1 1 0 0 0\n( 32 0 0 ) ( 32 0 1 ) ( 32 1 0 ) a 0 0 0 1 1 0 0 0\n"
              "( 0 0 0 ) ( 0 0 1 ) ( 1 0 0 ) a 0 0 0 1 1 0 0 0\n( 0 32 0 ) ( 1 32 0 ) ( 0 32 1 ) a 0 0 0 1 1 0 0 0\n"
              "( 0 0 0 ) ( 1 0 0 ) ( 0 1 0 ) a 0 0 0 1 1 0 0 0\n( 0 0 128 ) ( 0 1 128 ) ( 1 0 128 ) a 0 0 0 1 1 0 0 0\n}\n}\n");
    Dateisystem d;
    d.ordnerHinzufuegen(text(o / "spiel" / "base") + "/", "sp");
    const std::vector<Asset> a = assetsSammeln(&d, text(o / "import"));
    const auto f = [&](AssetArt art, const char* q) { return assetsFiltern(a, art, q); };
    expect("ein Modell, ein Klang, ein Prefab", f(AssetArt::Modell, "").size() == 1 && f(AssetArt::Klang, "").size() == 1 && f(AssetArt::Prefab, "").size() == 1);
    expect("Pfade im Spiel bzw. voller Pfad des Prefabs",
           !f(AssetArt::Modell, "").empty() && a[f(AssetArt::Modell, "")[0]].pfad == "models/map_objects/kiste.md3" &&
               a[f(AssetArt::Klang, "")[0]].pfad == "sound/ambience/wind.wav" && a[f(AssetArt::Prefab, "")[0]].name == "saeule.map");
    expect("Suche: alle Woerter ohne Gross/klein", f(AssetArt::Modell, "MAP kiste").size() == 1 && f(AssetArt::Modell, "map tisch").empty());
    std::string md3;
    (void)d.dateiLesen("models/map_objects/kiste.md3", md3);
    expect("erster Shader des MD3", md3ErsterShader(md3) == "models/import/kiste/holz");
    std::string roh;
    (void)d.dateiLesen("sound/ambience/wind.wav", roh);
    const sound::Sound so = sound::decodeWav(reinterpret_cast<const unsigned char*>(roh.data()), roh.size());
    const auto w = wellenform(so);
    expect("Wellenform: erste Haelfte ~0,5, zweite Haelfte still", so.ok && w[5] > 0.45F && w[5] <= 0.51F && w[60] == 0.0F);
    std::string pt;
    {
        std::ifstream i(o / "import" / "prefabs" / "saeule.map", std::ios::binary);
        pt.assign(std::istreambuf_iterator<char>(i), std::istreambuf_iterator<char>());
    }
    const PrefabUmriss u = prefabUmriss(pt);
    expect("Prefab-Grundriss: ein Brush 0..32 x 0..32", u.pinsel == 1 && u.x0 == 0 && u.y0 == 0 && u.x1 == 32 && u.y1 == 32);
    fs::remove_all(o, ec);
    std::printf("%s (%d Fehler)\n", g_fehler == 0 ? "OK" : "FEHLER", g_fehler);
    return g_fehler == 0 ? 0 : 1;
}
