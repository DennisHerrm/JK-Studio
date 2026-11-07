// modell/animcfg.cpp - animation.cfg, Karten-GLA und Plaetze (siehe bhed/modell/animcfg.h)
//
//   Rules ported from OpenJK (code/game/NPC_stats.cpp G_ParseAnimationFile/G_ParseAnimFileSet, codemp/game/bg_panimate.c
//   BG_ParseAnimationFile, codeJK2/game/NPC_stats.cpp; Copyright (C) 1999-2005 Id Software, Inc., 2000-2013 Raven
//   Software, Inc., 2013-2015 OpenJK contributors) and Movie Duels Update 9 (same files), licensed under the GNU General
//   Public License version 2. This file is part of BehavEd-Remake and licensed under the GNU General Public License
//   version 2 or later (LICENSE).
#include "bhed/modell/animcfg.h"
#include "bhed/modell/g2.h"

#include <algorithm>
#include <climits>
#include <cctype>
#include <cstdio>
#include <cstring>
#include <unordered_map>

namespace bhed::modell {
namespace {

bool gleichOhneGross(const std::string& a, const std::string& b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

// Eine Zeile wie parseAnimationCfg (gla.cpp) zerlegen: "//" beendet sie, Felder durch Leerzeichen/Tab getrennt,
// mindestens Name und zwei Zahlen.
bool zeileLesen(const std::string& zeile, AnimEntry& e) {
    std::string z = zeile.substr(0, zeile.find("//"));
    std::vector<std::string> teile;
    std::size_t i = 0;
    while (i < z.size()) {
        while (i < z.size() && (z[i] == ' ' || z[i] == '\t' || z[i] == '\r')) { ++i; }
        const std::size_t a = i;
        while (i < z.size() && z[i] != ' ' && z[i] != '\t' && z[i] != '\r') { ++i; }
        if (i > a) { teile.push_back(z.substr(a, i - a)); }
    }
    if (teile.size() < 3) { return false; }
    try {
        e = AnimEntry{};
        e.name = teile[0];
        e.firstFrame = std::stoi(teile[1]);
        e.numFrames = std::stoi(teile[2]);
        if (teile.size() > 3) { e.loopFrame = std::stoi(teile[3]); }
        if (teile.size() > 4) { e.fps = std::stoi(teile[4]); }
    } catch (...) {
        return false;
    }
    return true;
}

}  // namespace

std::vector<AnimEntry> AnimCfg::eintraege() const {
    std::vector<AnimEntry> v;
    for (const CfgZeile& z : zeilen) {
        if (z.eintrag && z.e.numFrames > 0) { v.push_back(z.e); }   // wie parseAnimationCfg
    }
    return v;
}

int AnimCfg::zeileVon(const std::string& name) const {
    for (std::size_t i = 0; i < zeilen.size(); ++i) {
        if (zeilen[i].eintrag && gleichOhneGross(zeilen[i].e.name, name)) { return static_cast<int>(i); }
    }
    return -1;
}

void AnimCfg::setzen(const AnimEntry& e) {
    const int i = zeileVon(e.name);
    if (i >= 0) {
        CfgZeile& z = zeilen[static_cast<std::size_t>(i)];
        if (z.e.name == e.name && z.e.firstFrame == e.firstFrame && z.e.numFrames == e.numFrames && z.e.loopFrame == e.loopFrame &&
            z.e.fps == e.fps) {
            return;
        }
        z.e = e;
        z.geaendert = true;
        return;
    }
    CfgZeile z;
    z.eintrag = true;
    z.e = e;
    z.geaendert = true;
    zeilen.push_back(std::move(z));
}

bool AnimCfg::entfernen(const std::string& name) {
    const int i = zeileVon(name);
    if (i < 0) { return false; }
    zeilen.erase(zeilen.begin() + i);
    return true;
}

AnimCfg animCfgLesen(const std::string& text) {
    AnimCfg c;
    std::size_t at = 0;
    int mitCr = 0;
    int ohneCr = 0;
    while (at < text.size()) {
        std::size_t eol = text.find('\n', at);
        const bool umbruch = eol != std::string::npos;
        if (!umbruch) { eol = text.size(); }
        CfgZeile z;
        z.text = text.substr(at, eol - at);
        z.eintrag = zeileLesen(z.text, z.e);
        if (umbruch) { (!z.text.empty() && z.text.back() == '\r' ? mitCr : ohneCr) += 1; }
        c.zeilen.push_back(std::move(z));
        at = eol + 1;
        c.schlussUmbruch = umbruch;
    }
    c.crlf = mitCr >= ohneCr;
    return c;
}

std::string animCfgZeile(const AnimEntry& e) {
    char b[160];
    // Wie die Zeilen der Raven-Datei (models/players/_humanoid/animation.cfg aus JKA): Name auf 20 Zeichen, dann Tabs.
    std::snprintf(b, sizeof b, "%-20s\t%d\t%d\t%d\t%d", e.name.c_str(), e.firstFrame, e.numFrames, e.loopFrame, e.fps);
    return b;
}

std::string animCfgSchreiben(const AnimCfg& c) {
    std::string o;
    for (std::size_t i = 0; i < c.zeilen.size(); ++i) {
        const CfgZeile& z = c.zeilen[i];
        const bool letzte = i + 1 == c.zeilen.size();
        if (z.geaendert) {
            o += animCfgZeile(z.e);
            if (c.crlf) { o += '\r'; }
        } else {
            o += z.text;
        }
        if (!letzte || c.schlussUmbruch) { o += '\n'; }
    }
    return o;
}

const CfgGrenzen& cfgGrenzen(Zielspiel s) noexcept {
    // Puffer der Leser (der Text muss kuerzer sein als Puffer - 1): OpenJK code/game/NPC_stats.cpp:881/903 char
    // text[80000]; codemp/game/bg_panimate.c:2313/2376 BGPAFtext[60000]; codeJK2/game/NPC_stats.cpp:356/375 text[40000];
    // Movie Duels Update 9 code/game/NPC_stats.cpp:1057-1058 std::vector<char> (keine Grenze), codemp bg_panimate.c:5034
    // BG_ANIM_TEXT_BUFFER_SIZE 524288. MAX_ANIM_FILES: OpenJK code/game/bg_public.h:489 und codemp :324 16,
    // codeJK2/game/anims.h:1432 64, MD9 code/game/bg_public.h:748 und codemp :472 64. Karten-GLA: nur
    // G_ParseAnimFileSet im SP-Spiel (NPC_stats.cpp:1137-1147, MD9 :1262-1290); codemp und codeJK2 kennen kein
    // "_humanoid_%s". BOTH_CIN_1..: OpenJK code/game/anims.h:1762-1811 50, MD9 code/game/anims.h:3177 ff. 125.
    static const CfgGrenzen kWerte[kZielspiele] = {
        {79998, 16, true, 50},     // JKA SP
        {59998, 16, false, 50},    // JKA MP
        {39998, 64, false, 0},     // JK2 SP
        {0, 64, true, 125},        // Movie Duels SP
        {524286, 64, false, 125},  // Movie Duels MP
    };
    const auto i = static_cast<std::size_t>(s);
    return kWerte[i < static_cast<std::size_t>(kZielspiele) ? i : 0U];
}

std::vector<CfgBefund> cfgPruefen(const std::vector<AnimEntry>& anims, int glaBilder, std::size_t bytes, Zielspiel s,
                                  const std::vector<std::string>& bekannt) {
    std::vector<CfgBefund> b;
    const CfgGrenzen& g = cfgGrenzen(s);
    if (g.maxBytes > 0 && bytes > static_cast<std::size_t>(g.maxBytes)) {
        b.push_back({CfgBefundArt::ZuGross, "animation.cfg", static_cast<int>(bytes), g.maxBytes});
    }
    for (const AnimEntry& e : anims) {
        // assert(atoi(token) >= 0 && atoi(token) < 65536) fuer firstFrame und numFrames (unsigned short in animation_t),
        // loopFrames assert(>= -1 && < 128) (signed char) - NPC_stats.cpp:952-979.
        if (e.firstFrame < 0 || e.firstFrame > 65535) { b.push_back({CfgBefundArt::ErsterFrame, e.name, e.firstFrame, 65535}); }
        if (e.numFrames < 0 || e.numFrames > 65535) { b.push_back({CfgBefundArt::Anzahl, e.name, e.numFrames, 65535}); }
        if (e.loopFrame < -1 || e.loopFrame > 127) { b.push_back({CfgBefundArt::Schleife, e.name, e.loopFrame, 127}); }
        // G2_bones.cpp:838-839: endFrame = firstFrame + numFrames muss <= numFrames der GLA sein (animInnerhalb, g2.h)
        if (glaBilder > 0 && !animInnerhalb(e, glaBilder)) {
            const long long ende = static_cast<long long>(e.firstFrame) + e.numFrames;   // pfix N9: ohne Ueberlauf
            b.push_back({CfgBefundArt::HinterGla, e.name, static_cast<int>(std::clamp<long long>(ende, INT_MIN, INT_MAX)), glaBilder});
        }
        if (!bekannt.empty() && e.name != "ROOT" &&
            std::none_of(bekannt.begin(), bekannt.end(), [&](const std::string& n) { return gleichOhneGross(n, e.name); })) {
            b.push_back({CfgBefundArt::Unbekannt, e.name, 0, 0});   // GetIDForString == -1: die Zeile wird uebergangen
        }
    }
    return b;
}

std::string kartenGlaPfad(const std::string& skelett, const std::string& karte) {
    const std::string n = skelett + "_" + karte;
    return "models/players/" + n + "/" + n;
}

std::string kopfNameErwartet(const std::string& pfad) {
    // models/players/<ordner>/<datei>: der Kopf nennt den Pfad ohne Endung wie Ravens GLAs ("models/players/_humanoid/
    // _humanoid"); erwartet: Datei = Ordner.
    const std::size_t s = pfad.rfind('/');
    if (s == std::string::npos || s == 0) { return pfad; }
    const std::size_t s2 = pfad.rfind('/', s - 1);
    const std::string ordner = pfad.substr(s2 == std::string::npos ? 0 : s2 + 1, s - (s2 == std::string::npos ? 0 : s2 + 1));
    return pfad.substr(0, s + 1) + ordner;
}

std::vector<CfgBefund> kartenGlaPruefen(const GlaAnimation& a, const std::string& pfad, Zielspiel s) {
    std::vector<CfgBefund> b;
    if (!cfgGrenzen(s).kartenGla) { b.push_back({CfgBefundArt::KeineKartenGla, pfad, 0, 0}); }
    // Die Karten-GLA haengt am Satz _humanoid (glaIndex 1, gleiche Knochen): 53 im JKA- und MD-Skelett.
    if (a.bones.size() != 53U) { b.push_back({CfgBefundArt::Knochen, a.skeletonName, static_cast<int>(a.bones.size()), 53}); }
    const std::string erwartet = kopfNameErwartet(pfad);
    if (!gleichOhneGross(a.skeletonName, erwartet) || !gleichOhneGross(pfad, erwartet)) {
        b.push_back({CfgBefundArt::NameKopf, a.skeletonName + " / " + erwartet, 0, 0});
    }
    return b;
}

std::vector<CfgBefund> humanoidGlaPruefen(Zielspiel s) {
    std::vector<CfgBefund> b;
    // codemp BG_ParseAnimationFile liest _humanoid auf Server und Client je fuer sich (bg_panimate.c): eine andere Datei auf
    // einer Seite laesst Figuren anders laufen, als der Server rechnet.
    if (s == Zielspiel::JkaMp || s == Zielspiel::MdMp) { b.push_back({CfgBefundArt::MpHinweis, "_humanoid.gla", 0, 0}); }
    return b;
}

std::vector<CfgBefund> eigeneGlaPruefen(const GlaAnimation& a, const std::string& pfad, Zielspiel s) {
    std::vector<CfgBefund> b;
    // Stufe 0 (nachweise_im_spiel.md): eine GLA mit eigenem Skelett laedt der Einzelspieler (G_ParseAnimFileSet je Skelett);
    // im Mehrspieler nur ueber ein Spielermodell mit derselben GLA auf Server und Client - hier als nicht unterstuetzt gemeldet.
    if (s == Zielspiel::JkaMp || s == Zielspiel::MdMp) { b.push_back({CfgBefundArt::NurSp, pfad, 0, 0}); }
    const std::string erwartet = kopfNameErwartet(pfad);
    if (!gleichOhneGross(a.skeletonName, erwartet) || !gleichOhneGross(pfad, erwartet)) {
        b.push_back({CfgBefundArt::NameKopf, a.skeletonName + " / " + erwartet, 0, 0});
    }
    return b;
}

std::vector<CfgBefund> animDateienPruefen(int saetze, Zielspiel s) {
    std::vector<CfgBefund> b;
    const int g = cfgGrenzen(s).animDateien;
    // G_ParseAnimFileSet: level.numKnownAnimFileSets == MAX_ANIM_FILES -> G_Error (ERR_DROP); MP nur assert.
    if (saetze > g) { b.push_back({CfgBefundArt::AnimDateien, "MAX_ANIM_FILES", saetze, g}); }
    return b;
}

std::vector<Platz> kinoPlaetze(Zielspiel s, const std::vector<AnimEntry>& vorhanden) {
    std::vector<Platz> p;
    const int n = cfgGrenzen(s).kinoPlaetze;
    for (int i = 1; i <= n; ++i) {
        Platz x;
        x.name = "BOTH_CIN_" + std::to_string(i);
        for (const AnimEntry& e : vorhanden) {
            if (gleichOhneGross(e.name, x.name)) {
                x.belegt = true;
                x.quelle.sequenz = e;
            }
        }
        p.push_back(std::move(x));
    }
    return p;
}

bool zusammenfuehren(const std::vector<Platz>& plaetze, const std::string& kopfName, GlaAnimation& aus, std::vector<AnimEntry>& cfg,
                     ZusammenBericht* bericht) {
    ZusammenBericht leer;
    ZusammenBericht& br = bericht != nullptr ? *bericht : leer;
    br = ZusammenBericht{};
    aus = GlaAnimation{};
    cfg.clear();
    const auto fehler = [&](const std::string& f) {
        br.fehler = f;
        return false;
    };
    const GlaAnimation* basis = nullptr;
    for (const Platz& p : plaetze) {
        if (p.belegt && p.quelle.gla != nullptr) {
            basis = p.quelle.gla;
            break;
        }
    }
    if (basis == nullptr) { return fehler("kein Platz mit einer Quelle belegt"); }
    aus.skeletonName = kopfName;
    aus.scale = basis->scale;
    aus.bones = basis->bones;
    const std::size_t nb = aus.bones.size();
    std::unordered_map<std::string, std::uint32_t> vorrat;
    for (const Platz& p : plaetze) {
        if (!p.belegt || p.quelle.gla == nullptr) { continue; }
        const GlaAnimation& q = *p.quelle.gla;
        // Knochen der Quelle je Knochen des Ziels (nach Namen); dieselbe Grundstellung, sonst bedeutete ein Eintrag etwas
        // anderes (G2_TransformBone setzt den Eintrag mit BasePose/BasePoseInv der GLA zusammen, tr_ghoul2.cpp).
        std::vector<std::size_t> qk(nb);
        for (std::size_t b = 0; b < nb; ++b) {
            const int k = boneIndex(q, aus.bones[b].name);
            if (k < 0) { return fehler(p.quelle.glaName + ": Knochen " + aus.bones[b].name + " fehlt"); }
            const GlaBone& x = q.bones[static_cast<std::size_t>(k)];
            if (std::memcmp(&x.basePose, &aus.bones[b].basePose, sizeof(BoneMatrix)) != 0) {
                return fehler(p.quelle.glaName + ": Grundstellung von " + aus.bones[b].name + " anders");
            }
            qk[b] = static_cast<std::size_t>(k);
        }
        const AnimEntry& s = p.quelle.sequenz;
        if (s.numFrames <= 0 || s.firstFrame < 0 || s.firstFrame + s.numFrames > q.numFrames) {
            return fehler(p.name + ": Frames " + std::to_string(s.firstFrame) + "+" + std::to_string(s.numFrames) + " nicht in " +
                          p.quelle.glaName + " (" + std::to_string(q.numFrames) + " Frames)");
        }
        AnimEntry z = s;
        z.name = p.name;
        z.firstFrame = aus.numFrames;
        cfg.push_back(z);
        const std::size_t qnb = q.bones.size();
        for (int f = s.firstFrame; f < s.firstFrame + s.numFrames; ++f) {
            for (std::size_t b = 0; b < nb; ++b) {
                const std::uint32_t qi = q.indexes[static_cast<std::size_t>(f) * qnb + qk[b]];
                if (static_cast<std::size_t>(qi) * 14U + 14U > q.bonePool.size()) { return fehler(p.quelle.glaName + ": Verweis hinter dem Vorrat"); }
                const std::string eintrag(reinterpret_cast<const char*>(q.bonePool.data()) + static_cast<std::size_t>(qi) * 14U, 14U);
                const auto [it, neu] = vorrat.emplace(eintrag, static_cast<std::uint32_t>(aus.bonePool.size() / 14U));
                if (neu) { aus.bonePool.insert(aus.bonePool.end(), eintrag.begin(), eintrag.end()); }
                aus.indexes.push_back(it->second);
            }
            ++aus.numFrames;
        }
    }
    br.bilder = aus.numFrames;
    br.vorrat = static_cast<int>(aus.bonePool.size() / 14U);
    if (aus.bonePool.size() / 14U > 0xFFFFFFU) { return fehler("Vorrat groesser als ein 24-Bit-Verweis erreicht"); }
    return true;
}

bool anhaengen(const GlaAnimation& basis, const std::vector<Platz>& plaetze, GlaAnimation& aus, std::vector<AnimEntry>& cfg, ZusammenBericht* bericht) {
    ZusammenBericht leer;
    ZusammenBericht& br = bericht != nullptr ? *bericht : leer;
    br = ZusammenBericht{};
    cfg.clear();
    // die neuen Frames wie zusammenfuehren, ohne Kopf
    GlaAnimation neu;
    if (!zusammenfuehren(plaetze, basis.skeletonName, neu, cfg, &br)) { return false; }
    if (neu.bones.size() != basis.bones.size()) {
        br.fehler = "anderes Skelett als die GLA";
        return false;
    }
    std::vector<std::size_t> zuBasis(neu.bones.size());
    for (std::size_t b = 0; b < neu.bones.size(); ++b) {
        const int k = boneIndex(basis, neu.bones[b].name);
        if (k < 0 || std::memcmp(&basis.bones[static_cast<std::size_t>(k)].basePose, &neu.bones[b].basePose, sizeof(BoneMatrix)) != 0) {
            br.fehler = "Knochen " + neu.bones[b].name + " fehlt in der GLA oder hat eine andere Grundstellung";
            return false;
        }
        zuBasis[b] = static_cast<std::size_t>(k);
    }
    aus = basis;
    aus.anhang = nullptr;
    std::unordered_map<std::string, std::uint32_t> vorrat;
    vorrat.reserve(aus.bonePool.size() / 14U);
    for (std::size_t i = 0; i + 14U <= aus.bonePool.size(); i += 14U) {
        vorrat.emplace(std::string(reinterpret_cast<const char*>(aus.bonePool.data()) + i, 14U), static_cast<std::uint32_t>(i / 14U));
    }
    const std::size_t nb = aus.bones.size();
    const int anfang = aus.numFrames;
    aus.indexes.resize(aus.indexes.size() + static_cast<std::size_t>(neu.numFrames) * nb);
    for (int f = 0; f < neu.numFrames; ++f) {
        for (std::size_t b = 0; b < nb; ++b) {
            const std::uint32_t qi = neu.indexes[static_cast<std::size_t>(f) * nb + b];
            const std::string e(reinterpret_cast<const char*>(neu.bonePool.data()) + static_cast<std::size_t>(qi) * 14U, 14U);
            const auto [it, frisch] = vorrat.emplace(e, static_cast<std::uint32_t>(aus.bonePool.size() / 14U));
            if (frisch) { aus.bonePool.insert(aus.bonePool.end(), e.begin(), e.end()); }
            aus.indexes[static_cast<std::size_t>(anfang + f) * nb + zuBasis[b]] = it->second;
        }
    }
    aus.numFrames += neu.numFrames;
    for (AnimEntry& e : cfg) { e.firstFrame += anfang; }
    br.bilder = aus.numFrames;
    br.vorrat = static_cast<int>(aus.bonePool.size() / 14U);
    if (aus.bonePool.size() / 14U > 0xFFFFFFU) {
        br.fehler = "Vorrat groesser als ein 24-Bit-Verweis erreicht";
        return false;
    }
    return true;
}

}  // namespace bhed::modell
