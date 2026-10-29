// spuren.cpp - das Modell der Zeitleiste (siehe bhed/spuren.h)
#include "bhed/spuren.h"

#include "bhed/edit.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <cstring>
#include <map>
#include <set>

namespace bhed::spuren {
namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::string gross(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    return s;
}

// "\"a b\" " -> "a b"
std::string bloss(std::string t) {
    t.erase(std::remove(t.begin(), t.end(), '"'), t.end());
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.front())) != 0) { t.erase(t.begin()); }
    while (!t.empty() && std::isspace(static_cast<unsigned char>(t.back())) != 0) { t.pop_back(); }
    return t;
}

// Ein Befehl so, wie er im Skript steht: name ( a, b ).
std::string befehlsText(const Node& n) {
    std::string l = n.name + " (";
    for (std::size_t i = 0; i < n.args.size(); ++i) {
        l += (i == 0 ? " " : ", ");
        l += n.args[i].kind == Arg::Kind::String ? "\"" + n.args[i].text + "\"" : n.args[i].text;
    }
    return l + " )";
}

// Name und erstes Argument aus einem Label "name ( a, b )".
void ausLabel(const std::string& label, std::string& name, std::string& erstes, std::string& innen) {
    const std::size_t auf = label.find(" (");
    name = label.substr(0, auf);
    innen.clear();
    erstes.clear();
    if (auf == std::string::npos) { return; }
    const std::size_t zu = label.rfind(')');
    innen = bloss(label.substr(auf + 2, zu == std::string::npos || zu < auf + 2 ? std::string::npos : zu - auf - 2));
    erstes = bloss(innen.substr(0, innen.find(',')));
}

bool nachgebildetName(const std::string& name, const std::string& erstes) {
    // Was der Nachbau ausfuehrt (src/ablauf.cpp, Verteiler je Befehl), dazu
    // run - es startet ein Skript. camera TRACK/DISTANCE baut camtrack.cpp
    // nicht nach; play kennt das Spiel nur als PLAY_ROFF.
    static const std::set<std::string> kBekannt = {
        "affect", "camera", "declare", "do", "dowait", "else", "flush", "free", "if", "kill", "loop", "move",
        "play", "remove", "rotate", "run", "set", "signal", "sound", "task", "use", "wait", "waitsignal"};
    const std::string n = klein(name);
    if (kBekannt.count(n) == 0U) { return false; }
    const std::string a = gross(erstes);
    if (n == "camera") { return a != "TRACK" && a != "DISTANCE"; }
    if (n == "play") { return a == "PLAY_ROFF"; }
    return true;
}

// Die Kamerabefehle einer Gruppe, die sich gegenseitig ablosen: ein neuer
// MOVE ersetzt den laufenden MOVE, ein PAN den laufenden PAN - und ein ROLL
// ueber alle Achsen wirkt wie ein PAN (camtrack.cpp ROLL).
int kameraGruppe(const CamSegment& s) {
    switch (s.kind) {
        case CamSegment::Kind::Move: return 0;
        case CamSegment::Kind::Pan: return 1;
        case CamSegment::Kind::Roll: return s.alleAchsen ? 1 : 3;
        case CamSegment::Kind::Zoom: return 2;
        default: return -1;
    }
}

Art kameraArt(CamSegment::Kind k) {
    switch (k) {
        case CamSegment::Kind::Move: return Art::Fahrt;
        case CamSegment::Kind::Pan: return Art::Schwenk;
        case CamSegment::Kind::Zoom: return Art::Zoom;
        case CamSegment::Kind::Roll: return Art::Rollen;
        case CamSegment::Kind::Enable: return Art::KameraAn;
        case CamSegment::Kind::Disable: return Art::KameraAus;
        case CamSegment::Kind::Fade: return Art::Blende;
        case CamSegment::Kind::Shake: return Art::Wackeln;
        case CamSegment::Kind::Follow: return Art::Folgen;
        case CamSegment::Kind::Pfad: return Art::KameraBahn;
    }
    return Art::Befehl;
}

std::string koordinaten(const float v[3]) {
    char b[96];
    std::snprintf(b, sizeof(b), "(%ld, %ld, %ld)", std::lround(v[0]), std::lround(v[1]), std::lround(v[2]));
    return b;
}

class Bauer {
public:
    explicit Bauer(const SpurenQuellen& q) : q_(q) {}

