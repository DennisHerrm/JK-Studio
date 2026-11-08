// modell/rig.cpp - Knochen-Erkennung, Control Rig, Schicht und Backen (siehe bhed/modell/rig.h)
//
//   Uses G2C (third_party/g2c, DennisHerrm/G2C-toolkit, GNU General Public License version 2) for compressBone. This file
//   is part of BehavEd-Remake and licensed under the GNU General Public License version 2 or later (LICENSE).
#include "bhed/modell/rig.h"

#include "bhed/modell/animkurven.h"
#include "bhed/modell/fehler.h"
#include "bhed/modell/glavorrat.h"
#include "bhed/modell/ik.h"
#include "bhed/modell/physik.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstring>
#include <map>
#include <string>
#include <tuple>
#include <unordered_map>

#include "g2/compress.h"

namespace bhed::modell {
namespace {

constexpr float kPi = 3.14159265358979323846F;

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

bool enthaelt(const std::string& s, const char* w) { return s.find(w) != std::string::npos; }
bool beginnt(const std::string& s, const char* w) { return s.rfind(w, 0) == 0; }
bool endet(const std::string& s, const std::string& w) { return s.size() >= w.size() && s.compare(s.size() - w.size(), w.size(), w) == 0; }

void ort(const BoneMatrix& m, float p[3]) {
    p[0] = m.m[0][3];
    p[1] = m.m[1][3];
    p[2] = m.m[2][3];
}

float abstand(const float a[3], const float b[3]) {
    return std::sqrt((a[0] - b[0]) * (a[0] - b[0]) + (a[1] - b[1]) * (a[1] - b[1]) + (a[2] - b[2]) * (a[2] - b[2]));
}

BoneMatrix verschiebung(const float d[3]) {
    BoneMatrix m = BoneMatrix::identity();
    m.m[0][3] = d[0];
    m.m[1][3] = d[1];
    m.m[2][3] = d[2];
    return m;
}

// --- Namen -------------------------------------------------------------------------------------------------------------

// Woerter, vor denen ein einzelnes l/r die Seite ist (lhumerus, rfemurYZ, ltalus, lhang_tag_bone, rhand_tag_bone ...).
// Aus den Skeletten von JKA (53), JK2 (72) und Movie Duels Update 9 gelesen (gladump ueber alle Archive).
const char* const kSeitenWoerter[] = {"humerus", "radius", "hand", "femur", "tibia", "talus", "tarsal", "clavical", "clavicle", "eye",
                                      "tail", "blip", "tlip", "hang", "foot", "thigh", "calf", "shoulder", "arm", "leg", "toe", "heel",
                                      "knee", "elbow", "wrist", "ankle", "brow"};

// Seite aus dem Namen und der Name ohne die Seite.
Seite seiteAusName(const std::string& n, std::string& rest, std::string* spiegel) {
    const std::string k = klein(n);
    const auto tausch = [&](std::size_t pos, std::size_t len, const std::string& neu) {
        if (spiegel == nullptr) { return; }
        std::string s = n;
        std::string ersatz = neu;
        // Gross-/Kleinschreibung des Originals behalten (L_thigh -> R_thigh, leg_left -> leg_right)
        if (len > 0 && std::isupper(static_cast<unsigned char>(n[pos])) != 0) {
            for (char& c : ersatz) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
            if (len > 1 && std::islower(static_cast<unsigned char>(n[pos + 1])) != 0) {
                for (std::size_t i = 1; i < ersatz.size(); ++i) { ersatz[i] = static_cast<char>(std::tolower(static_cast<unsigned char>(ersatz[i]))); }
            }
        }
        s.replace(pos, len, ersatz);
        *spiegel = s;
    };
    for (const auto& [wort, seite, gegen] : {std::tuple<const char*, Seite, const char*>{"left", Seite::Links, "right"}, {"right", Seite::Rechts, "left"}}) {
        const std::size_t p = k.find(wort);
        if (p != std::string::npos) {
            rest = k;
            rest.erase(p, std::strlen(wort));
            tausch(p, std::strlen(wort), gegen);
            return seite;
        }
    }
    if (k.size() > 2 && (k[0] == 'l' || k[0] == 'r') && k[1] == '_') {
        rest = k.substr(2);
        tausch(0, 1, k[0] == 'l' ? "r" : "l");
        return k[0] == 'l' ? Seite::Links : Seite::Rechts;
    }
    if (k.size() > 2 && (endet(k, "_l") || endet(k, "_r"))) {
        rest = k.substr(0, k.size() - 2);
        tausch(k.size() - 1, 1, k.back() == 'l' ? "r" : "l");
        return k.back() == 'l' ? Seite::Links : Seite::Rechts;
    }
    if (k.size() > 2 && (k[0] == 'l' || k[0] == 'r')) {
        for (const char* w : kSeitenWoerter) {
            if (beginnt(k.substr(1), w)) {
                rest = k.substr(1);
                tausch(0, 1, k[0] == 'l' ? "r" : "l");
                return k[0] == 'l' ? Seite::Links : Seite::Rechts;
            }
        }
    }
    rest = k;
    if (spiegel != nullptr) { spiegel->clear(); }
    return Seite::Mitte;
}

// Allgemeine Glieder ohne eigene Rolle im Namen (Droideka: r_armbone, leg_left_bone01): die Rolle kommt aus der
// Reihenfolge in der Kette.
enum class Allgemein : std::uint8_t { Nein, Arm, Bein };

Rolle rolleAusName(const std::string& original, const std::string& rest, Seite seite, Allgemein& allgemein) {
    allgemein = Allgemein::Nein;
    const std::string& b = rest;
    const bool twist = !original.empty() && original.back() == 'X';   // lhumerusX, rradiusX, lfemurX (JKA, JK2)
    if (b == "model_root" || b == "root" || endet(b, "_root")) { return Rolle::Wurzel; }
    if (b == "motion") { return Rolle::Bewegung; }
    if (enthaelt(b, "tag")) { return Rolle::TagKnochen; }
    // Stoff und Wackelknochen (Marka Ragnos L_Chest_Cloth_01, Rancor back_fat/neck_fat): Anhang, nicht die Rolle des Worts davor
    if (enthaelt(b, "cloth") || enthaelt(b, "fat") || enthaelt(b, "cape") || enthaelt(b, "robe") || enthaelt(b, "skirt")) { return Rolle::Anhang; }
    // das Becken liegt in der Mitte; "L_Hip" (Howler) ist das Hueftgelenk eines Beins, nicht das Becken
    if (seite == Seite::Mitte && (enthaelt(b, "pelvis") || b == "hip_bone" || b == "hips" || b == "hip")) { return Rolle::Becken; }
    // Ober-/Unterschenkel und -arm als zusammengesetzte Namen (Howler L_Upper_Leg, Tauntaun R_leg_upper, L_arm_lower)
    if (enthaelt(b, "upper_leg") || enthaelt(b, "leg_upper") || enthaelt(b, "upperleg")) { return Rolle::Oberschenkel; }
    if (enthaelt(b, "lower_leg") || enthaelt(b, "leg_lower") || enthaelt(b, "lowerleg")) { return Rolle::Unterschenkel; }
    if (enthaelt(b, "arm_upper")) { return Rolle::Oberarm; }
    if (enthaelt(b, "arm_lower")) { return Rolle::Unterarm; }
    if (enthaelt(b, "lumbar") || enthaelt(b, "spine") || enthaelt(b, "thoracic") || enthaelt(b, "thorax") || enthaelt(b, "chest")) {
        return Rolle::Wirbel;
    }
    if (enthaelt(b, "cervical") || enthaelt(b, "neck")) { return Rolle::Hals; }
    if (enthaelt(b, "face") || enthaelt(b, "jaw") || enthaelt(b, "eye") || enthaelt(b, "brow") || enthaelt(b, "lip") || enthaelt(b, "mouth") ||
        enthaelt(b, "tooth") || enthaelt(b, "teeth") || enthaelt(b, "tongue") || enthaelt(b, "cheek") || enthaelt(b, "nose")) {
        return Rolle::Gesicht;
    }
    if (enthaelt(b, "cranium") || enthaelt(b, "cranic") || enthaelt(b, "head") || enthaelt(b, "skull")) { return Rolle::Kopf; }
    if (enthaelt(b, "clavical") || enthaelt(b, "clavicle") || enthaelt(b, "shoulder")) { return Rolle::Schluessel; }
    // Finger: r_d1_j1 (JKA/JK2), mc5/mc7 (JK2 Mittelhand), index/mid/thumb (Rancor)
    {
        const std::size_t d = b.find('d');
        const bool dj = d != std::string::npos && d + 3 < b.size() && std::isdigit(static_cast<unsigned char>(b[d + 1])) != 0 &&
                        b[d + 2] == '_' && b[d + 3] == 'j';
        const bool mc = b.size() >= 3 && b[0] == 'm' && b[1] == 'c' && std::isdigit(static_cast<unsigned char>(b[2])) != 0;
        if (dj || mc || enthaelt(b, "finger") || enthaelt(b, "thumb") || enthaelt(b, "index") || beginnt(b, "mid_") || enthaelt(b, "pinky") ||
            beginnt(b, "ring_")) {
            return Rolle::Finger;
        }
    }
    if (enthaelt(b, "humerus")) { return twist ? Rolle::OberarmTwist : Rolle::Oberarm; }
    if (enthaelt(b, "upper_arm") || enthaelt(b, "upperarm")) { return Rolle::Oberarm; }
    if (enthaelt(b, "radius")) { return twist ? Rolle::UnterarmTwist : Rolle::Unterarm; }
    if (enthaelt(b, "lower_arm") || enthaelt(b, "forearm") || enthaelt(b, "ulna")) { return Rolle::Unterarm; }
    if (enthaelt(b, "hand")) { return Rolle::Hand; }
    if (enthaelt(b, "femur")) { return twist ? Rolle::OberschenkelTwist : Rolle::Oberschenkel; }
    if (enthaelt(b, "thigh")) { return Rolle::Oberschenkel; }
    if (enthaelt(b, "tibia") || enthaelt(b, "calve") || enthaelt(b, "calf") || enthaelt(b, "shin") || enthaelt(b, "knee")) { return Rolle::Unterschenkel; }
    if (enthaelt(b, "talus") || enthaelt(b, "foot") || enthaelt(b, "ankle") || enthaelt(b, "heel")) { return Rolle::Fuss; }
    if (enthaelt(b, "tarsal") || enthaelt(b, "toe")) { return Rolle::Zehen; }
    if (enthaelt(b, "tail")) { return Rolle::Anhang; }
    if (enthaelt(b, "gun") || enthaelt(b, "shield") || enthaelt(b, "wing") || enthaelt(b, "weapon") || enthaelt(b, "barrel")) {
        return Rolle::Sonstige;
    }
    if (enthaelt(b, "arm")) {
        allgemein = Allgemein::Arm;
        return Rolle::Keine;
    }
    if (enthaelt(b, "leg")) {
        allgemein = Allgemein::Bein;
        return Rolle::Keine;
    }
    return Rolle::Keine;
}

// Rang in einer Kette: Arm Schluessel < Oberarm < Unterarm < Hand < Finger, Bein Oberschenkel < Unterschenkel < Fuss <
// Zehen; Drehknochen zwischen ihren Segmenten. Mitte Becken < Wirbel < Hals < Kopf.
float rang(Rolle r) {
    switch (r) {
        case Rolle::Schluessel: return 0.0F;
        case Rolle::Oberarm: case Rolle::Oberschenkel: return 1.0F;
        case Rolle::OberarmTwist: case Rolle::OberschenkelTwist: return 1.5F;
        case Rolle::Unterarm: case Rolle::Unterschenkel: return 2.0F;
        case Rolle::UnterarmTwist: return 2.5F;
        case Rolle::Hand: case Rolle::Fuss: return 3.0F;
        case Rolle::TagKnochen: return 3.5F;
        case Rolle::Finger: case Rolle::Zehen: return 4.0F;
        case Rolle::Becken: return 0.0F;
        case Rolle::Wirbel: return 1.0F;
        case Rolle::Hals: return 2.0F;
        case Rolle::Kopf: return 3.0F;
        default: return -1.0F;
    }
}

bool hauptGlied(Rolle r) {
    return r == Rolle::Oberarm || r == Rolle::Unterarm || r == Rolle::Hand || r == Rolle::Oberschenkel || r == Rolle::Unterschenkel ||
           r == Rolle::Fuss || r == Rolle::Zehen;
}

bool mittelKette(Rolle r) { return r == Rolle::Becken || r == Rolle::Wirbel || r == Rolle::Hals || r == Rolle::Kopf; }

// Arm <-> Bein (eine Rolle am falschen Ansatz)
Rolle gegenGlied(Rolle r) {
    switch (r) {
        case Rolle::Oberarm: return Rolle::Oberschenkel;
        case Rolle::OberarmTwist: return Rolle::OberschenkelTwist;
        case Rolle::Unterarm: return Rolle::Unterschenkel;
        case Rolle::Hand: return Rolle::Fuss;
        case Rolle::Finger: return Rolle::Zehen;
        case Rolle::Oberschenkel: return Rolle::Oberarm;
        case Rolle::OberschenkelTwist: return Rolle::OberarmTwist;
        case Rolle::Unterschenkel: return Rolle::Unterarm;
        case Rolle::Fuss: return Rolle::Hand;
        case Rolle::Zehen: return Rolle::Finger;
        default: return r;
    }
}

int tiefe(const GlaAnimation& g, int b) {
    int t = 0;
    const int n = static_cast<int>(g.bones.size());
    for (int p = b; p >= 0 && p < n && t <= n; p = g.bones[static_cast<std::size_t>(p)].parent) { ++t; }
    return t;
}

bool istNachfahre(const GlaAnimation& g, int b, int vorfahr) {
    const int n = static_cast<int>(g.bones.size());
    int schritte = 0;
    for (int p = b >= 0 && b < n ? g.bones[static_cast<std::size_t>(b)].parent : -1; p >= 0 && p < n && schritte <= n;
         p = g.bones[static_cast<std::size_t>(p)].parent, ++schritte) {
        if (p == vorfahr) { return true; }
    }
    return false;
}

}  // namespace

const char* rollenSchluessel(Rolle r) noexcept {
    static const char* const kNamen[kRollen] = {"none",     "root",     "motion",       "pelvis",  "spine",      "neck",         "head",     "face",
                                                "clavicle", "upperarm", "upperarm_twist", "forearm", "forearm_twist", "hand",       "finger",   "thigh",
                                                "thigh_twist", "calf",  "foot",         "toe",     "tail",       "tag_bone",     "other"};
    const auto i = static_cast<std::size_t>(r);
    return i < static_cast<std::size_t>(kRollen) ? kNamen[i] : "none";
}

bool rolleAusSchluessel(const std::string& s, Rolle& r) noexcept {
    for (int i = 0; i < kRollen; ++i) {
        if (s == rollenSchluessel(static_cast<Rolle>(i))) {
            r = static_cast<Rolle>(i);
            return true;
        }
    }
    return false;
}

bool armRolle(Rolle r) noexcept {
    return r == Rolle::Schluessel || r == Rolle::Oberarm || r == Rolle::OberarmTwist || r == Rolle::Unterarm || r == Rolle::UnterarmTwist ||
           r == Rolle::Hand || r == Rolle::Finger;
}

bool beinRolle(Rolle r) noexcept {
    return r == Rolle::Oberschenkel || r == Rolle::OberschenkelTwist || r == Rolle::Unterschenkel || r == Rolle::Fuss || r == Rolle::Zehen;
}

std::string spiegelName(const std::string& name) {
    std::string rest;
    std::string sp;
    (void)seiteAusName(name, rest, &sp);
    return sp;
}

Erkennung erkenneKnochen(const GlaAnimation& g) { return erkenneKnochen(g, {}); }

Erkennung erkenneKnochen(const GlaAnimation& g, const std::vector<Zuordnung>& vonHand, const Inhalt* in, int stufe) {
    Erkennung e;
    const int n = static_cast<int>(g.bones.size());
    e.knochen.assign(static_cast<std::size_t>(n), KnochenErkennung{});
    if (n == 0) { return e; }
    std::vector<std::array<float, 3>> p(static_cast<std::size_t>(n));
    float zMin = 0.0F;
    float zMax = 0.0F;
    for (int b = 0; b < n; ++b) {
        ort(g.bones[static_cast<std::size_t>(b)].basePose, p[static_cast<std::size_t>(b)].data());
        zMin = b == 0 ? p[0][2] : std::min(zMin, p[static_cast<std::size_t>(b)][2]);
        zMax = b == 0 ? p[0][2] : std::max(zMax, p[static_cast<std::size_t>(b)][2]);
    }
    e.hoehe = zMax - zMin;
    // 1. Namen
    std::vector<Allgemein> allg(static_cast<std::size_t>(n), Allgemein::Nein);
    std::map<std::string, int> nachName;
    for (int b = 0; b < n; ++b) { nachName[g.bones[static_cast<std::size_t>(b)].name] = b; }
    for (int b = 0; b < n; ++b) {
        KnochenErkennung& k = e.knochen[static_cast<std::size_t>(b)];
        std::string rest;
        k.seiteName = seiteAusName(g.bones[static_cast<std::size_t>(b)].name, rest, nullptr);
        k.nachName = rolleAusName(g.bones[static_cast<std::size_t>(b)].name, rest, k.seiteName, allg[static_cast<std::size_t>(b)]);
        k.rolle = k.nachName;
    }
    // 2. Seitenachse: die Achse, auf der sich die Namenspaare am weitesten unterscheiden; links = Vorzeichen der Linken.
    {
        double diff[2] = {0.0, 0.0};
        double vz = 0.0;
        for (int b = 0; b < n; ++b) {
            if (e.knochen[static_cast<std::size_t>(b)].seiteName != Seite::Links) { continue; }
            const auto it = nachName.find(spiegelName(g.bones[static_cast<std::size_t>(b)].name));
            if (it == nachName.end()) { continue; }
            for (int a = 0; a < 2; ++a) {
                diff[a] += std::fabs(static_cast<double>(p[static_cast<std::size_t>(b)][static_cast<std::size_t>(a)] -
                                                         p[static_cast<std::size_t>(it->second)][static_cast<std::size_t>(a)]));
            }
        }
        e.seitenAchse = diff[1] > diff[0] ? 1 : 0;
        for (int b = 0; b < n; ++b) {
            if (e.knochen[static_cast<std::size_t>(b)].seiteName != Seite::Links) { continue; }
            const auto it = nachName.find(spiegelName(g.bones[static_cast<std::size_t>(b)].name));
            if (it == nachName.end()) { continue; }
            vz += static_cast<double>(p[static_cast<std::size_t>(b)][static_cast<std::size_t>(e.seitenAchse)] -
                                      p[static_cast<std::size_t>(it->second)][static_cast<std::size_t>(e.seitenAchse)]);
        }
        // Ohne Namenspaare: Ghoul2-Skelette tragen links bei +X (JKA _humanoid lhumerus +5,92, rhumerus -5,92).
        e.linksVorzeichen = vz < 0.0 ? -1.0F : 1.0F;
    }
    // 3. Seite aus der Lage: |x| ueber 3 % der Hoehe. Gemessen an JKA/JK2/MD: der seitlichste Mittelknochen (leye) liegt
    // bei 1,8 %, der innerste Seitenknochen (lfemurYZ) bei 5,7 % der Hoehe.
    const float tol = 0.03F * std::max(e.hoehe, 1e-3F);
    const auto seiteBei = [&](float x) {
        const float s = x * e.linksVorzeichen;
        return s > tol ? Seite::Links : (s < -tol ? Seite::Rechts : Seite::Mitte);
    };
    // Die Ecken, die ein Knochen traegt (groesstes Gewicht), als Lage fuer Knochen in der Mitte.
    std::vector<double> eckSumme(static_cast<std::size_t>(n), 0.0);
    std::vector<int> eckZahl(static_cast<std::size_t>(n), 0);
    if (in != nullptr) {
        for (const Teil& t : in->teile) {
            if (t.istTag() || t.stufen.empty()) { continue; }
            const Netz& netz = t.stufen[static_cast<std::size_t>(std::clamp(stufe, 0, static_cast<int>(t.stufen.size()) - 1))];
            for (const Ecke& q : netz.ecken) {
                int best = 0;
                for (int k = 1; k < 4; ++k) {
                    if (q.gewicht[k] > q.gewicht[best]) { best = k; }
                }
                if (q.gewicht[best] <= 0.0F || q.knochen[best] >= n) { continue; }
                eckSumme[q.knochen[best]] += static_cast<double>(q.ort[e.seitenAchse]);
                ++eckZahl[q.knochen[best]];
            }
        }
    }
    for (int b = 0; b < n; ++b) {
        KnochenErkennung& k = e.knochen[static_cast<std::size_t>(b)];
        k.seiteLage = seiteBei(p[static_cast<std::size_t>(b)][static_cast<std::size_t>(e.seitenAchse)]);
        if (k.seiteLage == Seite::Mitte && eckZahl[static_cast<std::size_t>(b)] > 0) {
            k.seiteLage = seiteBei(static_cast<float>(eckSumme[static_cast<std::size_t>(b)] / eckZahl[static_cast<std::size_t>(b)]));
        }
        if (k.seiteName != Seite::Mitte && k.seiteLage != Seite::Mitte && k.seiteName != k.seiteLage) {
            k.seite = k.seiteLage;
            k.unklar = true;
            k.grund = "seite";
        } else {
            k.seite = k.seiteName != Seite::Mitte ? k.seiteName : k.seiteLage;
        }
    }
    // 4. Ansatz je Seitenknochen: der erste Vorfahr in der Mitte. Je Ansatz stimmen die Namen ab (Bein- gegen Armrollen,
    // allgemeine Glieder mitgezaehlt); ohne Mehrheit traegt der Ansatz die Beine, der am tiefsten reicht. Marka Ragnos hat keine
    // Beine: dort war sonst die Schulter, die am tiefsten reicht, der "Beinansatz" (mb4_rl_jka: 18 unklar).
    std::vector<int> ansatz(static_cast<std::size_t>(n), -1);
    std::map<int, float> tiefsterPunkt;
    std::map<int, int> stimmen;   // Ansatz -> Beinrollen minus Armrollen der Namen
    // Dazu je Glied (der oberste Seitenknochen unter dem Ansatz): die Kreaturen haengen Arme und Beine an denselben Knochen der
    // Mitte (Howler), dort entscheidet das Glied selbst. Nur ein Knochen im Glied (JK2: alles flach): es gilt der Ansatz.
    std::vector<int> gliedWurzel(static_cast<std::size_t>(n), -1);
    std::map<int, int> stimmenGlied;
    std::map<int, int> gliedZahl;
    for (int b = 0; b < n; ++b) {
        if (e.knochen[static_cast<std::size_t>(b)].seiteLage == Seite::Mitte) { continue; }
        int a = g.bones[static_cast<std::size_t>(b)].parent;
        int schritte = 0;
        while (a >= 0 && a < n && e.knochen[static_cast<std::size_t>(a)].seiteLage != Seite::Mitte && schritte++ <= n) {
            a = g.bones[static_cast<std::size_t>(a)].parent;
        }
        if (a < 0 || a >= n) { continue; }
        ansatz[static_cast<std::size_t>(b)] = a;
        int w = b;
        for (int q = g.bones[static_cast<std::size_t>(b)].parent, s2 = 0; q >= 0 && q < n && q != a && s2 <= n; q = g.bones[static_cast<std::size_t>(q)].parent, ++s2) {
            w = q;
        }
        gliedWurzel[static_cast<std::size_t>(b)] = w;
        // nur Glieder zaehlen (Droideka: die Schildknochen reichen tiefer als die Beine)
        const Rolle rn = e.knochen[static_cast<std::size_t>(b)].nachName;
        if (!armRolle(rn) && !beinRolle(rn) && allg[static_cast<std::size_t>(b)] == Allgemein::Nein) { continue; }
        const float z = p[static_cast<std::size_t>(b)][2];
        const auto it = tiefsterPunkt.find(a);
        tiefsterPunkt[a] = it == tiefsterPunkt.end() ? z : std::min(it->second, z);
        const int stimme = (beinRolle(rn) || allg[static_cast<std::size_t>(b)] == Allgemein::Bein) ? 1 : -1;
        stimmen[a] += stimme;
        stimmenGlied[w] += stimme;
        gliedZahl[w] += 1;
    }
    int tiefster = -1;
    for (const auto& [a, z] : tiefsterPunkt) {
        if (tiefster < 0 || z < tiefsterPunkt[tiefster]) { tiefster = a; }
    }
    const auto istBeinAnsatz = [&](int a) {
        const auto it = stimmen.find(a);
        const int s = it != stimmen.end() ? it->second : 0;
        return s != 0 ? s > 0 : a == tiefster;
    };
    const auto istBein = [&](int b) {
        const int w = gliedWurzel[static_cast<std::size_t>(b)];
        const auto it = stimmenGlied.find(w);
        if (w >= 0 && gliedZahl[w] >= 2 && it != stimmenGlied.end() && it->second != 0) { return it->second > 0; }
        return istBeinAnsatz(ansatz[static_cast<std::size_t>(b)]);
    };
    // 5. Arm oder Bein: eine Gliedrolle am falschen Ansatz wird zur Rolle der anderen Gliedmasse (unklar).
    for (int b = 0; b < n; ++b) {
        KnochenErkennung& k = e.knochen[static_cast<std::size_t>(b)];
        const int a = ansatz[static_cast<std::size_t>(b)];
        if (a < 0) { continue; }
        const bool bein = istBein(b);
        if ((bein && armRolle(k.rolle) && k.rolle != Rolle::Schluessel) || (!bein && beinRolle(k.rolle))) {
            k.rolle = gegenGlied(k.rolle);
            k.unklar = true;
            k.grund = "glied";
        }
    }
    // 6. Ketten je (Seite, Ansatz, Arm/Bein): die Hauptglieder nach dem Abstand vom Ansatz; die Rollen muessen in dieser
    // Reihenfolge steigen, sonst werden sie in dieser Reihenfolge neu verteilt (unklar). Allgemeine Glieder (armbone,
    // leg_left_bone01) bekommen die freien Rollen der Kette in derselben Reihenfolge.
    {
        std::map<std::tuple<int, int, int>, std::vector<int>> ketten;
        for (int b = 0; b < n; ++b) {
            const KnochenErkennung& k = e.knochen[static_cast<std::size_t>(b)];
            const int a = ansatz[static_cast<std::size_t>(b)];
            const bool glied = hauptGlied(k.rolle) || allg[static_cast<std::size_t>(b)] != Allgemein::Nein;
            if (!glied) { continue; }
            int schluessel = a;
            if (a < 0) {
                // Glieder in der Mitte (Droideka: leg_back_bone01..03): die Kette ueber die Hierarchie
                schluessel = b;
                int q = g.bones[static_cast<std::size_t>(b)].parent;
                int schritte = 0;
                while (q >= 0 && q < n && allg[static_cast<std::size_t>(q)] != Allgemein::Nein && schritte++ <= n) {
                    schluessel = q;
                    q = g.bones[static_cast<std::size_t>(q)].parent;
                }
                schluessel = -2 - schluessel;
            }
            const bool bein = a >= 0 ? istBein(b) : allg[static_cast<std::size_t>(b)] == Allgemein::Bein;
            ketten[{static_cast<int>(k.seite), schluessel, bein ? 1 : 0}].push_back(b);
        }
        for (auto& [schluessel, liste] : ketten) {
            const int a = std::get<1>(schluessel);
            const bool bein = std::get<2>(schluessel) == 1;
            const float* bezug = a >= 0 ? p[static_cast<std::size_t>(a)].data() : nullptr;
            // Reihenfolge entlang des Glieds: zuerst die Tiefe in der Hierarchie (ein Kind kommt nach seinem Eltern, auch bei
            // gebeugten Beinen wie dem Assassin-Droiden, deren Knoechel naeher am Becken liegt als das Knie), bei gleicher Tiefe
            // (JK2: alles flach) der Abstand vom Ansatz.
            std::vector<std::pair<float, int>> sortiert;
            for (const int b : liste) {
                const float d = bezug != nullptr ? abstand(p[static_cast<std::size_t>(b)].data(), bezug) : 0.0F;
                sortiert.emplace_back(d, b);
            }
            std::sort(sortiert.begin(), sortiert.end(), [&](const std::pair<float, int>& x, const std::pair<float, int>& y) {
                const int tx = tiefe(g, x.second);
                const int ty = tiefe(g, y.second);
                return tx != ty ? tx < ty : x.first < y.first;
            });
            // allgemeine Glieder: die Rollen der Kette, die noch keiner hat
            std::vector<Rolle> frei;
            for (const Rolle r : bein ? std::vector<Rolle>{Rolle::Oberschenkel, Rolle::Unterschenkel, Rolle::Fuss, Rolle::Zehen}
                                      : std::vector<Rolle>{Rolle::Oberarm, Rolle::Unterarm, Rolle::Hand}) {
                const bool belegt = std::any_of(liste.begin(), liste.end(), [&](int b) { return e.knochen[static_cast<std::size_t>(b)].rolle == r; });
                if (!belegt) { frei.push_back(r); }
            }
            std::size_t f = 0;
            for (const auto& [d, b] : sortiert) {
                (void)d;
                if (allg[static_cast<std::size_t>(b)] != Allgemein::Nein && e.knochen[static_cast<std::size_t>(b)].rolle == Rolle::Keine && f < frei.size()) {
                    e.knochen[static_cast<std::size_t>(b)].rolle = frei[f++];
                }
            }
            // Reihenfolge pruefen, nur wo Hierarchie oder Lage sie belegen (je Paar der Hauptglieder):
            //   Vorfahr/Nachfahr      der Vorfahr hat keinen hoeheren Rang (JKA: rfemurYZ -> rtibia -> rtalus)
            //   Geschwister           unter demselben Eltern, der keine Wurzel ist (JK2: alles flach unter pelvis/thoracic): der
            //                         naehere hat keinen hoeheren Rang, wenn sie mehr als tol auseinander liegen
            // Sonst gilt keine Reihenfolge: Fuesse unter der Wurzel (IK-Fuesse von Tauntaun und Assassin-Droide liegen naeher am
            // Ursprung als das Knie), Zehen gleichauf mit dem Fuss (Rancor, 0,6 Einheiten).
            bool steigt = true;
            std::vector<Rolle> rollen;
            for (const auto& [d, b] : sortiert) {
                (void)d;
                if (hauptGlied(e.knochen[static_cast<std::size_t>(b)].rolle)) { rollen.push_back(e.knochen[static_cast<std::size_t>(b)].rolle); }
            }
            for (std::size_t i = 0; i < sortiert.size() && steigt; ++i) {
                for (std::size_t j = 0; j < sortiert.size() && steigt; ++j) {
                    const int x = sortiert[i].second;
                    const int y = sortiert[j].second;
                    const Rolle rx = e.knochen[static_cast<std::size_t>(x)].rolle;
                    const Rolle ry = e.knochen[static_cast<std::size_t>(y)].rolle;
                    if (i == j || !hauptGlied(rx) || !hauptGlied(ry) || rang(rx) <= rang(ry)) { continue; }
                    // rx hat den hoeheren Rang: verkehrt, wenn x belegt vor y liegt
                    if (istNachfahre(g, y, x)) { steigt = false; }
                    const int px = g.bones[static_cast<std::size_t>(x)].parent;
                    const bool geschwister = px >= 0 && px == g.bones[static_cast<std::size_t>(y)].parent &&
                                             e.knochen[static_cast<std::size_t>(px)].nachName != Rolle::Wurzel && g.bones[static_cast<std::size_t>(px)].parent >= 0;
                    if (geschwister && sortiert[j].first - sortiert[i].first >= tol) { steigt = false; }
                }
            }
            if (steigt) { continue; }
            std::sort(rollen.begin(), rollen.end(), [](Rolle x, Rolle y) { return rang(x) < rang(y); });
            std::size_t i = 0;
            for (const auto& [d, b] : sortiert) {
                (void)d;
                KnochenErkennung& k = e.knochen[static_cast<std::size_t>(b)];
                if (!hauptGlied(k.rolle) || i >= rollen.size()) { continue; }
                if (k.rolle != rollen[i]) {
                    k.rolle = rollen[i];
                    k.unklar = true;
                    k.grund = "reihenfolge";
                }
                ++i;
            }
        }
    }
    // 6b. Ein Knochen der Mitte ohne Rolle unter einem Wirbel, an dem die Arme ansetzen, ist der oberste Wirbel (Droideka:
    // randombone zwischen spinebone05 und den Schultern).
    for (int b = 0; b < n; ++b) {
        KnochenErkennung& k = e.knochen[static_cast<std::size_t>(b)];
        const int par = g.bones[static_cast<std::size_t>(b)].parent;
        if (k.rolle != Rolle::Keine || k.seiteLage != Seite::Mitte || par < 0 || par >= n || e.knochen[static_cast<std::size_t>(par)].rolle != Rolle::Wirbel) {
            continue;
        }
        bool armAnsatz = false;
        for (int c = 0; c < n && !armAnsatz; ++c) {
            armAnsatz = ansatz[static_cast<std::size_t>(c)] == b && armRolle(e.knochen[static_cast<std::size_t>(c)].rolle);
        }
        if (armAnsatz) { k.rolle = Rolle::Wirbel; }
    }
    // 7. Mitte: Becken < Wirbel < Hals < Kopf entlang der Hierarchie. Steht eine Rolle ueber einer kleineren, werden die
    // Rollen der Mitte nach der Tiefe neu verteilt (unklar).
    {
        std::vector<int> mitte;
        bool verkehrt = false;
        for (int b = 0; b < n; ++b) {
            if (!mittelKette(e.knochen[static_cast<std::size_t>(b)].rolle)) { continue; }
            mitte.push_back(b);
            for (int q = g.bones[static_cast<std::size_t>(b)].parent, s = 0; q >= 0 && q < n && s <= n; q = g.bones[static_cast<std::size_t>(q)].parent, ++s) {
                if (mittelKette(e.knochen[static_cast<std::size_t>(q)].rolle)) {
                    if (rang(e.knochen[static_cast<std::size_t>(q)].rolle) > rang(e.knochen[static_cast<std::size_t>(b)].rolle)) { verkehrt = true; }
                    break;
                }
            }
        }
        if (verkehrt) {
            std::vector<Rolle> rollen;
            for (const int b : mitte) { rollen.push_back(e.knochen[static_cast<std::size_t>(b)].rolle); }
            std::sort(rollen.begin(), rollen.end(), [](Rolle x, Rolle y) { return rang(x) < rang(y); });
            std::stable_sort(mitte.begin(), mitte.end(), [&](int x, int y) { return tiefe(g, x) < tiefe(g, y); });
            for (std::size_t i = 0; i < mitte.size(); ++i) {
                KnochenErkennung& k = e.knochen[static_cast<std::size_t>(mitte[i])];
                if (k.rolle != rollen[i]) {
                    k.rolle = rollen[i];
                    k.unklar = true;
                    k.grund = "reihenfolge";
                }
            }
        }
    }
    // 8. Zuordnungen von Hand gelten zuletzt
    for (const Zuordnung& z : vonHand) {
        if (z.knochen < 0 || z.knochen >= n) { continue; }
        KnochenErkennung& k = e.knochen[static_cast<std::size_t>(z.knochen)];
        k.rolle = z.rolle;
        k.seite = z.seite;
        k.unklar = false;
        k.vonHand = true;
        k.grund.clear();
    }
    for (const KnochenErkennung& k : e.knochen) {
        e.erkannt += k.rolle != Rolle::Keine ? 1 : 0;
        e.unklar += k.unklar ? 1 : 0;
    }
    return e;
}

std::vector<int> spiegelPartner(const GlaAnimation& g, const Erkennung& e) {
    const int n = static_cast<int>(g.bones.size());
    std::vector<int> aus(static_cast<std::size_t>(n), -1);
    std::map<std::string, int> nachName;
    for (int b = 0; b < n; ++b) { nachName[g.bones[static_cast<std::size_t>(b)].name] = b; }
    for (int b = 0; b < n && static_cast<std::size_t>(b) < e.knochen.size(); ++b) {
        const KnochenErkennung& k = e.knochen[static_cast<std::size_t>(b)];
        if (k.seite == Seite::Mitte) {
            aus[static_cast<std::size_t>(b)] = b;
            continue;
        }
        const auto it = nachName.find(spiegelName(g.bones[static_cast<std::size_t>(b)].name));
        if (it != nachName.end() && e.knochen[static_cast<std::size_t>(it->second)].seite != k.seite) {
            aus[static_cast<std::size_t>(b)] = it->second;
            continue;
        }
        // ueber die gespiegelte Lage: der naechste Knochen gleicher Rolle auf der Gegenseite
        float pb[3];
        ort(g.bones[static_cast<std::size_t>(b)].basePose, pb);
        pb[e.seitenAchse] = -pb[e.seitenAchse];
        float best = 1e30F;
        for (int c = 0; c < n; ++c) {
            const KnochenErkennung& kc = e.knochen[static_cast<std::size_t>(c)];
            if (kc.rolle != k.rolle || kc.seite == k.seite || kc.seite == Seite::Mitte) { continue; }
            float pc[3];
            ort(g.bones[static_cast<std::size_t>(c)].basePose, pc);
            const float d = abstand(pb, pc);
            if (d < best) {
                best = d;
                aus[static_cast<std::size_t>(b)] = c;
            }
        }
    }
    return aus;
}

std::vector<TagErkennung> erkenneTags(const Inhalt& in, int stufe, const Erkennung& e) {
    std::vector<TagErkennung> aus;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const Teil& t = in.teile[i];
        if (!t.istTag() || t.stufen.empty()) { continue; }
        const Netz& netz = t.stufen[static_cast<std::size_t>(std::clamp(stufe, 0, static_cast<int>(t.stufen.size()) - 1))];
        TagErkennung x;
        x.teil = static_cast<int>(i);
        x.name = t.name;
        if (!netz.ecken.empty()) {
            const Ecke& ek = netz.ecken[0];
            int best = 0;
            for (int k = 1; k < 4; ++k) {
                if (ek.gewicht[k] > ek.gewicht[best]) { best = k; }
            }
            x.knochen = ek.gewicht[best] > 0.0F ? ek.knochen[best] : -1;
            for (const Ecke& q : netz.ecken) {
                for (int k = 0; k < 3; ++k) { x.ort[k] += q.ort[k] / static_cast<float>(netz.ecken.size()); }
            }
        }
        const std::string k = klein(t.name);
        if (enthaelt(k, "r_hand")) {
            x.art = TagArt::SaebelRechts;
            x.seite = Seite::Rechts;
        } else if (enthaelt(k, "l_hand")) {
            x.art = TagArt::SaebelLinks;
            x.seite = Seite::Links;
        } else if (enthaelt(k, "hip")) {
            x.art = TagArt::Huefte;
        } else if (enthaelt(k, "head")) {
            x.art = TagArt::Kopf;
        } else if (enthaelt(k, "back")) {
            x.art = TagArt::Ruecken;
        }
        // Die Hand-Tags muessen an einem Knochen der Seite haengen (Hand, Unterarm, Finger, Tag-Knochen).
        if ((x.art == TagArt::SaebelRechts || x.art == TagArt::SaebelLinks) && x.knochen >= 0 && static_cast<std::size_t>(x.knochen) < e.knochen.size()) {
            const KnochenErkennung& kb = e.knochen[static_cast<std::size_t>(x.knochen)];
            if (kb.seite != x.seite) {
                x.unklar = true;
                x.seite = kb.seite;
            }
        }
        aus.push_back(x);
    }
    return aus;
}

