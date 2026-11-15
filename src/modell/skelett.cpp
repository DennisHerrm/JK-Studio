// modell/skelett.cpp - eigenes Skelett anlegen, Modell binden (CW-MODELL, 15.11.2026) - siehe bhed/modell/skelett.h
#include "bhed/modell/skelett.h"

#include <algorithm>
#include <cctype>
#include <cstring>

#include "g2/compress.h"

namespace bhed::modell {
namespace {

std::size_t uz(int i) { return static_cast<std::size_t>(i); }

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool nameFrei(const GlaAnimation& a, const std::string& name, int ausser) {
    if (name.empty() || name.size() > 63U) { return false; }
    for (const char c : name) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0) { return false; }
    }
    const std::string k = klein(name);
    for (std::size_t i = 0; i < a.bones.size(); ++i) {
        if (static_cast<int>(i) != ausser && klein(a.bones[i].name) == k) { return false; }
    }
    return true;
}

BoneMatrix verschiebung(const float ort[3]) {
    BoneMatrix m = BoneMatrix::identity();
    for (std::size_t k = 0; k < 3; ++k) { m.m[k][3] = ort[k]; }
    return m;
}

BoneMatrix umgekehrt(const float ort[3]) {
    const float neg[3] = {-ort[0], -ort[1], -ort[2]};
    return verschiebung(neg);
}

// Der Eintrag im Vorrat fuer eine lokale Stellung (compressBone wie Carcass/G2C); gleiche Eintraege teilen sich einen Platz.
std::uint32_t vorratEintrag(GlaAnimation& a, const BoneMatrix& lokal) {
    g2::Mat3x4 m;
    std::memcpy(m.m, lokal.m, sizeof m.m);
    g2::CompressStats st;
    const g2::fmt::CompQuatBone c = g2::compressBone(m, g2::CompressOptions{}, st);
    constexpr std::size_t kGross = 14U;
    for (std::size_t i = 0; i + kGross <= a.bonePool.size(); i += kGross) {
        if (std::memcmp(a.bonePool.data() + i, c.comp, kGross) == 0) { return static_cast<std::uint32_t>(i / kGross); }
    }
    a.bonePool.insert(a.bonePool.end(), c.comp, c.comp + kGross);
    return static_cast<std::uint32_t>(a.bonePool.size() / kGross - 1U);
}

void kinderNeu(GlaAnimation& a) {
    for (GlaBone& b : a.bones) { b.kinder.clear(); }
    for (std::size_t i = 0; i < a.bones.size(); ++i) {
        const int p = a.bones[i].parent;
        if (p >= 0 && uz(p) < a.bones.size()) { a.bones[uz(p)].kinder.push_back(static_cast<int>(i)); }
    }
}

}  // namespace

GlaAnimation skelettNeu(const std::string& kopfPfad) {
    GlaAnimation a;
    a.name = kopfPfad;
    a.skeletonName = kopfPfad;
    a.scale = 1.0F;
    a.numFrames = 1;
    GlaBone w;
    w.name = "model_root";
    w.parent = -1;
    w.basePose = BoneMatrix::identity();
    w.basePoseInv = BoneMatrix::identity();
    a.bones.push_back(w);
    a.indexes.push_back(vorratEintrag(a, BoneMatrix::identity()));
    return a;
}

int knochenAnlegen(GlaAnimation& a, const std::string& name, int eltern, const float ort[3]) {
    if (!nameFrei(a, name, -1) || eltern < -1 || eltern >= static_cast<int>(a.bones.size()) || a.bones.size() >= 255U || !a.roh.empty()) {
        return -1;
    }
    GlaBone b;
    b.name = name;
    b.parent = eltern;
    b.basePose = verschiebung(ort);
    b.basePoseInv = umgekehrt(ort);
    const std::size_t alt = a.bones.size();
    a.bones.push_back(b);
    kinderNeu(a);
    // je Bild einen Verweis mehr: der neue Knochen mit A = Einheit (geht starr mit seinem Eltern)
    const std::uint32_t eins = vorratEintrag(a, BoneMatrix::identity());
    std::vector<std::uint32_t> neu;
    neu.reserve(static_cast<std::size_t>(std::max(a.numFrames, 0)) * (alt + 1U));
    for (int f = 0; f < a.numFrames; ++f) {
        for (std::size_t k = 0; k < alt; ++k) {
            const std::size_t at = static_cast<std::size_t>(f) * alt + k;
            neu.push_back(at < a.indexes.size() ? a.indexes[at] : eins);
        }
        neu.push_back(eins);
    }
    a.indexes = std::move(neu);
    return static_cast<int>(alt);
}

bool knochenVerschieben(GlaAnimation& a, int knochen, const float ort[3]) {
    if (knochen < 0 || uz(knochen) >= a.bones.size()) { return false; }
    a.bones[uz(knochen)].basePose = verschiebung(ort);
    a.bones[uz(knochen)].basePoseInv = umgekehrt(ort);
    return true;
}

