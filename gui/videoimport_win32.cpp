// videoimport_win32.cpp - Menue-Editor M2 Punkt 2 / M4 Punkt 5: Videodateien (mp4,
// webm, avi, wmv, mov ...) ueber Windows Media Foundation lesen: Bilder im Fluss fuer
// den RoQ-Kodierer (src/menue/roq_import.cpp), die Tonspur als PCM und bei fehlendem
// Decoder den FourCC des Videostroms.
//
// Eigener Code. Der Quellleser dekodiert mit dem Videoprozessor nach RGB32 (nur die
// sichtbare Flaeche, MF_MT_MINIMUM_DISPLAY_APERTURE); die Zeitstempel geben die
// Anzeigezeit je Bild.
#include "videoimport.h"

#include "bhed/utf8pfad.h"

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mfapi.h>
#include <mfidl.h>
#include <mfreadwrite.h>
#include <mferror.h>
#pragma comment(lib, "mfplat.lib")
#pragma comment(lib, "mfreadwrite.lib")
#pragma comment(lib, "mfuuid.lib")
#endif

#include <algorithm>
#include <cstring>
#include <string>

namespace bhed::gui {

#if defined(_WIN32)
namespace {

// Ein COM-Zeiger, der sich selbst freigibt
template <class T>
struct Com {
    T* p = nullptr;
    Com() = default;
    Com(const Com&) = delete;
    Com& operator=(const Com&) = delete;
    ~Com() { reset(); }
    void reset() {
        if (p != nullptr) { p->Release(); }
        p = nullptr;
    }
    T** operator&() {
        reset();
        return &p;
    }
    T* operator->() const { return p; }
    explicit operator bool() const { return p != nullptr; }
};

// Media Foundation fehlt in den N-Fassungen von Windows: verzoegert gebunden (CMakeLists,
// /DELAYLOAD) und hier vorher geprueft - sonst liesse sich das Programm dort gar nicht starten.
bool mfVorhanden() {
    static const bool da = LoadLibraryExW(L"mfplat.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32) != nullptr &&
                           LoadLibraryExW(L"mfreadwrite.dll", nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32) != nullptr;
    return da;
}

struct MfSitzung {
    bool ok = false;
    HRESULT co = E_FAIL;
    MfSitzung() {
        if (!mfVorhanden()) { return; }
        co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
        ok = SUCCEEDED(MFStartup(MF_VERSION, MFSTARTUP_LITE));
    }
    MfSitzung(const MfSitzung&) = delete;
    MfSitzung& operator=(const MfSitzung&) = delete;
    ~MfSitzung() {
        if (ok) { MFShutdown(); }
        if (SUCCEEDED(co)) { CoUninitialize(); }
    }
};

constexpr DWORD kVideo = static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM);
constexpr DWORD kTon = static_cast<DWORD>(MF_SOURCE_READER_FIRST_AUDIO_STREAM);

bool oeffne(const std::string& pfad, bool videoVerarbeitung, Com<IMFSourceReader>& leser) {
    Com<IMFAttributes> attr;
    if (FAILED(MFCreateAttributes(&attr, 1))) { return false; }
    if (videoVerarbeitung) { attr->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE); }
    const std::wstring w = utf8Pfad(pfad).wstring();
    return SUCCEEDED(MFCreateSourceReaderFromURL(w.c_str(), attr.p, &leser));
}

// Videostrom auf RGB32 stellen; false = kein Decoder (codec = FourCC des Stroms)
bool videoNachRgb(IMFSourceReader* leser, VideoInfo& info) {
    Com<IMFMediaType> nativ;
    if (FAILED(leser->GetNativeMediaType(kVideo, 0, &nativ))) {
        info.fehler = VideoFehler::KeinVideo;
        return false;
    }
    GUID sub{};
    if (SUCCEEDED(nativ->GetGUID(MF_MT_SUBTYPE, &sub))) { info.codec = sub.Data1; }
    UINT32 zaehler = 0, nenner = 0;
    if (SUCCEEDED(MFGetAttributeRatio(nativ.p, MF_MT_FRAME_RATE, &zaehler, &nenner)) && nenner != 0) {
        info.fps = static_cast<double>(zaehler) / nenner;
    }
    leser->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    leser->SetStreamSelection(kVideo, TRUE);
    Com<IMFMediaType> typ;
    if (FAILED(MFCreateMediaType(&typ))) { return false; }
    typ->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
    typ->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
    if (FAILED(leser->SetCurrentMediaType(kVideo, nullptr, typ.p))) {
        info.fehler = VideoFehler::CodecFehlt;
        return false;
    }
    return true;
}

}  // namespace
#endif

