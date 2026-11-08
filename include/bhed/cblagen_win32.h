// cblagen_win32.h - die Versaetze eines Konstantenpuffers aus dem uebersetzten Shader lesen (nur Windows/MSVC)
//
// Die Lagen der Puffer stehen auf der CPU als Konstanten (gpuconst.h fuer JeStapel, gpushader.h fuer JeStufe). Weicht der
// HLSL-Text davon ab (Packregel falsch erraten, Feld bei einem Merge verschoben), schreibt die CPU an die falsche Stelle,
// ohne dass etwas bricht. Hier fragt D3DReflect den Uebersetzer selbst. Genutzt vom Programm (pruefeStapelLagen,
// pruefeStufenLagen in gpumap_win32.cpp) und von den Proben (figurglanz) - eine Rechnung fuer beide.
#ifndef BHED_CBLAGEN_WIN32_H
#define BHED_CBLAGEN_WIN32_H

#include <windows.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>

#include <cstddef>
#include <string>

namespace bhed::gpu {

// `soll`: Feld mit .name und .at (float). `floats` > 0: der ganze Puffer muss so gross sein. `fehlendIstFehler`: ein Name, den
// der Shader nicht kennt, zaehlt als Abweichung (JeStufe deklariert immer alles; JeStapel je nach Merkmalen nicht).
// `bericht` bekommt "name@ist(soll x)" je Eintrag. Rueckgabe: alles passt.
template <class Lage, std::size_t N>
bool cbLagenPruefen(const void* code, std::size_t groesse, const char* puffer, const Lage (&soll)[N], std::size_t floats,
                    bool fehlendIstFehler, std::string& bericht) {
    ID3D11ShaderReflection* r = nullptr;
    if (FAILED(D3DReflect(code, groesse, __uuidof(ID3D11ShaderReflection), reinterpret_cast<void**>(&r))) || r == nullptr) {
        bericht += " D3DReflect gescheitert";
        return false;
    }
    bool gut = true;
    ID3D11ShaderReflectionConstantBuffer* cb = r->GetConstantBufferByName(puffer);
    D3D11_SHADER_BUFFER_DESC bd{};
    if (cb == nullptr || FAILED(cb->GetDesc(&bd))) {
        bericht += std::string(" kein Puffer ") + puffer;
        r->Release();
        return false;
    }
    if (floats > 0U && bd.Size != floats * 4U) {
        bericht += " Groesse " + std::to_string(bd.Size / 4U) + "(soll " + std::to_string(floats) + ")";
        gut = false;
    }
    for (const Lage& s : soll) {
        ID3D11ShaderReflectionVariable* v = cb->GetVariableByName(s.name);
        D3D11_SHADER_VARIABLE_DESC vd{};
        if (v == nullptr || FAILED(v->GetDesc(&vd))) {
            if (fehlendIstFehler) {
                bericht += " " + std::string(s.name) + "(fehlt)";
                gut = false;
            }
            continue;
        }
        const std::size_t ist = vd.StartOffset / 4U;
        bericht += " " + std::string(s.name) + "@" + std::to_string(ist);
        if (ist != s.at) {
            bericht += "(soll " + std::to_string(s.at) + ")";
            gut = false;
        }
    }
    r->Release();
    return gut;
}

}  // namespace bhed::gpu

#endif
