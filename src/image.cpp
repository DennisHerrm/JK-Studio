#include "bhed/image.h"

#include <algorithm>
#include <cstddef>
#include <cmath>
#include <cctype>
#include <cstring>

namespace bhed::image {

void shrinkTo(const Image& im, int maxSize, int& outW, int& outH,
              std::vector<unsigned char>& outRgba) {
    const int scale = std::max(1, std::max(im.width, im.height) / maxSize);
    outW = std::max(1, im.width / scale);
    outH = std::max(1, im.height / scale);
    outRgba.resize(static_cast<std::size_t>(outW) *
                   static_cast<std::size_t>(outH) * 4U);
    // --- Ueber den ganzen Block MITTELN, nicht einen Punkt herausgreifen --
    //
    // Gebeten: "mach Mipmaps und shrinkTo gleich auch richtig."
    //
    // Vorher wurde je Zielpunkt EIN Quellpunkt genommen: bei scale 2 also
    // einer von vieren, bei scale 4 einer von sechzehn. Der Rest fiel
    // ersatzlos weg. Eine 1024er Textur, die auf 512 gebracht wird,
    // verliert so drei Viertel ihrer Bildpunkte - und mit ihnen jedes
    // feine Muster, das dann als Flimmern zurueckkommt.
    //
    // Jetzt der Mittelwert ueber den ganzen Block. Das ist genau das, was
    // die Engine beim Bauen der Mipmaps tut (R_MipMap in tr_image.cpp:
    // je vier Nachbarn zu einem).
    //
    // Der Alphakanal wird MITgemittelt. Das ist bei alphagetesteten
    // Texturen (Gitter, Fluegel) wichtig: nimmt man dort einen einzelnen
    // Punkt, faellt je nach Zufall die halbe Struktur weg.
    for (int y = 0; y < outH; ++y) {
        for (int x = 0; x < outW; ++x) {
            unsigned sum[4] = {0, 0, 0, 0};
            unsigned n = 0;
            for (int sy = 0; sy < scale; ++sy) {
                const int py = y * scale + sy;
                if (py >= im.height) {
                    break;
                }
                for (int sx = 0; sx < scale; ++sx) {
                    const int px = x * scale + sx;
                    if (px >= im.width) {
                        break;
                    }
                    const std::size_t src =
                        (static_cast<std::size_t>(py) *
                             static_cast<std::size_t>(im.width) +
                         static_cast<std::size_t>(px)) * 4U;
                    for (int k = 0; k < 4; ++k) {
                        sum[k] += im.rgba[src + static_cast<std::size_t>(k)];
                    }
                    ++n;
                }
            }
            const std::size_t dst =
                (static_cast<std::size_t>(y) * static_cast<std::size_t>(outW) +
                 static_cast<std::size_t>(x)) * 4U;
            if (n == 0) {
                continue;   // kann nur am Rand bei entarteten Groessen sein
            }
            for (int k = 0; k < 4; ++k) {
                outRgba[dst + static_cast<std::size_t>(k)] =
                    static_cast<unsigned char>(sum[k] / n);
            }
        }
    }
}

std::string mappingName(const std::string& name) {
    std::string out;
    out.reserve(name.size());
    for (const char c : name) {
        if (c == '.') {
            break;
        }
        out.push_back((c == '\\')
                          ? '/'
                          : static_cast<char>(std::tolower(
                                static_cast<unsigned char>(c))));
    }
    return out;
}
namespace {

// Eine Obergrenze fuer die Bildgroesse. Eine Textur mit 30000x30000 waere
// 3.6 GB - kein JKA-Material sieht so aus, und ein Kopf, der es behauptet,
// ist entweder kaputt oder boesartig.
constexpr int kMaxDimension = 16384;

bool tooBig(int width, int height) {
    if (width <= 0 || height <= 0) { return true;
}
    if (width > kMaxDimension || height > kMaxDimension) { return true;
}
    // 4 Byte je Bildpunkt, mit Reserve gegen Ueberlauf gerechnet.
    return static_cast<int64_t>(width) * height > 64LL * 1024 * 1024;
}

}  // namespace

Format sniff(const unsigned char* data, size_t size) {
    if ((data == nullptr) || size < 4) { return Format::Unknown;
}

    // PNG: 89 50 4E 47 0D 0A 1A 0A
    if (size >= 8 && data[0] == 0x89 && data[1] == 'P' && data[2] == 'N' &&
        data[3] == 'G') {
        return Format::Png;
    }
    // JPEG: FF D8 FF
    if (data[0] == 0xFF && data[1] == 0xD8 && data[2] == 0xFF) { return Format::Jpeg;
}

    // Targa hat keine Kennung am Anfang - das ist die Schwaeche des Formats.
    // Neuere Dateien tragen eine Fusszeile mit "TRUEVISION-XFILE"; sonst
    // bleibt nur, den Kopf auf Plausibilitaet zu pruefen.
    if (size >= 26) {
        const char* footer = reinterpret_cast<const char*>(data + size - 18);
        if (std::memcmp(footer, "TRUEVISION-XFILE", 16) == 0) { return Format::Tga;
}
    }
    if (size >= 18) {
        const unsigned char imageType = data[2];
        const unsigned char depth = data[16];
        const bool knownType = imageType == 1 || imageType == 2 || imageType == 3 ||
                               imageType == 9 || imageType == 10 || imageType == 11;
        const bool knownDepth = depth == 8 || depth == 15 || depth == 16 ||
                                depth == 24 || depth == 32;
        if (knownType && knownDepth) { return Format::Tga;
}
    }
    return Format::Unknown;
}

const char* formatName(Format format) {
    switch (format) {
        case Format::Tga: return "TGA";
        case Format::Png: return "PNG";
        case Format::Jpeg: return "JPEG";
        default: return "unknown";
    }
}

// rc609 F14: 1:1 wie die Engine - OpenJK und Movie Duels 9 lesen mit demselben
// LoadTGA (code/rd-common/tr_image_tga.cpp). Was das Spiel abweist, weist auch die
// Vorschau ab; sonst zeigt sie eine Textur, die im Spiel fehlt (und umgekehrt).
//   - keine Farbtabelle (byColourmapType 0, w1stColourMapEntry 0, Laenge 0 oder 256,
//     Eintragsgroesse 0 oder 24 - auch wenn keine Tabelle folgt, wird sie nicht uebersprungen)
//   - nur Typ 2 (RGB), 3 (Grau) und 10 (RLE-RGB); 8, 24 oder 32 Bit je Bildpunkt
//   - Typ 10 nur von unten nach oben und nur mit 24/32 Bit
//   - Bit 0x10 (rechts nach links) waehlt dort zwar iXStart/iXStep, geschrieben wird
//     aber mit *pOut++ - die Zeile bleibt also ungespiegelt, wie hier
//   - ein RLE-Paket ueber das letzte Bild hinaus wird abgeschnitten (goto breakOut)
// Abweichend nur, wo die Engine ueber das Dateiende hinaus liest: hier Fehler.
Image decodeTga(const unsigned char* data, size_t size) {
    Image out;
    auto fail = [&](const char* why) {
        out.error = why;
        return out;
    };
    if ((data == nullptr) || size < 18) { return fail("too small for a TGA header");
}

    const unsigned idLength = data[0];
    const unsigned colourMapType = data[1];
    const unsigned imageType = data[2];
    const unsigned firstColourMapEntry =
        static_cast<unsigned>(data[3]) | (static_cast<unsigned>(data[4]) << 8);
    const unsigned colourMapLength =
        static_cast<unsigned>(data[5]) | (static_cast<unsigned>(data[6]) << 8);
    const unsigned colourMapDepth = data[7];
    const int width = static_cast<int>(data[12]) | (static_cast<int>(data[13]) << 8);
    const int height = static_cast<int>(data[14]) | (static_cast<int>(data[15]) << 8);
    const unsigned planes = data[16];
    const unsigned scanLineOrder = data[17];

    // Die Kopfpruefungen in der Reihenfolge von LoadTGA.
    if (colourMapType != 0) { return fail("LoadTGA: colourmaps not supported"); }
    if (imageType != 2 && imageType != 3 && imageType != 10) {
        return fail("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RLE-RGB) images supported");
    }
    if (firstColourMapEntry != 0) { return fail("LoadTGA: colourmaps not supported"); }
    if (colourMapLength != 0 && colourMapLength != 256) {
        return fail("LoadTGA: ColourMapLength must be either 0 or 256");
    }
    if (colourMapDepth != 0 && colourMapDepth != 24) {
        return fail("LoadTGA: ColourMapEntrySize must be either 0 or 24");
    }
    if ((planes != 24 && planes != 32) && (planes != 8 && imageType != 3)) {
        return fail("LoadTGA: Only type 2 (RGB), 3 (gray), and 10 (RGB) TGA images supported");
    }
    if (imageType == 10) {
        if ((scanLineOrder & 0x30) != 0x00) {
            return fail("LoadTGA: RLE-RGB Images (type 10) must be in bottom-to-top format");
        }
        if (planes != 24 && planes != 32) {
            return fail("LoadTGA: RLE-RGB Images (type 10) must be 24 or 32 bit");
        }
    }
    // Typ 3 mit z. B. 16 Bit kommt durch die Kopfpruefung und scheitert in der
    // Bildpunktschleife am default-Zweig (TGA_FORMAT_ERROR) - das ganze Bild.
    if (planes != 8 && planes != 24 && planes != 32) {
        return fail("LoadTGA: Image can only have 8, 24 or 32 planes for RGB/greyscale");
    }
    if (tooBig(width, height)) { return fail("implausible image size");
}

    size_t offset = 18 + idLength;   // pIn: hinter dem Kopf und dem Kommentar
    if (offset > size) { return fail("truncated after the header");
}

    const size_t bytesPerPixel = planes / 8;
    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height);
    out.rgba.assign(pixelCount * 4, 0);