int Rig::griffVon(int knochen) const noexcept {
    for (std::size_t i = 0; i < griffe.size(); ++i) {
        if (griffe[i].knochen == knochen && !griffe[i].ik && griffe[i].art != GriffArt::Saebelgriff) { return static_cast<int>(i); }
    }
    return -1;
}

// Vorn ist die Richtung der Gesichtsknochen vom Kopf aus (JKA: leye y -2,85 gegen cranium +0,39 -> vorn -Y); ohne
// Gesichtsknochen -Y wie bei den Figuren der Spiele. REST M9: eine Stelle fuer Rig, IK, Retarget und die UV-Naehte.
void figurVorn(const GlaAnimation& g, const Erkennung& e, float vorn[3]) {
    vorn[0] = 0.0F;
    vorn[1] = -1.0F;
    vorn[2] = 0.0F;
    const int n = static_cast<int>(std::min(g.bones.size(), e.knochen.size()));
    int kopf = -1;
    for (int b = 0; b < n && kopf < 0; ++b) {
        if (e.knochen[static_cast<std::size_t>(b)].rolle == Rolle::Kopf && e.knochen[static_cast<std::size_t>(b)].seite == Seite::Mitte) { kopf = b; }
    }
    if (kopf < 0) { return; }
    float pk[3];
    ort(g.bones[static_cast<std::size_t>(kopf)].basePose, pk);
    float summe[3] = {0.0F, 0.0F, 0.0F};
    int z = 0;
    for (int b = 0; b < n; ++b) {
        if (e.knochen[static_cast<std::size_t>(b)].rolle != Rolle::Gesicht) { continue; }
        float pb[3];
        ort(g.bones[static_cast<std::size_t>(b)].basePose, pb);
        for (int k = 0; k < 3; ++k) { summe[k] += pb[k] - pk[k]; }
        ++z;
    }
    const int quer = (e.seitenAchse == 0) ? 1 : 0;
    if (z > 0 && std::fabs(summe[quer]) > 1e-4F) {
        vorn[0] = vorn[1] = vorn[2] = 0.0F;
        vorn[quer] = summe[quer] < 0.0F ? -1.0F : 1.0F;
    }
}