bool knochenUmbenennen(GlaAnimation& a, int knochen, const std::string& name) {
    if (knochen < 0 || uz(knochen) >= a.bones.size() || !nameFrei(a, name, knochen)) { return false; }
    a.bones[uz(knochen)].name = name;
    return true;
}

bool knochenEltern(GlaAnimation& a, int knochen, int eltern) {
    if (knochen < 0 || uz(knochen) >= a.bones.size() || eltern < -1 || eltern >= static_cast<int>(a.bones.size()) || eltern == knochen) { return false; }
    // keine Schleife: der neue Eltern darf nicht unter dem Knochen haengen
    for (int p = eltern; p >= 0; p = a.bones[uz(p)].parent) {
        if (p == knochen) { return false; }
    }
    a.bones[uz(knochen)].parent = eltern;
    kinderNeu(a);
    return true;
}

bool knochenEntfernen(GlaAnimation& a, Inhalt& in, int knochen) {
    if (knochen < 0 || uz(knochen) >= a.bones.size() || a.bones.size() < 2U || !a.bones[uz(knochen)].kinder.empty() || !a.roh.empty()) { return false; }
    const int ziel = std::max(a.bones[uz(knochen)].parent, 0);
    const int zielNeu = ziel > knochen ? ziel - 1 : ziel;
    const std::size_t alt = a.bones.size();
    // die Verweise je Bild ohne den Knochen
    std::vector<std::uint32_t> neu;
    for (int f = 0; f < a.numFrames; ++f) {
        for (std::size_t k = 0; k < alt; ++k) {
            const std::size_t at = static_cast<std::size_t>(f) * alt + k;
            if (static_cast<int>(k) != knochen && at < a.indexes.size()) { neu.push_back(a.indexes[at]); }
        }
    }
    a.indexes = std::move(neu);
    a.bones.erase(a.bones.begin() + knochen);
    for (GlaBone& b : a.bones) {
        if (b.parent > knochen) { --b.parent; }
    }
    kinderNeu(a);
    // die Ecken: an ihn gebundene an den Eltern, die Nummern dahinter eins herunter
    for (Teil& t : in.teile) {
        for (Netz& n : t.stufen) {
            bool geaendert = false;
            std::vector<Ecke> e = n.ecken;
            for (Ecke& x : e) {
                for (std::size_t k = 0; k < 4; ++k) {
                    const int b = x.knochen[k];
                    const int nb = b == knochen ? zielNeu : (b > knochen ? b - 1 : b);
                    if (nb != b) {
                        x.knochen[k] = static_cast<std::uint8_t>(nb);
                        x.ausDatei = false;
                        geaendert = true;
                    }
                }
            }
            if (!geaendert) { continue; }
            n.ecken = std::move(e);   // dieselben Dreiecke und Orte: die Halbkanten bleiben
            n.ausDatei = false;
            n.dateiRefs.clear();
            n.dateiRefsAufgeloest.clear();
        }
    }
    in.knochenZahl = static_cast<int>(a.bones.size());
    ++in.netzStand;
    return true;
}

void flaechenAnKnochen(Inhalt& in, const std::vector<int>& teile, int knochen) {
    for (const int ti : teile) {
        if (ti < 0 || uz(ti) >= in.teile.size()) { continue; }
        for (Netz& n : in.teile[uz(ti)].stufen) {
            for (Ecke& e : n.ecken) {
                for (std::size_t k = 0; k < 4; ++k) {
                    e.knochen[k] = 0;
                    e.gewicht[k] = 0.0F;
                }
                e.knochen[0] = static_cast<std::uint8_t>(std::clamp(knochen, 0, 255));
                e.gewicht[0] = 1.0F;
                e.ausDatei = false;
            }
            n.ausDatei = false;
            n.dateiRefs.clear();
            n.dateiRefsAufgeloest.clear();
            n.mal.clear();
        }
    }
    ++in.netzStand;
}

void modellAnSkelett(Inhalt& in, const GlaAnimation& a) {
    in.glaPfad = a.skeletonName;
    in.dateiGla.clear();
    in.knochenZahl = static_cast<int>(a.bones.size());
    in.dateiKnochen = in.knochenZahl;
    in.jk2Umgewandelt = false;
    in.jkaUmgewandelt = false;
    for (Teil& t : in.teile) {
        for (Netz& n : t.stufen) {
            bool raus = false;
            for (Ecke& e : n.ecken) {
                for (std::size_t k = 0; k < 4; ++k) {
                    if (e.knochen[k] >= a.bones.size()) {
                        e.knochen[k] = 0;
                        e.ausDatei = false;
                        raus = true;
                    }
                }
            }
            if (raus) {
                n.ausDatei = false;
                n.dateiRefs.clear();
                n.dateiRefsAufgeloest.clear();
            }
        }
    }
    ++in.netzStand;
}

bool bildAnhaengen(GlaAnimation& a, const std::vector<BoneMatrix>& lokal) {
    if (lokal.size() != a.bones.size() || !a.roh.empty()) { return false; }
    for (const BoneMatrix& m : lokal) { a.indexes.push_back(vorratEintrag(a, m)); }
    ++a.numFrames;
    return true;
}

}  // namespace bhed::modell
