// modell/xsiaufnahme.cpp - eine dotXSI-Animation in eine GLA aufnehmen (siehe bhed/modell/xsiaufnahme.h)
//
//   Uses G2C (third_party/g2c, DennisHerrm/G2C-toolkit, GNU General Public License version 2). This file is part of
//   BehavEd-Remake and licensed under the GNU General Public License version 2 or later (LICENSE).
#include "bhed/modell/xsiaufnahme.h"

#include "bhed/modell/fehler.h"
#include "bhed/modell/glavorrat.h"

#include <cstring>
#include <exception>
#include <string>
#include <unordered_map>

#include "g2/compress.h"
#include "g2/model.h"
#include "g2/xsi.h"
#include "g2/xsi_anim.h"

namespace bhed::modell {

bool xsiAufnehmen(const std::string& xsiText, const GlaAnimation& skelett, const std::string& kopfName, GlaAnimation& aus, XsiBericht* bericht) {
    XsiBericht leer;
    XsiBericht& br = bericht != nullptr ? *bericht : leer;
    br = XsiBericht{};
    aus = GlaAnimation{};
    if (skelett.bones.empty()) {
        br.fehler = fehlerText(ModellFehler{FehlerArt::KeinSkelett});
        return false;
    }
    // Das Referenzskelett wie carbuild.cpp (G2C): Knochen, Eltern, Grundstellung, Flaggen, Massstab der GLA.
    g2::Skeleton ref;
    ref.name = kopfName;
    ref.scale = skelett.scale;
    for (const GlaBone& b : skelett.bones) {
        g2::Bone x;
        x.name = b.name;
        x.parent = b.parent;
        x.flags = b.flags;
        std::memcpy(x.basePose.m, b.basePose.m, sizeof x.basePose.m);
        ref.bones.push_back(x);
    }
    g2::xsi::EvalResult ev;
    try {
        const g2::xsi::Document doc = g2::xsi::parse(xsiText);
        const g2::xsi::AnimFile anim = g2::xsi::loadAnimation(doc);
        g2::xsi::EvalOptions opt;
        opt.scale = ref.scale > 0.0F ? ref.scale : 1.0F;   // carbuild.cpp:173
        ev = g2::xsi::evaluate(ref, anim, opt);
    } catch (const std::exception& e) {
        br.fehler = e.what();
        return false;
    }
    br.fehlendeKnochen = ev.missingBones;
    br.zusatzKnochen = ev.extraBones;
    const int nb = static_cast<int>(skelett.bones.size());
    const int nf = ev.frames.frameCount();
    aus.skeletonName = kopfName;
    aus.scale = skelett.scale;
    aus.bones = skelett.bones;
    aus.numFrames = nf;
    // writeMdxa (G2C mdxa.cpp): je Bild und Knochen compressBone, gleiche Eintraege einmal im Vorrat, in Bildreihenfolge.
    GlaVorrat vorrat(aus.bonePool);   // REST N5
    g2::CompressStats st;
    const g2::CompressOptions co{};
    for (int f = 0; f < nf; ++f) {
        for (int b = 0; b < nb; ++b) {
            const g2::fmt::CompQuatBone c = g2::compressBone(ev.frames.at(f, b), co, st);
            aus.indexes.push_back(vorrat.dazu(c.comp));
        }
    }
    br.bilder = nf;
    br.vorrat = vorrat.eintraege();
    if (vorrat.zuGross()) {
        br.fehler = fehlerText(ModellFehler{FehlerArt::VorratZuGross});
        return false;
    }
    return nf > 0;
}

}  // namespace bhed::modell
