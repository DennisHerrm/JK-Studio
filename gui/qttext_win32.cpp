// qttext_win32.cpp - Karten-Editor: Textbreiten so, wie Qt 5 sie bei NRC misst.
//
// NetRadiant-Custom 20260114 laeuft mit AA_EnableHighDpiScaling (main.cpp:410);
// bei 150 % rundet Qt auf den Massstab 2. Dann ist QHighDpiScaling aktiv, und
// Qt 5.15 nimmt unter Windows fuer jede Schrift den DirectWrite-Font-Engine
// (qwindowsfontdatabase.cpp useDirectWrite: "QHighDpiScaling::isActive() &&
// hintingPreference == PreferDefaultHinting"). Dessen Vorschuebe sind die
// ungehinteten Entwurfsmasse (qwindowsfontenginedirectwrite.cpp:474-500
// recalcAdvances: GetDesignGlyphMetrics, DESIGN_TO_LOGICAL = Entwurf *
// pixelSize / unitsPerEm als QFixed, also auf 1/64 gerundet). Gesetzt wird mit
// HarfBuzz samt Unterschneidung (GPOS 'kern'; die alte 'kern'-Tabelle nimmt
// HarfBuzz nur ohne GPOS - Tahoma hat "Tr" nur dort); QTextLine::naturalTextWidth
// zaehlt eine negative rechte Peilung des letzten Zeichens dazu
// (qtextlayout.cpp:2064-2068), und fm.boundingRect / fm.size runden das
// Rechteck nach aussen (QRectF::toAlignedRect, qpainter.cpp:5985), also auf.
//
// Die Schriftgroesse ist die logische: QFont-Punkte * 96 / 72 auf 1/100
// gerundet (qfontdatabase.cpp:2763). Die Dialogschrift kommt aus
// GetStockObject(DEFAULT_GUI_FONT) (qwindowsfontdatabase.cpp:2049, "MS Shell
// Dlg 2" = Tahoma, lfHeight -11 bei 96 dpi = 8,25 pt) und ist darum 11 Punkte
// gross, die Menueschrift (lfMenuFont, Segoe UI 9 pt) 12. Am Foto
// dlg_Misc_Find_brush_0.png: die Felder beginnen bei 86 = 11 + 69 + 6, und
// "Entity number" ist in Tahoma 11 nach dieser Rechnung 68,36 -> 69 (GDI mit
// gehinteten 11-Punkt-Vorschueben: 67).
//
// Gemessen wird mit IDWriteTextLayout in DWRITE_MEASURING_MODE_NATURAL (die
// Entwurfsmasse ohne Raster, mit der Unterschneidung der Schrift wie
// HarfBuzz) und GetDesignGlyphMetrics fuer die Peilung des letzten Zeichens.
#include "platform.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dwrite.h>

#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <tuple>
#include <vector>

namespace bhed::platform {
namespace {

std::wstring breit(const std::string& text) {
    if (text.empty()) { return {}; }
    const int n = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), nullptr, 0);
    std::wstring w(static_cast<std::size_t>(std::max(n, 0)), L'\0');
    if (n > 0) { MultiByteToWideChar(CP_UTF8, 0, text.c_str(), static_cast<int>(text.size()), w.data(), n); }
    return w;
}

// dwrite.dll wird erst zur Laufzeit geladen: die Importtabelle von behaved
// bleibt bei den DLLs, die jedes Windows hat (ctest exe_importe). Fehlt
// DirectWrite, liefern die Funktionen -1 und die Aufrufer messen selbst.
IDWriteFactory* fabrik() {
    static IDWriteFactory* f = [] {
        using Erzeuger = HRESULT(WINAPI*)(DWRITE_FACTORY_TYPE, REFIID, IUnknown**);
        HMODULE dll = LoadLibraryW(L"dwrite.dll");
        if (dll == nullptr) { return static_cast<IDWriteFactory*>(nullptr); }
        const auto erzeugen = reinterpret_cast<Erzeuger>(reinterpret_cast<void*>(GetProcAddress(dll, "DWriteCreateFactory")));
        IDWriteFactory* neu = nullptr;
        if (erzeugen == nullptr ||
            FAILED(erzeugen(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(&neu)))) {
            return static_cast<IDWriteFactory*>(nullptr);
        }
        return neu;
    }();
    return f;
}

