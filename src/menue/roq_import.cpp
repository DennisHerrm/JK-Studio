// roq_import.cpp - Menue-Editor M2 Punkt 2: Bildfolgen (animiertes GIF,
// Videos ueber die Oberflaeche) in ein RoQ verwandeln.
//
// Eigener Code. Das GIF liest stb_image (public domain, Sean Barrett,
// third_party/assimp/contrib/stb/stb_image.h - nur der GIF-Teil, mit
// STB_IMAGE_STATIC in dieser Datei, getrennt von der Kopie in assimp).
#include "bhed/menue/roq.h"

#include <algorithm>
#include <cstring>

#if defined(_MSC_VER)
#pragma warning(push, 0)
#pragma warning(disable : 4100 4127 4244 4245 4456 4457 4505 4701 4702 6001 6011 6246 6262 6385 6386 26451)
#endif
#define STB_IMAGE_STATIC
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_GIF
#define STBI_NO_STDIO
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#include "../../third_party/assimp/contrib/stb/stb_image.h"
#if defined(_MSC_VER)
#pragma warning(pop)
#endif

namespace bhed::menue {

bool gifLesen(const std::string& daten, Bildfolge& out) {
    out = Bildfolge{};
    if (daten.size() < 6 || daten.compare(0, 3, "GIF") != 0) { return false; }
    int* dauer = nullptr;
    int w = 0, h = 0, n = 0, comp = 0;
    stbi_uc* p = stbi_load_gif_from_memory(reinterpret_cast<const stbi_uc*>(daten.data()), static_cast<int>(daten.size()), &dauer, &w, &h, &n,
                                           &comp, 4);
    if (p == nullptr || w <= 0 || h <= 0 || n <= 0) {
        if (p != nullptr) { stbi_image_free(p); }
        if (dauer != nullptr) { STBI_FREE(dauer); }
        return false;
    }
    const std::size_t groesse = static_cast<std::size_t>(w) * static_cast<std::size_t>(h) * 4;
    for (int i = 0; i < n; ++i) {
        RoqBild b;
        b.breite = w;
        b.hoehe = h;
        b.rgba.assign(p + groesse * static_cast<std::size_t>(i), p + groesse * static_cast<std::size_t>(i + 1));
        // Durchsichtiges wie die Engine es zeichnet: auf Schwarz
        for (std::size_t k = 0; k < groesse; k += 4) {
            const unsigned a = b.rgba[k + 3];
            for (std::size_t c = 0; c < 3; ++c) { b.rgba[k + c] = static_cast<std::uint8_t>(b.rgba[k + c] * a / 255U); }
            b.rgba[k + 3] = 255;
        }
        out.bilder.push_back(std::move(b));
        // GIF: Verzoegerung in 1/100 s; 0 oder 1 spielen Browser mit 100 ms
        const int d = dauer != nullptr ? dauer[i] : 100;
        out.dauerMs.push_back(d <= 10 ? 100 : d);
    }
    stbi_image_free(p);
    if (dauer != nullptr) { STBI_FREE(dauer); }
    return true;
}

std::vector<std::size_t> aufFps(const Bildfolge& f, int fps) {
    std::vector<std::size_t> r;
    if (f.bilder.empty() || fps <= 0) { return r; }
    long long gesamt = 0;
    for (std::size_t i = 0; i < f.bilder.size(); ++i) { gesamt += i < f.dauerMs.size() ? std::max(1, f.dauerMs[i]) : 1000 / fps; }
    const long long n = std::max<long long>(1, gesamt * fps / 1000);
    std::size_t bild = 0;
    long long bisMs = f.dauerMs.empty() ? 1000 / fps : std::max(1, f.dauerMs[0]);
    for (long long k = 0; k < n; ++k) {
        const long long t = k * 1000 / fps;
        while (t >= bisMs && bild + 1 < f.bilder.size()) {
            ++bild;
            bisMs += bild < f.dauerMs.size() ? std::max(1, f.dauerMs[bild]) : 1000 / fps;
        }
        r.push_back(bild);
    }
    return r;
}

std::string kodiereRoq(const Bildfolge& f, const RoqOptionen& o, int maxSeite, const std::function<bool(int, int)>& fortschritt) {
    if (f.bilder.empty()) { return {}; }
    const std::vector<std::size_t> folge = aufFps(f, o.fps);
    RoqKodierer k;
    const RoqBild erstes = bildAnpassen(f.bilder.front(), maxSeite);
    if (!k.beginne(erstes.breite, erstes.hoehe, o)) { return {}; }
    std::size_t zuletzt = static_cast<std::size_t>(-1);
    RoqBild angepasst;
    for (std::size_t i = 0; i < folge.size(); ++i) {
        if (folge[i] != zuletzt) {
            angepasst = bildAnpassen(f.bilder[folge[i]], maxSeite);
            zuletzt = folge[i];
        }
        k.bild(angepasst);
        if (fortschritt && !fortschritt(static_cast<int>(i + 1), static_cast<int>(folge.size()))) { return {}; }
    }
    return k.fertig();
}

}  // namespace bhed::menue