    Spuren baue() {
        const Timeline& tl = *q_.zeitleiste;
        const CameraTrack& ct = *q_.kamera;
        s_.dauerMs = std::max({tl.practicalEndMs, ct.durationMs, 1.0});
        if (q_.ablauf != nullptr && q_.ablauf->kartenwechselMs >= 0.0) {
            s_.kartenwechselMs = q_.ablauf->kartenwechselMs;
            s_.kartenwechselKarte = q_.ablauf->kartenwechselKarte;
        }
        signaleSammeln();
        kamera();
        kurven();
        ton();
        skript();
        darsteller();
        return std::move(s_);
    }

private:
    const SpurenQuellen& q_;
    Spuren s_;
    // signal-Name (klein) -> wann und von wem
    std::map<std::string, std::vector<std::pair<double, std::string>>> signale_;

    const Node* knoten(const Path& p) const {
        if (p.empty() || q_.skript == nullptr) { return nullptr; }
        const Node* n = nodeAt(*q_.skript, p);
        return (n != nullptr && n->kind == Node::Kind::Command) ? n : nullptr;
    }

    std::string traegerName(const std::string& entity) const {
        return entity.empty() && q_.ablauf != nullptr ? q_.ablauf->traeger : entity;
    }

    void signaleSammeln() {
        for (const TimelineTrack& tr : q_.zeitleiste->tracks) {
            for (const TimelineEvent& e : tr.events) {
                std::string name;
                std::string erstes;
                std::string innen;
                ausLabel(e.label, name, erstes, innen);
                if (name == "signal") { signale_[klein(erstes)].emplace_back(e.startMs, traegerName(tr.entity)); }
            }
        }
    }