// Die Schrift als IDWriteFontFace (fuer die Peilung des letzten Zeichens).
IDWriteFontFace* schriftFlaeche(const std::wstring& name, bool fett) {
    static std::map<std::pair<std::wstring, bool>, IDWriteFontFace*> gemerkt;
    const auto schluessel = std::make_pair(name, fett);
    const auto it = gemerkt.find(schluessel);
    if (it != gemerkt.end()) { return it->second; }
    IDWriteFontFace* flaeche = nullptr;
    IDWriteFontCollection* sammlung = nullptr;
    if (fabrik() != nullptr && SUCCEEDED(fabrik()->GetSystemFontCollection(&sammlung, FALSE))) {
        UINT32 index = 0;
        BOOL da = FALSE;
        IDWriteFontFamily* familie = nullptr;
        IDWriteFont* schrift = nullptr;
        if (SUCCEEDED(sammlung->FindFamilyName(name.c_str(), &index, &da)) && da != FALSE &&
            SUCCEEDED(sammlung->GetFontFamily(index, &familie)) &&
            SUCCEEDED(familie->GetFirstMatchingFont(fett ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STRETCH_NORMAL,
                                                    DWRITE_FONT_STYLE_NORMAL, &schrift))) {
            (void)schrift->CreateFontFace(&flaeche);
        }
        if (schrift != nullptr) { schrift->Release(); }
        if (familie != nullptr) { familie->Release(); }
        sammlung->Release();
    }
    gemerkt.emplace(schluessel, flaeche);
    return flaeche;
}

// Sammelt die Glyphen eines IDWriteTextLayout (DrawGlyphRun): Glyphe und
// Vorschub samt Unterschneidung. Mit der Schriftgroesse = unitsPerEm sind die
// Vorschuebe im Modus NATURAL genau die Entwurfseinheiten.
class GlyphenSammler final : public IDWriteTextRenderer {
public:
    struct Glyphe {
        UINT16 index;
        float vorschub;
        IDWriteFontFace* flaeche;
    };
    std::vector<Glyphe> glyphen;

