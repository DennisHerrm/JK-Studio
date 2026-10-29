// roq.cpp - Menue-Editor M2 Punkt 2: RoQ abspielen (portiert aus OpenJK
// Portiert aus OpenJK (code/client/cl_cin.cpp).
// code/client/cl_cin.cpp, Herkunft und Lizenz siehe include/bhed/menue/roq.h)
// und schreiben (eigener Code, roq_kodierer.cpp).
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
// Abweichungen nur gegen Abstuerze: die Zeigertabellen q_status sind Versaetze
// in linbuf (-1 = NULL), linbuf und q_status wachsen mit dem Bild statt fest
// 512x512 zu sein, und Kopien, die ausserhalb von linbuf laegen (kaputte
// Datei), entfallen. Der Ton (ZA_SOUND_*) wird beim Abspielen wie mit CIN_silent
// nicht dekodiert - die Menues spielen ihre Videos immer stumm (UI_PlayCinematic,
// videoMap); roqTonDekodieren() liest ihn fuer den Rundlauftest des Kodierers.
#include "bhed/menue/roq.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

namespace bhed::menue {

namespace {
constexpr long kMaxSize = 8;    // MAXSIZE
constexpr long kMinSize = 4;    // MINSIZE

long g_yy[256], g_ub[256], g_ug[256], g_vg[256], g_vr[256];
bool g_tabellen = false;

// ROQ_GenYUVTables
void genYuvTables() {
    if (g_tabellen) { return; }
    g_tabellen = true;
    constexpr float tUb = 1.77200F / 2.0F * static_cast<float>(1 << 6) + 0.5F;
    constexpr float tVr = 1.40200F / 2.0F * static_cast<float>(1 << 6) + 0.5F;
    constexpr float tUg = 0.34414F / 2.0F * static_cast<float>(1 << 6) + 0.5F;
    constexpr float tVg = 0.71414F / 2.0F * static_cast<float>(1 << 6) + 0.5F;
    for (long i = 0; i < 256; i++) {
        const float x = static_cast<float>(2 * i - 255);
        g_ub[i] = static_cast<long>(tUb * x + (1 << 5));
        g_vr[i] = static_cast<long>(tVr * x + (1 << 5));
        g_ug[i] = static_cast<long>((-tUg * x));
        g_vg[i] = static_cast<long>(-tVg * x + (1 << 5));
        g_yy[i] = i << 6 | i >> 2;
    }
}

// yuv_to_rgb24 (Bytefolge R G B A wie LittleLong auf x86)
std::uint32_t yuvToRgb24(long y, long u, long v) {
    const long yy = g_yy[y];
    long r = (yy + g_vr[v]) >> 6;
    long g = (yy + g_ug[u] + g_vg[v]) >> 6;
    long b = (yy + g_ub[u]) >> 6;
    r = std::clamp(r, 0L, 255L);
    g = std::clamp(g, 0L, 255L);
    b = std::clamp(b, 0L, 255L);
    return static_cast<std::uint32_t>(r) | static_cast<std::uint32_t>(g) << 8 | static_cast<std::uint32_t>(b) << 16 | 255U << 24;
}

// VQ2TO4 (Zeiger auf 32-Bit-Bildpunkte)
void vq2to4(const std::uint32_t*& a, const std::uint32_t*& b, std::uint32_t*& c, std::uint32_t*& d) {
    *c++ = a[0];
    *d++ = a[0];
    *d++ = a[0];
    *c++ = a[1];
    *d++ = a[1];
    *d++ = a[1];
    *c++ = b[0];
    *d++ = b[0];
    *d++ = b[0];
    *c++ = b[1];
    *d++ = b[1];
    *d++ = b[1];
    *d++ = a[0];
    *d++ = a[0];
    *d++ = a[1];
    *d++ = a[1];
    *d++ = b[0];
    *d++ = b[0];
    *d++ = b[1];
    *d++ = b[1];
    a += 2;
    b += 2;
}
}  // namespace

void RoqSpieler::dateiLesen(std::uint8_t* ziel, std::size_t n) {   // FS_Read
    const std::size_t da = lesePos_ < datei_.size() ? std::min(n, datei_.size() - lesePos_) : 0;
    if (da > 0) { std::memcpy(ziel, datei_.data() + lesePos_, da); }
    lesePos_ += da;
}

bool RoqSpieler::start(std::string datei, int startMs, bool schleife, bool shader) {
    datei_ = std::move(datei);
    lesePos_ = 0;
    jetzt_ = startMs;
    file_.assign(65536 + 16, 0);
    linbuf_.clear();
    qStatus_[0].clear();
    qStatus_[1].clear();
    std::memset(mcomp_, 0, sizeof(mcomp_));
    oldXOff_ = oldYOff_ = 0;   // memset(&cin, 0, ...)
    oldXSize_ = oldYSize_ = 0;
    roqSize_ = static_cast<long>(datei_.size());
    if (roqSize_ <= 0) { return false; }
    looping_ = schleife;
    shader_ = shader;
    holdAtEnd_ = false;
    cinHeight_ = 512;   // DEFAULT_CIN_HEIGHT
    cinWidth_ = 512;    // DEFAULT_CIN_WIDTH
    xsize_ = ysize_ = 0;
    bufVersatz_ = -1;
    dirty_ = false;
    // initRoQ
    samplesPerPixel_ = 4;
    genYuvTables();
    vq2_.assign(256 * 4, 0);
    vq4_.assign(256 * 16, 0);
    vq8_.assign(256 * 64, 0);
    dateiLesen(file_.data(), 16);
    const unsigned id = static_cast<unsigned>(file_[0]) + static_cast<unsigned>(file_[1]) * 256U;
    if (id != kRoqKennung) {
        status_ = Status::Eof;
        return false;
    }
    roqInit();
    status_ = Status::Play;
    return true;
}

void RoqSpieler::roqInit() {   // RoQ_init
    startTime_ = lastTime_ = static_cast<unsigned>(jetzt_);
    roqPlayed_ = 24;
    roqFPS_ = file_[6] + file_[7] * 256;
    if (roqFPS_ == 0) { roqFPS_ = 30; }
    numQuads_ = -1;
    roqId_ = file_[8] + file_[9] * 256U;
    roqFrameSize_ = file_[10] + file_[11] * 256U + file_[12] * 65536U;
    roqFlags_ = file_[14] + file_[15] * 256;
}

void RoqSpieler::roqReset() {   // RoQReset
    lesePos_ = 0;
    dateiLesen(file_.data(), 16);
    roqInit();
    status_ = Status::Looped;
}

void RoqSpieler::readQuadInfo(const std::uint8_t* q) {
    xsize_ = q[0] + q[1] * 256U;
    ysize_ = q[2] + q[3] * 256U;
    maxsize_ = q[4] + q[5] * 256U;
    minsize_ = q[6] + q[7] * 256U;
    cinHeight_ = static_cast<long>(ysize_);
    cinWidth_ = static_cast<long>(xsize_);
    samplesPerLine_ = cinWidth_ * samplesPerPixel_;
    screenDelta_ = cinHeight_ * samplesPerLine_;
    t_[0] = screenDelta_;
    t_[1] = -screenDelta_;
    // cin.linbuf: zwei Bilder (die Engine hat fest 512x512x4x2)
    linbuf_.assign(static_cast<std::size_t>(std::max(1L, screenDelta_ * 2)), 0);
}

void RoqSpieler::recurseQuad(long startX, long startY, long quadSize, long xOff, long yOff) {
    const long offset = screenDelta_;
    const long lowx = 0, lowy = 0;
    long bigx = static_cast<long>(xsize_);
    long bigy = static_cast<long>(ysize_);
    if (bigx > cinWidth_) { bigx = cinWidth_; }
    if (bigy > cinHeight_) { bigy = cinHeight_; }
    if (startX >= lowx && startX + quadSize <= bigx && startY + quadSize <= bigy && startY >= lowy && quadSize <= kMaxSize) {
        const long useY = startY;
        const long scroff = (useY + ((cinHeight_ - bigy) >> 1) + yOff) * samplesPerLine_ + ((startX + xOff) * samplesPerPixel_);
        const auto i = static_cast<std::size_t>(onQuad_++);
        if (i < qStatus_[0].size()) {
            qStatus_[0][i] = scroff;
            qStatus_[1][i] = scroff + offset;
        }
    }
    if (quadSize != kMinSize) {
        quadSize >>= 1;
        recurseQuad(startX, startY, quadSize, xOff, yOff);
        recurseQuad(startX + quadSize, startY, quadSize, xOff, yOff);
        recurseQuad(startX, startY + quadSize, quadSize, xOff, yOff);
        recurseQuad(startX + quadSize, startY + quadSize, quadSize, xOff, yOff);
    }
}

void RoqSpieler::setupQuad(long xOff, long yOff) {
    if (xOff == oldXOff_ && yOff == oldYOff_ && ysize_ == static_cast<unsigned>(oldYSize_) && xsize_ == static_cast<unsigned>(oldXSize_)) {
        return;
    }
    oldXOff_ = xOff;
    oldYOff_ = yOff;
    oldYSize_ = static_cast<long>(ysize_);
    oldXSize_ = static_cast<long>(xsize_);
    long numQuadCels = static_cast<long>(xsize_ * ysize_ / 16);
    numQuadCels += numQuadCels / 4;
    numQuadCels += 64;   // for overflow
    // cin.q_status: memset 0 beim Start = NULL
    qStatus_[0].assign(static_cast<std::size_t>(numQuadCels), -1);
    qStatus_[1].assign(static_cast<std::size_t>(numQuadCels), -1);
    onQuad_ = 0;
    for (long y = 0; y < static_cast<long>(ysize_); y += 16) {
        for (long x = 0; x < static_cast<long>(xsize_); x += 16) { recurseQuad(x, y, 16, xOff, yOff); }
    }
    for (long i = numQuadCels - 64; i < numQuadCels; i++) {
        qStatus_[0][static_cast<std::size_t>(i)] = -1;   // eoq
        qStatus_[1][static_cast<std::size_t>(i)] = -1;
    }
}

void RoqSpieler::prepMcomp(long xoff, long yoff) {   // RoQPrepMcomp
    long i = samplesPerLine_;
    long j = samplesPerPixel_;
    if (xsize_ == ysize_ * 4) {   // && !half
        j = j + j;
        i = i + i;
    }
    for (long y = 0; y < 16; y++) {
        const long temp2 = (y + yoff - 8) * i;
        for (long x = 0; x < 16; x++) {
            const long temp = (x + xoff - 8) * j;
            mcomp_[x * 16 + y] = normalBuffer0_ - (temp2 + temp);
        }
    }
}

void RoqSpieler::decodeCodeBook(const std::uint8_t* input, unsigned short flags) {
    long two = 0, four = 0;
    if (flags == 0) {
        two = four = 256;
    } else {
        two = flags >> 8;
        if (two == 0) { two = 256; }
        four = flags & 0xff;
    }
    four *= 2;
    // samplesPerPixel 4, normale Hoehe (half und smootheddouble sind nach
    // readQuadInfo immer qfalse)
    std::uint32_t* b = vq2_.data();
    for (long i = 0; i < two; i++) {
        const long y0 = *input++;
        const long y1 = *input++;
        const long y2 = *input++;
        const long y3 = *input++;
        const long cr = *input++;
        const long cb = *input++;
        *b++ = yuvToRgb24(y0, cr, cb);
        *b++ = yuvToRgb24(y1, cr, cb);
        *b++ = yuvToRgb24(y2, cr, cb);
        *b++ = yuvToRgb24(y3, cr, cb);
    }
    std::uint32_t* c = vq4_.data();
    std::uint32_t* d = vq8_.data();
    for (long i = 0; i < four && i < 512; i++) {
        const std::uint32_t* a = vq2_.data() + static_cast<std::ptrdiff_t>(*input++) * 4;
        const std::uint32_t* bb = vq2_.data() + static_cast<std::ptrdiff_t>(*input++) * 4;
        for (int j = 0; j < 2; j++) { vq2to4(a, bb, c, d); }
    }
}

void RoqSpieler::blitVQQuad32fs(const std::vector<long>& status, const std::uint8_t* data) {
    const std::uint8_t* const ende = file_.data() + file_.size();   // kaputte Datei: nicht ueber cin.file hinaus lesen
    const long spl = samplesPerLine_;
    const long n = static_cast<long>(linbuf_.size());
    std::uint8_t* lb = linbuf_.data();
    // blit8_32 / blit4_32 / blit2_32 / move8_32 / move4_32 mit Grenzpruefung
    auto blit = [&](const std::uint32_t* src, long dst, int seite) {
        const long zeile = seite * 4;
        if (dst < 0 || dst + (seite - 1) * spl + zeile > n) { return; }
        for (int i = 0; i < seite; ++i) {
            std::memcpy(lb + dst, src, static_cast<std::size_t>(zeile));
            src += seite;
            dst += spl;
        }
    };
    auto move = [&](long src, long dst, int seite) {
        const long zeile = seite * 4;
        const long ende = (seite - 1) * spl + zeile;
        if (dst < 0 || src < 0 || dst + ende > n || src + ende > n) { return; }
        for (int i = 0; i < seite; ++i) {
            std::memmove(lb + dst, lb + src, static_cast<std::size_t>(zeile));
            src += spl;
            dst += spl;
        }
    };
    auto blit2 = [&](const std::uint32_t* src, long dst) {
        if (dst < 0 || dst + spl + 8 > n) { return; }
        std::memcpy(lb + dst, src, 8);
        std::memcpy(lb + dst + spl, src + 2, 8);
    };
    std::size_t index = 0;
    unsigned short newd = 0;
    unsigned short celdata = 0;
    const std::size_t max = status.size();
    auto st = [&](std::size_t k) { return k < max ? status[k] : -1L; };
    do {
        if (data + 32 > ende) { return; }
        if (newd == 0) {
            newd = 7;
            celdata = static_cast<unsigned short>(data[0] + data[1] * 256);
            data += 2;
        } else {
            newd--;
        }
        auto code = static_cast<unsigned short>(celdata & 0xc000);
        celdata = static_cast<unsigned short>(celdata << 2);
        switch (code) {
            case 0x8000:   // vq code
                blit(&vq8_[static_cast<std::size_t>(*data) * 64], st(index), 8);
                data++;
                index += 5;
                break;
            case 0xc000:   // drop
                index++;   // skip 8x8
                for (int i = 0; i < 4; i++) {
                    if (newd == 0) {
                        newd = 7;
                        celdata = static_cast<unsigned short>(data[0] + data[1] * 256);
                        data += 2;
                    } else {
                        newd--;
                    }
                    code = static_cast<unsigned short>(celdata & 0xc000);
                    celdata = static_cast<unsigned short>(celdata << 2);
                    const long s = st(index);
                    switch (code) {
                        case 0x8000:   // 4x4 vq code
                            blit(&vq4_[static_cast<std::size_t>(*data) * 16], s, 4);
                            data++;
                            break;
                        case 0xc000:   // 2x2 vq code
                            blit2(&vq2_[static_cast<std::size_t>(*data) * 4], s);
                            data++;
                            blit2(&vq2_[static_cast<std::size_t>(*data) * 4], s + 8);
                            data++;
                            blit2(&vq2_[static_cast<std::size_t>(*data) * 4], s + spl * 2);
                            data++;
                            blit2(&vq2_[static_cast<std::size_t>(*data) * 4], s + spl * 2 + 8);
                            data++;
                            break;
                        case 0x4000:   // motion compensation
                            if (s >= 0) { move(s + mcomp_[*data], s, 4); }
                            data++;
                            break;
                        default:
                            break;
                    }
                    index++;
                }
                break;
            case 0x4000:   // motion compensation
                if (st(index) >= 0) { move(st(index) + mcomp_[*data], st(index), 8); }
                data++;
                index += 5;
                break;
            case 0x0000:
                index += 5;
                break;
            default:
                break;
        }
    } while (st(index) != -1);
}

void RoqSpieler::interrupt() {   // RoQInterrupt (stumm)
    dateiLesen(file_.data(), std::min<std::size_t>(roqFrameSize_ + 8, file_.size()));
    if (roqPlayed_ >= roqSize_) {
        if (!holdAtEnd_) {
            if (looping_) {
                roqReset();
            } else {
                status_ = Status::Eof;
            }
        } else {
            status_ = Status::Idle;
        }
        return;
    }
    std::size_t fd = 0;   // framedata = cin.file
    auto framedata = [&]() { return file_.data() + std::min(fd, file_.size() - 8); };
    while (true) {   // redump:
        switch (roqId_) {
            case kRoqQuadVq:
                if (xsize_ == 0 || qStatus_[0].empty()) { break; }
                if ((numQuads_ & 1) != 0) {
                    normalBuffer0_ = t_[1];
                    prepMcomp(roqF0_, roqF1_);
                    blitVQQuad32fs(qStatus_[1], framedata());
                    bufVersatz_ = screenDelta_;
                } else {
                    normalBuffer0_ = t_[0];
                    prepMcomp(roqF0_, roqF1_);
                    blitVQQuad32fs(qStatus_[0], framedata());
                    bufVersatz_ = 0;
                }
                if (numQuads_ == 0) {   // first frame
                    std::memcpy(linbuf_.data() + screenDelta_, linbuf_.data(), static_cast<std::size_t>(samplesPerLine_) * ysize_);
                }
                numQuads_++;
                dirty_ = true;
                break;
            case kRoqCodebook:
                decodeCodeBook(framedata(), static_cast<unsigned short>(roqFlags_));
                break;
            case kRoqTonMono:
            case kRoqTonStereo:
                break;   // CIN_silent
            case kRoqQuadInfo:
                if (numQuads_ == -1) {
                    readQuadInfo(framedata());
                    setupQuad(0, 0);
                    startTime_ = lastTime_ = static_cast<unsigned>(jetzt_);
                }
                if (numQuads_ != 1) { numQuads_ = 0; }
                break;
            case kRoqPacket:
                inMemory_ = roqFlags_;
                roqFrameSize_ = 0;
                break;
            case kRoqQuadHang:
                roqFrameSize_ = 0;
                break;
            case kRoqQuadJpeg:
                break;
            default:
                status_ = Status::Eof;
                break;
        }
        // naechsten Stueckkopf lesen
        if (roqPlayed_ >= roqSize_) {
            if (!holdAtEnd_) {
                if (looping_) {
                    roqReset();
                } else {
                    status_ = Status::Eof;
                }
            } else {
                status_ = Status::Idle;
            }
            return;
        }
        fd += roqFrameSize_;
        const std::uint8_t* f = framedata();
        roqId_ = f[0] + f[1] * 256U;
        roqFrameSize_ = f[2] + f[3] * 256U + f[4] * 65536U;
        roqFlags_ = f[6] + f[7] * 256;
        roqF0_ = static_cast<signed char>(f[7]);
        roqF1_ = static_cast<signed char>(f[6]);
        if (roqFrameSize_ > 65536 || roqId_ == 0x1084) {
            status_ = Status::Eof;
            if (looping_) { roqReset(); }
            return;
        }
        if (inMemory_ != 0 && status_ != Status::Eof) {
            inMemory_--;
            fd += 8;
            continue;   // goto redump
        }
        break;
    }
    roqPlayed_ += static_cast<long>(roqFrameSize_) + 8;
}

RoqSpieler::Status RoqSpieler::lauf(int jetztMs) {   // CIN_RunCinematic
    if (status_ == Status::Eof) { return Status::Eof; }
    if (status_ == Status::Idle) { return status_; }
    jetzt_ = jetztMs;
    const auto thisTime = static_cast<unsigned>(jetztMs);
    if (shader_ && std::abs(static_cast<long long>(thisTime) - static_cast<long long>(lastTime_)) > 100) {
        startTime_ += thisTime - lastTime_;   // videoMap: haelt an, solange die Stufe nicht gezeichnet wird
    }
    tfps_ = static_cast<long>((static_cast<long long>(thisTime) - static_cast<long long>(startTime_)) * roqFPS_ / 1000);
    unsigned start = startTime_;
    int schutz = 0;
    while (tfps_ != numQuads_ && status_ == Status::Play && schutz++ < 1000000) {
        interrupt();
        if (start != startTime_) {
            tfps_ = static_cast<long>((static_cast<long long>(thisTime) - static_cast<long long>(startTime_)) * roqFPS_ / 1000);
            start = startTime_;
        }
    }
    lastTime_ = thisTime;
    if (status_ == Status::Looped) { status_ = Status::Play; }
    if (status_ == Status::Eof) {
        if (looping_) {
            roqReset();
        } else {
            bufVersatz_ = -1;   // RoQShutdown
            return Status::Eof;
        }
    }
    return status_;
}

const std::uint8_t* RoqSpieler::bild() const {
    if (bufVersatz_ < 0 || linbuf_.empty() || xsize_ == 0) { return nullptr; }
    return linbuf_.data() + bufVersatz_;
}

// --- Ton (RllSetupTable, RllDecodeMonoToStereo, RllDecodeStereoToStereo) --------------
// RoQInterrupt gibt mono und stereo als Stereo an S_RawSamples (22050 Hz); die Zeit hier
// ist nur die Folge der Stuecke - abgespielt wird nichts (Menues: CIN_silent).
std::vector<std::int16_t> roqTonDekodieren(const std::string& roq, int* kanaele) {
    std::vector<std::int16_t> out;
    if (kanaele != nullptr) { *kanaele = 0; }
    short sqrTable[256];
    for (int z = 0; z < 128; ++z) {
        sqrTable[z] = static_cast<short>(z * z);
        sqrTable[z + 128] = static_cast<short>(-sqrTable[z]);
    }
    const auto u8 = [&](std::size_t i) { return static_cast<unsigned>(static_cast<unsigned char>(roq[i])); };
    std::size_t p = 8;
    while (p + 8 <= roq.size()) {
        const unsigned id = u8(p) | u8(p + 1) << 8;
        const std::size_t groesse = u8(p + 2) | u8(p + 3) << 8 | u8(p + 4) << 16;
        const unsigned flag = u8(p + 6) | u8(p + 7) << 8;
        const std::size_t daten = p + 8;
        if (daten + groesse >= roq.size()) { break; }   // das letzte Stueck liest die Engine nie
        const auto* from = reinterpret_cast<const unsigned char*>(roq.data() + daten);
        if (id == kRoqTonMono) {
            int prev = static_cast<int>(flag);   // signedOutput 0
            for (std::size_t z = 0; z < groesse; ++z) {
                prev = static_cast<short>(prev + sqrTable[from[z]]);
                out.push_back(static_cast<std::int16_t>(prev));
                out.push_back(static_cast<std::int16_t>(prev));
            }
            if (kanaele != nullptr && *kanaele == 0) { *kanaele = 1; }
        } else if (id == kRoqTonStereo) {
            int prevL = static_cast<int>(flag & 0xff00U);
            int prevR = static_cast<int>((flag & 0x00ffU) << 8);
            for (std::size_t z = 0; z + 1 < groesse; z += 2) {
                prevL = static_cast<short>(prevL + sqrTable[from[z]]);
                prevR = static_cast<short>(prevR + sqrTable[from[z + 1]]);
                out.push_back(static_cast<std::int16_t>(prevL));
                out.push_back(static_cast<std::int16_t>(prevR));
            }
            if (kanaele != nullptr && *kanaele == 0) { *kanaele = 2; }
        }
        p = daten + groesse;
    }
    return out;
}

}  // namespace bhed::menue