    // --- Kamera ------------------------------------------------------------
    void kamera() {
        const CameraTrack& ct = *q_.kamera;
        std::vector<Clip> alle;
        alle.reserve(ct.segments.size());
        for (const CamSegment& seg : ct.segments) {
            Clip c;
            c.art = kameraArt(seg.kind);
            c.startMs = seg.startMs;
            c.endMs = std::max(seg.endMs, seg.startMs);
            c.pfad = seg.path;
            c.fremd = seg.fremd;
            const Node* n = knoten(seg.path);
            if (n != nullptr) { c.befehl = befehlsText(*n); }
            std::string marke;
            bool winkel = false;
            const bool mitMarke = n != nullptr && n->args.size() >= 2 && kameraMarke(n->args[1].text, marke, winkel);
            switch (seg.kind) {
                case CamSegment::Kind::Move:
                    c.text = mitMarke ? marke : koordinaten(seg.to);
                    break;
                case CamSegment::Kind::Pan:
                    if (mitMarke) { c.text = marke; }
                    break;
                case CamSegment::Kind::Zoom: {
                    // ENABLE setzt den Bildwinkel zurueck (CGCam_Enable: FOV =
                    // CAMERA_DEFAULT_FOV) - das ist kein ZOOM des Skripts.
                    if (n != nullptr && (n->args.empty() || gross(bloss(n->args[0].text)) != "ZOOM")) { c.art = Art::Befehl; }
                    char b[32];
                    std::snprintf(b, sizeof(b), "%g", std::round(static_cast<double>(seg.to[0]) * 10.0) / 10.0);
                    c.text = b;
                    c.wert = seg.to[0];
                    break;
                }
                case CamSegment::Kind::Fade:
                    c.wert = seg.farbeZu[3];
                    break;
                case CamSegment::Kind::Follow:
                    c.text = seg.group;
                    break;
                default:
                    break;
            }
            for (int k = 0; k < 3; ++k) {
                c.von[k] = seg.from[k];
                c.nach[k] = seg.to[k];
            }
            if (!c.pfad.empty() && c.fremd.empty()) {
                c.verschiebbar = true;
                c.laengeZiehbar = n != nullptr && durationArgIndex(*n) >= 0;
            }
            alle.push_back(std::move(c));
        }
        // Zurueckspringen: ein Befehl derselben Gruppe laeuft noch, wenn
        // dieser beginnt (CGCam_Update rechnet vom alten Anfang).
        for (std::size_t i = 0; i < alle.size(); ++i) {
            const int g = kameraGruppe(ct.segments[i]);
            if (g < 0 || alle[i].art == Art::Befehl) { continue; }
            double vorher = -1.0;
            for (std::size_t j = 0; j < alle.size(); ++j) {
                if (j == i || kameraGruppe(ct.segments[j]) != g || alle[j].art == Art::Befehl) { continue; }
                const Clip& o = alle[j];
                if (o.startMs < alle[i].startMs - 0.5 && o.endMs > alle[i].startMs + 0.5) { vorher = std::max(vorher, o.startMs); }
            }
            if (vorher >= 0.0) {
                alle[i].springt = true;
                alle[i].vorigerStartMs = vorher;
                ++s_.ueberlappungen;
            }
        }
        // Detailzeilen
        for (const Clip& c : alle) {
            switch (c.art) {
                case Art::Fahrt: s_.fahrt.clips.push_back(c); break;
                case Art::Schwenk: s_.schwenk.clips.push_back(c); break;
                case Art::Zoom: s_.zoom.clips.push_back(c); break;
                case Art::Blende: s_.blende.clips.push_back(c); break;
                case Art::Rollen: s_.rollen.clips.push_back(c); break;
                case Art::Wackeln: s_.wackeln.clips.push_back(c); break;
                case Art::Folgen: s_.folgen.clips.push_back(c); break;
                case Art::KameraBahn: s_.kameraBahn.clips.push_back(c); break;
                case Art::KameraAn:
                case Art::KameraAus:
                    s_.fahrt.clips.push_back(c);
                    s_.ort.clips.push_back(c);
                    break;
                default: break;
            }
        }
        // Uebersicht: je MOVE eine Einstellung bis zum naechsten MOVE (oder
        // bis die Kamera ausgeht bzw. die Szene endet).
        std::vector<const Clip*> moves;
        for (const Clip& c : s_.fahrt.clips) {
            if (c.art == Art::Fahrt) { moves.push_back(&c); }
        }
        std::stable_sort(moves.begin(), moves.end(), [](const Clip* a, const Clip* b) { return a->startMs < b->startMs; });
        for (std::size_t i = 0; i < moves.size(); ++i) {
            Clip e = *moves[i];
            e.art = Art::Einstellung;
            double bis = i + 1 < moves.size() ? moves[i + 1]->startMs : s_.dauerMs;
            for (const Clip& c : alle) {
                if (c.art == Art::KameraAus && c.startMs > e.startMs && c.startMs < bis) { bis = c.startMs; }
            }
            e.endMs = std::max(bis, e.startMs);
            for (const Clip& c : alle) {
                if (c.springt && c.startMs >= e.startMs && c.startMs < bis) { e.problem = true; }
            }
            s_.ort.clips.push_back(std::move(e));
        }
        for (const Clip& c : s_.schwenk.clips) { s_.drehung.clips.push_back(c); }
        for (Zeile* z : {&s_.ort, &s_.drehung, &s_.fahrt, &s_.schwenk, &s_.zoom, &s_.blende, &s_.rollen, &s_.wackeln,
                         &s_.folgen, &s_.kameraBahn}) {
            std::stable_sort(z->clips.begin(), z->clips.end(),
                             [](const Clip& a, const Clip& b) { return a.startMs < b.startMs; });
        }
        for (Zeile* z : {&s_.drehung, &s_.fahrt, &s_.schwenk, &s_.zoom, &s_.blende, &s_.rollen, &s_.wackeln, &s_.folgen,
                         &s_.kameraBahn}) {
            bahnenVerteilen(*z, 0.0);
        }
    }