    // Ein Bildpunkt wie in LoadTGA: BGR(A); 8 Bit ist Grau, Alpha 255.
    auto writePixel = [&](size_t index, const unsigned char* pixel) {
        unsigned char* o = out.rgba.data() + index * 4;
        if (planes == 8) {
            o[0] = o[1] = o[2] = pixel[0];
            o[3] = 255;
        } else {
            o[0] = pixel[2];
            o[1] = pixel[1];
            o[2] = pixel[0];
            o[3] = planes == 32 ? pixel[3] : 255;
        }
    };

    // Zeilen: Bit 0x20 gesetzt heisst oben beginnend (iYStart 0), sonst unten.
    // Typ 10 ist immer von unten (oben geprueft).
    const bool topDown = (scanLineOrder & 0x20) != 0;

    if (imageType == 10) {
        size_t written = 0;
        std::vector<unsigned char> pixel(bytesPerPixel);
        while (written < pixelCount) {
            if (offset >= size) { return fail("truncated RLE data");
}
            const unsigned packet = data[offset++];
            // Ueber das letzte Bild hinaus: abgeschnitten (goto breakOut).
            const size_t count = std::min<size_t>((packet & 0x7FU) + 1U, pixelCount - written);
            if ((packet & 0x80U) != 0U) {
                if (offset + bytesPerPixel > size) { return fail("truncated RLE run");
}
                std::memcpy(pixel.data(), data + offset, bytesPerPixel);
                offset += bytesPerPixel;
                for (size_t i = 0; i < count; ++i) { writePixel(written + i, pixel.data()); }
            } else {
                if (offset + count * bytesPerPixel > size) { return fail("truncated RLE literals");
}
                for (size_t i = 0; i < count; ++i) { writePixel(written + i, data + offset + i * bytesPerPixel); }
                offset += count * bytesPerPixel;
            }
            written += count;
        }
    } else {
        if (pixelCount * bytesPerPixel > size - offset) { return fail("truncated pixel data");
}
        for (size_t i = 0; i < pixelCount; ++i) { writePixel(i, data + offset + i * bytesPerPixel); }
    }

    // Von unten nach oben gelesen: Zeilen tauschen (iYStart wHeight-1, iYStep -1).
    if (!topDown && height > 1) {
        const size_t rowBytes = static_cast<size_t>(width) * 4;
        std::vector<unsigned char> row(rowBytes);
        for (int y = 0; y < height / 2; ++y) {
            unsigned char* upper = out.rgba.data() + static_cast<size_t>(y) * rowBytes;
            unsigned char* lower =
                out.rgba.data() + static_cast<size_t>(height - 1 - y) * rowBytes;
            std::memcpy(row.data(), upper, rowBytes);
            std::memcpy(upper, lower, rowBytes);
            std::memcpy(lower, row.data(), rowBytes);
        }
    }

    out.width = width;
    out.height = height;
    out.hasAlpha = planes == 32;
    out.ok = true;
    return out;
}

Image decode(const unsigned char* data, size_t size) {
    Image out;
    switch (sniff(data, size)) {
        case Format::Tga:
            return decodeTga(data, size);
        case Format::Png:
            return decodePng(data, size);
        case Format::Jpeg:
            return decodeJpeg(data, size);
        default:
            out.error = "unrecognised image format";
            return out;
    }
}

}  // namespace bhed::image