    HRESULT STDMETHODCALLTYPE QueryInterface(REFIID riid, void** obj) override {
        if (riid == __uuidof(IUnknown) || riid == __uuidof(IDWritePixelSnapping) || riid == __uuidof(IDWriteTextRenderer)) {
            *obj = this;
            return S_OK;
        }
        *obj = nullptr;
        return E_NOINTERFACE;
    }
    ULONG STDMETHODCALLTYPE AddRef() override { return 1; }
    ULONG STDMETHODCALLTYPE Release() override { return 1; }
    HRESULT STDMETHODCALLTYPE IsPixelSnappingDisabled(void*, BOOL* aus) override {
        *aus = TRUE;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetCurrentTransform(void*, DWRITE_MATRIX* m) override {
        *m = DWRITE_MATRIX{1.0F, 0.0F, 0.0F, 1.0F, 0.0F, 0.0F};
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE GetPixelsPerDip(void*, FLOAT* p) override {
        *p = 1.0F;
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE DrawGlyphRun(void*, FLOAT, FLOAT, DWRITE_MEASURING_MODE, const DWRITE_GLYPH_RUN* lauf,
                                           const DWRITE_GLYPH_RUN_DESCRIPTION*, IUnknown*) override {
        for (UINT32 i = 0; i < lauf->glyphCount; ++i) { glyphen.push_back({lauf->glyphIndices[i], lauf->glyphAdvances[i], lauf->fontFace}); }
        return S_OK;
    }
    HRESULT STDMETHODCALLTYPE DrawUnderline(void*, FLOAT, FLOAT, const DWRITE_UNDERLINE*, IUnknown*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE DrawStrikethrough(void*, FLOAT, FLOAT, const DWRITE_STRIKETHROUGH*, IUnknown*) override { return S_OK; }
    HRESULT STDMETHODCALLTYPE DrawInlineObject(void*, FLOAT, FLOAT, IDWriteInlineObject*, BOOL, BOOL, IUnknown*) override { return S_OK; }
};

// QFixed::fromReal: auf 1/64 gerundet
double qfixed(double x) { return std::round(x * 64.0) / 64.0; }

}  // namespace

QtTextMass qtTextMass(const std::string& utf8, const wchar_t* schrift, float pixel, bool fett) {
    using Schluessel = std::tuple<std::string, std::wstring, float, bool>;
    static std::map<Schluessel, QtTextMass> gemerkt;
    Schluessel s{utf8, schrift, pixel, fett};
    const auto it = gemerkt.find(s);
    if (it != gemerkt.end()) { return it->second; }
    QtTextMass m;
    const std::wstring text = breit(utf8);
    IDWriteFactory* f = fabrik();
    IDWriteFontFace* flaeche = schriftFlaeche(schrift, fett);
    if (f == nullptr || flaeche == nullptr) { return m; }
    if (text.empty()) {
        m.gueltig = true;
        gemerkt.emplace(std::move(s), m);
        return m;
    }
    DWRITE_FONT_METRICS fm{};
    flaeche->GetMetrics(&fm);
    if (fm.designUnitsPerEm == 0) { return m; }
    const double em = fm.designUnitsPerEm;
    IDWriteTextFormat* format = nullptr;
    if (FAILED(f->CreateTextFormat(schrift, nullptr, fett ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL, DWRITE_FONT_STYLE_NORMAL,
                                   DWRITE_FONT_STRETCH_NORMAL, static_cast<FLOAT>(em), L"en-us", &format))) {
        return m;
    }
    (void)format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);
    IDWriteTextLayout* layout = nullptr;
    GlyphenSammler sammler;
    if (SUCCEEDED(f->CreateTextLayout(text.c_str(), static_cast<UINT32>(text.size()), format, 1.0e9F, 1.0e9F, &layout))) {
        if (SUCCEEDED(layout->Draw(nullptr, &sammler, 0.0F, 0.0F))) { m.gueltig = true; }
        layout->Release();
    }
    format->Release();
    if (!m.gueltig) { return m; }
    // Je Glyphe: Vorschub aus dem Font-Engine (DESIGN_TO_LOGICAL, QFixed) plus
    // die Unterschneidung, die HarfBuzz in 26.6 skaliert (hb_font_set_scale
    // = pixelSize * 64, qharfbuzzng.cpp) - beides auf 1/64 gerundet.
    double summe = 0.0;
    for (const GlyphenSammler::Glyphe& g : sammler.glyphen) {
        DWRITE_GLYPH_METRICS gm{};
        IDWriteFontFace* ff = g.flaeche != nullptr ? g.flaeche : flaeche;
        if (FAILED(ff->GetDesignGlyphMetrics(&g.index, 1, &gm, FALSE))) { continue; }
        const double entwurf = gm.advanceWidth;
        summe += qfixed(entwurf * pixel / em) + qfixed((static_cast<double>(g.vorschub) - entwurf) * pixel / em);
    }
    m.vorschub = summe;
    // Negative rechte Peilung des letzten Zeichens (LineBreakHelper::
    // calculateRightBearing: nur negative zaehlen; Leerzeichen am Ende nicht).
    if (!sammler.glyphen.empty() && text.back() != L' ') {
        const GlyphenSammler::Glyphe& g = sammler.glyphen.back();
        IDWriteFontFace* ff = g.flaeche != nullptr ? g.flaeche : flaeche;
        DWRITE_GLYPH_METRICS gm{};
        if (SUCCEEDED(ff->GetDesignGlyphMetrics(&g.index, 1, &gm, FALSE))) {
            const double peilung = qfixed(static_cast<double>(gm.rightSideBearing) * pixel / em);
            if (peilung < 0.0) { m.rechts = -peilung; }
        }
    }
    gemerkt.emplace(std::move(s), m);
    return m;
}

int qtTextBreite(const std::string& utf8, const wchar_t* schrift, float pixel, bool fett) {
    const QtTextMass m = qtTextMass(utf8, schrift, pixel, fett);
    if (!m.gueltig) { return -1; }
    // QRectF(0, y, naturalTextWidth, h).toAlignedRect(): rechts aufgerundet
    // (1/64 Spielraum fuer die Rundung der QFixed-Vorschuebe).
    return static_cast<int>(std::ceil(m.vorschub + m.rechts));
}

int qtTextVorschub(const std::string& utf8, const wchar_t* schrift, float pixel, bool fett) {
    const QtTextMass m = qtTextMass(utf8, schrift, pixel, fett);
    if (!m.gueltig) { return -1; }
    return static_cast<int>(std::lround(m.vorschub));   // QFontMetrics::horizontalAdvance: qRound
}

}  // namespace bhed::platform
