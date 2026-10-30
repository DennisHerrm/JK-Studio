// gesicht.cpp - Blinzeln und Leerlaufmimik (bhed/gesicht.h)
//
// CG_G2PlayerHeadAnims / CG_G2SetHeadBlink (cg_players.cpp:5046-5232 OpenJK),
// Q3_Face (Q3_Interface.cpp OpenJK). Movie Duels und codeJK2 gleich.
#include "bhed/gesicht.h"

#include "bhed/todesanim.h"   // EngineZufall

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

namespace bhed {
namespace {

constexpr float kLidZu = -38.0F;   // desiredAngles[YAW]

}  // namespace

bool gesichtsBefehlArt(const std::string& name, GesichtsBefehl::Art& art) {
    using A = GesichtsBefehl::Art;
    static const std::array<std::pair<const char*, A>, 11> kNamen = {{
        {"SET_FACEEYESCLOSED", A::AugenZu},
        {"SET_FACEEYESOPENED", A::AugenAuf},
        {"SET_FACEBLINK", A::Blinzeln},
        {"SET_FACEAUX", A::Aux},
        {"SET_FACEBLINKFROWN", A::BlinzelnFrown},
        {"SET_FACEFROWN", A::Frown},
        {"SET_FACESMILE", A::Smile},
        {"SET_FACEGLAD", A::Glad},
        {"SET_FACEHAPPY", A::Happy},
        {"SET_FACESHOCKED", A::Shocked},
        {"SET_FACENORMAL", A::Normal},
    }};
    for (const auto& [n, a] : kNamen) {
        if (name == n) {
            art = a;
            return true;
        }
    }
    return false;
}

GesichtsPlan gesichtPlanen(const GesichtsEingabe& e) {
    GesichtsPlan p;
    p.todMs = e.todMs;
    p.sprechen = e.sprechen;
    EngineZufall z(e.saat);
    double blink = 0.0;   // client->facial_blink
    double timer = 0.0;   // client->facial_timer
    // client->facial_anim: G_Spawn nullt - Animation 0 ist BOTH_1CRUFTFORGIL (anims.h)
    std::string anim = "BOTH_1CRUFTFORGIL";
    const auto spricht = [&e](double t) {
        for (const auto& [von, bis] : e.sprechen) {
            if (t >= von && t <= bis) { return true; }
        }
        return false;
    };
    std::size_t befehl = 0;
    double t = e.abMs;
    while (t <= e.bisMs) {
        // Q3_Face aus dem Skript dieses Bilds (level.time) vor dem Zeichnen
        while (befehl < e.befehle.size() && e.befehle[befehl].ms <= t) {
            const GesichtsBefehl& b = e.befehle[befehl++];
            const double halten = static_cast<double>(b.halten) * 1000.0;
            using A = GesichtsBefehl::Art;
            switch (b.art) {
                case A::AugenZu: blink = 1.0; break;
                case A::AugenAuf: blink = -1.0; break;
                case A::Blinzeln: timer = -(b.ms + halten); break;
                case A::Aux:
                    timer = -(b.ms + halten);
                    anim = "FACE_ALERT";
                    break;
                case A::BlinzelnFrown:
                    blink = -(b.ms + halten);
                    timer = -(b.ms + halten);   // "case SET_FACEFROWN:" ohne break davor
                    anim = "FACE_FROWN";
                    break;
                case A::Frown:
                    timer = -(b.ms + halten);
                    anim = "FACE_FROWN";
                    break;
                case A::Smile:
                    blink = -(b.ms + halten);
                    timer = -(b.ms + halten);
                    anim = "FACE_SMILE";
                    break;
                case A::Glad:
                    blink = 1.0;
                    timer = -(b.ms + halten);
                    anim = "FACE_TALK1";
                    break;
                case A::Happy:
                    blink = -(b.ms + halten);
                    timer = -(b.ms + halten);
                    anim = "FACE_TALK1";
                    break;
                case A::Shocked:
                    blink = -1.0;
                    timer = -(b.ms + halten);
                    anim = "FACE_TALK3";
                    break;
                case A::Normal:
                    timer = b.ms + static_cast<double>(z.flrand(6000.0F, 10000.0F));
                    blink = b.ms + static_cast<double>(z.flrand(3000.0F, 5000.0F));
                    break;
            }
        }
        // "Dead people close their eyes and don't make faces!"
        if (e.todMs >= 0.0 && t >= e.todMs) {
            p.mimik.emplace_back(e.todMs, "FACE_DEAD");
            break;
        }
        if (blink == 0.0) {
            blink = t + static_cast<double>(z.flrand(4000.0F, 8000.0F));
            timer = t + static_cast<double>(z.flrand(6000.0F, 10000.0F));
        }
        if (blink < 0.0) {
            if (-blink < t) {
                blink = t + static_cast<double>(z.flrand(4000.0F, 8000.0F));
                p.lider.push_back({t, 0.0F, false});   // CG_G2SetHeadBlink( cent, qfalse )
            }
        } else if (blink < t) {
            // "if ( !in_camera && Q_flrand(0.0f, 1.0f) > 0.95f )" - der Zug nur ohne Kamera
            const bool zwinkern = !e.kamera && z.flrand(0.0F, 1.0F) > 0.95F;
            p.lider.push_back({t, kLidZu, zwinkern});
            blink = blink == 1.0 ? -(t + 99999999.0) : -(t + 300.0);
        }
        const bool jetztSpricht = spricht(t);
        if (jetztSpricht) {
            // je Bild neu - es zaehlt das letzte Bild, in dem die Figur spricht
            timer = t + static_cast<double>(z.flrand(2000.0F, 7000.0F));
        } else if (timer < 0.0) {
            if (-timer < t) {
                timer = t + static_cast<double>(z.flrand(7000.0F, 10000.0F));
            } else if (p.mimik.empty() || p.mimik.back().second != anim) {
                p.mimik.emplace_back(t, anim);
            }
        } else if (timer < t) {
            static const char* const kAux[3] = {"FACE_ALERT", "FACE_SMILE", "FACE_FROWN"};
            anim = kAux[z.irand(0, 2)];
            timer = -(t + 2000.0);
            p.mimik.emplace_back(t, anim);
        }
        // das naechste Bild, in dem sich etwas aendern kann
        double n = e.bisMs + 1.0;
        if (befehl < e.befehle.size()) { n = std::min(n, std::max(t + 1.0, e.befehle[befehl].ms)); }
        if (e.todMs > t) { n = std::min(n, e.todMs); }
        // cg.time ist ganzzahlig: das erste Bild mit "deadline < cg.time"
        n = std::min(n, std::floor(blink < 0.0 ? -blink : blink) + 1.0);
        if (jetztSpricht) {
            for (const auto& [von, bis] : e.sprechen) {
                if (t >= von && t <= bis) { n = std::min(n, std::max(t + 1.0, bis)); }
            }
            // das letzte Bild im Sprechen, dann das erste danach
            if (n >= t + 1.0 && !spricht(n)) { n = t + 1.0; }
        } else {
            for (const auto& [von, bis] : e.sprechen) {
                if (von > t) { n = std::min(n, von); }
            }
            n = std::min(n, std::floor(timer < 0.0 ? -timer : timer) + 1.0);
        }
        t = std::max(n, t + 1.0);
    }
    return p;
}

void gesichtBei(const GesichtsPlan& p, double ms, float& links, float& rechts, std::string& mimik, double& sprachEnde) {
    links = 0.0F;
    rechts = 0.0F;
    for (const GesichtsPlan::Lid& l : p.lider) {
        if (l.ms > ms) { break; }
        links = l.winkel;
        if (!l.nurLinks) { rechts = l.winkel; }   // "if (!bWink)" - das rechte bleibt
    }
    mimik.clear();
    double mimikAb = -1.0e300;
    for (const auto& [ab, name] : p.mimik) {
        if (ab > ms) { break; }
        mimik = name;
        mimikAb = ab;
    }
    // CG_G2SetHeadAnim setzt nur, wenn anim != -1 - nach dem Sprechen ohne
    // Mimik bleibt der letzte Mund stehen
    sprachEnde = -1.0;
    for (const auto& [von, bis] : p.sprechen) {
        if (bis < ms && bis > mimikAb && bis > sprachEnde) { sprachEnde = bis; }
    }
}

}  // namespace bhed