bool videoInfo(const std::string& pfad, VideoInfo& out) {
    out = VideoInfo{};
#if defined(_WIN32)
    MfSitzung mf;
    if (!mf.ok) {
        out.fehler = VideoFehler::KeinMediaFoundation;
        return false;
    }
    Com<IMFSourceReader> leser;
    if (!oeffne(pfad, true, leser)) {
        out.fehler = VideoFehler::NichtLesbar;
        return false;
    }
    PROPVARIANT v;
    PropVariantInit(&v);
    if (SUCCEEDED(leser->GetPresentationAttribute(static_cast<DWORD>(MF_SOURCE_READER_MEDIASOURCE), MF_PD_DURATION, &v)) && v.vt == VT_UI8) {
        out.dauerMs = static_cast<long long>(v.uhVal.QuadPart / 10000);
    }
    PropVariantClear(&v);
    Com<IMFMediaType> ton;
    if (SUCCEEDED(leser->GetNativeMediaType(kTon, 0, &ton))) {
        out.ton = true;
        out.tonKanaele = static_cast<int>(MFGetAttributeUINT32(ton.p, MF_MT_AUDIO_NUM_CHANNELS, 0));
        out.tonRate = static_cast<int>(MFGetAttributeUINT32(ton.p, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0));
    }
    if (!videoNachRgb(leser.p, out)) { return false; }
    Com<IMFMediaType> typ;
    if (SUCCEEDED(leser->GetCurrentMediaType(kVideo, &typ))) {
        UINT32 bw = 0, bh = 0;
        MFGetAttributeSize(typ.p, MF_MT_FRAME_SIZE, &bw, &bh);
        MFVideoArea flaeche{};
        if (SUCCEEDED(typ->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, reinterpret_cast<UINT8*>(&flaeche), sizeof(flaeche), nullptr))) {
            bw = static_cast<UINT32>(flaeche.Area.cx);
            bh = static_cast<UINT32>(flaeche.Area.cy);
        }
        out.breite = static_cast<int>(bw);
        out.hoehe = static_cast<int>(bh);
    }
    return true;
#else
    (void)pfad;
    out.fehler = VideoFehler::KeinMediaFoundation;
    return false;
#endif
}