Rig rigAnlegen(const GlaAnimation& g, const Inhalt& in, int stufe, const std::vector<Zuordnung>& vonHand) {
    Rig r;
    r.erkennung = erkenneKnochen(g, vonHand, &in, stufe);
    r.partner = spiegelPartner(g, r.erkennung);
    const int n = static_cast<int>(g.bones.size());
    if (n == 0) { return r; }
    const auto& ek = r.erkennung.knochen;
    const auto rolle = [&](int b) { return ek[static_cast<std::size_t>(b)].rolle; };
    const auto seite = [&](int b) { return ek[static_cast<std::size_t>(b)].seite; };
    const auto ortVon = [&](int b, float q[3]) { ort(g.bones[static_cast<std::size_t>(b)].basePose, q); };
    // der Knochen einer Rolle und Seite in derselben Gliedmasse (gleicher Ansatz ueber die Seite): der naechste
    const auto finde = [&](Rolle ro, Seite se, int naheVon) {
        int best = -1;
        float bd = 1e30F;
        float pn[3] = {0.0F, 0.0F, 0.0F};
        if (naheVon >= 0) { ortVon(naheVon, pn); }
        for (int b = 0; b < n; ++b) {
            if (rolle(b) != ro || seite(b) != se) { continue; }
            float pb[3];
            ortVon(b, pb);
            const float d = naheVon >= 0 ? abstand(pb, pn) : 0.0F;
            if (d < bd) {
                bd = d;
                best = b;
            }
        }
        return best;
    };
    static const std::pair<Rolle, GriffArt> kFk[] = {
        {Rolle::Becken, GriffArt::Becken},   {Rolle::Wirbel, GriffArt::Wirbel},         {Rolle::Hals, GriffArt::Hals},
        {Rolle::Kopf, GriffArt::Kopf},       {Rolle::Schluessel, GriffArt::Schluessel}, {Rolle::Oberarm, GriffArt::Oberarm},
        {Rolle::Unterarm, GriffArt::Unterarm}, {Rolle::Hand, GriffArt::Hand},           {Rolle::Oberschenkel, GriffArt::Oberschenkel},
        {Rolle::Unterschenkel, GriffArt::Unterschenkel}, {Rolle::Fuss, GriffArt::Fuss}, {Rolle::Zehen, GriffArt::Zehen}};
    std::map<std::pair<int, int>, int> zaehler;
    for (const auto& [ro, art] : kFk) {
        std::vector<int> liste;
        for (int b = 0; b < n; ++b) {
            if (rolle(b) == ro) { liste.push_back(b); }
        }
        std::stable_sort(liste.begin(), liste.end(), [&](int x, int y) { return tiefe(g, x) < tiefe(g, y); });
        for (const int b : liste) {
            Griff gr;
            gr.art = art;
            gr.seite = seite(b);
            gr.knochen = b;
            gr.nummer = ++zaehler[{static_cast<int>(art), static_cast<int>(gr.seite)}];
            ortVon(b, gr.ort);
            // Mitnehmen: Knochen derselben Gliedmasse mit hoeherem Rang, die nicht unter diesem Knochen haengen (JKA: Finger
            // unter rradius; JK2: alle Glieder flach unter thoracic/pelvis).
            const bool arm = armRolle(ro);
            const bool bein = beinRolle(ro);
            if (arm || bein) {
                float pb[3];
                ortVon(b, pb);
                for (int c = 0; c < n; ++c) {
                    if (c == b || seite(c) != gr.seite) { continue; }
                    const Rolle rc = rolle(c);
                    const bool gleich = arm ? (armRolle(rc) || rc == Rolle::TagKnochen) : beinRolle(rc);
                    if (!gleich || rang(rc) <= rang(ro) || istNachfahre(g, c, b)) { continue; }
                    // dieselbe Gliedmasse: c liegt weiter vom Ansatz als b und naeher an b als an jedem anderen Knochen
                    // gleicher Rolle wie b (Kreaturen mit mehreren Beinen je Seite)
                    float pc[3];
                    ortVon(c, pc);
                    const int naechster = finde(ro, gr.seite, c);
                    if (naechster != b) { continue; }
                    // ein Knochen, der schon unter einem mitgenommenen haengt, kommt ueber die Hierarchie mit
                    gr.mit.push_back(c);
                }
                std::vector<int> ohne;
                for (const int c : gr.mit) {
                    const bool darunter = std::any_of(gr.mit.begin(), gr.mit.end(), [&](int d) { return d != c && istNachfahre(g, c, d); });
                    if (!darunter) { ohne.push_back(c); }
                }
                gr.mit = ohne;
            }
            // Drehknochen des Segments davor: Hand <- UnterarmTwist (Unterarm -> Hand), Unterarm <- OberarmTwist, Unterschenkel <-
            // OberschenkelTwist. Anteil = Lage des Drehknochens auf der Strecke in der Grundstellung.
            Rolle tw = Rolle::Keine;
            Rolle von = Rolle::Keine;
            if (ro == Rolle::Hand) { tw = Rolle::UnterarmTwist, von = Rolle::Unterarm; }
            if (ro == Rolle::Unterarm) { tw = Rolle::OberarmTwist, von = Rolle::Oberarm; }
            if (ro == Rolle::Unterschenkel) { tw = Rolle::OberschenkelTwist, von = Rolle::Oberschenkel; }
            if (tw != Rolle::Keine) {
                const int x = finde(tw, gr.seite, b);
                const int s = finde(von, gr.seite, b);
                if (x >= 0 && s >= 0) {
                    float ps[3];
                    float px[3];
                    float pe[3];
                    ortVon(s, ps);
                    ortVon(x, px);
                    ortVon(b, pe);
                    const float l = abstand(ps, pe);
                    if (l > 1e-4F) {
                        gr.twist = x;
                        gr.twistVon = s;
                        gr.twistAnteil = std::clamp(abstand(ps, px) / l, 0.0F, 1.0F);
                    }
                }
            }
            r.griffe.push_back(gr);
        }
    }
    float vorn[3];
    figurVorn(g, r.erkennung, vorn);
    // Pol-Ziele (fuer IK in Stufe 4): Ellbogen nach hinten, Knie nach vorn, eine Oberarm- bzw. Oberschenkellaenge weit.
    for (const Seite se : {Seite::Links, Seite::Rechts}) {
        for (const bool bein : {false, true}) {
            const int o = finde(bein ? Rolle::Oberschenkel : Rolle::Oberarm, se, -1);
            const int u = o >= 0 ? finde(bein ? Rolle::Unterschenkel : Rolle::Unterarm, se, o) : -1;
            if (o < 0 || u < 0) { continue; }
            Griff gr;
            gr.art = bein ? GriffArt::PolKnie : GriffArt::PolEllbogen;
            gr.seite = se;
            gr.knochen = u;
            gr.ik = true;
            float po[3];
            float pu[3];
            ortVon(o, po);
            ortVon(u, pu);
            const float l = abstand(po, pu);
            for (int k = 0; k < 3; ++k) { gr.ort[k] = pu[k] + (bein ? vorn[k] : -vorn[k]) * l; }
            r.griffe.push_back(gr);
        }
    }
    // Fussrolle: Ferse (hinterste) und Zehe (vorderste Ecke mit dem groessten Gewicht am Fuss), Ballen am Zehenknochen.
    if (!in.teile.empty()) {
        for (const Seite se : {Seite::Links, Seite::Rechts}) {
            const int fuss = finde(Rolle::Fuss, se, -1);
            if (fuss < 0) { continue; }
            bool da = false;
            float hinten[3]{};
            float vorne[3]{};
            float hv = 0.0F;
            float vv = 0.0F;
            for (const Teil& t : in.teile) {
                if (t.istTag() || t.stufen.empty()) { continue; }
                const Netz& netz = t.stufen[static_cast<std::size_t>(std::clamp(stufe, 0, static_cast<int>(t.stufen.size()) - 1))];
                for (const Ecke& q : netz.ecken) {
                    int best = 0;
                    for (int k = 1; k < 4; ++k) {
                        if (q.gewicht[k] > q.gewicht[best]) { best = k; }
                    }
                    if (q.knochen[best] != fuss || q.gewicht[best] <= 0.0F) { continue; }
                    const float v = q.ort[0] * vorn[0] + q.ort[1] * vorn[1] + q.ort[2] * vorn[2];
                    if (!da || v < hv) {
                        hv = v;
                        std::memcpy(hinten, q.ort, sizeof hinten);
                    }
                    if (!da || v > vv) {
                        vv = v;
                        std::memcpy(vorne, q.ort, sizeof vorne);
                    }
                    da = true;
                }
            }
            if (da) {
                Griff f;
                f.art = GriffArt::Ferse;
                f.seite = se;
                f.knochen = fuss;
                f.ik = true;
                std::memcpy(f.ort, hinten, sizeof f.ort);
                r.griffe.push_back(f);
                Griff z = f;
                z.art = GriffArt::Zehe;
                std::memcpy(z.ort, vorne, sizeof z.ort);
                r.griffe.push_back(z);
            }
            const int zeh = finde(Rolle::Zehen, se, fuss);
            if (zeh >= 0) {
                Griff b;
                b.art = GriffArt::Ballen;
                b.seite = se;
                b.knochen = fuss;
                b.ik = true;
                ortVon(zeh, b.ort);
                r.griffe.push_back(b);
            }
        }
    }
    // Tags und Saebelgriff (*r_hand / *l_hand): dreht die Hand, gezeigt am Tag.
    r.tags = erkenneTags(in, stufe, r.erkennung);
    for (const TagErkennung& t : r.tags) {
        if (t.art != TagArt::SaebelRechts && t.art != TagArt::SaebelLinks) { continue; }
        const int hand = finde(Rolle::Hand, t.seite, t.knochen);
        const int gi = hand >= 0 ? r.griffVon(hand) : -1;
        if (gi < 0) { continue; }
        Griff s = r.griffe[static_cast<std::size_t>(gi)];
        s.art = GriffArt::Saebelgriff;
        s.tag = t.name;
        std::memcpy(s.ort, t.ort, sizeof s.ort);
        r.griffe.push_back(s);
    }
    return r;
}

