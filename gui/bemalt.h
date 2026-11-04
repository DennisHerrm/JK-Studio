// bemalt.h - rc609 T1: Leerlauf je Fenster, was in diesem Bild bemalt wurde
//
// Ein eigenes Fenster (Viewport) zeigt nur dann ein neues Bild, wenn sich
// seine Zeichenliste geaendert hat oder eine Textur darin in diesem Bild neu
// bemalt wurde (backend_win32.cpp, plattformFenster). Die 3D-Ansichten malen
// in Renderziele, die ImGui als Textur zeigt - die Zeichenliste bleibt dabei
// gleich. Deshalb geht jedes Bemalen ueber diese Haken: Renderziel setzen,
// kopieren, aufloesen, hochladen. Sie tun genau dasselbe wie der Aufruf am
// Kontext und merken sich das Ziel; zu viel gemerkt kostet nur ein Bild, zu
// wenig liesse ein Fenster ein altes Bild zeigen.
#ifndef BHED_GUI_BEMALT_H
#define BHED_GUI_BEMALT_H

#include <d3d11.h>

namespace bhed::render {

// Das Ziel wurde in diesem Bild bemalt (backend_win32.cpp).
void bemaltMerken(ID3D11Resource* ziel);

inline void bemaltSicht(ID3D11View* sicht) {
    if (sicht == nullptr) { return; }
    ID3D11Resource* r = nullptr;
    sicht->GetResource(&r);
    if (r != nullptr) {
        bemaltMerken(r);
        r->Release();
    }
}

inline void zielSetzen(ID3D11DeviceContext* c, UINT n, ID3D11RenderTargetView* const* ziele, ID3D11DepthStencilView* tiefe) {
    c->OMSetRenderTargets(n, ziele, tiefe);
    for (UINT i = 0; ziele != nullptr && i < n; ++i) { bemaltSicht(ziele[i]); }
}

inline void kopieren(ID3D11DeviceContext* c, ID3D11Resource* ziel, ID3D11Resource* quelle) {
    c->CopyResource(ziel, quelle);
    bemaltMerken(ziel);
}

inline void kopierenBereich(ID3D11DeviceContext* c, ID3D11Resource* ziel, UINT zielTeil, UINT x, UINT y, UINT z, ID3D11Resource* quelle,
                            UINT quellTeil, const D3D11_BOX* kasten) {
    c->CopySubresourceRegion(ziel, zielTeil, x, y, z, quelle, quellTeil, kasten);
    bemaltMerken(ziel);
}

inline void aufloesen(ID3D11DeviceContext* c, ID3D11Resource* ziel, UINT zielTeil, ID3D11Resource* quelle, UINT quellTeil, DXGI_FORMAT format) {
    c->ResolveSubresource(ziel, zielTeil, quelle, quellTeil, format);
    bemaltMerken(ziel);
}

inline void hochladen(ID3D11DeviceContext* c, ID3D11Resource* ziel, UINT teil, const D3D11_BOX* kasten, const void* daten, UINT zeile, UINT ebene) {
    c->UpdateSubresource(ziel, teil, kasten, daten, zeile, ebene);
    bemaltMerken(ziel);
}

inline void leeren(ID3D11DeviceContext* c, ID3D11RenderTargetView* ziel, const FLOAT farbe[4]) {
    c->ClearRenderTargetView(ziel, farbe);
    bemaltSicht(ziel);
}

// Eine neue Sicht kann die Adresse einer freigegebenen haben - ihr Inhalt ist neu.
inline HRESULT sichtAnlegen(ID3D11Device* d, ID3D11Resource* r, const D3D11_SHADER_RESOURCE_VIEW_DESC* art, ID3D11ShaderResourceView** sicht) {
    const HRESULT hr = d->CreateShaderResourceView(r, art, sicht);
    bemaltMerken(r);
    return hr;
}

}  // namespace bhed::render
#endif