    // Bildwinkel und Blende als Knickpunkte. Zwischen zwei Knicken
    // interpoliert das Spiel linear (CGCam_Update, CGCam_UpdateFade) - nur
    // fovaccel aus einer Kamera-ROFF (CGCam_ZoomAccel) und camera PATH sind
    // es nicht; dort wird im Spieltakt (50 ms) abgetastet.
    void kurven() {
        const CameraTrack& ct = *q_.kamera;
        std::vector<double> t{0.0, s_.dauerMs};
        for (const CamSegment& seg : ct.segments) {
            if (seg.kind != CamSegment::Kind::Zoom && seg.kind != CamSegment::Kind::Fade &&
                seg.kind != CamSegment::Kind::Enable && seg.kind != CamSegment::Kind::Disable &&
                seg.kind != CamSegment::Kind::Pfad) {
                continue;
            }
            t.push_back(seg.startMs);
            t.push_back(seg.endMs);
            if (seg.lerpStartMs >= 0.0) { t.push_back(seg.lerpStartMs); }
            if (seg.beschleunigt || seg.kind == CamSegment::Kind::Pfad) {
                for (double m = seg.startMs; m < seg.endMs; m += 50.0) { t.push_back(m); }
            }
        }
        std::sort(t.begin(), t.end());
        t.erase(std::unique(t.begin(), t.end(), [](double a, double b) { return std::fabs(a - b) < 0.001; }), t.end());
        const auto dazu = [](std::vector<Knick>& k, double ms, float w) {
            if (!k.empty() && std::fabs(k.back().ms - ms) < 0.001 && std::fabs(k.back().wert - w) < 1.0e-4F) { return; }
            k.push_back({ms, w});
        };
        for (const double ms : t) {
            if (ms < 0.0 || ms > s_.dauerMs + 0.001) { continue; }
            if (ms > 0.0) {
                const CameraState vor = ct.at(ms - 0.01);
                dazu(s_.sichtfeld, ms, sichtFovX(vor));
                dazu(s_.schwarz, ms, vor.fadeColor[3]);
            }
            const CameraState st = ct.at(ms);
            dazu(s_.sichtfeld, ms, sichtFovX(st));
            dazu(s_.schwarz, ms, st.fadeColor[3]);
        }
        // Gleichbleibende Strecken auf ihre Enden kuerzen.
        for (std::vector<Knick>* k : {&s_.sichtfeld, &s_.schwarz}) {
            std::vector<Knick> neu;
            for (std::size_t i = 0; i < k->size(); ++i) {
                const bool mitte = i > 0 && i + 1 < k->size() && std::fabs((*k)[i - 1].wert - (*k)[i].wert) < 1.0e-4F &&
                                   std::fabs((*k)[i + 1].wert - (*k)[i].wert) < 1.0e-4F;
                if (!mitte) { neu.push_back((*k)[i]); }
            }
            *k = std::move(neu);
        }
    }

    // --- Ton ---------------------------------------------------------------
    void ton() {
        for (const TimelineTrack& tr : q_.zeitleiste->tracks) {
            for (const TimelineEvent& e : tr.events) {
                if (e.kind != TimelineEvent::Kind::Sound || e.sound.empty()) { continue; }
                s_.sprache.clips.push_back(satz(e, tr.entity));
            }
        }
        std::stable_sort(s_.sprache.clips.begin(), s_.sprache.clips.end(),
                         [](const Clip& a, const Clip& b) { return a.startMs < b.startMs; });
        bahnenVerteilen(s_.sprache, 300.0);
        // Musik: die Levelmusik ab 0, jedes use auf ein target_play_music
        // (auch ueber target_relay & Co., Ablauf::benutzt) ab seiner Zeit -
        // das Spiel kennt nur EINE Musik zur Zeit (CS_MUSIC).
        std::vector<Clip> musik;
        if (!q_.weltMusik.empty()) {
            Clip c;
            c.art = Art::Musik;
            c.datei = q_.weltMusik;
            c.text = q_.weltMusik;
            musik.push_back(std::move(c));
        }
        if (q_.ablauf != nullptr && q_.musikVon) {
            for (const Ablauf::Benutzung& b : q_.ablauf->benutzt) {
                const std::string datei = q_.musikVon(b.name);
                if (datei.empty()) { continue; }
                Clip c;
                c.art = Art::Musik;
                c.startMs = b.ms;
                c.datei = datei;
                c.text = datei;
                c.wartetAuf = b.name;   // das target_play_music
                c.wer = b.ausloeser;
                // Die Zeile im Skript: das use mit diesem Ziel zu dieser Zeit.
                for (const TimelineTrack& tr : q_.zeitleiste->tracks) {
                    for (const TimelineEvent& e : tr.events) {
                        if (e.kind == TimelineEvent::Kind::Use && klein(bloss(e.target)) == klein(b.name) &&
                            std::fabs(e.startMs - b.ms) < 1.0 && c.pfad.empty() && c.fremd.empty()) {
                            c.pfad = e.path;
                            c.fremd = e.fremd;
                            c.befehl = e.label;
                        }
                    }
                }
                musik.push_back(std::move(c));
            }
        }
        std::stable_sort(musik.begin(), musik.end(), [](const Clip& a, const Clip& b) { return a.startMs < b.startMs; });
        for (std::size_t i = 0; i < musik.size(); ++i) {
            musik[i].endMs = i + 1 < musik.size() ? musik[i + 1].startMs : s_.dauerMs;
            musik[i].endMs = std::max(musik[i].endMs, musik[i].startMs);
        }
        s_.musik.clips = std::move(musik);
    }