// --- Matrizen ----------------------------------------------------------------------------------------------------------

BoneMatrix starrInvers(const BoneMatrix& a) noexcept {
    // allgemeine Umkehr der 3x3 (die Matrizen der GLA sind nach MC_UnCompressQuat nicht ganz orthonormal)
    const float(&m)[3][4] = a.m;
    const float c00 = m[1][1] * m[2][2] - m[1][2] * m[2][1];
    const float c01 = m[1][2] * m[2][0] - m[1][0] * m[2][2];
    const float c02 = m[1][0] * m[2][1] - m[1][1] * m[2][0];
    const float det = m[0][0] * c00 + m[0][1] * c01 + m[0][2] * c02;
    BoneMatrix r = BoneMatrix::identity();
    if (std::fabs(det) < 1e-12F) { return r; }
    const float id = 1.0F / det;
    r.m[0][0] = c00 * id;
    r.m[0][1] = (m[0][2] * m[2][1] - m[0][1] * m[2][2]) * id;
    r.m[0][2] = (m[0][1] * m[1][2] - m[0][2] * m[1][1]) * id;
    r.m[1][0] = c01 * id;
    r.m[1][1] = (m[0][0] * m[2][2] - m[0][2] * m[2][0]) * id;
    r.m[1][2] = (m[0][2] * m[1][0] - m[0][0] * m[1][2]) * id;
    r.m[2][0] = c02 * id;
    r.m[2][1] = (m[0][1] * m[2][0] - m[0][0] * m[2][1]) * id;
    r.m[2][2] = (m[0][0] * m[1][1] - m[0][1] * m[1][0]) * id;
    for (int i = 0; i < 3; ++i) { r.m[i][3] = -(r.m[i][0] * m[0][3] + r.m[i][1] * m[1][3] + r.m[i][2] * m[2][3]); }
    return r;
}

