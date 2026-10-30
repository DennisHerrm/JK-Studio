// roq.h - Menue-Editor M2 Punkt 2: RoQ-Videos (cinematic) abspielen wie die
// Portiert aus OpenJK (code/client/cl_cin.cpp).
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
    // shader: CIN_shader (videoMap) - nach mehr als 100 ms ohne lauf haelt das Video an.
    bool start(std::string datei, int startMs = 0, bool schleife = true, bool shader = false);
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
    bool looping_ = true, holdAtEnd_ = false, dirty_ = false, shader_ = false;
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

// --- Ton (M4 Punkt 5b) ------------------------------------------------------------
// cl_cin.cpp spielt ZA_SOUND_MONO/STEREO mit S_RawSamples(..., 22050, ...): eine Probe
// je Byte und Kanal, DPCM ueber sqrTable (RllSetupTable: z*z, -z*z).
inline constexpr int kRoqTonRate = 22050;
struct RoqTon {
    std::vector<std::int16_t> pcm;   // 22050 Hz, Kanaele verschraenkt
    int kanaele = 0;                 // 1 (ZA_SOUND_MONO) oder 2 (ZA_SOUND_STEREO); 0 = kein Ton
    [[nodiscard]] std::size_t proben() const noexcept { return kanaele > 0 ? pcm.size() / static_cast<std::size_t>(kanaele) : 0; }
};
// PCM (rate, kanaeleEin verschraenkt) auf 22050 Hz und kanaeleAus (1/2) bringen:
// Kanaele mitteln bzw. verdoppeln, dann Fenster-Sinc (Hann, 16 Nullstellen, Grenze
// bei der kleineren Nyquist-Frequenz - kein Ueberlappen beim Verkleinern).
[[nodiscard]] RoqTon tonAuf22050(const std::vector<std::int16_t>& pcm, int rate, int kanaeleEin, int kanaeleAus);
// Alle Tonstuecke eines RoQ so dekodieren, wie RoQInterrupt sie an S_RawSamples gibt
// (RllDecodeMonoToStereo / RllDecodeStereoToStereo, das letzte Stueck der Datei liest
// die Engine nie): Stereo verschraenkt, 22050 Hz. kanaele: 1/2 der Datei, 0 = ohne Ton.
[[nodiscard]] std::vector<std::int16_t> roqTonDekodieren(const std::string& roq, int* kanaele = nullptr);

class RoqKodierer {
public:
    // Breite und Hoehe muessen Vielfache von 16 sein (die Engine zerlegt das
    // Bild in 16x16-Bloecke); bildSkalieren() bringt ein Bild dorthin.
    // ton (22050 Hz, schon auf die Laenge des Videos gekuerzt): wie die Videos von
    // JKA ein Vorlaufstueck fuer kTonVorlauf Bilder vor ROQ_QUAD_INFO, dann nach
    // jedem Bild das Stueck bis (Bild + 1 + kTonVorlauf) * 22050 / fps.
    bool beginne(int breite, int hoehe, const RoqOptionen& o, const RoqTon* ton = nullptr);
    void bild(const RoqBild& b);
    [[nodiscard]] std::string fertig();   // die .roq-Bytes
    [[nodiscard]] int bilderAnzahl() const noexcept { return bilder_; }
    [[nodiscard]] std::size_t groesse() const noexcept { return aus_.size(); }   // bisher geschriebene Bytes
    static constexpr int kTonVorlauf = 8;   // ja01.roq: 5888 Proben vorab (8 Bilder bei 30 fps)

private:
    void tonBis(std::size_t probe);
    std::string aus_;
    int breite_ = 0, hoehe_ = 0, bilder_ = 0;
    RoqOptionen opt_;
    RoqTon ton_;
    std::size_t tonGeschrieben_ = 0;
    std::vector<std::uint8_t> vorher_;      // Puffer 1 des Spielers (RGBA)
    std::vector<std::uint8_t> vorvorher_;   // Puffer 0 des Spielers (RGBA)
};

// --- Import (M2 Punkt 2, M4 Punkt 5a) ---------------------------------------------

