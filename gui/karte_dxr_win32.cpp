// karte_dxr_win32.cpp - Karten-Editor, Echtzeit-Licht: Schatten per Raytracing (DXR 1.1); siehe karte_dxr.h.
//
// Eigene D3D12-Teile (Beschleunigungsstrukturen) neben dem Abgeber: gebaut wird in die Befehlsliste des Bildes
// (rahmen::liste()), die Eingaben liegen im Hochladespeicher des Bildes (rahmen::hochladePlatz), freigegeben wird ueber
// rahmen::spaeterFrei - erst wenn die Grafikkarte alle Listen abgearbeitet hat, die das Objekt nennen. Alles, was der
// Shader sonst braucht (Filterbilder, Filterdaten, die DXIL-Shader selbst), sind gewoehnliche Objekte der Schnittstelle
// ID3D11Device des Abgebers.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#include "karte_dxr.h"

#include "bhed/diag.h"

#include <cstdlib>
#include <cstring>

#if defined(BHED_WITH_D3D11) && defined(BHED_DXR_SHADER)

#include "abgeber12.h"
#include "backend.h"
#include "bemalt.h"

#include <d3d11.h>
#include <d3d12.h>

#include <algorithm>
#include <chrono>
#include <string>
#include <unordered_map>
#include <utility>