BoneMatrix drehungUm(const float achse[3], float grad) noexcept {
    const float l = std::sqrt(achse[0] * achse[0] + achse[1] * achse[1] + achse[2] * achse[2]);
    BoneMatrix r = BoneMatrix::identity();
    if (l < 1e-9F) { return r; }
    const float x = achse[0] / l;
    const float y = achse[1] / l;
    const float z = achse[2] / l;
    const float w = grad * kPi / 180.0F;
    const float c = std::cos(w);
    const float s = std::sin(w);
    const float t = 1.0F - c;
    r.m[0][0] = t * x * x + c;
    r.m[0][1] = t * x * y - s * z;
    r.m[0][2] = t * x * z + s * y;
    r.m[1][0] = t * x * y + s * z;
    r.m[1][1] = t * y * y + c;
    r.m[1][2] = t * y * z - s * x;
    r.m[2][0] = t * x * z - s * y;
    r.m[2][1] = t * y * z + s * x;
    r.m[2][2] = t * z * z + c;
    return r;
}

void quatAusMatrix(const BoneMatrix& m, float q[4]) noexcept {
    g2::Mat3x4 x;
    std::memcpy(x.m, m.m, sizeof x.m);
    const g2::Quat r = g2::normalize(g2::matrixToQuat(x));
    q[0] = r.w;
    q[1] = r.x;
    q[2] = r.y;
    q[3] = r.z;
}

