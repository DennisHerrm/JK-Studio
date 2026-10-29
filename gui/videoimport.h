// videoimport.h - Menue-Editor M2 Punkt 2 / M4 Punkt 5: Videos (mp4, webm, avi,
// wmv ...) ueber Windows Media Foundation lesen - Bilder im Fluss, Tonspur als PCM,
// und bei fehlendem Decoder den Codec des Videostroms fuer eine klare Meldung.
//
// Eigener Code.
#ifndef BHED_GUI_VIDEOIMPORT_H
#define BHED_GUI_VIDEOIMPORT_H

#include "bhed/menue/roq.h"

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bhed::gui {

enum class VideoFehler {
    Keiner,
    KeinMediaFoundation,   // N-Fassung von Windows ohne Media Feature Pack
    NichtLesbar,           // Datei/Behaelter nicht zu oeffnen (kaputt oder unbekannt)
    CodecFehlt,            // Videostrom da, aber kein Decoder (codec sagt welcher)
    KeinVideo,             // kein Videostrom
    KeineBilder,           // Decoder lieferte kein Bild
    Abbruch,
};

struct VideoInfo {
    int breite = 0, hoehe = 0;
    double fps = 0;
    long long dauerMs = 0;
    bool ton = false;
    int tonKanaele = 0, tonRate = 0;
    std::uint32_t codec = 0;   // FourCC des Videostroms (MF_MT_SUBTYPE.Data1)
    VideoFehler fehler = VideoFehler::Keiner;
};

// Kopfdaten ohne zu dekodieren; prueft, ob ein Decoder nach RGB32 da ist.
bool videoInfo(const std::string& pfad, VideoInfo& out);

// Bilder nacheinander (RGBA, Quellgroesse) mit Anzeigezeit in ms; proBild false bricht ab.
// dauerMs: Ende des letzten Bildes. Hoechstens maxMs.
bool leseVideoBilder(const std::string& pfad, const std::function<bool(const menue::RoqBild&, long long)>& proBild, long long maxMs,
                     long long& dauerMs, VideoInfo& info);

// Tonspur als 16-Bit-PCM (Quellrate und -kanaele, verschraenkt); false = keine Tonspur.
bool leseVideoTon(const std::string& pfad, std::vector<std::int16_t>& pcm, int& rate, int& kanaele, long long maxMs);

}  // namespace bhed::gui

#endif  // BHED_GUI_VIDEOIMPORT_H
