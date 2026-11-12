// dxr_quelle.cpp - Bauwerkzeug: der HLSL-Text des Echtzeit-Lichts fuer dxc (Raytracing-Schatten, gui/karte_dxr.hlsl).
//
// Den Echtzeit-Licht-Shader gibt es EINMAL: als Text kShader4 in gui/karte_gpu_win32.cpp. Mit den Schattenkarten uebersetzt
// ihn das Programm zur Laufzeit (D3DCompile, DXBC). Fuer die Raytracing-Schatten braucht es DXIL (RayQuery, Shader Model 6.5),
// und das kann nur dxc - zur Bauzeit. Dieses Werkzeug schreibt dafuer denselben Text in eine Datei:
//   #define DXR_SCHATTEN 1, #define LM_FAKTOR (gpu::kSpielLightmapFaktor), #include "karte_dxr.hlsl",
//   gpu::tcModHlsl() und kShader4 (aus der Quelldatei gelesen: alle Roh-Literale der Definition, wie der Uebersetzer sie
//   aneinanderhaengt).
// Findet es die Definition nicht mehr, scheitert der Bau - laut, statt mit einem veralteten Shader.
//
//   dxr_quelle <gui/karte_gpu_win32.cpp> <ausgabe.hlsl>
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "bhed/gpushader.h"

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

namespace {

// Die Roh-Literale R"( ... )" einer Definition "const char* const <name> = R"( ... )" [Kommentar] R"( ... )";" aneinandergehaengt.
bool literalLesen(const std::string& quelle, const std::string& name, std::string& aus) {
    const std::string kopf = "const char* const " + name + " = R\"(";
    std::size_t ort = quelle.find(kopf);
    if (ort == std::string::npos) { return false; }
    ort += kopf.size();
    for (;;) {
        const std::size_t ende = quelle.find(")\"", ort);
        if (ende == std::string::npos) { return false; }
        aus.append(quelle, ort, ende - ort);
        ort = ende + 2;
        // bis zum naechsten Literal oder zum Semikolon: Leerraum und Zeilenkommentare
        for (;;) {
            while (ort < quelle.size() && (quelle[ort] == ' ' || quelle[ort] == '\t' || quelle[ort] == '\r' || quelle[ort] == '\n')) { ++ort; }
            if (quelle.compare(ort, 2, "//") == 0) {
                ort = quelle.find('\n', ort);
                if (ort == std::string::npos) { return false; }
                continue;
            }
            break;
        }
        if (ort < quelle.size() && quelle[ort] == ';') { return true; }
        if (quelle.compare(ort, 3, "R\"(") != 0) { return false; }
        ort += 3;
    }
}

}  // namespace

int main(int argc, char** argv) {
    if (argc != 3) {
        std::fprintf(stderr, "dxr_quelle <karte_gpu_win32.cpp> <ausgabe.hlsl>\n");
        return 2;
    }
    std::ifstream ein(argv[1], std::ios::binary);
    if (!ein) {
        std::fprintf(stderr, "dxr_quelle: %s nicht lesbar\n", argv[1]);
        return 1;
    }
    std::ostringstream puffer;
    puffer << ein.rdbuf();
    std::string shader;
    if (!literalLesen(puffer.str(), "kShader4", shader) || shader.find("DXR_SCHATTEN") == std::string::npos ||
        shader.find("float4 ps4(") == std::string::npos) {
        std::fprintf(stderr, "dxr_quelle: kShader4 (mit DXR_SCHATTEN und ps4) nicht in %s gefunden - die Stelle wurde geaendert\n", argv[1]);
        return 1;
    }
    char faktor[32];
    std::snprintf(faktor, sizeof(faktor), "%.6f", static_cast<double>(bhed::gpu::kSpielLightmapFaktor));   // wie lightmapFaktorText()
    std::string text = "// erzeugt von tools/dxr/dxr_quelle.cpp aus gui/karte_gpu_win32.cpp (kShader4) - nicht bearbeiten\n";
    text += "#define DXR_SCHATTEN 1\n#define LM_FAKTOR ";
    text += faktor;
    text += "\n#include \"karte_dxr.hlsl\"\n";
    text += bhed::gpu::tcModHlsl();
    text += shader;
    std::ofstream aus(argv[2], std::ios::binary);
    aus << text;
    if (!aus) {
        std::fprintf(stderr, "dxr_quelle: %s nicht schreibbar\n", argv[2]);
        return 1;
    }
    return 0;
}