// Eine Bildfolge mit Anzeigedauer je Bild (GIF).
struct Bildfolge {
    std::vector<RoqBild> bilder;
    std::vector<int> dauerMs;
};
// Animiertes GIF lesen (stb_image); Durchsichtiges auf Schwarz.
[[nodiscard]] bool gifLesen(const std::string& daten, Bildfolge& out);
// Welches Bild zu jedem Takt bei fps gezeigt wird.
[[nodiscard]] std::vector<std::size_t> aufFps(const Bildfolge& f, int fps);

// Was die Engine an Groessen abspielt (OpenJK cl_cin.cpp / tr_draw.cpp):
//   - RE_StretchRaw (Vollbild-Videos, cinematic-Items der MP-Menues) bricht bei Seiten,
//     die keine Zweierpotenz sind, mit ERR_DROP ab ("size not a power of 2");
//     RE_UploadCinematic (videoMap) prueft das nicht.
//   - cin.linbuf hat Platz fuer zwei Bilder zu 512x512 (DEFAULT_CIN_WIDTH/HEIGHT):
//     breite * hoehe > 512 * 512 schreibt ueber den Puffer hinaus.
enum class RoqGroesse { Ok, KeineZweierpotenz, ZuGross };
[[nodiscard]] RoqGroesse roqGroessePruefen(int breite, int hoehe);

enum class Einpassen {
    Strecken,      // ganzes Bild auf die Zielgroesse (das Spiel streckt beim Zeichnen ohnehin aufs Rechteck)
    Balken,        // Seitenverhaeltnis halten, schwarze Balken
    Zuschneiden,   // Seitenverhaeltnis halten, Ueberstand mittig abschneiden
};
struct VideoZiel {
    int breite = 512, hoehe = 512;   // wie alle Videos von JKA und Movie Duels
    Einpassen einpassen = Einpassen::Strecken;
};
// Ein Bild auf die Zielgroesse bringen: je Achse verkleinern mit Flaechenmittel
// (jeder Zielpunkt mittelt die ueberdeckten Quellpunkte anteilig), vergroessern bilinear.
[[nodiscard]] RoqBild bildSkalieren(const RoqBild& q, const VideoZiel& z);

// Kodieren im Fluss (Videos sind zu gross, um alle Bilder vorher zu halten):
// bild(quelle, zeitMs) mit der Anzeigezeit des Quellbildes, fertig(dauerMs) mit dem
// Ende des letzten Bildes. Takt k (Zeit k * 1000 / fps) zeigt das letzte Quellbild,
// dessen Zeit <= k * 1000 / fps ist (wie aufFps).
class RoqImport {
public:
    bool beginne(const VideoZiel& z, const RoqOptionen& o, const RoqTon* ton);
    void bild(const RoqBild& quelle, long long zeitMs);
    [[nodiscard]] std::string fertig(long long dauerMs);
    [[nodiscard]] int takte() const noexcept { return takt_; }
    [[nodiscard]] std::size_t bytes() const noexcept { return kod_.groesse(); }
    // Takte fuer eine Dauer (mindestens 1)
    [[nodiscard]] static long long takteFuer(long long dauerMs, int fps);

private:
    void bisZeit(long long zeitMs);
    void zeige();   // ein Takt mit dem aktuellen Quellbild
    RoqKodierer kod_;
    VideoZiel ziel_;
    int fps_ = 30;
    int takt_ = 0;
    long long erste_ = -1;
    RoqBild quelle_;            // erst skaliert, wenn ein Takt es zeigt
    RoqBild aktuell_;
    bool skaliert_ = false;
};

// Ganze GIF-Folge kodieren; fortschritt(fertig, gesamt) false = abbrechen ("" zurueck).
[[nodiscard]] std::string kodiereRoq(const Bildfolge& f, const RoqOptionen& o, const VideoZiel& z, const RoqTon* ton = nullptr,
                                     const std::function<bool(int, int)>& fortschritt = {});

// Fehlende Decoder (M4 Punkt 5c): welche Art von Codec hinter dem FourCC des
// Videostroms steckt (Media Foundation: MF_MT_SUBTYPE.Data1).
enum class CodecGruppe { Unbekannt, Hevc, Av1, Vp8, Vp9, DivxXvid };
[[nodiscard]] CodecGruppe codecGruppe(std::uint32_t fourcc);
[[nodiscard]] std::string fourccText(std::uint32_t fourcc);

}  // namespace bhed::menue

#endif  // BHED_MENUE_ROQ_H
