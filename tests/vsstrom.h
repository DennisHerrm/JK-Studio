// vsstrom.h - Hilfe fuer die GPU-Proben (turbgpu, defgpu): einen Vertex-Shader so laufen lassen, wie das Programm ihn baut.
//
// FIX5 P35: Die Proben rechneten den HLSL-Text als Compute-Shader (cs_5_0, Fahnen 0); die Vorschau uebersetzte Karte und Figur als
// vs_4_0 mit den Fahnen 0 (uebersetzFahnen, gpumap_win32.cpp), das Echtzeit-Licht als vs_4_0, vs_4_1 oder vs_5_0 mit
// D3DCOMPILE_OPTIMIZATION_LEVEL3 (karte_gpu_win32.cpp) - vs_4_x war auf der Intel UHD Graphics nicht bitgleich, seitdem ab
// Merkmalsstufe 11_0 ueberall vs_5_0. Hier laeuft der Vertex-Shader selbst - mit Profil und Fahnen wie dort - ueber
// Stream-Output (ohne Rasterung): je Ecke ein Punkt, SV_Position und TEXCOORD0 kommen in einen Puffer. Dazu die Pruefung des Bytecodes
// (D3DDisassemble): in vs_4_x darf der Kopf kein refactoringAllowed tragen (dann darf der Treiber nichts umstellen und keine mul + add
// zu einem mad verschmelzen); in vs_5_0 traegt jede Rechnung aus precise-Zeilen die Kennzeichnung [precise].
#ifndef BHED_TESTS_VSSTROM_H
#define BHED_TESTS_VSSTROM_H

#include <d3d11.h>
#include <d3dcompiler.h>
#include <windows.h>

#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