bool leseVideoBilder(const std::string& pfad, const std::function<bool(const menue::RoqBild&, long long)>& proBild, long long maxMs,
                     long long& dauerMs, VideoInfo& info) {
    dauerMs = 0;
    info = VideoInfo{};
#if defined(_WIN32)
    MfSitzung mf;
    if (!mf.ok) {
        info.fehler = VideoFehler::KeinMediaFoundation;
        return false;
    }
    Com<IMFSourceReader> leser;
    if (!oeffne(pfad, true, leser)) {
        info.fehler = VideoFehler::NichtLesbar;
        return false;
    }
    if (!videoNachRgb(leser.p, info)) { return false; }
    Com<IMFMediaType> typ;
    if (FAILED(leser->GetCurrentMediaType(kVideo, &typ))) {
        info.fehler = VideoFehler::CodecFehlt;
        return false;
    }
    UINT32 bw = 0, bh = 0;
    MFGetAttributeSize(typ.p, MF_MT_FRAME_SIZE, &bw, &bh);
    UINT32 schritt = 0;
    if (FAILED(typ->GetUINT32(MF_MT_DEFAULT_STRIDE, &schritt))) { schritt = bw * 4; }
    // sichtbare Flaeche (H.264 1080 Zeilen kommen als 1088 mit Blende)
    LONG ax = 0, ay = 0;
    UINT32 ab = bw, ah = bh;
    MFVideoArea flaeche{};
    if (SUCCEEDED(typ->GetBlob(MF_MT_MINIMUM_DISPLAY_APERTURE, reinterpret_cast<UINT8*>(&flaeche), sizeof(flaeche), nullptr))) {
        ax = std::clamp<LONG>(flaeche.OffsetX.value, 0, static_cast<LONG>(bw));
        ay = std::clamp<LONG>(flaeche.OffsetY.value, 0, static_cast<LONG>(bh));
        ab = std::min<UINT32>(static_cast<UINT32>(flaeche.Area.cx), bw - static_cast<UINT32>(ax));
        ah = std::min<UINT32>(static_cast<UINT32>(flaeche.Area.cy), bh - static_cast<UINT32>(ay));
    }
    if (ab == 0 || ah == 0) {
        info.fehler = VideoFehler::KeineBilder;
        return false;
    }
    info.breite = static_cast<int>(ab);
    info.hoehe = static_cast<int>(ah);
    const auto st = static_cast<int>(static_cast<std::int32_t>(schritt));
    menue::RoqBild b;
    b.breite = static_cast<int>(ab);
    b.hoehe = static_cast<int>(ah);
    b.rgba.resize(static_cast<std::size_t>(ab) * ah * 4);
    long long erste = -1;
    int bilder = 0;
    bool abbruch = false;
    for (;;) {
        DWORD flags = 0;
        LONGLONG zeit = 0;
        Com<IMFSample> probe;
        if (FAILED(leser->ReadSample(kVideo, 0, nullptr, &flags, &zeit, &probe))) { break; }
        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) { break; }
        if (!probe) { continue; }
        const long long ms = zeit / 10000;
        if (erste < 0) { erste = ms; }
        if (ms - erste > maxMs) { break; }
        LONGLONG dauer = 0;
        if (FAILED(probe->GetSampleDuration(&dauer))) { dauer = 0; }
        Com<IMFMediaBuffer> puffer;
        if (FAILED(probe->ConvertToContiguousBuffer(&puffer))) { continue; }
        BYTE* p = nullptr;
        DWORD laenge = 0;
        if (FAILED(puffer->Lock(&p, nullptr, &laenge))) { continue; }
        bool ganz = true;
        for (UINT32 y = 0; y < ah && ganz; ++y) {
            // negativer Schritt = von unten nach oben
            const UINT32 zy = static_cast<UINT32>(ay) + y;
            const BYTE* zeile = st >= 0 ? p + static_cast<std::size_t>(zy) * static_cast<std::size_t>(st)
                                        : p + static_cast<std::size_t>(bh - 1 - zy) * static_cast<std::size_t>(-st);
            zeile += static_cast<std::size_t>(ax) * 4;
            if (zeile + static_cast<std::size_t>(ab) * 4 > p + laenge) {
                ganz = false;
                break;
            }
            for (UINT32 x = 0; x < ab; ++x) {   // BGRA -> RGBA
                std::uint8_t* o = &b.rgba[(static_cast<std::size_t>(y) * ab + x) * 4];
                o[0] = zeile[x * 4 + 2];
                o[1] = zeile[x * 4 + 1];
                o[2] = zeile[x * 4 + 0];
                o[3] = 255;
            }
        }
        puffer->Unlock();
        if (!ganz) { continue; }
        ++bilder;
        dauerMs = std::max(dauerMs, ms - erste + std::max<long long>(1, dauer / 10000));
        if (!proBild(b, ms - erste)) {
            abbruch = true;
            break;
        }
    }
    if (abbruch) {
        info.fehler = VideoFehler::Abbruch;
        return false;
    }
    if (bilder == 0) {
        info.fehler = VideoFehler::KeineBilder;
        return false;
    }
    return true;