namespace bhed::gui::kartegpu::dxr {

namespace {

namespace r12 = ::bhed::render::d3d12;

// DXIL (Shader Model 6.5) aus kShader4 mit gui/karte_dxr.hlsl, im Bau mit dxc uebersetzt (CMakeLists.txt, Block DXR)
#include "g_karte_dxr_vs4.h"
#include "g_karte_dxr_ps4.h"
#include "g_karte_dxr_ps4nebel.h"

template <class T>
void los(T*& p) {
    if (p != nullptr) {
        p->Release();
        p = nullptr;
    }
}

// Seiten fuer die unteren Strukturen: eine Struktur je Eimer waere als eigene Zuteilung mindestens 64 KB gross (kleine Eimer
// brauchen wenige KB). 4 MiB je Seite: bei 64 KB mittlerer Groesse rund 60 Eimer je Seite; groessere Strukturen bekommen ihre
// eigene Seite.
constexpr UINT64 kSeitenBytes = 4U * 1024U * 1024U;
// Bewegt ist ein Eimer, der sich binnen dieser Zeit wieder aendert (ein Pinsel wird gezogen: jede Mausbewegung ein neuer
// Stand); ruht er so lange, wird er einmal auf schnelle Strahlen neu gebaut. Eine halbe Sekunde: so lange haelt die Hand beim
// Ziehen selten still, und kuerzer baute ein langsames Ziehen jeden Stand doppelt.
constexpr double kBewegtSekunden = 0.5;
// Seite des Atlas der Filterbilder (mindestens; ein groesseres Bild vergroessert sie)
constexpr int kAtlasSeite = 1024;

using Uhr = std::chrono::steady_clock;

struct Seite {
    ID3D12Resource* res = nullptr;
    UINT64 groesse = 0;
    UINT64 belegt = 0;
    std::size_t lebend = 0;
};

struct Blas {
    std::uint64_t summe = 0;
    int art = -1;
    void* bild = nullptr;
    int filter = 0;
    int seite = -1;
    UINT64 bytes = 0;
    D3D12_GPU_VIRTUAL_ADDRESS adresse = 0;
    std::size_t dreiecke = 0;
    std::uint32_t lauf = 0;            // geoSetzen, das den Eimer zuletzt sah
    Uhr::time_point gebaut{};
    bool schnell = false;              // mit schnellem Bau (bewegt): nach der Ruhezeit nachziehen
    std::vector<float> orte;           //   ... dafuer seine Ecken (je Dreieck neun Werte)
    std::vector<float> st;             // art 3: je Dreieck sechs Werte
    int filterNummer = -1;             // art 3: Nummer des Filter-Teils (Kennung der Instanz)
};

struct FilterBild {
    int breite = 0;
    int hoehe = 0;
    std::vector<unsigned char> rgba;
    bool lesbar = false;
    int seite = 0;                     // Platz im Atlas
    int x = 0;
    int y = 0;
    std::uint32_t lauf = 0;
};

struct Zustand {
    bool geprueft = false;
    unsigned geraet = ~0U;             // render::texturStand() der Pruefung
    Grund grund = Grund::NichtD3d12;
    ID3D12Device5* geraet5 = nullptr;
    ID3D11VertexShader* vs = nullptr;
    ID3D11PixelShader* ps = nullptr;
    ID3D11PixelShader* psNebel = nullptr;
    std::vector<Seite> seiten;
    int offeneSeite = -1;              // in dieser wird vergeben
    std::unordered_map<std::uint64_t, Blas> blas;
    ID3D12Resource* kratz = nullptr;   // Arbeitspuffer der Baue
    UINT64 kratzGroesse = 0;
    ID3D12Resource* tlas = nullptr;
    UINT64 tlasGroesse = 0;
    ID3D11ShaderResourceView* tlasSicht = nullptr;
    ID3D11Buffer* daten = nullptr;
    ID3D11ShaderResourceView* datenSrv = nullptr;
    std::size_t datenBytes = 0;
    ID3D11Texture2D* bilder = nullptr;
    ID3D11ShaderResourceView* bilderSrv = nullptr;
    std::size_t bilderBytes = 0;
    std::unordered_map<void*, FilterBild> filterBilder;
    bool atlasAlt = true;              // der Atlas passt nicht mehr zu filterBilder
    bool szeneSteht = false;
    bool an = false;
    std::uint32_t lauf = 0;
    std::size_t schnelle = 0;          // Eimer mit schnellem Bau
    std::size_t gemeldet = static_cast<std::size_t>(-1);   // Zahl der Strukturen der letzten Protokollzeile
    Stand stand;
};
Zustand g;

bool gespielt(const char* was) {
    const char* f = std::getenv("BHED_DXR_FEHLER");
    return f != nullptr && std::strcmp(f, was) == 0;
}

// Ein D3D12-Objekt abgeben: die Grafikkarte kann es noch brauchen
void spaeterLos(ID3D12Resource*& r) {
    if (r == nullptr) { return; }
    if (r12::rahmen::geraet() != nullptr) {
        r12::rahmen::spaeterFrei(r);
    } else {
        r->Release();
    }
    r = nullptr;
}

ID3D12Resource* pufferAnlegen(UINT64 groesse, D3D12_RESOURCE_STATES zustand) {
    D3D12_HEAP_PROPERTIES hp{};
    hp.Type = D3D12_HEAP_TYPE_DEFAULT;
    D3D12_RESOURCE_DESC rd{};
    rd.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
    rd.Width = groesse;
    rd.Height = 1;
    rd.DepthOrArraySize = 1;
    rd.MipLevels = 1;
    rd.SampleDesc.Count = 1;
    rd.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
    rd.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
    ID3D12Resource* r = nullptr;
    if (FAILED(g.geraet5->CreateCommittedResource(&hp, D3D12_HEAP_FLAG_NONE, &rd, zustand, nullptr, IID_PPV_ARGS(&r)))) { return nullptr; }
    return r;
}

Grund pruefen() {
    if (render::current() != render::Backend::Direct3D12 || r12::rahmen::geraet() == nullptr || gespielt("d3d12")) { return Grund::NichtD3d12; }
    if (gespielt("shader")) { return Grund::KeinShader; }
    ID3D12Device* d = r12::rahmen::geraet();
    D3D12_FEATURE_DATA_D3D12_OPTIONS5 o5{};
    D3D12_FEATURE_DATA_SHADER_MODEL sm{D3D_SHADER_MODEL_6_5};
    if (gespielt("geraet") || FAILED(d->CheckFeatureSupport(D3D12_FEATURE_D3D12_OPTIONS5, &o5, sizeof(o5))) ||
        o5.RaytracingTier < D3D12_RAYTRACING_TIER_1_1 || FAILED(d->CheckFeatureSupport(D3D12_FEATURE_SHADER_MODEL, &sm, sizeof(sm))) ||
        sm.HighestShaderModel < D3D_SHADER_MODEL_6_5 || FAILED(d->QueryInterface(IID_PPV_ARGS(&g.geraet5))) || g.geraet5 == nullptr) {
        return Grund::KeinDxr;
    }
    auto* d11 = static_cast<ID3D11Device*>(render::d3dDevice());
    if (d11 == nullptr || FAILED(d11->CreateVertexShader(g_karte_dxr_vs4, sizeof(g_karte_dxr_vs4), nullptr, &g.vs)) ||
        FAILED(d11->CreatePixelShader(g_karte_dxr_ps4, sizeof(g_karte_dxr_ps4), nullptr, &g.ps)) ||
        FAILED(d11->CreatePixelShader(g_karte_dxr_ps4nebel, sizeof(g_karte_dxr_ps4nebel), nullptr, &g.psNebel))) {
        diag::error("Karten-Editor: Raytracing-Schatten: die DXIL-Shader lassen sich nicht anlegen");
        return Grund::Fehler;
    }
    return Grund::Keiner;
}

// Platz fuer eine untere Struktur; false: kein Grafikspeicher
bool platzNehmen(UINT64 bytes, int& seite, D3D12_GPU_VIRTUAL_ADDRESS& adresse) {
    bytes = (bytes + D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1U) & ~static_cast<UINT64>(D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BYTE_ALIGNMENT - 1U);
    if (g.offeneSeite < 0 || g.seiten[static_cast<std::size_t>(g.offeneSeite)].belegt + bytes > g.seiten[static_cast<std::size_t>(g.offeneSeite)].groesse) {
        // die bisher offene Seite ist voll: lebt in ihr nichts mehr, geht sie zurueck
        if (g.offeneSeite >= 0) {
            Seite& alt = g.seiten[static_cast<std::size_t>(g.offeneSeite)];
            if (alt.lebend == 0) {
                g.stand.strukturBytes -= static_cast<std::size_t>(alt.groesse);
                spaeterLos(alt.res);
                alt = Seite{};
            }
        }
        Seite neu;
        neu.groesse = std::max(kSeitenBytes, bytes);
        neu.res = pufferAnlegen(neu.groesse, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
        if (neu.res == nullptr) { return false; }
        g.stand.strukturBytes += static_cast<std::size_t>(neu.groesse);
        int frei = -1;
        for (std::size_t i = 0; i < g.seiten.size() && frei < 0; ++i) {
            if (g.seiten[i].res == nullptr) { frei = static_cast<int>(i); }
        }
        if (frei < 0) {
            frei = static_cast<int>(g.seiten.size());
            g.seiten.emplace_back();
        }
        g.seiten[static_cast<std::size_t>(frei)] = neu;
        g.offeneSeite = frei;
    }
    Seite& s = g.seiten[static_cast<std::size_t>(g.offeneSeite)];
    seite = g.offeneSeite;
    adresse = s.res->GetGPUVirtualAddress() + s.belegt;
    s.belegt += bytes;
    ++s.lebend;
    return true;
}

// Den Platz einer unteren Struktur zurueckgeben. Vergeben wird nur am Ende der offenen Seite, nie in Luecken: eine Seite geht
// als Ganzes zurueck, wenn nichts mehr in ihr lebt (ueber rahmen::spaeterFrei - Listen in Arbeit koennen sie noch lesen).
void platzGeben(Blas& b) {
    if (b.seite < 0) { return; }
    Seite& s = g.seiten[static_cast<std::size_t>(b.seite)];
    if (s.lebend > 0) { --s.lebend; }
    if (s.lebend == 0 && b.seite != g.offeneSeite) {
        g.stand.strukturBytes -= static_cast<std::size_t>(s.groesse);
        spaeterLos(s.res);
        s = Seite{};
    }
    b.seite = -1;
    b.adresse = 0;
    b.bytes = 0;
}

bool kratzSichern(UINT64 bytes) {
    if (bytes <= g.kratzGroesse && g.kratz != nullptr) { return true; }
    spaeterLos(g.kratz);
    g.kratzGroesse = std::max<UINT64>(bytes + bytes / 2U, 256U * 1024U);
    // COMMON: Puffer entstehen immer so (die Debugschicht warnte bei UNORDERED_ACCESS: "Ignoring InitialState"); der Bau
    // hebt ihn implizit in den UAV-Zustand
    g.kratz = pufferAnlegen(g.kratzGroesse, D3D12_RESOURCE_STATE_COMMON);
    if (g.kratz == nullptr) {
        g.kratzGroesse = 0;
        return false;
    }
    return true;
}

// Die Baue teilen sich den Arbeitspuffer: der naechste darf erst beginnen, wenn dieser fertig ist
void kratzSperre(ID3D12GraphicsCommandList4* l) {
    D3D12_RESOURCE_BARRIER b{};
    b.Type = D3D12_RESOURCE_BARRIER_TYPE_UAV;
    b.UAV.pResource = g.kratz;
    l->ResourceBarrier(1, &b);
}

// Eine untere Struktur aus orte (je Dreieck neun Werte, Wicklung schon ausgerichtet) bauen
bool blasBauen(ID3D12GraphicsCommandList4* l, Blas& b, const std::vector<float>& orte, bool schnell) {
    const UINT64 bytes = orte.size() * sizeof(float);
    ID3D12Resource* hoch = nullptr;
    std::uint64_t versatz = 0;
    unsigned char* zeiger = nullptr;
    if (bytes == 0 || !r12::rahmen::hochladePlatz(bytes, false, hoch, versatz, zeiger) || hoch == nullptr || zeiger == nullptr) { return false; }
    std::memcpy(zeiger, orte.data(), static_cast<std::size_t>(bytes));
    D3D12_RAYTRACING_GEOMETRY_DESC geo{};
    geo.Type = D3D12_RAYTRACING_GEOMETRY_TYPE_TRIANGLES;
    // Filter-Teile melden jedes Dreieck dem Shader (dxrFilter), genau einmal; alle anderen verdecken ohne Rueckfrage
    geo.Flags = b.art == 3 ? D3D12_RAYTRACING_GEOMETRY_FLAG_NO_DUPLICATE_ANYHIT_INVOCATION : D3D12_RAYTRACING_GEOMETRY_FLAG_OPAQUE;
    geo.Triangles.VertexBuffer.StartAddress = hoch->GetGPUVirtualAddress() + versatz;
    geo.Triangles.VertexBuffer.StrideInBytes = 3U * sizeof(float);
    geo.Triangles.VertexFormat = DXGI_FORMAT_R32G32B32_FLOAT;
    geo.Triangles.VertexCount = static_cast<UINT>(orte.size() / 3U);
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bau{};
    bau.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_BOTTOM_LEVEL;
    bau.Inputs.Flags = schnell ? D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_BUILD
                               : D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    bau.Inputs.NumDescs = 1;
    bau.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    bau.Inputs.pGeometryDescs = &geo;
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
    g.geraet5->GetRaytracingAccelerationStructurePrebuildInfo(&bau.Inputs, &info);
    if (info.ResultDataMaxSizeInBytes == 0 || !kratzSichern(info.ScratchDataSizeInBytes)) { return false; }
    platzGeben(b);
    if (!platzNehmen(info.ResultDataMaxSizeInBytes, b.seite, b.adresse)) { return false; }
    b.bytes = info.ResultDataMaxSizeInBytes;
    bau.DestAccelerationStructureData = b.adresse;
    bau.ScratchAccelerationStructureData = g.kratz->GetGPUVirtualAddress();
    l->BuildRaytracingAccelerationStructure(&bau, 0, nullptr);
    kratzSperre(l);
    b.dreiecke = orte.size() / 9U;
    b.gebaut = Uhr::now();
    if (schnell && !b.schnell) {
        ++g.schnelle;
    } else if (!schnell && b.schnell && g.schnelle > 0) {
        --g.schnelle;
    }
    b.schnell = schnell;
    ++g.stand.gebaut;
    if (schnell) { ++g.stand.schnellGebaut; }
    return true;
}

// Die Ecken eines Teils fuer den Bau: nur die Orte, die Wicklung nach der Normale der Ecken ausgerichtet - vorn (der Strahl
// trifft im Uhrzeigersinn, wie Direct3D es fuer RAY_FLAG_CULL_BACK_FACING_TRIANGLES rechnet) ist, wohin die Normale zeigt.
// Die Schattengeometrie kommt aus Pinseln, Patches und Modellen mit je eigener Wicklung; die Normale ist die verlaessliche
// Angabe (psHimmel der Schattenkarten fragt ebenso sie). st: dieselbe Reihenfolge.
void eckenRichten(const Punkt3* p, std::size_t dreiecke, std::vector<float>& orte, std::vector<float>* st) {
    orte.resize(dreiecke * 9U);
    if (st != nullptr) { st->resize(dreiecke * 6U); }
    for (std::size_t i = 0; i < dreiecke; ++i) {
        const Punkt3& a = p[i * 3U];
        const Punkt3& b = p[i * 3U + 1U];
        const Punkt3& c = p[i * 3U + 2U];
        const float u[3] = {b.pos[0] - a.pos[0], b.pos[1] - a.pos[1], b.pos[2] - a.pos[2]};
        const float v[3] = {c.pos[0] - a.pos[0], c.pos[1] - a.pos[1], c.pos[2] - a.pos[2]};
        const float n[3] = {u[1] * v[2] - u[2] * v[1], u[2] * v[0] - u[0] * v[2], u[0] * v[1] - u[1] * v[0]};
        const bool tauschen = n[0] * a.normale[0] + n[1] * a.normale[1] + n[2] * a.normale[2] < 0.0F;
        const Punkt3* reihe[3] = {&a, tauschen ? &c : &b, tauschen ? &b : &c};
        for (std::size_t k = 0; k < 3U; ++k) {
            std::memcpy(&orte[i * 9U + k * 3U], reihe[k]->pos, 3U * sizeof(float));
            if (st != nullptr) { std::memcpy(&(*st)[i * 6U + k * 2U], reihe[k]->st, 2U * sizeof(float)); }
        }
    }
}

// Das Bild eines Filter-Teils lesen (einmal je Bild; nearest wie TraceTriangle braucht die Bildpunkte selbst)
FilterBild& filterBild(void* bild) {
    const auto it = g.filterBilder.find(bild);
    if (it != g.filterBilder.end()) {
        it->second.lauf = g.lauf;
        return it->second;
    }
    FilterBild f;
    int stufen = 0;
    f.lesbar = texturLesen(bild, 0, f.rgba, f.breite, f.hoehe, stufen) && f.breite > 0 && f.hoehe > 0;
    if (!f.lesbar) { f.rgba.clear(); }
    f.lauf = g.lauf;
    g.atlasAlt = true;
    return g.filterBilder.emplace(bild, std::move(f)).first->second;
}

// Die Filterbilder in einen Atlas (Seiten gleicher Groesse, Zeilen von oben nach der Hoehe gefuellt)
bool atlasBauen() {
    los(g.bilderSrv);
    los(g.bilder);
    g.stand.bytes -= g.bilderBytes;
    g.bilderBytes = 0;
    std::vector<std::pair<void*, FilterBild*>> reihe;
    int seite = 1;   // ohne Filterbild ein einzelner Bildpunkt (der Platz t12 braucht eine Textur)
    for (auto& [bild, f] : g.filterBilder) {
        if (!f.lesbar) { continue; }
        reihe.emplace_back(bild, &f);
        seite = std::max(seite, std::max(kAtlasSeite, std::max(f.breite, f.hoehe)));
    }
    // die hohen zuerst; bei gleicher Hoehe nach der Adresse, damit die Lage nicht von der Reihenfolge der Tabelle abhaengt
    std::sort(reihe.begin(), reihe.end(), [](const auto& a, const auto& b) {
        return a.second->hoehe != b.second->hoehe ? a.second->hoehe > b.second->hoehe : a.first < b.first;
    });
    int seiten = 1;
    int x = 0;
    int y = 0;
    int zeile = 0;
    for (auto& [bild, f] : reihe) {
        (void)bild;
        if (x + f->breite > seite) {
            x = 0;
            y += zeile;
            zeile = 0;
        }
        if (y + f->hoehe > seite) {
            ++seiten;
            x = 0;
            y = 0;
            zeile = 0;
        }
        f->seite = seiten - 1;
        f->x = x;
        f->y = y;
        x += f->breite;
        zeile = std::max(zeile, f->hoehe);
    }
    const std::size_t seitenBytes = static_cast<std::size_t>(seite) * static_cast<std::size_t>(seite) * 4U;
    std::vector<unsigned char> daten(seitenBytes * static_cast<std::size_t>(seiten), 0);
    for (auto& [bild, f] : reihe) {
        (void)bild;
        for (int zy = 0; zy < f->hoehe; ++zy) {
            std::memcpy(&daten[static_cast<std::size_t>(f->seite) * seitenBytes +
                               (static_cast<std::size_t>(f->y + zy) * static_cast<std::size_t>(seite) + static_cast<std::size_t>(f->x)) * 4U],
                        &f->rgba[static_cast<std::size_t>(zy) * static_cast<std::size_t>(f->breite) * 4U], static_cast<std::size_t>(f->breite) * 4U);
        }
    }
    auto* d = static_cast<ID3D11Device*>(render::d3dDevice());
    D3D11_TEXTURE2D_DESC td{};
    td.Width = static_cast<UINT>(seite);
    td.Height = static_cast<UINT>(seite);
    td.MipLevels = 1;
    td.ArraySize = static_cast<UINT>(seiten);
    td.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    td.SampleDesc.Count = 1;
    td.Usage = D3D11_USAGE_DEFAULT;
    td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    std::vector<D3D11_SUBRESOURCE_DATA> teile(static_cast<std::size_t>(seiten));
    for (int i = 0; i < seiten; ++i) {
        teile[static_cast<std::size_t>(i)].pSysMem = &daten[static_cast<std::size_t>(i) * seitenBytes];
        teile[static_cast<std::size_t>(i)].SysMemPitch = static_cast<UINT>(seite) * 4U;
    }
    D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format = td.Format;
    sv.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;   // auch mit einer Seite ein Feld (der Shader liest Texture2DArray)
    sv.Texture2DArray.MipLevels = 1;
    sv.Texture2DArray.ArraySize = td.ArraySize;
    if (d == nullptr || FAILED(d->CreateTexture2D(&td, teile.data(), &g.bilder)) || g.bilder == nullptr ||
        FAILED(::bhed::render::sichtAnlegen(d, g.bilder, &sv, &g.bilderSrv)) || g.bilderSrv == nullptr) {
        return false;
    }
    g.bilderBytes = daten.size();
    g.stand.bytes += g.bilderBytes;
    g.atlasAlt = false;
    return true;
}

// Die Filterdaten (karte_dxr.hlsl, dxrDaten): Kopf, je Filter-Teil zwei Werte, je Dreieck zwei Werte. Vergibt die Nummern.
bool datenBauen() {
    std::vector<std::pair<std::uint64_t, Blas*>> reihe;
    for (auto& [kennung, b] : g.blas) {
        b.filterNummer = -1;
        if (b.art == 3 && b.adresse != 0U) { reihe.emplace_back(kennung, &b); }
    }
    std::sort(reihe.begin(), reihe.end(), [](const auto& a, const auto& b) { return a.first < b.first; });
    std::vector<float> w((1U + reihe.size() * 2U) * 4U, 0.0F);
    w[0] = static_cast<float>(reihe.size());
    for (std::size_t i = 0; i < reihe.size(); ++i) {
        Blas& b = *reihe[i].second;
        b.filterNummer = static_cast<int>(i);
        const FilterBild& f = g.filterBilder[b.bild];
        float* kopf = &w[(1U + i * 2U) * 4U];   // (die Tabelle waechst unten weiter: erst schreiben, dann anhaengen)
        kopf[0] = static_cast<float>(f.seite);
        kopf[1] = static_cast<float>(f.x);
        kopf[2] = static_cast<float>(f.y);
        kopf[3] = static_cast<float>(b.filter);
        kopf[4] = static_cast<float>(f.breite);
        kopf[5] = static_cast<float>(f.hoehe);
        kopf[6] = static_cast<float>(w.size() / 4U);
        for (std::size_t t = 0; t + 5U < b.st.size(); t += 6U) {
            const float zeilen[8] = {b.st[t], b.st[t + 1U], b.st[t + 2U], b.st[t + 3U], b.st[t + 4U], b.st[t + 5U], 0.0F, 0.0F};
            w.insert(w.end(), zeilen, zeilen + 8);
        }
    }
    g.stand.filterTeile = reihe.size();
    los(g.datenSrv);
    los(g.daten);
    g.stand.bytes -= g.datenBytes;
    g.datenBytes = 0;
    auto* d = static_cast<ID3D11Device*>(render::d3dDevice());
    D3D11_BUFFER_DESC bd{};
    bd.ByteWidth = static_cast<UINT>(w.size() * sizeof(float));
    bd.Usage = D3D11_USAGE_DEFAULT;
    bd.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    D3D11_SUBRESOURCE_DATA anfang{};
    anfang.pSysMem = w.data();
    D3D11_SHADER_RESOURCE_VIEW_DESC sv{};
    sv.Format = DXGI_FORMAT_R32G32B32A32_FLOAT;
    sv.ViewDimension = D3D11_SRV_DIMENSION_BUFFER;
    sv.Buffer.FirstElement = 0;
    sv.Buffer.NumElements = static_cast<UINT>(w.size() / 4U);
    if (d == nullptr || FAILED(d->CreateBuffer(&bd, &anfang, &g.daten)) || g.daten == nullptr ||
        FAILED(::bhed::render::sichtAnlegen(d, g.daten, &sv, &g.datenSrv)) || g.datenSrv == nullptr) {
        return false;
    }
    g.datenBytes = bd.ByteWidth;
    g.stand.bytes += g.datenBytes;
    return true;
}

// Die obere Struktur: eine Instanz je Eimer
bool szeneBauen(ID3D12GraphicsCommandList4* l) {
    std::vector<D3D12_RAYTRACING_INSTANCE_DESC> inst;
    inst.reserve(g.blas.size());
    for (const auto& [kennung, b] : g.blas) {
        (void)kennung;
        if (b.adresse == 0U || b.art < 0 || b.art > 4) { continue; }
        D3D12_RAYTRACING_INSTANCE_DESC i{};
        i.Transform[0][0] = i.Transform[1][1] = i.Transform[2][2] = 1.0F;
        // karte_dxr.hlsl: Filter-Teile tragen ihre Nummer, die anderen kDxrKennungArt + art; die Maske ist 1 << art
        i.InstanceID = b.art == 3 ? static_cast<UINT>(b.filterNummer) : 0xFFFF00U + static_cast<UINT>(b.art);
        i.InstanceMask = 1U << static_cast<UINT>(b.art);
        // nur Himmelsflaechen zaehlen einseitig (der Strahl zur Sonne schneidet Rueckseiten weg); alles andere beidseitig
        // wie TraceTriangle und wie die Schattenkarten (CullMode NONE)
        i.Flags = b.art == 2 ? D3D12_RAYTRACING_INSTANCE_FLAG_NONE : D3D12_RAYTRACING_INSTANCE_FLAG_TRIANGLE_CULL_DISABLE;
        i.AccelerationStructure = b.adresse;
        inst.push_back(i);
    }
    D3D12_BUILD_RAYTRACING_ACCELERATION_STRUCTURE_DESC bau{};
    bau.Inputs.Type = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_TYPE_TOP_LEVEL;
    bau.Inputs.Flags = D3D12_RAYTRACING_ACCELERATION_STRUCTURE_BUILD_FLAG_PREFER_FAST_TRACE;
    bau.Inputs.NumDescs = static_cast<UINT>(inst.size());
    bau.Inputs.DescsLayout = D3D12_ELEMENTS_LAYOUT_ARRAY;
    if (!inst.empty()) {
        const UINT64 bytes = inst.size() * sizeof(D3D12_RAYTRACING_INSTANCE_DESC);
        ID3D12Resource* hoch = nullptr;
        std::uint64_t versatz = 0;
        unsigned char* zeiger = nullptr;
        if (!r12::rahmen::hochladePlatz(bytes, false, hoch, versatz, zeiger) || hoch == nullptr || zeiger == nullptr) { return false; }
        std::memcpy(zeiger, inst.data(), static_cast<std::size_t>(bytes));
        bau.Inputs.InstanceDescs = hoch->GetGPUVirtualAddress() + versatz;
    }
    D3D12_RAYTRACING_ACCELERATION_STRUCTURE_PREBUILD_INFO info{};
    g.geraet5->GetRaytracingAccelerationStructurePrebuildInfo(&bau.Inputs, &info);
    if (info.ResultDataMaxSizeInBytes == 0 || !kratzSichern(info.ScratchDataSizeInBytes)) { return false; }
    if (g.tlas == nullptr || info.ResultDataMaxSizeInBytes > g.tlasGroesse) {
        // groesser anlegen; die alte Struktur und ihre Sicht leben, bis die Listen in Arbeit durch sind
        g.stand.strukturBytes -= static_cast<std::size_t>(g.tlasGroesse);
        spaeterLos(g.tlas);
        los(g.tlasSicht);
        g.tlasGroesse = info.ResultDataMaxSizeInBytes + info.ResultDataMaxSizeInBytes / 2U;
        g.tlas = pufferAnlegen(g.tlasGroesse, D3D12_RESOURCE_STATE_RAYTRACING_ACCELERATION_STRUCTURE);
        if (g.tlas == nullptr) {
            g.tlasGroesse = 0;
            return false;
        }
        g.stand.strukturBytes += static_cast<std::size_t>(g.tlasGroesse);
        g.tlasSicht = r12::abgeber::strukturSicht(g.tlas->GetGPUVirtualAddress());
        if (g.tlasSicht == nullptr) { return false; }
    }
    bau.DestAccelerationStructureData = g.tlas->GetGPUVirtualAddress();
    bau.ScratchAccelerationStructureData = g.kratz->GetGPUVirtualAddress();
    l->BuildRaytracingAccelerationStructure(&bau, 0, nullptr);
    kratzSperre(l);
    ++g.stand.szenen;
    return true;
}

ID3D12GraphicsCommandList4* listeHolen() {
    ID3D12GraphicsCommandList* l = r12::rahmen::liste();
    ID3D12GraphicsCommandList4* l4 = nullptr;
    if (l == nullptr || FAILED(l->QueryInterface(IID_PPV_ARGS(&l4)))) { return nullptr; }
    return l4;
}

void zaehlen() {
    g.stand.teile = 0;
    g.stand.dreiecke = 0;
    for (const auto& [kennung, b] : g.blas) {
        (void)kennung;
        if (b.adresse == 0U) { continue; }
        ++g.stand.teile;
        g.stand.dreiecke += b.dreiecke;
    }
}

}  // namespace

Stand stand() {
    Stand s = g.stand;
    s.bytes += s.strukturBytes + static_cast<std::size_t>(g.kratzGroesse);
    return s;
}

namespace {

// Alles freigeben und den Zustand leeren; die Zaehler seit dem Start bleiben. lebt: die Objekte gehoeren dem laufenden
// Grafikgeraet (sonst nur vergessen - nach einem Wechsel des Geraets gehoeren die Zeiger dem alten, wie bei Puffer3).
void allesFrei(bool lebt) {
    if (lebt) {
        // die Plaetze des Kontexts leeren, ehe die Sichten gehen (gebundene Objekte leben sonst weiter)
        if (auto* c = static_cast<ID3D11DeviceContext*>(render::d3dContext()); c != nullptr && g.tlasSicht != nullptr) {
            ID3D11ShaderResourceView* leer[3] = {nullptr, nullptr, nullptr};
            c->PSSetShaderResources(11, 3, leer);
        }
        los(g.tlasSicht);
        los(g.datenSrv);
        los(g.daten);
        los(g.bilderSrv);
        los(g.bilder);
        los(g.vs);
        los(g.ps);
        los(g.psNebel);
        for (Seite& s : g.seiten) { spaeterLos(s.res); }
        spaeterLos(g.kratz);
        spaeterLos(g.tlas);
        los(g.geraet5);
    }
    const Stand zaehler = g.stand;
    g = Zustand{};
    g.stand.gebaut = zaehler.gebaut;
    g.stand.schnellGebaut = zaehler.schnellGebaut;
    g.stand.nachgezogen = zaehler.nachgezogen;
    g.stand.szenen = zaehler.szenen;
    g.stand.gebunden = zaehler.gebunden;
}

// Es geht auf diesem Geraet nicht: alles zurueckgeben und den Grund behalten (kein neuer Versuch je Bild)
bool scheitern(const char* was) {
    diag::error(std::string("Karten-Editor: Raytracing-Schatten: ") + was + " - weiter mit Schattenkarten");
    allesFrei(true);
    g.geprueft = true;
    g.geraet = render::texturStand();
    g.grund = Grund::Fehler;
    return false;
}

}  // namespace

Grund grund() {
    if (!g.geprueft || g.geraet != render::texturStand()) {
        if (g.geprueft) { loesen(); }
        g.geprueft = true;
        g.geraet = render::texturStand();
        g.grund = pruefen();
        if (g.grund != Grund::Keiner) {
            los(g.vs);
            los(g.ps);
            los(g.psNebel);
            los(g.geraet5);
        }
    }
    return g.grund;
}

bool geoSetzen(const std::vector<Punkt3>& ecken, const std::vector<Teil>& teile) {
    if (!moeglich()) { return false; }
    if (gespielt("bau")) { return scheitern("der Bau der Beschleunigungsstruktur wurde abgelehnt (gespielt)"); }
    ++g.lauf;
    // 1. Die Bilder der Filter-Teile lesen - VOR dem Griff zur Befehlsliste: das Lesen schickt die offene Liste ab
    for (const Teil& t : teile) {
        if (t.art == 3 && t.bild != nullptr && (t.filter & 3) != 0) { (void)filterBild(t.bild); }
    }
    // 2. Geaenderte Eimer neu bauen
    ID3D12GraphicsCommandList4* l = listeHolen();
    if (l == nullptr) { return scheitern("die Befehlsliste kann keine Beschleunigungsstrukturen bauen"); }
    const Uhr::time_point jetzt = Uhr::now();
    bool geaendert = false;
    bool filterGeaendert = g.daten == nullptr;
    bool gut = true;
    std::vector<float> orte;
    for (const Teil& t : teile) {
        if (t.anzahl < 3U || t.erstes + t.anzahl > ecken.size()) { continue; }
        Blas& b = g.blas[t.kennung];
        const bool neu = b.adresse == 0U;
        b.lauf = g.lauf;
        // ein Bild, das sich nicht lesen laesst, filtert nicht: der Teil verdeckt dann ganz (wie ohne Bild)
        int art = t.art;
        if (art == 3) {
            const auto f = (t.bild != nullptr && (t.filter & 3) != 0) ? g.filterBilder.find(t.bild) : g.filterBilder.end();
            if (f == g.filterBilder.end() || !f->second.lesbar) { art = 0; }
        }
        if (!neu && b.summe == t.summe && b.art == art && b.bild == (art == 3 ? t.bild : nullptr) && b.filter == (art == 3 ? (t.filter & 3) : 0)) { continue; }
        filterGeaendert = filterGeaendert || art == 3 || b.art == 3;
        // bewegt: der Eimer wurde eben erst gebaut und aendert sich schon wieder
        const bool schnell = !neu && std::chrono::duration<double>(jetzt - b.gebaut).count() < kBewegtSekunden;
        b.summe = t.summe;
        b.art = art;
        b.bild = art == 3 ? t.bild : nullptr;
        b.filter = art == 3 ? (t.filter & 3) : 0;
        b.st.clear();
        eckenRichten(ecken.data() + t.erstes, t.anzahl / 3U, orte, art == 3 ? &b.st : nullptr);
        if (!blasBauen(l, b, orte, schnell)) {
            gut = false;
            break;
        }
        if (schnell) {
            b.orte = orte;
        } else {
            b.orte.clear();
            b.orte.shrink_to_fit();
        }
        geaendert = true;
    }
    for (auto it = g.blas.begin(); gut && it != g.blas.end();) {   // Eimer, die es nicht mehr gibt
        if (it->second.lauf != g.lauf) {
            filterGeaendert = filterGeaendert || it->second.art == 3;
            if (it->second.schnell && g.schnelle > 0) { --g.schnelle; }
            platzGeben(it->second);
            it = g.blas.erase(it);
            geaendert = true;
        } else {
            ++it;
        }
    }
    l->Release();
    l = nullptr;
    // 3. Filterbilder und -daten (Objekte des Abgebers - nicht, waehrend die Befehlsliste gehalten wird)
    if (gut && filterGeaendert) {
        for (auto it = g.filterBilder.begin(); it != g.filterBilder.end();) {   // Bilder, die kein Teil mehr nennt
            if (it->second.lauf != g.lauf) {
                g.atlasAlt = g.atlasAlt || it->second.lesbar;
                it = g.filterBilder.erase(it);
            } else {
                ++it;
            }
        }
        gut = (!g.atlasAlt && g.bilder != nullptr) || atlasBauen();
        gut = gut && datenBauen();
        geaendert = true;   // die Nummern der Filter-Teile stehen in den Instanzen
    }
    // 4. Die Szene
    if (gut && (geaendert || g.tlas == nullptr)) {
        l = listeHolen();
        gut = l != nullptr && szeneBauen(l);
        if (l != nullptr) { l->Release(); }
    }
    r12::abgeber::fremdGezeichnet();
    zaehlen();
    if (!gut) { return scheitern("kein Grafikspeicher fuer die Beschleunigungsstruktur"); }
    g.szeneSteht = true;
    if (g.stand.teile != g.gemeldet) {   // ins Protokoll, wenn sich die Zahl der Strukturen aendert (nicht je gezogenem Bild)
        g.gemeldet = g.stand.teile;
        const Stand s = stand();
        diag::detail("Karten-Editor: Raytracing-Schatten: " + std::to_string(s.teile) + " Strukturen, " + std::to_string(s.dreiecke) + " Dreiecke, " +
                     std::to_string(s.filterTeile) + " mit Filter, Grafikspeicher " + std::to_string(s.bytes / 1024U) + " KB (Strukturen " +
                     std::to_string(s.strukturBytes / 1024U) + " KB)");
    }
    return true;
}

bool szeneSteht() { return g.szeneSteht && g.geprueft && g.geraet == render::texturStand() && g.grund == Grund::Keiner; }

void bildSetzen(bool an) {
    g.an = an && g.szeneSteht && g.grund == Grund::Keiner && g.geprueft && g.geraet == render::texturStand() && g.tlasSicht != nullptr;
    if (!g.an || g.schnelle == 0) { return; }
    // Nachziehen: Eimer mit schnellem Bau, die seit der Ruhezeit unveraendert sind, einmal auf schnelle Strahlen
    const Uhr::time_point jetzt = Uhr::now();
    ID3D12GraphicsCommandList4* l = nullptr;
    bool gut = true;
    for (auto& [kennung, b] : g.blas) {
        (void)kennung;
        if (!b.schnell || std::chrono::duration<double>(jetzt - b.gebaut).count() < kBewegtSekunden) { continue; }
        if (l == nullptr) { l = listeHolen(); }
        if (l == nullptr) { return; }
        const std::vector<float> orte = std::move(b.orte);
        b.orte.clear();
        if (!blasBauen(l, b, orte, false)) {
            gut = false;
            break;
        }
        ++g.stand.nachgezogen;
    }
    if (l == nullptr) { return; }
    gut = gut && szeneBauen(l);
    l->Release();
    r12::abgeber::fremdGezeichnet();
    if (!gut) { (void)scheitern("kein Grafikspeicher fuer die Beschleunigungsstruktur"); }
}

bool aktiv() { return g.an; }

bool nachziehenFaellig() {
    if (!g.an || g.schnelle == 0) { return false; }
    const Uhr::time_point jetzt = Uhr::now();
    for (const auto& [kennung, b] : g.blas) {
        (void)kennung;
        if (b.schnell && std::chrono::duration<double>(jetzt - b.gebaut).count() >= kBewegtSekunden) { return true; }
    }
    return false;
}

void lichtBinden(void* kontext, bool nebelNur) {
    if (!g.an || kontext == nullptr) { return; }
    auto* c = static_cast<ID3D11DeviceContext*>(kontext);
    c->VSSetShader(g.vs, nullptr, 0);
    c->PSSetShader(nebelNur ? g.psNebel : g.ps, nullptr, 0);
    ID3D11ShaderResourceView* srv[3] = {g.tlasSicht, g.bilderSrv, g.datenSrv};
    c->PSSetShaderResources(11, 3, srv);
    ++g.stand.gebunden;
}

void loesen() { allesFrei(g.geprueft && g.geraet == render::texturStand()); }

}  // namespace bhed::gui::kartegpu::dxr

#else   // ohne Direct3D oder ohne dxc gebaut: die Schattenart gibt es nicht

namespace bhed::gui::kartegpu::dxr {

Grund grund() {
#if defined(BHED_WITH_D3D11)
    return Grund::KeinShader;
#else
    return Grund::NichtD3d12;
#endif
}
bool geoSetzen(const std::vector<Punkt3>&, const std::vector<Teil>&) { return false; }
bool szeneSteht() { return false; }
void bildSetzen(bool) {}
bool aktiv() { return false; }
bool nachziehenFaellig() { return false; }
Stand stand() { return Stand{}; }
void lichtBinden(void*, bool) {}
void loesen() {}

}  // namespace bhed::gui::kartegpu::dxr

#endif