namespace vsstrom {

template <class T>
void frei(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

// Eine Ecke fuer beide Eingaben: Karte (POSITION, NORMAL, TEXCOORD0, TEXCOORD1 @32, COLOR0 @40) und Figur (POSITION, NORMAL, TEXCOORD0,
// BLENDINDICES @32, BLENDWEIGHT @48) - 64 Byte.
struct Ecke {
    float pos[3];
    float normal[3];
    float uv[2];
    float a[4];   // Karte: uvLm.xy, farbe.rg; Figur: bones
    float b[4];   // Karte: farbe.ba (ab 40 gelesen); Figur: weights
};
static_assert(sizeof(Ecke) == 64, "64 Byte je Ecke");

enum class Eingabe { Karte, Figur };

struct Puffer {
    UINT slot;
    std::vector<float> daten;
};

// Der Bytecode als Text (D3DDisassemble)
inline std::string zerlegt(ID3DBlob* blob) {
    ID3DBlob* t = nullptr;
    std::string s;
    if (SUCCEEDED(D3DDisassemble(blob->GetBufferPointer(), blob->GetBufferSize(), 0, nullptr, &t)) && t != nullptr) {
        s.assign(static_cast<const char*>(t->GetBufferPointer()), t->GetBufferSize());
    }
    frei(t);
    return s;
}

// Darf der Treiber umstellen oder verschmelzen? "" = nein (Beleg), sonst die Stelle. vs_4_x: kein refactoringAllowed im Kopf
// (das ist alles, was SM4 ausdruecken kann - die Intel UHD Graphics hielt sich nicht daran, siehe gpumap_win32.cpp kartenVsZiel);
// vs_5_0: es gibt [precise]-Anweisungen, und mit keinMad (ein Shader nur aus precise-Rechnung) keine mad ohne [precise].
inline std::string umstellbar(const std::string& dis, const char* profil, bool keinMad = false) {
    const bool sm5 = std::strncmp(profil, "vs_5", 4) == 0;
    std::size_t p = 0;
    bool precise = false;
    while (p < dis.size()) {
        std::size_t e = dis.find('\n', p);
        if (e == std::string::npos) { e = dis.size(); }
        const std::string z = dis.substr(p, e - p);
        p = e + 1;
        if (z.find("//") != std::string::npos && z.find_first_not_of(' ') == z.find("//")) { continue; }
        if (z.find("refactoringAllowed") != std::string::npos && !sm5) { return "vs_4_x mit refactoringAllowed: " + z; }
        if (z.find("[precise") != std::string::npos) { precise = true; }
        const std::size_t w = z.find_first_not_of(' ');
        if (sm5 && keinMad && w != std::string::npos && z.compare(w, 4, "mad ") == 0) { return "mad ohne [precise]: " + z; }
    }
    if (sm5 && !precise) { return "vs_5_0 ohne [precise]"; }
    return "";
}

// Den Vertex-Shader `quelle` (Einstieg main) mit Profil und Fahnen uebersetzen und ueber die Ecken laufen lassen; aus: je Ecke
// SV_Position (4) und TEXCOORD0 (2) als Bitmuster. false: Geraet nicht da (name leer) oder nicht uebersetzt. zerlegung: der
// Bytecode als Text. zusatz: Werte je Ecke als Buffer<float4> an t0 (vs_4_0 kennt keine StructuredBuffer).
inline bool laufen(D3D_DRIVER_TYPE art, const std::string& quelle, const char* profil, UINT fahnen, Eingabe eingabe,
                   const std::vector<Puffer>& cbs, const std::vector<Ecke>& ecken, std::vector<std::uint32_t>& aus, std::string& name,
                   std::string* zerlegung = nullptr, const std::vector<float>* zusatz = nullptr) {
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    const D3D_FEATURE_LEVEL stufe = D3D_FEATURE_LEVEL_11_0;
    name.clear();
    if (FAILED(D3D11CreateDevice(nullptr, art, nullptr, 0, &stufe, 1, D3D11_SDK_VERSION, &dev, nullptr, &ctx))) { return false; }
    IDXGIDevice* dx = nullptr;
    IDXGIAdapter* ad = nullptr;
    DXGI_ADAPTER_DESC ds{};
    if (SUCCEEDED(dev->QueryInterface(__uuidof(IDXGIDevice), reinterpret_cast<void**>(&dx))) && SUCCEEDED(dx->GetAdapter(&ad))) { ad->GetDesc(&ds); }
    frei(ad);
    frei(dx);
    char n[128];
    std::snprintf(n, sizeof n, "%ls", ds.Description);
    ID3DBlob* blob = nullptr;
    ID3DBlob* err = nullptr;
    if (FAILED(D3DCompile(quelle.data(), quelle.size(), "vsstrom", nullptr, nullptr, "main", profil, fahnen, 0, &blob, &err))) {
        std::printf("%s\n", err != nullptr ? static_cast<const char*>(err->GetBufferPointer()) : "D3DCompile");
        frei(err);
        frei(ctx);
        frei(dev);
        name = n;
        return false;
    }
    frei(err);
    name = n;
    if (zerlegung != nullptr) { *zerlegung = zerlegt(blob); }
    ID3D11VertexShader* vs = nullptr;
    dev->CreateVertexShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, &vs);
    const D3D11_SO_DECLARATION_ENTRY so[] = {{0, "SV_POSITION", 0, 0, 4, 0}, {0, "TEXCOORD", 0, 0, 2, 0}};
    const UINT schritt = 24;
    ID3D11GeometryShader* gs = nullptr;
    dev->CreateGeometryShaderWithStreamOutput(blob->GetBufferPointer(), blob->GetBufferSize(), so, 2, &schritt, 1,
                                              D3D11_SO_NO_RASTERIZED_STREAM, nullptr, &gs);
    const D3D11_INPUT_ELEMENT_DESC karte[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                                              {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
                                              {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
                                              {"TEXCOORD", 1, DXGI_FORMAT_R32G32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
                                              {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    const D3D11_INPUT_ELEMENT_DESC figur[] = {{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
                                              {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
                                              {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0},
                                              {"BLENDINDICES", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 32, D3D11_INPUT_PER_VERTEX_DATA, 0},
                                              {"BLENDWEIGHT", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 48, D3D11_INPUT_PER_VERTEX_DATA, 0}};
    ID3D11InputLayout* il = nullptr;
    if (eingabe == Eingabe::Karte) {
        dev->CreateInputLayout(karte, 5, blob->GetBufferPointer(), blob->GetBufferSize(), &il);
    } else {
        dev->CreateInputLayout(figur, 5, blob->GetBufferPointer(), blob->GetBufferSize(), &il);
    }
    frei(blob);
    const auto anzahl = static_cast<UINT>(ecken.size());
    D3D11_BUFFER_DESC bv{};
    bv.ByteWidth = anzahl * static_cast<UINT>(sizeof(Ecke));
    bv.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA sv{ecken.data(), 0, 0};
    ID3D11Buffer* vb = nullptr;
    dev->CreateBuffer(&bv, &sv, &vb);
    D3D11_BUFFER_DESC bs{};
    bs.ByteWidth = anzahl * schritt;
    bs.BindFlags = D3D11_BIND_STREAM_OUTPUT;
    ID3D11Buffer* sob = nullptr;
    dev->CreateBuffer(&bs, nullptr, &sob);
    D3D11_BUFFER_DESC bl = bs;
    bl.BindFlags = 0;
    bl.Usage = D3D11_USAGE_STAGING;
    bl.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    ID3D11Buffer* lesen = nullptr;
    dev->CreateBuffer(&bl, nullptr, &lesen);
    std::vector<ID3D11Buffer*> kb;
    for (const Puffer& p : cbs) {
        std::vector<float> d = p.daten;
        d.resize((d.size() + 3U) / 4U * 4U, 0.0F);
        D3D11_BUFFER_DESC bd{};
        bd.ByteWidth = static_cast<UINT>(d.size() * 4U);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        D3D11_SUBRESOURCE_DATA sd{d.data(), 0, 0};
        ID3D11Buffer* b = nullptr;
        dev->CreateBuffer(&bd, &sd, &b);
        ctx->VSSetConstantBuffers(p.slot, 1, &b);
        kb.push_back(b);
    }
    ID3D11Buffer* zb = nullptr;
    ID3D11ShaderResourceView* zsrv = nullptr;
    if (zusatz != nullptr && !zusatz->empty()) {
        D3D11_BUFFER_DESC bz{};
        bz.ByteWidth = static_cast<UINT>(zusatz->size() * 4U);
        bz.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA sz{zusatz->data(), 0, 0};
        dev->CreateBuffer(&bz, &sz, &zb);
        D3D11_SHADER_RESOURCE_VIEW_DESC sr{};
        sr.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
        sr.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
        sr.Buffer.NumElements = static_cast<UINT>(zusatz->size() / 4U);
        dev->CreateShaderResourceView(zb, &sr, &zsrv);
        ctx->VSSetShaderResources(0, 1, &zsrv);
    }
    const UINT stride = sizeof(Ecke);
    const UINT versatz = 0;
    ctx->IASetVertexBuffers(0, 1, &vb, &stride, &versatz);
    ctx->IASetInputLayout(il);
    ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);
    ctx->VSSetShader(vs, nullptr, 0);
    ctx->GSSetShader(gs, nullptr, 0);
    ctx->SOSetTargets(1, &sob, &versatz);
    ctx->Draw(anzahl, 0);
    ID3D11Buffer* keins = nullptr;
    ctx->SOSetTargets(1, &keins, &versatz);
    ctx->CopyResource(lesen, sob);
    D3D11_MAPPED_SUBRESOURCE map{};
    aus.assign(static_cast<std::size_t>(anzahl) * 6U, 0U);
    const bool gut = vs != nullptr && gs != nullptr && il != nullptr && SUCCEEDED(ctx->Map(lesen, 0, D3D11_MAP_READ, 0, &map));
    if (gut) {
        std::memcpy(aus.data(), map.pData, aus.size() * 4U);
        ctx->Unmap(lesen, 0);
    }
    for (ID3D11Buffer*& b : kb) { frei(b); }
    frei(zsrv);
    frei(zb);
    frei(lesen);
    frei(sob);
    frei(vb);
    frei(il);
    frei(gs);
    frei(vs);
    frei(ctx);
    frei(dev);
    return gut;
}

}  // namespace vsstrom

#endif  // BHED_TESTS_VSSTROM_H
