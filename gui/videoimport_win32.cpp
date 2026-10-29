// videoimport_win32.cpp - Menue-Editor M2 Punkt 2: Videodateien (mp4, webm,
// avi, wmv, mov ...) ueber Windows Media Foundation in Einzelbilder lesen,
// die der RoQ-Kodierer (src/menue/roq_kodierer.cpp) dann schreibt.
//
// Eigener Code. Der Quellleser dekodiert mit dem Videoprozessor nach RGB32;
// die Zeitstempel geben die Anzeigedauer je Bild.
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

bool leseVideoDatei(const std::string& pfad, menue::Bildfolge& out, int maxBilder, std::string* fehler) {
    out = menue::Bildfolge{};
#if defined(_WIN32)
    auto melde = [&](const char* t) {
        if (fehler != nullptr) { *fehler = t; }
        return false;
    };
    const HRESULT co = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_LITE))) {
        if (SUCCEEDED(co)) { CoUninitialize(); }
        return melde("Media Foundation nicht verfuegbar");
    }
    bool ok = false;
    IMFAttributes* attr = nullptr;
    IMFSourceReader* leser = nullptr;
    IMFMediaType* typ = nullptr;
    do {
        if (FAILED(MFCreateAttributes(&attr, 1))) { break; }
        attr->SetUINT32(MF_SOURCE_READER_ENABLE_VIDEO_PROCESSING, TRUE);
        const std::wstring w = utf8Pfad(pfad).wstring();
        if (FAILED(MFCreateSourceReaderFromURL(w.c_str(), attr, &leser))) {
            melde("Video nicht lesbar");
            break;
        }
        leser->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_ALL_STREAMS), FALSE);
        leser->SetStreamSelection(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), TRUE);
        if (FAILED(MFCreateMediaType(&typ))) { break; }
        typ->SetGUID(MF_MT_MAJOR_TYPE, MFMediaType_Video);
        typ->SetGUID(MF_MT_SUBTYPE, MFVideoFormat_RGB32);
        if (FAILED(leser->SetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), nullptr, typ))) {
            melde("kein Videostrom");
            break;
        }
        typ->Release();
        typ = nullptr;
        if (FAILED(leser->GetCurrentMediaType(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), &typ))) { break; }
        UINT32 bw = 0, bh = 0;
        MFGetAttributeSize(typ, MF_MT_FRAME_SIZE, &bw, &bh);
        UINT32 schritt = 0;
        if (FAILED(typ->GetUINT32(MF_MT_DEFAULT_STRIDE, &schritt))) { schritt = bw * 4; }
        if (bw == 0 || bh == 0) { break; }
        LONGLONG letzteZeit = -1;
        while (static_cast<int>(out.bilder.size()) < maxBilder) {
            DWORD flags = 0;
            LONGLONG zeit = 0;
            IMFSample* probe = nullptr;
            if (FAILED(leser->ReadSample(static_cast<DWORD>(MF_SOURCE_READER_FIRST_VIDEO_STREAM), 0, nullptr, &flags, &zeit, &probe))) { break; }
            if ((flags & MF_SOURCE_READERF_ENDOFSTREAM) != 0) {
                if (probe != nullptr) { probe->Release(); }
                break;
            }
            if (probe == nullptr) { continue; }
            IMFMediaBuffer* puffer = nullptr;
            if (SUCCEEDED(probe->ConvertToContiguousBuffer(&puffer))) {
                BYTE* p = nullptr;
                DWORD laenge = 0;
                if (SUCCEEDED(puffer->Lock(&p, nullptr, &laenge))) {
                    menue::RoqBild b;
                    b.breite = static_cast<int>(bw);
                    b.hoehe = static_cast<int>(bh);
                    b.rgba.resize(static_cast<std::size_t>(bw) * bh * 4);
                    const auto st = static_cast<int>(static_cast<std::int32_t>(schritt));
                    for (UINT32 y = 0; y < bh; ++y) {
                        // negativer Schritt = von unten nach oben
                        const BYTE* zeile = st >= 0 ? p + static_cast<std::size_t>(y) * static_cast<std::size_t>(st)
                                                    : p + static_cast<std::size_t>(bh - 1 - y) * static_cast<std::size_t>(-st);
                        if (zeile + bw * 4 > p + laenge) { break; }
                        for (UINT32 x = 0; x < bw; ++x) {   // BGRA -> RGBA
                            std::uint8_t* o = &b.rgba[(static_cast<std::size_t>(y) * bw + x) * 4];
                            o[0] = zeile[x * 4 + 2];
                            o[1] = zeile[x * 4 + 1];
                            o[2] = zeile[x * 4 + 0];
                            o[3] = 255;
                        }
                    }
                    puffer->Unlock();
                    if (letzteZeit >= 0 && !out.dauerMs.empty()) {
                        out.dauerMs.back() = std::max(1, static_cast<int>((zeit - letzteZeit) / 10000));
                    }
                    letzteZeit = zeit;
                    out.bilder.push_back(std::move(b));
                    out.dauerMs.push_back(33);
                }
                puffer->Release();
            }
            probe->Release();
        }
        ok = !out.bilder.empty();
        if (!ok) { melde("keine Bilder im Video"); }
    } while (false);
    if (typ != nullptr) { typ->Release(); }
    if (leser != nullptr) { leser->Release(); }
    if (attr != nullptr) { attr->Release(); }
    MFShutdown();
    if (SUCCEEDED(co)) { CoUninitialize(); }
    return ok;
#else
    (void)pfad;
    (void)maxBilder;
    if (fehler != nullptr) { *fehler = "nur unter Windows"; }
    return false;
#endif
}

}  // namespace bhed::gui