    Clip satz(const TimelineEvent& e, const std::string& entity) const {
        Clip c;
        c.art = Art::Satz;
        c.startMs = e.startMs;
        c.datei = bloss(e.sound);
        c.kanal = e.kanal;
        c.klangMs = q_.klangMs ? q_.klangMs(c.datei) : 0.0;
        c.endMs = c.startMs + std::max(0.0, c.klangMs);
        if (q_.untertitel) { c.untertitel = q_.untertitel(c.datei, &c.schluessel); }
        c.wer = traegerName(entity);
        c.text = c.untertitel.empty() ? c.datei : c.untertitel;
        c.befehl = e.label;
        c.pfad = e.path;
        c.fremd = e.fremd;
        c.verschiebbar = !c.pfad.empty() && c.fremd.empty();
        return c;
    }

    // --- Ablauf ------------------------------------------------------------
    Clip ablaufClip(const TimelineEvent& e, const std::string& entity) const {
        if (e.kind == TimelineEvent::Kind::Sound && !e.sound.empty()) { return satz(e, entity); }
        Clip c;
        c.startMs = e.startMs;
        c.endMs = std::max(e.endMs, e.startMs);
        c.pfad = e.path;
        c.fremd = e.fremd;
        c.wer = traegerName(entity);
        const Node* n = knoten(e.path);
        c.befehl = n != nullptr ? befehlsText(*n) : e.label;
        std::string name;
        std::string erstes;
        std::string innen;
        ausLabel(n != nullptr ? befehlsText(*n) : e.label, name, erstes, innen);
        const bool eigen = !c.pfad.empty() && c.fremd.empty();
        switch (e.kind) {
            case TimelineEvent::Kind::Wait: {
                c.art = Art::Warten;
                const std::string befehl = n != nullptr ? n->name : name;
                if (befehl == "dowait") {
                    c.warteArt = WarteArt::Aufgabe;
                    c.wartetAuf = erstes;
                } else if (befehl == "waitsignal" ||
                           (befehl == "wait" && n != nullptr && !n->args.empty() && n->args[0].kind == Arg::Kind::String)) {
                    c.warteArt = WarteArt::Signal;
                    c.wartetAuf = erstes;
                    const auto it = signale_.find(klein(erstes));
                    if (it != signale_.end()) { c.sender = it->second; }
                }
                // Nur ein wait mit Zahl hat eine Kante zum Ziehen.
                c.laengeZiehbar = eigen && n != nullptr && n->name == "wait" && !n->args.empty() &&
                                  n->args[0].kind == Arg::Kind::Number;
                return c;
            }
            case TimelineEvent::Kind::Use:
                c.art = Art::Benutzen;
                c.text = bloss(e.target);
                break;
            case TimelineEvent::Kind::Set:
            case TimelineEvent::Kind::Saber:
                c.art = Art::Setzen;
                c.text = innen;
                break;
            case TimelineEvent::Kind::Move:
                c.art = Art::Befehl;
                c.text = name;
                c.laengeZiehbar = eigen && n != nullptr && durationArgIndex(*n) >= 0;
                break;
            default:
                c.art = name == "signal" ? Art::Signal : Art::Befehl;
                c.text = name == "signal" ? erstes : name;
                break;
        }
        if (!nachgebildetName(name, erstes)) {
            c.art = Art::NichtNachgebildet;
            c.text = name;
        }
        c.verschiebbar = eigen;
        return c;
    }

    void skript() {
        for (const TimelineTrack& tr : q_.zeitleiste->tracks) {
            if (!tr.entity.empty()) { continue; }
            for (const TimelineEvent& e : tr.events) {
                if (e.kind == TimelineEvent::Kind::Camera) { continue; }
                Clip c = ablaufClip(e, tr.entity);
                if (c.art == Art::NichtNachgebildet) { ++s_.nichtNachgebildet; }
                s_.skript.clips.push_back(std::move(c));
            }
        }
        // Kamerabefehle, die die Vorschau nicht nachbildet (camera TRACK,
        // DISTANCE), haben keinen Kameraabschnitt - als Marke im Ablauf.
        for (const TimelineTrack& tr : q_.zeitleiste->tracks) {
            for (const TimelineEvent& e : tr.events) {
                if (e.kind != TimelineEvent::Kind::Camera) { continue; }
                std::string name;
                std::string erstes;
                std::string innen;
                ausLabel(e.label, name, erstes, innen);
                if (nachgebildetName(name, erstes)) { continue; }
                Clip c = ablaufClip(e, tr.entity);
                c.art = Art::NichtNachgebildet;
                c.text = name + " " + erstes;
                c.endMs = c.startMs;
                ++s_.nichtNachgebildet;
                s_.skript.clips.push_back(std::move(c));
            }
        }
        if (s_.kartenwechselMs >= 0.0) {
            Clip c;
            c.art = Art::Kartenwechsel;
            c.startMs = s_.kartenwechselMs;
            c.endMs = s_.kartenwechselMs;
            c.text = s_.kartenwechselKarte;
            s_.skript.clips.push_back(std::move(c));
        }
        std::stable_sort(s_.skript.clips.begin(), s_.skript.clips.end(),
                         [](const Clip& a, const Clip& b) { return a.startMs < b.startMs; });
    }