BoneMatrix matrixAusQuat(const float q[4]) noexcept {
    g2::Quat r;
    r.w = q[0];
    r.x = q[1];
    r.y = q[2];
    r.z = q[3];
    g2::Mat3x4 x;
    g2::quatToMatrix(g2::normalize(r), x);
    BoneMatrix m;
    std::memcpy(m.m, x.m, sizeof m.m);
    m.m[0][3] = m.m[1][3] = m.m[2][3] = 0.0F;
    return m;
}

// REST N14: ein Schluessel als Matrix (Drehung aus dem Quaternion, Ort aus t) - einmal fuer Rig und IK
BoneMatrix schluesselMatrix(const RigSchluessel& k) {
    BoneMatrix m = matrixAusQuat(k.q);
    m.m[0][3] = k.t[0];
    m.m[1][3] = k.t[1];
    m.m[2][3] = k.t[2];
    return m;
}

namespace {

void slerp(const float a0[4], const float b0[4], float t, float out[4]) {
    // Stufe 3b: die Schieber aendern die Kanaele einzeln (wie Blender), q ist dann nicht mehr ganz Einheit - normiert
    float a[4] = {a0[0], a0[1], a0[2], a0[3]};
    float b[4] = {b0[0], b0[1], b0[2], b0[3]};
    for (float* x : {a, b}) {
        const double l = std::sqrt(static_cast<double>(x[0]) * x[0] + static_cast<double>(x[1]) * x[1] + static_cast<double>(x[2]) * x[2] +
                                   static_cast<double>(x[3]) * x[3]);
        for (int k = 0; k < 4 && l > 0.0; ++k) { x[k] = static_cast<float>(x[k] / l); }
    }
    double d = 0.0;
    for (int k = 0; k < 4; ++k) { d += static_cast<double>(a[k]) * static_cast<double>(b[k]); }
    if (d < 0.0) {
        for (float& v : b) { v = -v; }
        d = -d;
    }
    double wa = 1.0 - t;
    double wb = t;
    if (d < 0.9995) {
        const double th = std::acos(std::min(d, 1.0));
        const double s = std::sin(th);
        wa = std::sin((1.0 - t) * th) / s;
        wb = std::sin(static_cast<double>(t) * th) / s;
    }
    double l = 0.0;
    for (int k = 0; k < 4; ++k) {
        out[k] = static_cast<float>(wa * static_cast<double>(a[k]) + wb * static_cast<double>(b[k]));
        l += static_cast<double>(out[k]) * static_cast<double>(out[k]);
    }
    l = std::sqrt(l);
    for (int k = 0; k < 4 && l > 0.0; ++k) { out[k] = static_cast<float>(static_cast<double>(out[k]) / l); }
}

}  // namespace

bool RigSchicht::leer() const noexcept {
    return std::all_of(knochen.begin(), knochen.end(), [](const std::vector<RigSchluessel>& v) { return v.empty(); });
}

int RigSchicht::schluesselZahl() const noexcept {
    int z = 0;
    for (const auto& v : knochen) { z += static_cast<int>(v.size()); }
    return z;
}

bool RigSchicht::hatSchluessel(int k, int bild) const noexcept {
    if (const int sp = ikWahlSpur(k); sp >= 0) {
        if (static_cast<std::size_t>(sp) >= ik.size()) { return false; }
        const auto& z = ik[static_cast<std::size_t>(sp)].ziel;
        return std::any_of(z.begin(), z.end(), [&](const RigSchluessel& x) { return x.bild == bild; });
    }
    if (k < 0 || static_cast<std::size_t>(k) >= knochen.size()) { return false; }
    const auto& v = knochen[static_cast<std::size_t>(k)];
    return std::any_of(v.begin(), v.end(), [&](const RigSchluessel& s) { return s.bild == bild; });
}

bool rigWert(const RigSchicht& s, int k, float bild, BoneMatrix& m) {
    if (k < 0 || static_cast<std::size_t>(k) >= s.knochen.size()) { return false; }
    return schluesselWert(s.knochen[static_cast<std::size_t>(k)], bild, m);
}

bool schluesselWert(const std::vector<RigSchluessel>& v, float bild, BoneMatrix& m) {
    if (v.empty()) { return false; }
    if (bild <= static_cast<float>(v.front().bild)) {
        m = schluesselMatrix(v.front());
        return true;
    }
    if (bild >= static_cast<float>(v.back().bild)) {
        m = schluesselMatrix(v.back());
        return true;
    }
    for (std::size_t i = 0; i + 1 < v.size(); ++i) {
        if (bild >= static_cast<float>(v[i].bild) && bild <= static_cast<float>(v[i + 1].bild)) {
            if (bild == static_cast<float>(v[i + 1].bild)) {
                m = schluesselMatrix(v[i + 1]);
                return true;
            }
            RigSchluessel z;
            // Stufe 3b: der linke Schluessel bestimmt den Abschnitt (wie BezTriple::ipo, fcurve_eval_keyframes_interpolate)
            if (v[i].interp == Interp::Konstant) {
                z = v[i];
            } else if (v[i].interp == Interp::Bezier) {
                // je Kanal eine F-Kurve wie Blender, das Quaternion danach normiert (BKE_pchan_to_mat4: normalize_qt_qt)
                for (int c = 0; c < kKanaele; ++c) { kanalSetzen(z, c, kanalAuswerten(v, c, bild)); }
            } else {
                const float t = (bild - static_cast<float>(v[i].bild)) / static_cast<float>(std::max(v[i + 1].bild - v[i].bild, 1));
                slerp(v[i].q, v[i + 1].q, t, z.q);
                for (int a = 0; a < 3; ++a) { z.t[a] = v[i].t[a] + (v[i + 1].t[a] - v[i].t[a]) * t; }
            }
            m = schluesselMatrix(z);
            return true;
        }
    }
    m = schluesselMatrix(v.back());
    return true;
}

void rigNachMal(const RigSchicht& s, float bild, std::vector<std::pair<int, BoneMatrix>>& aus) {
    aus.clear();
    for (std::size_t k = 0; k < s.knochen.size(); ++k) {
        BoneMatrix m;
        if (rigWert(s, static_cast<int>(k), bild, m)) { aus.emplace_back(static_cast<int>(k), m); }
    }
}

void rigSchluesselSetzen(RigSchicht& s, int k, int bild, const BoneMatrix& m) {
    if (k < 0) { return; }
    if (s.knochen.size() <= static_cast<std::size_t>(k)) { s.knochen.resize(static_cast<std::size_t>(k) + 1U); }
    auto& v = s.knochen[static_cast<std::size_t>(k)];
    RigSchluessel neu;
    neu.bild = bild;
    neu.interp = s.neuInterp;
    quatAusMatrix(m, neu.q);
    neu.t[0] = m.m[0][3];
    neu.t[1] = m.m[1][3];
    neu.t[2] = m.m[2][3];
    const auto it = std::lower_bound(v.begin(), v.end(), bild, [](const RigSchluessel& a, int b) { return a.bild < b; });
    // Stufe 3b: q in der Halbkugel des Nachbarn (die Kanaele liefen sonst ueber 0; q und -q drehen gleich)
    const RigSchluessel* nachbar = nullptr;
    if (it != v.begin()) {
        nachbar = &*(it - 1);
    } else if (it != v.end() && it->bild != bild) {
        nachbar = &*it;
    } else if (it != v.end() && it + 1 != v.end()) {
        nachbar = &*(it + 1);
    }
    if (nachbar != nullptr && neu.q[0] * nachbar->q[0] + neu.q[1] * nachbar->q[1] + neu.q[2] * nachbar->q[2] + neu.q[3] * nachbar->q[3] < 0.0F) {
        for (float& x : neu.q) { x = -x; }
    }
    if (it != v.end() && it->bild == bild) {
        // ersetzt: Interpolation und Henkel bleiben (die Henkel sind relativ, sie gehen mit dem Wert)
        std::memcpy(it->q, neu.q, sizeof neu.q);
        std::memcpy(it->t, neu.t, sizeof neu.t);
    } else {
        v.insert(it, neu);
    }
    henkelNeu(v);
}

