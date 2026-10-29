// roq.h - Menue-Editor M2 Punkt 2: RoQ-Videos (cinematic) abspielen wie die
// Engine und eigene Videos ins RoQ-Format schreiben.
//
// Der Spieler ist aus OpenJK portiert (code/client/cl_cin.cpp: RoQ_init,
// RoQInterrupt, readQuadInfo, setupQuad, recurseQuad, RoQPrepMcomp,
// decodeCodeBook, blitVQQuad32fs, ROQ_GenYUVTables, yuv_to_rgb24,
// CIN_RunCinematic, CIN_PlayCinematic mit CIN_loop | CIN_silent wie
// UI_PlayCinematic):
//
//   Copyright (C) 1999 - 2005, Id Software, Inc.
//   Copyright (C) 2000 - 2013, Raven Software, Inc.
//   Copyright (C) 2001 - 2013, Activision, Inc.
//   Copyright (C) 2013 - 2015, OpenJK contributors
//
//   OpenJK is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License version 2 as published by the
//   Free Software Foundation. This program is distributed in the hope that it
//   will be useful, but WITHOUT ANY WARRANTY; without even the implied
//   warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//   GNU General Public License for more details (LICENSE).
//
// Der Kodierer (RoqKodierer) ist eigener Code: er schreibt genau die Stuecke,
// die der Spieler oben liest (ROQ_QUAD_INFO, ROQ_CODEBOOK, ROQ_QUAD_VQ), mit
// Codebuechern aus k-Means ueber 2x2- und 4x4-Bloecke im YUV-Raum der Engine
// und einer Entscheidung je Block (ueberspringen, Bewegung, 8x8-Code, vier
// 4x4-Teile aus 4x4-Codes oder 2x2-Codes) nach dem kleinsten Fehler fuer die
// Bits - die Guete steuert den Tausch Groesse gegen Fehler.
#ifndef BHED_MENUE_ROQ_H
#define BHED_MENUE_ROQ_H

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace bhed::menue {

// cl_cin.cpp: die Stueckkennungen
inline constexpr unsigned kRoqKennung = 0x1084;      // RoQ_ID (Dateikopf)
inline constexpr unsigned kRoqQuadInfo = 0x1001;     // ROQ_QUAD_INFO
inline constexpr unsigned kRoqCodebook = 0x1002;     // ROQ_CODEBOOK
inline constexpr unsigned kRoqQuadVq = 0x1011;       // ROQ_QUAD_VQ
inline constexpr unsigned kRoqQuadJpeg = 0x1012;     // ROQ_QUAD_JPEG
inline constexpr unsigned kRoqQuadHang = 0x1013;     // ROQ_QUAD_HANG
inline constexpr unsigned kRoqPacket = 0x1030;       // ROQ_PACKET
inline constexpr unsigned kRoqTonMono = 0x1020;      // ZA_SOUND_MONO
inline constexpr unsigned kRoqTonStereo = 0x1021;    // ZA_SOUND_STEREO

class RoqSpieler {
public:
    enum class Status { Idle, Play, Eof, Looped };   // e_status FMV_*

    // CIN_PlayCinematic(..., CIN_loop | CIN_silent): false, wenn die Datei kein
    // RoQ ist (Kopf != 0x1084). Die Zeit beginnt bei startMs.
    bool start(std::string datei, int startMs = 0, bool schleife = true);
    // CIN_RunCinematic mit Sys_Milliseconds() = jetztMs (com_timescale 1).
    Status lauf(int jetztMs);
    // cinTable.buf: das zuletzt fertige Bild (RGBA, breite x hoehe), sonst nullptr.
    [[nodiscard]] const std::uint8_t* bild() const;
    [[nodiscard]] int breite() const noexcept { return static_cast<int>(xsize_); }
    [[nodiscard]] int hoehe() const noexcept { return static_cast<int>(ysize_); }
    [[nodiscard]] int fps() const noexcept { return static_cast<int>(roqFPS_); }
    [[nodiscard]] long bilder() const noexcept { return numQuads_; }
    [[nodiscard]] Status status() const noexcept { return status_; }
    // dirty: seit dem letzten Abholen ein neues Bild
    bool neuesBild() {
        const bool d = dirty_;
        dirty_ = false;
        return d;
    }

private:
    void roqInit();
    void roqReset();
    void interrupt();
    void readQuadInfo(const std::uint8_t* q);
    void setupQuad(long xOff, long yOff);
    void recurseQuad(long sx, long sy, long size, long xOff, long yOff);
    void prepMcomp(long xoff, long yoff);
    void decodeCodeBook(const std::uint8_t* input, unsigned short flags);
    void blitVQQuad32fs(const std::vector<long>& status, const std::uint8_t* data);
    void dateiLesen(std::uint8_t* ziel, std::size_t n);

    std::string datei_;
    std::size_t lesePos_ = 0;
    std::vector<std::uint8_t> file_;      // cin.file (65536 + Kopf)
    std::vector<std::uint8_t> linbuf_;    // cin.linbuf (zwei Bilder)
    std::vector<long> qStatus_[2];        // cin.q_status (Versatz in linbuf, -1 = NULL)
    long mcomp_[256] = {};
    std::vector<std::uint32_t> vq2_, vq4_, vq8_;
    long oldXOff_ = -1, oldYOff_ = -1, oldYSize_ = 0, oldXSize_ = 0;
    long cinWidth_ = 512, cinHeight_ = 512;
    unsigned xsize_ = 0, ysize_ = 0, maxsize_ = 0, minsize_ = 0;
    long samplesPerLine_ = 0, screenDelta_ = 0, samplesPerPixel_ = 4;
    long onQuad_ = 0, numQuads_ = -1;
    long roqPlayed_ = 0, roqSize_ = 0;
    unsigned roqFrameSize_ = 0, roqId_ = 0;
    long roqFlags_ = 0, roqF0_ = 0, roqF1_ = 0, roqFPS_ = 30;
    long t_[2] = {0, 0};
    long normalBuffer0_ = 0;
    long inMemory_ = 0;
    long bufVersatz_ = -1;   // cinTable.buf (-1 = NULL)
    unsigned startTime_ = 0, lastTime_ = 0;
    long tfps_ = 0;
    bool looping_ = true, holdAtEnd_ = false, dirty_ = false;
    Status status_ = Status::Eof;
    int jetzt_ = 0;
};

// --- Kodierer -----------------------------------------------------------------

struct RoqOptionen {
    int fps = 30;          // RoQ-Kopf (roqFPS)
    int guete = 70;        // 1 (klein) .. 100 (beste); steuert Fehler gegen Bits
    int schluesselAbstand = 0;   // alle n Bilder ohne Bewegung/Ueberspringen (0 = nie erzwingen)
};

// Ein Bild als RGBA (breite x hoehe x 4).
struct RoqBild {
    int breite = 0, hoehe = 0;
    std::vector<std::uint8_t> rgba;
};

class RoqKodierer {
public:
    // Breite und Hoehe muessen Vielfache von 16 sein (die Engine zerlegt das
    // Bild in 16x16-Bloecke); bildAnpassen() bringt ein Bild dorthin.
    bool beginne(int breite, int hoehe, const RoqOptionen& o);
    void bild(const RoqBild& b);
    [[nodiscard]] std::string fertig();   // die .roq-Bytes
    [[nodiscard]] int bilderAnzahl() const noexcept { return bilder_; }

private:
    std::string aus_;
    int breite_ = 0, hoehe_ = 0, bilder_ = 0;
    RoqOptionen opt_;
    std::vector<std::uint8_t> vorher_;      // Puffer 1 des Spielers (RGBA)
    std::vector<std::uint8_t> vorvorher_;   // Puffer 0 des Spielers (RGBA)
};

// Eine Bildfolge mit Anzeigedauer je Bild (GIF, Video).
struct Bildfolge {
    std::vector<RoqBild> bilder;
    std::vector<int> dauerMs;
};
// Animiertes GIF lesen (stb_image); Durchsichtiges auf Schwarz.
[[nodiscard]] bool gifLesen(const std::string& daten, Bildfolge& out);
// Welches Bild zu jedem Takt bei fps gezeigt wird.
[[nodiscard]] std::vector<std::size_t> aufFps(const Bildfolge& f, int fps);
// Ganze Folge kodieren; fortschritt(fertig, gesamt) false = abbrechen ("" zurueck).
[[nodiscard]] std::string kodiereRoq(const Bildfolge& f, const RoqOptionen& o, int maxSeite,
                                     const std::function<bool(int, int)>& fortschritt = {});

// Ein Bild auf Vielfache von 16 bringen (hoechstens maxSeite, Seitenverhaeltnis
// bleibt; bilinear), z. B. fuer GIF/Video-Import.
[[nodiscard]] RoqBild bildAnpassen(const RoqBild& b, int maxSeite);

}  // namespace bhed::menue

#endif  // BHED_MENUE_ROQ_H