    // --- Darsteller --------------------------------------------------------
    void darsteller() {
        const Scene& sc = *q_.szene;
        for (std::size_t ai = 0; ai < sc.actors.size(); ++ai) {
            const Actor& a = sc.actors[ai];
            Darsteller d;
            d.name = a.name;
            d.npcType = a.npcType;
            d.anzeige = anzeigeName(a.npcType, a.name);
            d.figur = static_cast<int>(ai);
            for (const TimelineTrack& tr : q_.zeitleiste->tracks) {
                if (tr.entity.empty() || klein(tr.entity) != klein(a.name)) { continue; }
                for (const TimelineEvent& e : tr.events) {
                    if (e.kind == TimelineEvent::Kind::Camera) { continue; }
                    Clip c = ablaufClip(e, tr.entity);
                    if (c.art == Art::NichtNachgebildet) { ++s_.nichtNachgebildet; }
                    d.befehle.clips.push_back(std::move(c));
                }
            }
            bewegungen(a, d);
            animationen(a, d);
            sichtbarkeit(a, d);
            for (Zeile* z : {&d.befehle, &d.laufen, &d.animation, &d.oben}) {
                std::stable_sort(z->clips.begin(), z->clips.end(),
                                 [](const Clip& x, const Clip& y) { return x.startMs < y.startMs; });
            }
            bahnenVerteilen(d.animation, 0.0);
            bahnenVerteilen(d.oben, 0.0);
            s_.darsteller.push_back(std::move(d));
        }
    }

    void bewegungen(const Actor& a, Darsteller& d) const {
        const std::vector<ActorStep>& st = a.steps;
        int nummer = 0;
        for (std::size_t i = 0; i < st.size(); ++i) {
            const ActorStep& s = st[i];
            Clip c;
            c.startMs = s.startMs;
            c.endMs = s.endMs;
            c.pfad = s.path;
            c.fremd = s.fremd;
            c.wer = a.name;
            c.text = s.text;
            const Node* n = knoten(s.path);
            if (n != nullptr) { c.befehl = befehlsText(*n); }
            switch (s.kind) {
                case ActorStep::Kind::WalkTo: {
                    if (!(s.endMs > s.startMs)) { continue; }
                    // Ein Weg aus mehreren Stuecken (anderes Tempo unterwegs,
                    // scene.cpp) ist EIN Lauf - vom ersten bis zum letzten.
                    bool fortsetzung = false;
                    for (std::size_t j = 0; j < i; ++j) {
                        fortsetzung = fortsetzung || (st[j].kind == ActorStep::Kind::WalkTo && st[j].teilweg &&
                                                      st[j].path == s.path && std::fabs(st[j].endMs - s.startMs) < 1.0);
                    }
                    if (fortsetzung) { continue; }
                    std::copy(std::begin(s.to), std::end(s.to), std::begin(c.nach));
                    for (std::size_t j = i + 1; j < st.size(); ++j) {
                        if (st[j].kind == ActorStep::Kind::WalkTo && st[j].path == s.path &&
                            std::fabs(st[j].startMs - c.endMs) < 1.0) {
                            c.endMs = st[j].endMs;
                            std::copy(std::begin(st[j].to), std::end(st[j].to), std::begin(c.nach));
                        }
                    }
                    std::copy(std::begin(s.from), std::end(s.from), std::begin(c.von));
                    c.art = Art::Laufen;
                    c.nummer = ++nummer;
                    // text traegt beim Weg die Gangart (BOTH_WALK1/BOTH_RUN1),
                    // das Ziel steht im SET_NAVGOAL.
                    c.gang = s.text;
                    c.text = (n != nullptr && n->args.size() >= 2) ? bloss(n->args[1].text) : std::string();
                    break;
                }
                case ActorStep::Kind::Glide:
                    if (!(s.endMs > s.startMs)) { continue; }
                    c.art = Art::Gleiten;
                    std::copy(std::begin(s.from), std::end(s.from), std::begin(c.von));
                    std::copy(std::begin(s.to), std::end(s.to), std::begin(c.nach));
                    break;
                case ActorStep::Kind::Turn:
                    if (!(s.endMs > s.startMs)) { continue; }
                    c.art = Art::Drehen;
                    std::copy(std::begin(s.fromAng), std::end(s.fromAng), std::begin(c.von));
                    std::copy(std::begin(s.toAng), std::end(s.toAng), std::begin(c.nach));
                    break;
                case ActorStep::Kind::Roff:
                    if (!(s.endMs > s.startMs)) { continue; }
                    c.art = Art::Roff;
                    std::copy(std::begin(s.from), std::end(s.from), std::begin(c.von));
                    break;
                default:
                    continue;
            }
            c.verschiebbar = !c.pfad.empty() && c.fremd.empty();
            d.laufen.clips.push_back(std::move(c));
        }
    }