void rigSchluesselLoeschen(RigSchicht& s, int k, int bild) {
    if (const int sp = ikWahlSpur(k); sp >= 0) {
        if (static_cast<std::size_t>(sp) < s.ik.size()) { ikSchluesselLoeschen(s.ik[static_cast<std::size_t>(sp)], bild); }
        return;
    }
    if (k < 0 || static_cast<std::size_t>(k) >= s.knochen.size()) { return; }
    auto& v = s.knochen[static_cast<std::size_t>(k)];
    v.erase(std::remove_if(v.begin(), v.end(), [&](const RigSchluessel& x) { return x.bild == bild; }), v.end());
    henkelNeu(v);
}

void rigNachMalAuf(const GlaAnimation& g, const RigSchicht& s, int bildA, int bildB, float anteil, std::vector<std::pair<int, BoneMatrix>>& aus,
                   const std::vector<BoneFrameOverride>& ueber) {
    rigNachMalFk(g, s, bildA, bildB, anteil, aus);
    const float bild = static_cast<float>(bildA) + (bildB != bildA ? anteil : 0.0F);
    if (!s.ik.empty()) {
        // Stufe 4: die IK auf der FK-Welt dieses Bilds
        std::vector<BoneMatrix> welt;
        g.worldMatricesLerp(bildA, bildB, anteil, ueber, aus, welt);
        (void)ikAnwenden(g, s, bild, welt, aus);
    }
    // Stufe 6: die Physik-Spur als Filter danach (X'' = X' P)
    physikAuf(s, bild, aus);
}

void rigNachMalFk(const GlaAnimation& g, const RigSchicht& s, int bildA, int bildB, float anteil, std::vector<std::pair<int, BoneMatrix>>& aus) {
    rigNachMal(s, static_cast<float>(bildA) + (bildB != bildA ? anteil : 0.0F), aus);
    if (!s.bindeBasis) { return; }
    // Bindebasis: X = X(p) B(p)^-1 B M soll gelten; worldMatricesLerp rechnet X(p) B(p)^-1 A B M_eff, also
    // M_eff = B^-1 A^-1 B M mit A wie dort (localMatrixLerp)
    for (auto& [k, m] : aus) {
        if (k < 0 || static_cast<std::size_t>(k) >= g.bones.size()) { continue; }
        const GlaBone& kb = g.bones[static_cast<std::size_t>(k)];
        const BoneMatrix a = g.localMatrixLerp(bildA, bildB, bildB != bildA ? anteil : 0.0F, k);
        m = kb.basePoseInv * starrInvers(a) * kb.basePose * m;
    }
}

void rigWelt(const GlaAnimation& g, const RigSchicht& s, int bildA, int bildB, float anteil, std::vector<BoneMatrix>& welt) {
    std::vector<std::pair<int, BoneMatrix>> nach;
    rigNachMalAuf(g, s, bildA, bildB, anteil, nach);
    g.worldMatricesLerp(bildA, bildB, anteil, {}, nach, welt);
}

namespace {

// M eines Knochens am Bild (Einheit ohne Schluessel; mit Bindebasis ohne Schluessel B^-1 A B - dieselbe Lage wie die GLA)
BoneMatrix wertOder(const GlaAnimation& g, const RigSchicht& s, int k, int bild) {
    BoneMatrix m = BoneMatrix::identity();
    if (rigWert(s, k, static_cast<float>(bild), m)) { return m; }
    if (s.bindeBasis && k >= 0 && static_cast<std::size_t>(k) < g.bones.size()) {
        const GlaBone& kb = g.bones[static_cast<std::size_t>(k)];
        return kb.basePoseInv * g.localMatrix(bild, k) * kb.basePose;
    }
    return m;
}

// X'(b) := T(P) R T(-P) X'(b): M_neu = M_alt * X'(b)^-1 T(P) R T(-P) X'(b)
void weltDrehungAuf(const GlaAnimation& g, RigSchicht& s, int k, int bild, const std::vector<BoneMatrix>& welt, const BoneMatrix& um) {
    if (k < 0 || static_cast<std::size_t>(k) >= welt.size()) { return; }
    const BoneMatrix& x = welt[static_cast<std::size_t>(k)];
    const BoneMatrix zusatz = starrInvers(x) * um * x;
    rigSchluesselSetzen(s, k, bild, wertOder(g, s, k, bild) * zusatz);
}

BoneMatrix umPunkt(const BoneMatrix& r, const float p[3]) {
    const float minus[3] = {-p[0], -p[1], -p[2]};
    return verschiebung(p) * r * verschiebung(minus);
}

}  // namespace

void rigGriffDrehen(const GlaAnimation& g, const Rig& r, int griff, int bild, const std::vector<BoneMatrix>& welt, const BoneMatrix& drehung,
                    const float weg[3], RigSchicht& s) {
    if (griff < 0 || static_cast<std::size_t>(griff) >= r.griffe.size()) { return; }
    const Griff& gr = r.griffe[static_cast<std::size_t>(griff)];
    if (gr.ik || gr.knochen < 0 || static_cast<std::size_t>(gr.knochen) >= welt.size()) { return; }
    float p[3];
    ort(welt[static_cast<std::size_t>(gr.knochen)], p);
    BoneMatrix um = umPunkt(drehung, p);
    if (weg != nullptr && (weg[0] != 0.0F || weg[1] != 0.0F || weg[2] != 0.0F)) { um = verschiebung(weg) * um; }
    weltDrehungAuf(g, s, gr.knochen, bild, welt, um);
    for (const int c : gr.mit) { weltDrehungAuf(g, s, c, bild, welt, um); }
    // Drehknochen: Anteil der Drehung um die Achse des Segments (twistVon -> knochen), am Drehknochen
    if (gr.twist >= 0 && gr.twistVon >= 0 && static_cast<std::size_t>(gr.twist) < welt.size() && static_cast<std::size_t>(gr.twistVon) < welt.size()) {
        float a[3];
        float e[3];
        ort(welt[static_cast<std::size_t>(gr.twistVon)], a);
        ort(welt[static_cast<std::size_t>(gr.knochen)], e);
        float achse[3] = {e[0] - a[0], e[1] - a[1], e[2] - a[2]};
        const float l = std::sqrt(achse[0] * achse[0] + achse[1] * achse[1] + achse[2] * achse[2]);
        if (l > 1e-5F) {
            for (float& v : achse) { v /= l; }
            float q[4];
            quatAusMatrix(drehung, q);
            const float proj = q[1] * achse[0] + q[2] * achse[1] + q[3] * achse[2];
            const float grad = 2.0F * std::atan2(proj, q[0]) * 180.0F / kPi;   // Twist-Anteil (Swing-Twist-Zerlegung)
            float px[3];
            ort(welt[static_cast<std::size_t>(gr.twist)], px);
            weltDrehungAuf(g, s, gr.twist, bild, welt, umPunkt(drehungUm(achse, grad * gr.twistAnteil), px));
        }
    }
}

void rigKnochenVerschieben(const GlaAnimation& g, RigSchicht& s, int knochen, int bild, const std::vector<BoneMatrix>& welt, const float weg[3]) {
    weltDrehungAuf(g, s, knochen, bild, welt, verschiebung(weg));
}

// --- Posen -------------------------------------------------------------------------------------------------------------

RigPose poseAus(const GlaAnimation& g, const std::vector<BoneMatrix>& welt) {
    RigPose p;
    for (std::size_t b = 0; b < g.bones.size() && b < welt.size(); ++b) {
        p.namen.push_back(g.bones[b].name);
        p.welt.push_back(welt[b]);
    }
    return p;
}

namespace {

// L(b) zum Bild ohne Schicht: B(p)^-1 A(b) B(b), an der Wurzel A(b) B(b). Mit Bindebasis ist A die Einheit (die
// Schluessel beschreiben den Knochen ganz).
BoneMatrix relativ(const GlaAnimation& g, const RigSchicht& s, int b, int bild) {
    const GlaBone& kb = g.bones[static_cast<std::size_t>(b)];
    const BoneMatrix a = s.bindeBasis ? BoneMatrix::identity() : g.localMatrix(bild, b);
    if (kb.parent < 0 || static_cast<std::size_t>(kb.parent) >= g.bones.size()) { return a * kb.basePose; }
    return g.bones[static_cast<std::size_t>(kb.parent)].basePoseInv * a * kb.basePose;
}

BoneMatrix spiegel(int achse) {
    BoneMatrix s = BoneMatrix::identity();
    s.m[achse][achse] = -1.0F;
    return s;
}

}  // namespace

int poseEinfuegen(const GlaAnimation& g, const Rig& r, const RigPose& p, bool gespiegelt, int bild, const std::vector<int>& nurKnochen,
                  RigSchicht& s) {
    const int n = static_cast<int>(g.bones.size());
    std::map<std::string, int> inPose;
    for (std::size_t i = 0; i < p.namen.size(); ++i) { inPose[p.namen[i]] = static_cast<int>(i); }
    // die Zielwelt je Knochen
    std::vector<BoneMatrix> ziel(static_cast<std::size_t>(n), BoneMatrix::identity());
    std::vector<char> da(static_cast<std::size_t>(n), 0);
    const BoneMatrix S = spiegel(r.erkennung.seitenAchse);
    for (int b = 0; b < n; ++b) {
        int quelle = b;
        if (gespiegelt) {
            quelle = static_cast<std::size_t>(b) < r.partner.size() ? r.partner[static_cast<std::size_t>(b)] : -1;
            if (quelle < 0) { quelle = b; }
        }
        const auto it = inPose.find(g.bones[static_cast<std::size_t>(quelle)].name);
        if (it == inPose.end()) { continue; }
        const BoneMatrix& w = p.welt[static_cast<std::size_t>(it->second)];
        if (gespiegelt) {
            // D'(b) = S D(q) S, D(q) = W(q) B(q)^-1; W'(b) = D'(b) B(b)
            const BoneMatrix d = w * g.bones[static_cast<std::size_t>(quelle)].basePoseInv;
            ziel[static_cast<std::size_t>(b)] = S * d * S * g.bones[static_cast<std::size_t>(b)].basePose;
        } else {
            ziel[static_cast<std::size_t>(b)] = w;
        }
        da[static_cast<std::size_t>(b)] = 1;
    }
    int gesetzt = 0;
    for (int b = 0; b < n; ++b) {
        if (da[static_cast<std::size_t>(b)] == 0) { continue; }
        if (!nurKnochen.empty() && std::find(nurKnochen.begin(), nurKnochen.end(), b) == nurKnochen.end()) { continue; }
        const int par = g.bones[static_cast<std::size_t>(b)].parent;
        BoneMatrix lStern;
        if (par < 0 || par >= n) {
            lStern = ziel[static_cast<std::size_t>(b)];
        } else {
            if (da[static_cast<std::size_t>(par)] == 0) { continue; }
            lStern = starrInvers(ziel[static_cast<std::size_t>(par)]) * ziel[static_cast<std::size_t>(b)];
        }
        rigSchluesselSetzen(s, b, bild, starrInvers(relativ(g, s, b, bild)) * lStern);
        ++gesetzt;
    }
    return gesetzt;
}