#else
    (void)pfad;
    (void)proBild;
    (void)maxMs;
    info.fehler = VideoFehler::KeinMediaFoundation;
    return false;
#endif
}

bool leseVideoTon(const std::string& pfad, std::vector<std::int16_t>& pcm, int& rate, int& kanaele, long long maxMs) {
    pcm.clear();
    rate = kanaele = 0;
#if defined(_WIN32)
    MfSitzung mf;
    if (!mf.ok) { return false; }
    Com<IMFSourceReader> leser;
    if (!oeffne(pfad, false, leser)) { return false; }
    Com<IMFMediaType> nativ;
    if (FAILED(leser->GetNativeMediaType(kTon, 0, &nativ))) { return false; }
    leser->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
    leser->SetStreamSelection(kTon, TRUE);
    // 16-Bit-PCM, sonst Gleitkomma (umgerechnet)
    bool gleit = false;
    {
        Com<IMFMediaType> typ;
        if (FAILED(MFCreateMediaType(&typ))) { return false; }
        typ->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Audio);
        typ->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_PCM);
        typ->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 16);
        if (FAILED(leser->SetCurrentMediaType(kTon, nullptr, typ.p))) {
            typ->SetGUID(MF_MT_SUBTYPE, MFAudioFormat_Float);
            typ->SetUINT32(MF_MT_AUDIO_BITS_PER_SAMPLE, 32);
            if (FAILED(leser->SetCurrentMediaType(kTon, nullptr, typ.p))) { return false; }
            gleit = true;
        }
    }
    Com<IMFMediaType> akt;
    if (FAILED(leser->GetCurrentMediaType(kTon, &akt))) { return false; }
    rate = static_cast<int>(MFGetAttributeUINT32(akt.p, MF_MT_AUDIO_SAMPLES_PER_SECOND, 0));
    kanaele = static_cast<int>(MFGetAttributeUINT32(akt.p, MF_MT_AUDIO_NUM_CHANNELS, 0));
    const UINT32 bits = MFGetAttributeUINT32(akt.p, MF_MT_AUDIO_BITS_PER_SAMPLE, gleit ? 32 : 16);
    if (rate <= 0 || kanaele <= 0 || (gleit ? bits != 32 : bits != 16)) { return false; }
    const std::size_t grenze = static_cast<std::size_t>(maxMs / 1000 + 1) * static_cast<std::size_t>(rate) * static_cast<std::size_t>(kanaele);
    for (;;) {
        DWORD flags = 0;
        LONGLONG zeit = 0;
        Com<IMFSample> probe;
        if (FAILED(leser->ReadSample(kTon, 0, nullptr, &flags, &zeit, &probe))) { break; }
        if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) { break; }
        if (!probe) { continue; }
        Com<IMFMediaBuffer> puffer;
        if (FAILED(probe->ConvertToContiguousBuffer(&puffer))) { continue; }
        BYTE* p = nullptr;
        DWORD laenge = 0;
        if (FAILED(puffer->Lock(&p, nullptr, &laenge))) { continue; }
        if (gleit) {
            const std::size_t n = laenge / sizeof(float);
            for (std::size_t i = 0; i < n; ++i) {
                float f = 0;
                std::memcpy(&f, p + i * sizeof(float), sizeof(float));
                pcm.push_back(static_cast<std::int16_t>(std::clamp(f * 32767.0F, -32768.0F, 32767.0F)));
            }
        } else {
            const std::size_t n = laenge / sizeof(std::int16_t);
            const std::size_t alt = pcm.size();
            pcm.resize(alt + n);
            std::memcpy(pcm.data() + alt, p, n * sizeof(std::int16_t));
        }
        puffer->Unlock();
        if (pcm.size() >= grenze) { break; }
    }
    if (pcm.size() > grenze) { pcm.resize(grenze); }
    return !pcm.empty();
#else
    (void)pfad;
    (void)maxMs;
    return false;
#endif
}

}  // namespace bhed::gui