    void animationen(const Actor& a, Darsteller& d) const {
        const std::vector<ActorStep>& st = a.steps;
        for (std::size_t i = 0; i < st.size(); ++i) {
            const ActorStep& s = st[i];
            if (s.kind != ActorStep::Kind::Anim || s.text.empty()) { continue; }
            Clip c;
            c.art = Art::Animation;
            c.startMs = s.startMs;
            c.endMs = s.endMs;
            if (!(c.endMs > c.startMs + 0.5)) {
                // Ohne bekannte Laenge (Haltezeit -1): bis zur naechsten
                // Animation desselben Teils.
                c.endMs = s_.dauerMs;
                for (std::size_t j = i + 1; j < st.size(); ++j) {
                    if (st[j].kind == ActorStep::Kind::Anim && st[j].part == s.part && st[j].startMs > s.startMs) {
                        c.endMs = st[j].startMs;
                        break;
                    }
                }
            }
            c.text = s.text;
            c.teil = s.part;
            c.pfad = s.path;
            c.fremd = s.fremd;
            c.wer = a.name;
            if (const Node* n = knoten(s.path); n != nullptr) { c.befehl = befehlsText(*n); }
            c.verschiebbar = !c.pfad.empty() && c.fremd.empty();
            d.animation.clips.push_back(c);
            // In der Uebersicht nur, was etwas aussagt: Stehen, Idle und das
            // blosse Sitzen sind die Grundhaltung.
            const std::string g = gross(s.text);
            const bool sitzen = g.rfind("BOTH_SIT", 0) == 0 &&
                                std::all_of(g.begin() + 8, g.end(), [](char ch) { return std::isdigit(static_cast<unsigned char>(ch)) != 0; });
            if (g.find("STAND") == std::string::npos && g.find("IDLE") == std::string::npos && !sitzen) {
                d.oben.clips.push_back(std::move(c));
            }
        }
    }