int bindePose(const GlaAnimation& g, int bild, const std::vector<int>& nurKnochen, RigSchicht& s) {
    const int n = static_cast<int>(g.bones.size());
    int gesetzt = 0;
    for (int b = 0; b < n; ++b) {
        if (!nurKnochen.empty() && std::find(nurKnochen.begin(), nurKnochen.end(), b) == nurKnochen.end()) { continue; }
        const GlaBone& kb = g.bones[static_cast<std::size_t>(b)];
        const BoneMatrix lBinde = (kb.parent < 0 || kb.parent >= n) ? kb.basePose : g.bones[static_cast<std::size_t>(kb.parent)].basePoseInv * kb.basePose;
        rigSchluesselSetzen(s, b, bild, starrInvers(relativ(g, s, b, bild)) * lBinde);
        ++gesetzt;
    }
    return gesetzt;
}

int handPoseSetzen(const GlaAnimation& g, const Rig& r, Seite seite, int quellBild, int bild, RigSchicht& s) {
    const int n = static_cast<int>(g.bones.size());
    int hand = -1;
    for (int b = 0; b < n; ++b) {
        if (r.erkennung.knochen[static_cast<std::size_t>(b)].rolle == Rolle::Hand && r.erkennung.knochen[static_cast<std::size_t>(b)].seite == seite) {
            hand = b;
            break;
        }
    }
    if (hand < 0) { return 0; }
    std::vector<BoneMatrix> quelle;
    if (quellBild >= 0) { g.worldMatrices(std::clamp(quellBild, 0, g.bildZahl() - 1), quelle); }
    std::vector<BoneMatrix> jetzt;
    RigSchicht ohnePhysik = s;   // Stufe 6: die Schluessel bekommen nur die Pose der Schicht (die Physik kaeme sonst doppelt)
    ohnePhysik.physik.e.an = false;
    rigWelt(g, ohnePhysik, bild, bild, 0.0F, jetzt);
    const BoneMatrix qHandInv = quellBild >= 0 ? starrInvers(quelle[static_cast<std::size_t>(hand)]) : g.bones[static_cast<std::size_t>(hand)].basePoseInv;
    std::vector<BoneMatrix> ziel = jetzt;
    std::vector<int> finger;
    for (int b = 0; b < n; ++b) {
        const KnochenErkennung& k = r.erkennung.knochen[static_cast<std::size_t>(b)];
        if (k.rolle != Rolle::Finger || k.seite != seite) { continue; }
        const BoneMatrix rel = qHandInv * (quellBild >= 0 ? quelle[static_cast<std::size_t>(b)] : g.bones[static_cast<std::size_t>(b)].basePose);
        ziel[static_cast<std::size_t>(b)] = jetzt[static_cast<std::size_t>(hand)] * rel;
        finger.push_back(b);
    }
    for (const int b : finger) {
        const int par = g.bones[static_cast<std::size_t>(b)].parent;
        const BoneMatrix lStern = (par < 0 || par >= n) ? ziel[static_cast<std::size_t>(b)]
                                                        : starrInvers(ziel[static_cast<std::size_t>(par)]) * ziel[static_cast<std::size_t>(b)];
        rigSchluesselSetzen(s, b, bild, starrInvers(relativ(g, s, b, bild)) * lStern);
    }
    return static_cast<int>(finger.size());
}

// --- Backen ------------------------------------------------------------------------------------------------------------

bool rigBacken(const GlaAnimation& q, const RigSchicht& s, int erstes, int anzahl, bool alleNeu, const std::string& kopfName, GlaAnimation& aus,
               BackBericht* bericht) {
    BackBericht leer;
    BackBericht& br = bericht != nullptr ? *bericht : leer;
    br = BackBericht{};
    aus = GlaAnimation{};
    const int nb = static_cast<int>(q.bones.size());
    if (nb == 0 || anzahl <= 0 || erstes < 0 || erstes + anzahl > q.bildZahl()) {
        br.fehler = fehlerText(ModellFehler{FehlerArt::BereichAusserhalb});   // REST N6
        return false;
    }
    aus.name = kopfName;
    aus.skeletonName = kopfName;
    aus.scale = q.scale;
    aus.bones = q.bones;
    aus.numFrames = anzahl;
    GlaVorrat vorrat(aus.bonePool);   // REST N5
    g2::CompressStats st;
    const g2::CompressOptions co{};   // wie xsiAufnehmen (G2C-Vorgabe: Nearest, Vorzeichen, optimizeQuat)
    std::vector<std::pair<int, BoneMatrix>> nach;
    std::vector<const BoneMatrix*> mJe(static_cast<std::size_t>(nb), nullptr);
    std::vector<std::pair<int, BoneMatrix>> eff;
    std::vector<BoneMatrix> welt;
    for (int f = erstes; f < erstes + anzahl; ++f) {
        rigNachMal(s, static_cast<float>(f), nach);
        if (!s.ik.empty()) {
            // Stufe 4: die Knochen, die die IK aendert, bekommen ihr M aus der IK (M = B^-1 A B M_eff mit Bindebasis);
            // die anderen bleiben die Schluessel der Schicht Bit fuer Bit
            rigNachMalFk(q, s, f, f, 0.0F, eff);
            q.worldMatricesLerp(f, f, 0.0F, {}, eff, welt);
            for (const int b : ikAnwenden(q, s, static_cast<float>(f), welt, eff)) {
                BoneMatrix m = BoneMatrix::identity();
                for (const auto& [k, mm] : eff) {
                    if (k == b) { m = mm; }
                }
                if (s.bindeBasis) {
                    const GlaBone& kb = q.bones[static_cast<std::size_t>(b)];
                    m = kb.basePoseInv * q.localMatrix(f, b) * kb.basePose * m;
                }
                bool da = false;
                for (auto& [k, mm] : nach) {
                    if (k == b) {
                        mm = m;
                        da = true;
                    }
                }
                if (!da) { nach.emplace_back(b, m); }
            }
        }
        if (physikWirkt(s)) {
            // Stufe 6: die Physik-Spur auf die M der Schicht (M P); ein Knochen ohne Schluessel bekommt P auf seiner Lage
            // (Bindebasis: B^-1 A B wie wertOder)
            std::vector<std::pair<int, BoneMatrix>> pk;
            physikAuf(s, static_cast<float>(f), pk);
            for (const auto& [b, p] : pk) {
                bool da = false;
                for (auto& [k, mm] : nach) {
                    if (k == b) {
                        mm = mm * p;
                        da = true;
                    }
                }
                if (!da && b >= 0 && b < nb) {
                    const GlaBone& kb = q.bones[static_cast<std::size_t>(b)];
                    nach.emplace_back(b, (s.bindeBasis ? kb.basePoseInv * q.localMatrix(f, b) * kb.basePose : BoneMatrix::identity()) * p);
                }
            }
        }
        std::fill(mJe.begin(), mJe.end(), nullptr);
        for (const auto& [k, m] : nach) {
            if (k >= 0 && k < nb) { mJe[static_cast<std::size_t>(k)] = &m; }
        }
        // die GLA, in der das Bild steht (Anhang: Karten-GLA hinter _humanoid)
        const GlaAnimation* g = &q;
        int fl = f;
        while (fl >= g->numFrames && g->anhang != nullptr) {
            fl -= g->numFrames;
            g = g->anhang;
        }
        for (int b = 0; b < nb; ++b) {
            std::uint8_t c[14];
            if (mJe[static_cast<std::size_t>(b)] == nullptr && !alleNeu) {
                const std::size_t at = static_cast<std::size_t>(fl) * static_cast<std::size_t>(nb) + static_cast<std::size_t>(b);
                const std::size_t off = at < g->indexes.size() ? static_cast<std::size_t>(g->indexes[at]) * 14U : g->bonePool.size();
                if (off + 14U > g->bonePool.size()) {
                    br.fehler = fehlerText(ModellFehler{FehlerArt::VerweisHinterVorrat});
                    return false;
                }
                std::memcpy(c, g->bonePool.data() + off, 14U);
                ++br.unveraendert;
            } else {
                BoneMatrix a = q.localMatrix(f, b);
                if (mJe[static_cast<std::size_t>(b)] != nullptr) {
                    const GlaBone& kb = q.bones[static_cast<std::size_t>(b)];
                    // Stufe 3b, Bindebasis: A' = B M B^-1 (ohne die Bilder der GLA)
                    a = (s.bindeBasis ? kb.basePose : a * kb.basePose) * *mJe[static_cast<std::size_t>(b)] * kb.basePoseInv;
                }
                g2::Mat3x4 m;
                std::memcpy(m.m, a.m, sizeof m.m);
                const g2::fmt::CompQuatBone cb = g2::compressBone(m, co, st);
                std::memcpy(c, cb.comp, 14U);
                ++br.neu;
            }
            aus.indexes.push_back(vorrat.dazu(c));
        }
    }
    br.bilder = anzahl;
    br.vorrat = vorrat.eintraege();
    if (vorrat.zuGross()) {
        br.fehler = fehlerText(ModellFehler{FehlerArt::VorratZuGross});
        return false;
    }
    return true;
}

std::vector<std::array<float, 3>> bewegungsPfad(const GlaAnimation& g, const RigSchicht& s, int knochen, int von, int bis) {
    std::vector<std::array<float, 3>> aus;
    if (knochen < 0 || static_cast<std::size_t>(knochen) >= g.bones.size()) { return aus; }
    std::vector<BoneMatrix> welt;
    for (int f = von; f <= bis; ++f) {
        rigWelt(g, s, f, f, 0.0F, welt);
        std::array<float, 3> p{};
        ort(welt[static_cast<std::size_t>(knochen)], p.data());
        aus.push_back(p);
    }
    return aus;
}

}  // namespace bhed::modell