    void sichtbarkeit(const Actor& a, Darsteller& d) const {
        // Sichtbar ist, was Actor::at sagt - abgefragt nur dort, wo es sich
        // aendern kann (Show/Hide/Gone, Kamerazeiten, Erscheinen).
        std::vector<double> t{0.0};
        for (const ActorStep& s : a.steps) {
            if (s.kind == ActorStep::Kind::Show || s.kind == ActorStep::Kind::Hide || s.kind == ActorStep::Kind::Gone) {
                t.push_back(s.startMs);
            }
        }
        for (const auto& [von, bis] : a.kameraVerborgen) {
            t.push_back(von);
            if (bis >= 0.0) { t.push_back(bis); }
        }
        if (q_.ablauf != nullptr) {
            const auto it = q_.ablauf->erscheint.find(a.name);
            if (it != q_.ablauf->erscheint.end()) { t.push_back(it->second); }
        }
        std::sort(t.begin(), t.end());
        t.erase(std::unique(t.begin(), t.end(), [](double x, double y) { return std::fabs(x - y) < 0.001; }), t.end());
        std::vector<std::pair<double, double>> sichtbar;
        for (std::size_t i = 0; i < t.size(); ++i) {
            if (t[i] > s_.dauerMs) { break; }
            const double bis = i + 1 < t.size() ? std::min(t[i + 1], s_.dauerMs) : s_.dauerMs;
            const ActorState z = a.at(t[i]);
            const bool an = z.visible && !z.removed;
            if (an) {
                if (!sichtbar.empty() && std::fabs(sichtbar.back().second - t[i]) < 0.001) {
                    sichtbar.back().second = bis;
                } else {
                    sichtbar.emplace_back(t[i], bis);
                }
            } else if (bis > t[i]) {
                Clip c;
                c.art = Art::Unsichtbar;
                c.startMs = t[i];
                c.endMs = bis;
                c.wer = a.name;
                if (!d.unsichtbar.clips.empty() && std::fabs(d.unsichtbar.clips.back().endMs - c.startMs) < 0.001) {
                    d.unsichtbar.clips.back().endMs = bis;
                } else {
                    d.unsichtbar.clips.push_back(std::move(c));
                }
            }
        }
        d.sichtbarIrgendwann = !sichtbar.empty();
        // Steht still: sichtbar und nicht unterwegs, mindestens 300 ms.
        for (const auto& [von, bis] : sichtbar) {
            double ab = von;
            for (const Clip& l : d.laufen.clips) {
                if (l.endMs <= ab || l.startMs >= bis) { continue; }
                if (l.startMs - ab >= 300.0) { stehen(d, a.name, ab, l.startMs); }
                ab = std::max(ab, l.endMs);
            }
            if (bis - ab >= 300.0) { stehen(d, a.name, ab, bis); }
        }
    }

    static void stehen(Darsteller& d, const std::string& wer, double von, double bis) {
        Clip c;
        c.art = Art::Stehen;
        c.startMs = von;
        c.endMs = bis;
        c.wer = wer;
        d.stehen.clips.push_back(std::move(c));
    }
};

}  // namespace

Spuren baue(const SpurenQuellen& q) {
    if (q.zeitleiste == nullptr || q.kamera == nullptr || q.szene == nullptr) { return {}; }
    return Bauer(q).baue();
}

float kurveBei(const std::vector<Knick>& k, double ms) {
    if (k.empty()) { return 0.0F; }
    if (ms <= k.front().ms) { return k.front().wert; }
    if (ms >= k.back().ms) { return k.back().wert; }
    // Der letzte Knick mit ms <= Zeit (bei einem Sprung der spaetere Wert).
    const auto it = std::upper_bound(k.begin(), k.end(), ms, [](double m, const Knick& x) { return m < x.ms; });
    const Knick& b = *it;
    const Knick& a = *(it - 1);
    if (b.ms - a.ms < 0.001) { return b.wert; }
    const double f = (ms - a.ms) / (b.ms - a.ms);
    return static_cast<float>(static_cast<double>(a.wert) + (static_cast<double>(b.wert) - static_cast<double>(a.wert)) * f);
}

std::string anzeigeName(const std::string& npcType, const std::string& name) {
    std::string t = npcType;
    if (klein(t).rfind("md_", 0) == 0) { t = t.substr(3); }
    std::replace(t.begin(), t.end(), '_', ' ');
    while (!t.empty() && t.front() == ' ') { t.erase(t.begin()); }
    if (t.empty()) { return name; }
    t[0] = static_cast<char>(std::toupper(static_cast<unsigned char>(t[0])));
    return t;
}

bool nachgebildet(const Node& n) {
    return nachgebildetName(n.name, n.args.empty() ? std::string() : bloss(n.args[0].text));
}

void bahnenVerteilen(Zeile& z, double mindestMs) {
    std::vector<std::size_t> reihe(z.clips.size());
    for (std::size_t i = 0; i < reihe.size(); ++i) { reihe[i] = i; }
    std::stable_sort(reihe.begin(), reihe.end(),
                     [&z](std::size_t a, std::size_t b) { return z.clips[a].startMs < z.clips[b].startMs; });
    std::vector<double> enden;
    for (const std::size_t i : reihe) {
        Clip& c = z.clips[i];
        const double ende = std::max(c.endMs, c.startMs + mindestMs);
        std::size_t k = 0;
        while (k < enden.size() && enden[k] > c.startMs + 0.5) { ++k; }
        if (k == enden.size()) { enden.push_back(0.0); }
        enden[k] = ende;
        c.bahn = static_cast<int>(k);
    }
    z.bahnen = std::max<int>(1, static_cast<int>(enden.size()));
}

}  // namespace bhed::spuren
