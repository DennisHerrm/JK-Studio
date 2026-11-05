// --- zeitleiste0: echte Daten fuer den Entwurf der neuen Zeitleiste (BHED_EDITORTEST=zeitleiste0) ---
//
// Nur im Erkundungszweig zeitleiste0, aendert nichts am Programm. Laedt
// academy1 (reines JKA, Reiter intro - Luke erklaert) und md_am_sith
// (eingerichteter Spielordner, Movie Duels - das Schiff) und schreibt je Szene:
//   BHED_ZL0_AUS\<szene>.json          alle Spuren, Ereignisse, Kamera, Figuren,
//                                      Mover, Klaenge (Dauer, Untertitel, Pegel)
//   BHED_ZL0_AUS\bilder\<szene>_tNNNNNN.png  Kamerabild alle 2 s (256 px breit)
//   BHED_ZL0_AUS\bilder\<szene>_heute.png    die heutige Zeitleiste (Ausgangslage)
// Die Werte stammen aus denselben Quellen wie die Oberflaeche: g_app->timeline
// (ICARUS-Nachbau, Pfade aufs offene Skript), g_app->camTrack, g_app->scene,
// g_app->moverSim, g_app->ablauf. Untertitel wie CG_CaptionText (cg_text.cpp:
// "<LEVEL>_<dateiname>" im StringEd-Paket strings/english/<level>.str).
namespace zl0 {

using ::bhed::json::Wert;

Wert zahl(double v) { return Wert::alsZahl(std::round(v * 1000.0) / 1000.0); }
Wert text(const std::string& s) { return Wert::alsText(s); }
Wert vek(const float* v, int n = 3) {
    Wert l = Wert::alsListe();
    for (int i = 0; i < n; ++i) { l.liste.push_back(zahl(static_cast<double>(v[i]))); }
    return l;
}
Wert pfad(const Path& p) {
    Wert l = Wert::alsListe();
    for (const std::size_t i : p) { l.liste.push_back(Wert::alsZahl(static_cast<double>(i))); }
    return l;
}

std::string gross(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::toupper(static_cast<unsigned char>(c))); }
    return s;
}

// Zeile im Skriptbaum (alle Bloecke offen, 1 = erster Knoten), -1 unbekannt.
int baumZeile(const std::vector<Node>& ns, const Path& p, std::size_t tiefe, int& zaehler) {
    for (std::size_t i = 0; i < ns.size(); ++i) {
        ++zaehler;
        if (tiefe < p.size() && i == p[tiefe]) {
            if (tiefe + 1 == p.size()) { return zaehler; }
            return baumZeile(ns[i].children, p, tiefe + 1, zaehler);
        }
        int rest = 0;
        std::vector<const Node*> stapel;
        for (const Node& k : ns[i].children) { stapel.push_back(&k); }
        while (!stapel.empty()) {
            const Node* k = stapel.back();
            stapel.pop_back();
            ++rest;
            for (const Node& kk : k->children) { stapel.push_back(&kk); }
        }
        zaehler += rest;
    }
    return -1;
}
int baumZeile(const Path& p) {
    if (p.empty()) { return -1; }
    int z = 0;
    return baumZeile(g_app->doc.script().nodes, p, 0, z);
}

// Herkunft eines Eintrags: welches Skript, welcher Pfad, welche Zeile.
void herkunft(Wert& o, const Path& p, const std::string& fremd) {
    if (!fremd.empty()) {
        o.setze("skript", text(fremd));
    } else if (!p.empty()) {
        o.setze("skript", text(aktiverSkriptPfad()));
        o.setze("pfad", pfad(p));
        o.setze("zeile", Wert::alsZahl(static_cast<double>(baumZeile(p))));
    } else {
        o.setze("skript", text("(Nachbau)"));   // vom ICARUS-Nachbau eingefuegt (Wartestueck)
    }
}

const char* eventArt(TimelineEvent::Kind k) {
    static const char* const n[] = {"camera", "sound", "wait", "set", "move", "other", "use", "saber"};
    const auto i = static_cast<std::size_t>(k);
    return i < std::size(n) ? n[i] : "?";
}
const char* camArt(CamSegment::Kind k) {
    static const char* const n[] = {"move", "pan", "zoom", "roll", "enable", "disable", "fade", "shake", "follow", "path"};
    const auto i = static_cast<std::size_t>(k);
    return i < std::size(n) ? n[i] : "?";
}
const char* schrittArt(ActorStep::Kind k) {
    static const char* const n[] = {"teleport", "walkTo", "face", "anim", "show", "hide", "hold", "look", "crouch",
                                    "watch", "turnRate", "gone", "saber", "fire", "shotSpacing", "glide", "turn",
                                    "roff", "waffe", "saberWahl", "saberFarbe", "klingeSchalter", "handModell",
                                    "verhalten", "saberKollision", "wunschgier"};
    const auto i = static_cast<std::size_t>(k);
    return i < std::size(n) ? n[i] : "?";
}
const char* teilName(AnimPart p) {
    return p == AnimPart::Upper ? "upper" : p == AnimPart::Lower ? "lower" : "both";
}

// Untertitel wie CG_CaptionText: Schluessel "<LEVEL>_<dateiname ohne Endung>"
// (cg_SP_GetStringTextStringWithRetry, stripLevelName aus cg_servercmds.cpp;
// von den Sonderfaellen dort nur der von JKA: YAVIN1B liest auch YAVIN1).
struct Untertitel {
    ::bhed::menue::Texte texte;
    std::vector<std::string> ebenen;
    std::vector<std::string> dateien;
    void laden(const std::string& karte) {
        std::string lv = karte;
        const std::size_t s = lv.find_last_of('/');
        if (s != std::string::npos) { lv = lv.substr(s + 1); }
        const std::size_t p = lv.find('.');
        if (p != std::string::npos) { lv = lv.substr(0, p); }
        ebenen = {gross(lv)};
        if (ebenen[0] == "YAVIN1B") { ebenen.emplace_back("YAVIN1"); }
        for (const std::string& e : ebenen) {
            std::string inhalt;
            std::string archiv;
            const std::string datei = "strings/english/" + e + ".str";
            std::string kl = datei;
            for (char& c : kl) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
            if (readFromArchives(kl, inhalt, &archiv) || readFromArchives(datei, inhalt, &archiv)) {
                texte.ladeStr(datei, inhalt, "ENGLISH");
                dateien.push_back(datei + " (" + archiv + ")");
            }
        }
    }
    [[nodiscard]] std::string fuer(const std::string& klang, std::string* schluessel) const {
        std::string n = klang;
        const std::size_t s = n.find_last_of("/\\");
        if (s == std::string::npos) { return {}; }   // CG_CaptionText: ohne '/' kein Text
        n = n.substr(s + 1);
        const std::size_t p = n.rfind('.');
        if (p != std::string::npos) { n = n.substr(0, p); }
        for (const std::string& e : ebenen) {
            const std::string k = e + "_" + gross(n);
            if (schluessel != nullptr && schluessel->empty()) { *schluessel = k; }
            const auto it = texte.werte.find(k);
            if (it != texte.werte.end()) {
                if (schluessel != nullptr) { *schluessel = k; }
                return it->second;
            }
        }
        return {};
    }
};

// Huellkurve eines Klangs: Spitze je 50 ms in Prozent (hoechstens 30 s).
Wert pegel(const std::string& datei) {
    Wert l = Wert::alsListe();
    const sound::Sound* s = soundFor(datei);
    if (s == nullptr || s->sampleRate <= 0 || s->channels <= 0) { return l; }
    const std::size_t fenster = static_cast<std::size_t>(s->sampleRate / 20) * static_cast<std::size_t>(s->channels);
    if (fenster == 0) { return l; }
    for (std::size_t a = 0; a < s->samples.size() && l.liste.size() < 600; a += fenster) {
        int spitze = 0;
        for (std::size_t i = a; i < std::min(s->samples.size(), a + fenster); ++i) {
            spitze = std::max(spitze, std::abs(static_cast<int>(s->samples[i])));
        }
        l.liste.push_back(Wert::alsZahl(std::round(static_cast<double>(spitze) * 100.0 / 32768.0)));
    }
    return l;
}

// Ein Befehl wie im Skript: "name ( a, b )".
std::string befehlsText(const Node& n) {
    std::string l = n.name + " (";
    for (std::size_t i = 0; i < n.args.size(); ++i) { l += (i == 0 ? " " : ", ") + n.args[i].text; }
    return l + " )";
}

// Ein Argument ohne Anfuehrungszeichen und Rand-Leerzeichen ("\"a b\" " -> "a b").
std::string bloss(std::string t) {
    t.erase(std::remove(t.begin(), t.end(), '"'), t.end());
    while (!t.empty() && t.front() == ' ') { t.erase(t.begin()); }
    while (!t.empty() && t.back() == ' ') { t.pop_back(); }
    return t;
}

// Was ein dowait/waitsignal erwartet: der Task bzw. das Signal und wo es herkommt.
Wert wartetAuf(const TimelineEvent& e) {
    Wert o = Wert::alsObjekt();
    const Node* n = e.path.empty() ? nullptr : nodeAt(g_app->doc.script(), e.path);
    if (n == nullptr || n->kind != Node::Kind::Command) {
        o.setze("befehl", text(e.path.empty() ? "wait (Nachbau)" : "?"));
        return o;
    }
    o.setze("befehl", text(n->name));
    const std::string arg = n->args.empty() ? std::string() : n->args[0].text;
    o.setze("argument", text(arg));
    const std::string name = bloss(arg);
    if (n->name == "dowait") {
        // Der Task-Block gleichen Namens: was darin laeuft, haelt das dowait auf.
        Wert inhalt = Wert::alsListe();
        std::vector<const std::vector<Node>*> offen{&g_app->doc.script().nodes};
        while (!offen.empty()) {
            const std::vector<Node>* ns = offen.back();
            offen.pop_back();
            for (const Node& k : *ns) {
                if (k.name == "task" && !k.args.empty() && bloss(k.args[0].text) == name) {
                    for (const Node& c : k.children) {
                        if (c.kind == Node::Kind::Command) { inhalt.liste.push_back(text(befehlsText(c))); }
                    }
                }
                if (!k.children.empty()) { offen.push_back(&k.children); }
            }
        }
        o.setze("task", text(name));
        o.setze("taskInhalt", inhalt);
    } else if (n->name == "waitsignal" || (n->name == "wait" && !n->args.empty() && n->args[0].kind == Arg::Kind::String)) {
        Wert von = Wert::alsListe();
        for (const TimelineTrack& tr : g_app->timeline.tracks) {
            for (const TimelineEvent& s : tr.events) {
                // Label "signal ( name )" (das Argument steht ohne Anfuehrungszeichen darin).
                const std::size_t zu = s.label.rfind(')');
                if (s.label.rfind("signal (", 0) == 0 && zu != std::string::npos && zu > 8 &&
                    bloss(s.label.substr(8, zu - 8)) == name) {
                    Wert q = Wert::alsObjekt();
                    q.setze("ms", zahl(s.startMs));
                    q.setze("darsteller", text(tr.entity));
                    von.liste.push_back(q);
                }
            }
        }
        o.setze("signal", text(name));
        o.setze("signalVon", von);
    }
    return o;
}

// Wie breit die heutige Zeitleiste ein Ereignis zeichnet (Ausschnitt "Fit").
struct Heute {
    int ereignisse = 0;
    int ohneText = 0;        // zu schmal fuer die Beschriftung (oder Zusammenfassungszeile)
    int dreiPx = 0;          // auf die Mindestbreite von 3 Bildpunkten gezogen
    int zusammenfassung = 0; // in einer zugeklappten Figurenzeile (dort nie Text)
};

Wert szeneJson(const std::string& szene, const std::string& spiel, const Untertitel& ut, Heute& h) {
    Wert w = Wert::alsObjekt();
    const ::bhed::Ablauf& ab = g_app->ablauf;
    const Timeline& tl = g_app->timeline;
    const CameraTrack& ct = g_app->camTrack;
    w.setze("format", text("jkstudio-zeitleiste0"));
    w.setze("version", Wert::alsZahl(1));
    w.setze("szene", text(szene));
    w.setze("spiel", text(spiel));
    w.setze("karte", text(g_app->map.path));
    w.setze("skript", text(aktiverSkriptPfad()));
    w.setze("reiter", text(g_app->shownName));
    w.setze("dauerMs", zahl(tl.durationMs));
    w.setze("praktischesEndeMs", zahl(tl.practicalEndMs));
    w.setze("kameraDauerMs", zahl(ct.durationMs));
    w.setze("ablaufEndeMs", zahl(ab.endeMs));
    w.setze("kartenwechselMs", zahl(ab.kartenwechselMs));
    w.setze("kartenwechselKarte", text(ab.kartenwechselKarte));
    w.setze("missionFailedMs", zahl(ab.missionFailedMs));
    w.setze("traeger", text(ab.traeger));
    {
        Wert l = Wert::alsListe();
        for (std::size_t i = 0; i < ab.skripte.size(); ++i) {
            Wert o = Wert::alsObjekt();
            o.setze("datei", text(ab.skripte[i]));
            if (i < ab.skriptBeginn.size()) { o.setze("beginnMs", zahl(ab.skriptBeginn[i])); }
            l.liste.push_back(o);
        }
        w.setze("skripte", l);
    }
    {
        Wert l = Wert::alsListe();
        for (const std::string& s : ab.hinweise) { l.liste.push_back(text(s)); }
        w.setze("hinweise", l);
        Wert n = Wert::alsListe();
        for (const Diag& d : tl.notes) {
            Wert o = Wert::alsObjekt();
            o.setze("text", text(d.message));
            o.setze("zeile", Wert::alsZahl(static_cast<double>(d.line)));
            n.liste.push_back(o);
        }
        w.setze("anmerkungen", n);
    }
    w.setze("weltMusik", text(weltMusik()));
    {
        Wert l = Wert::alsListe();
        for (const std::string& d : ut.dateien) { l.liste.push_back(text(d)); }
        w.setze("untertitelDateien", l);
    }

    // --- Spuren und Ereignisse (wie die heutige Zeitleiste sie hat) -------------
    const double gesamt = std::max({tl.practicalEndMs, ct.durationMs, 1.0});
    const double pxJeMs = static_cast<double>(std::max(1.0F, g_app->tlFeldW)) / gesamt;
    const double textAb = static_cast<double>(ImGui::GetFontSize()) * 2.0;
    Wert spuren = Wert::alsListe();
    for (const TimelineTrack& tr : tl.tracks) {
        Wert sp = Wert::alsObjekt();
        sp.setze("darsteller", text(tr.entity));
        sp.setze("startMs", zahl(tr.startMs));
        Wert evs = Wert::alsListe();
        for (const TimelineEvent& e : tr.events) {
            Wert o = Wert::alsObjekt();
            o.setze("startMs", zahl(e.startMs));
            o.setze("endMs", zahl(e.endMs));
            o.setze("art", text(eventArt(e.kind)));
            o.setze("befehl", text(e.label.substr(0, e.label.find(" ("))));
            o.setze("beschriftung", text(e.label));
            herkunft(o, e.path, e.fremd);
            if (e.kind == TimelineEvent::Kind::Sound && !e.sound.empty()) {
                Wert t = Wert::alsObjekt();
                const std::string datei = bloss(e.sound);
                t.setze("datei", text(datei));
                t.setze("kanal", text(e.kanal));
                t.setze("dauerMs", zahl(soundLengthMs(datei)));
                std::string schluessel;
                t.setze("untertitel", text(ut.fuer(datei, &schluessel)));
                t.setze("untertitelSchluessel", text(schluessel));
                t.setze("pegel", pegel(datei));
                o.setze("ton", t);
            }
            if (e.kind == TimelineEvent::Kind::Use) {
                const std::string ziel = bloss(e.target);
                bool musik = false;
                const std::string klang = useSoundFor(ziel, &musik);
                o.setze("ziel", text(ziel));
                if (!klang.empty()) {
                    o.setze("klang", text(klang));
                    o.setze("musik", Wert::alsBool(musik));
                }
            }
            if (e.kind == TimelineEvent::Kind::Saber) { o.setze("an", Wert::alsBool(e.on)); }
            if (e.kind == TimelineEvent::Kind::Wait) { o.setze("wartetAuf", wartetAuf(e)); }
            // Heute: gezeichnet wird alles ausser Kamera (Skriptzeile) und waits (aus).
            const bool gezeichnet = !(tr.entity.empty() && e.kind == TimelineEvent::Kind::Camera) &&
                                    e.kind != TimelineEvent::Kind::Wait;
            if (gezeichnet) {
                const double px = (e.endMs - e.startMs) * pxJeMs;
                ++h.ereignisse;
                const bool zus = !tr.entity.empty();   // zugeklappte Figurenzeile
                if (px < 3.0) { ++h.dreiPx; }
                if (zus) { ++h.zusammenfassung; }
                if (zus || px <= textAb) { ++h.ohneText; }
                Wert hh = Wert::alsObjekt();
                hh.setze("breitePx", zahl(std::max(3.0, px)));
                hh.setze("mitText", Wert::alsBool(!zus && px > textAb));
                o.setze("heute", hh);
            }
            evs.liste.push_back(o);
        }
        sp.setze("ereignisse", evs);
        spuren.liste.push_back(sp);
    }
    w.setze("spuren", spuren);

    // --- Kamera: Abschnitte und Proben -------------------------------------------
    Wert kam = Wert::alsObjekt();
    Wert abschn = Wert::alsListe();
    int ueberlappt = 0;
    for (const CamSegment& s : ct.segments) {
        Wert o = Wert::alsObjekt();
        o.setze("art", text(camArt(s.kind)));
        o.setze("startMs", zahl(s.startMs));
        o.setze("endMs", zahl(s.endMs));
        if (s.lerpStartMs >= 0.0) { o.setze("lerpStartMs", zahl(s.lerpStartMs)); }
        o.setze("von", vek(s.from));
        o.setze("nach", vek(s.to));
        if (s.kind == CamSegment::Kind::Fade) {
            o.setze("farbeVon", vek(s.farbeVon, 4));
            o.setze("farbeZu", vek(s.farbeZu, 4));
        }
        if (s.kind == CamSegment::Kind::Pan || s.kind == CamSegment::Kind::Pfad) {
            o.setze("vonWinkel", vek(s.vonWinkel));
            o.setze("zuWinkel", vek(s.zuWinkel));
        }
        if (!s.group.empty()) { o.setze("gruppe", text(s.group)); }
        if (s.kind == CamSegment::Kind::Follow) {
            o.setze("tempo", zahl(static_cast<double>(s.speed)));
            o.setze("initLerp", Wert::alsBool(s.initLerp));
        }
        herkunft(o, s.path, s.fremd);
        if (const Node* n = s.path.empty() ? nullptr : nodeAt(g_app->doc.script(), s.path); n != nullptr) {
            o.setze("befehl", text(befehlsText(*n)));
            // Heute steht im Block args[1] (bei FADE die Startfarbe, daher "0 0 0").
            if (n->args.size() >= 2) { o.setze("heuteText", text(n->args[1].text)); }
        }
        // Wie heute rot: ein Abschnitt derselben Gruppe beginnt, bevor dieser fertig ist
        // (CGCam_Update uebernimmt Ort/Winkel/FOV erst am Ende - cg_camera.cpp).
        if (s.kind == CamSegment::Kind::Move || s.kind == CamSegment::Kind::Pan || s.kind == CamSegment::Kind::Zoom ||
            s.kind == CamSegment::Kind::Roll) {
            const auto schwenk = [](const CamSegment& x) {
                return x.kind == CamSegment::Kind::Pan || (x.kind == CamSegment::Kind::Roll && x.alleAchsen);
            };
            const CamSegment* naechster = nullptr;
            for (const CamSegment& q : ct.segments) {
                const bool gleich = schwenk(s) ? schwenk(q) : q.kind == s.kind;
                if (&q != &s && gleich && q.startMs > s.startMs + 0.5 && (naechster == nullptr || q.startMs < naechster->startMs)) {
                    naechster = &q;
                }
            }
            if (naechster != nullptr && naechster->startMs < s.endMs - 0.5) {
                ++ueberlappt;
                o.setze("ueberlapptAbMs", zahl(naechster->startMs));
            }
        }
        abschn.liste.push_back(o);
    }
    kam.setze("abschnitte", abschn);
    kam.setze("ueberlappungen", Wert::alsZahl(ueberlappt));
    Wert proben = Wert::alsListe();
    const double kamEnde = std::max(ct.durationMs, tl.practicalEndMs);
    for (double ms = 0.0; ms <= kamEnde + 0.5; ms += 100.0) {
        const CameraState st = ct.at(ms);
        Wert o = Wert::alsObjekt();
        o.setze("ms", zahl(ms));
        o.setze("an", Wert::alsBool(st.enabled));
        o.setze("pos", vek(st.pos));
        o.setze("winkel", vek(st.angles));
        o.setze("fov", zahl(static_cast<double>(sichtFovX(st))));
        o.setze("fade", vek(st.fadeColor, 4));
        o.setze("balken", zahl(static_cast<double>(st.bars)));
        if (!st.followGroup.empty()) { o.setze("folgt", text(st.followGroup)); }
        proben.liste.push_back(o);
    }
    kam.setze("proben", proben);
    w.setze("kamera", kam);

    // --- Figuren und Brush-Modelle (Szene) ----------------------------------------
    Wert dar = Wert::alsListe();
    for (const Actor& a : g_app->scene.actors) {
        Wert o = Wert::alsObjekt();
        o.setze("name", text(a.name));
        o.setze("npcType", text(a.npcType));
        o.setze("kartenFigur", Wert::alsBool(a.kartenFigur));
        o.setze("brushModell", Wert::alsZahl(a.brushModel));
        const auto er = ab.erscheint.find(a.name);
        if (er != ab.erscheint.end()) { o.setze("erscheintMs", zahl(er->second)); }
        Wert st = Wert::alsListe();
        for (const ActorStep& s : a.steps) {
            Wert q = Wert::alsObjekt();
            q.setze("art", text(schrittArt(s.kind)));
            q.setze("startMs", zahl(s.startMs));
            q.setze("endMs", zahl(s.endMs));
            if (!s.text.empty()) { q.setze("text", text(s.text)); }
            if (s.kind == ActorStep::Kind::Anim || s.kind == ActorStep::Kind::Hold) {
                q.setze("teil", text(teilName(s.part)));
                q.setze("wert", zahl(static_cast<double>(s.value)));
            }
            if (s.kind == ActorStep::Kind::WalkTo || s.kind == ActorStep::Kind::Glide || s.kind == ActorStep::Kind::Teleport ||
                s.kind == ActorStep::Kind::Roff) {
                q.setze("von", vek(s.from));
                q.setze("nach", vek(s.to));
                if (s.ankunftMs >= 0.0) { q.setze("ankunftMs", zahl(s.ankunftMs)); }
                q.setze("linear", Wert::alsBool(s.linear));
            }
            herkunft(q, s.path, s.fremd);
            st.liste.push_back(q);
        }
        o.setze("schritte", st);
        Wert pr = Wert::alsListe();
        for (double ms = 0.0; ms <= tl.practicalEndMs + 0.5; ms += 250.0) {
            const ActorState z = a.at(ms);
            Wert q = Wert::alsObjekt();
            q.setze("ms", zahl(ms));
            q.setze("pos", vek(z.pos));
            q.setze("gier", zahl(static_cast<double>(z.angles[1])));
            q.setze("anim", text(z.animation));
            if (!z.torsoAnimation.empty()) { q.setze("torso", text(z.torsoAnimation)); }
            q.setze("sichtbar", Wert::alsBool(z.visible && !z.removed));
            q.setze("bewegt", Wert::alsBool(z.moving));
            if (z.saberActive) { q.setze("klinge", Wert::alsBool(true)); }
            if (!z.waffe.empty()) { q.setze("waffe", text(z.waffe)); }
            pr.liste.push_back(q);
        }
        o.setze("zustand", pr);
        dar.liste.push_back(o);
    }
    w.setze("darsteller", dar);

    // --- Mover der Karte (Tueren, Aufzuege, Zuege) und ihre Klaenge ---------------
    Wert mv = Wert::alsListe();
    for (const MoverSim::Mover& m : g_app->moverSim.mover()) {
        Wert stuecke = Wert::alsListe();
        for (const MoverSim::Stueck& s : m.bahn) {
            if (s.dauerMs <= 0.0) { continue; }
            Wert q = Wert::alsObjekt();
            q.setze("abMs", zahl(s.abMs));
            q.setze("dauerMs", zahl(s.dauerMs));
            q.setze("von", vek(s.von));
            q.setze("nach", vek(s.nach));
            q.setze("linear", Wert::alsBool(s.linear));
            stuecke.liste.push_back(q);
        }
        if (stuecke.liste.empty() && m.umschalten.empty() && m.sichtAb.empty()) { continue; }
        Wert o = Wert::alsObjekt();
        o.setze("klasse", text(m.klasse));
        if (m.entity < g_app->map.entities.size()) { o.setze("name", text(g_app->map.entities[m.entity].targetname)); }
        o.setze("modell", Wert::alsZahl(m.modell));
        o.setze("stuecke", stuecke);
        Wert um = Wert::alsListe();
        for (const double t : m.umschalten) { um.liste.push_back(zahl(t)); }
        o.setze("umschaltenMs", um);
        mv.liste.push_back(o);
    }
    w.setze("mover", mv);
    Wert mk = Wert::alsListe();
    for (const MoverKlang& k : g_app->moverSim.klaenge(tl.practicalEndMs)) {
        Wert o = Wert::alsObjekt();
        o.setze("ms", zahl(k.ms));
        o.setze("soundSet", text(k.soundSet));
        o.setze("modell", Wert::alsZahl(k.modell));
        mk.liste.push_back(o);
    }
    w.setze("moverKlaenge", mk);

    // --- use-Kette (auch target_relay/-delay/trigger_*) mit Musik ------------------
    Wert be = Wert::alsListe();
    for (const ::bhed::Ablauf::Benutzung& b : ab.benutzt) {
        Wert o = Wert::alsObjekt();
        o.setze("ms", zahl(b.ms));
        o.setze("name", text(b.name));
        o.setze("ausloeser", text(b.ausloeser));
        bool musik = false;
        const std::string klang = useSoundFor(b.name, &musik);
        if (!klang.empty()) {
            o.setze("klang", text(klang));
            o.setze("musik", Wert::alsBool(musik));
        }
        be.liste.push_back(o);
    }
    w.setze("benutzt", be);
    return w;
}

// Ein Foto (BMP des ganzen Fensters) ausschneiden, verkleinern, als PNG schreiben.
bool alsPng(const std::string& bmpPfad, const ImRect& r, int maxBreite, const std::string& pngPfad) {
    std::ifstream f(::bhed::utf8Pfad(bmpPfad), std::ios::binary);
    const std::string bmp((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    f.close();
    if (bmp.size() < 54) { return false; }
    const auto u32 = [&bmp](std::size_t at) {
        std::uint32_t v = 0;
        for (std::size_t k = 0; k < 4; ++k) { v |= static_cast<std::uint32_t>(static_cast<unsigned char>(bmp[at + k])) << (8U * k); }
        return v;
    };
    const int w = static_cast<int>(u32(18));
    const int hgt = std::abs(static_cast<int>(u32(22)));
    const std::size_t daten = u32(10);
    if (w <= 0 || hgt <= 0 || bmp.size() < daten + static_cast<std::size_t>(w) * static_cast<std::size_t>(hgt) * 4U) { return false; }
    const int x0 = std::clamp(static_cast<int>(r.Min.x), 0, w - 1);
    const int y0 = std::clamp(static_cast<int>(r.Min.y), 0, hgt - 1);
    const int x1 = std::clamp(static_cast<int>(r.Max.x), x0 + 1, w);
    const int y1 = std::clamp(static_cast<int>(r.Max.y), y0 + 1, hgt);
    const auto* q = reinterpret_cast<const unsigned char*>(bmp.data() + daten) +
                    (static_cast<std::size_t>(y0) * static_cast<std::size_t>(w) + static_cast<std::size_t>(x0)) * 4U;
    const std::string png = ::bhed::ki::pngAusBgra(q, x1 - x0, y1 - y0, w * 4, maxBreite);
    if (png.empty()) { return false; }
    // rc610 R1: das Bild der heutigen Leiste kommt vor dem JSON-Schritt, der den Ordner bisher erst anlegte - bei der
    // ersten Szene fehlte "bilder" noch, das PNG der ersten Leiste wurde nie geschrieben.
    std::error_code ecOrdner;
    std::filesystem::create_directories(::bhed::utf8Pfad(pngPfad).parent_path(), ecOrdner);
    std::ofstream o(::bhed::utf8Pfad(pngPfad), std::ios::binary);
    o.write(png.data(), static_cast<std::streamsize>(png.size()));
    std::error_code ec;
    std::filesystem::remove(::bhed::utf8Pfad(bmpPfad), ec);
    return o.good();
}

// Schritt: Foto anfordern, warten bis es da ist, ausschneiden und als PNG ablegen.
Schritt bild(const std::string& name, std::function<ImRect()> rahmen, int maxBreite, std::shared_ptr<int> zaehler) {
    return {"zeitleiste0: Bild " + name, [=](int b) {
                const char* aus = std::getenv("BHED_ZL0_AUS");
                if (aus == nullptr) { return true; }
                const std::string bmpPfad = platform::executableDirectory() + "\\zl0_" + name + ".bmp";
                if (b == 1) { render::fotoAnfordern(bmpPfad); }
                if (b < 4) { return false; }
                std::error_code ec;
                const auto groesse = std::filesystem::file_size(::bhed::utf8Pfad(bmpPfad), ec);
                if (ec || groesse <= 54) {
                    if (b > 200) {
                        meldeFehler("zeitleiste0: Foto " + name + " kam nicht");
                        return true;
                    }
                    return false;
                }
                const std::string png = std::string(aus) + "\\bilder\\" + name + ".png";
                if (alsPng(bmpPfad, rahmen(), maxBreite, png)) {
                    ++*zaehler;
                } else {
                    meldeFehler("zeitleiste0: PNG " + name + " nicht geschrieben");
                }
                return true;
            }};
}

// Wann ein Kamerabild entsteht: alle 2 s bis zum praktischen Ende der Kamerafahrt.
std::vector<double> bildZeiten() {
    std::vector<double> z;
    const double ende = std::min(std::max(g_app->camTrack.durationMs, 1.0), g_app->timeline.practicalEndMs);
    for (double t = 0.0; t <= ende + 0.5; t += 2000.0) { z.push_back(t); }
    return z;
}

struct Stand {
    std::string jka;
    std::string md;
    std::map<std::string, int> bilder;   // Szene -> geschriebene Bilder
};

// Eine geladene Szene ausschreiben: JSON, die heutige Leiste, Kamerabilder alle 2 s.
std::vector<Schritt> szeneSchritte(const std::string& szene, const std::string& spiel, std::shared_ptr<Stand> stand) {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto zaehler = std::make_shared<int>(0);
    add(tu("zeitleiste0: " + szene + " - Ansicht wie beim Anwender (Spuren an, Zeiger auf 0)", [] {
        g_app->playing = false;
        g_app->throughCamera = false;
        g_app->playMs = 0.0;
        g_app->timelineZoom = 1.0;
        g_app->timelineViewStartMs = 0.0;
        g_app->zlDetail = false;
        g_app->mapDirty = true;
    }));
    add(pause(1.0));
    add(bild(szene + "_heute", [] {
            const ImVec2 d = ImGui::GetIO().DisplaySize;
            return ImRect{0.0F, g_app->kartenAnsicht[3], d.x, d.y};
        }, 1400, zaehler));
    add(tu("zeitleiste0: " + szene + " - JSON schreiben", [=] {
        const char* aus = std::getenv("BHED_ZL0_AUS");
        if (aus == nullptr) {
            meldeFehler("zeitleiste0: BHED_ZL0_AUS fehlt");
            return;
        }
        std::error_code ec;
        std::filesystem::create_directories(::bhed::utf8Pfad(std::string(aus) + "\\bilder"), ec);
        Untertitel ut;
        ut.laden(g_app->map.path);
        Heute h;
        const auto t0 = std::chrono::steady_clock::now();
        Wert w = szeneJson(szene, spiel, ut, h);
        const double msJson = std::chrono::duration<double, std::milli>(std::chrono::steady_clock::now() - t0).count();
        // Ausgangslage der heutigen Leiste (Messung im gezeichneten Bild davor).
        Wert hw = Wert::alsObjekt();
        hw.setze("panelHoehePx", zahl(static_cast<double>(g_app->tlHoeheIst)));
        hw.setze("zeilenHoehePx", zahl(static_cast<double>(g_app->tlZeilenH)));
        hw.setze("zeitachseBreitePx", zahl(static_cast<double>(g_app->tlFeldW)));
        hw.setze("zeilenGezeichnet", Wert::alsZahl(static_cast<double>(g_app->tlZeilenY.size())));
        int ruhig = 0;
        for (const TimelineTrack& tr : g_app->timeline.tracks) {
            // "ruhig" wie in der alten Leiste: nach dem Anfang passiert nichts mehr
            bool still = true;
            for (const TimelineEvent& e : tr.events) { still = still && e.startMs <= 1.0 && e.endMs <= 1.0; }
            ruhig += still ? 1 : 0;
        }
        hw.setze("spuren", Wert::alsZahl(static_cast<double>(g_app->timeline.tracks.size())));
        hw.setze("ruhigeSpurenAusgeblendet", Wert::alsZahl(ruhig));
        hw.setze("ereignisseGezeichnet", Wert::alsZahl(h.ereignisse));
        hw.setze("ohneBeschriftung", Wert::alsZahl(h.ohneText));
        hw.setze("schmalerAls3Px", Wert::alsZahl(h.dreiPx));
        hw.setze("inZusammenfassungszeilen", Wert::alsZahl(h.zusammenfassung));
        hw.setze("kameraUeberlappungenRot", Wert::alsZahl(g_app->tlUeberlappungen));
        hw.setze("schriftPx", zahl(static_cast<double>(ImGui::GetFontSize())));
        w.setze("heute", hw);
        Wert bl = Wert::alsListe();
        for (const double t : bildZeiten()) {
            Wert o = Wert::alsObjekt();
            char n[64];
            std::snprintf(n, sizeof(n), "%s_t%06d.png", szene.c_str(), static_cast<int>(t));
            o.setze("ms", zahl(t));
            o.setze("datei", text(std::string("bilder/") + n));
            bl.liste.push_back(o);
        }
        w.setze("bilder", bl);
        w.setze("bildHeute", text("bilder/" + szene + "_heute.png"));
        const std::string json = ::bhed::json::schreibe(w);
        std::ofstream f(::bhed::utf8Pfad(std::string(aus) + "\\" + szene + ".json"), std::ios::binary);
        f << json;
        int toene = 0;
        int mitDauer = 0;
        int mitText = 0;
        for (const TimelineTrack& tr : g_app->timeline.tracks) {
            for (const TimelineEvent& e : tr.events) {
                if (e.kind != TimelineEvent::Kind::Sound || e.sound.empty()) { continue; }
                const std::string d = bloss(e.sound);
                ++toene;
                mitDauer += soundLengthMs(d) > 0.0 ? 1 : 0;
                mitText += ut.fuer(d, nullptr).empty() ? 0 : 1;
            }
        }
        char z[600];
        std::snprintf(z, sizeof(z),
                      "%s: %zu Spuren (%d ruhig), %d gezeichnete Ereignisse, davon %d ohne Beschriftung (%d schmaler als 3 px, %d in "
                      "Zusammenfassungszeilen), %d rote Ueberlappungen; Panel %.0f px, Zeile %.0f px, %zu Zeilen; %zu Kameraabschnitte, "
                      "%zu Figuren; Klaenge %d (Dauer %d, Untertitel %d); JSON %zu Bytes in %.1f ms",
                      szene.c_str(), g_app->timeline.tracks.size(), ruhig, h.ereignisse, h.ohneText, h.dreiPx, h.zusammenfassung,
                      g_app->tlUeberlappungen, static_cast<double>(g_app->tlHoeheIst), static_cast<double>(g_app->tlZeilenH),
                      g_app->tlZeilenY.size(), g_app->camTrack.segments.size(), g_app->scene.actors.size(), toene, mitDauer, mitText,
                      json.size(), msJson);
        diag::detail(std::string("Knopftest: zeitleiste0 - ") + z);
        for (const std::string& d : ut.dateien) { diag::detail("Knopftest: zeitleiste0 - Untertitel aus " + d); }
    }));
    add(pruefe("zeitleiste0: " + szene + " - JSON geschrieben, Spuren, Kamera und Klaenge mit Dauer", [=] {
        const char* aus = std::getenv("BHED_ZL0_AUS");
        if (aus == nullptr) { return false; }
        std::ifstream f(::bhed::utf8Pfad(std::string(aus) + "\\" + szene + ".json"), std::ios::binary);
        const std::string inhalt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        Wert w;
        std::string fehler;
        if (!::bhed::json::lies(inhalt, w, &fehler)) {
            diag::detail("Knopftest: zeitleiste0 - JSON unlesbar: " + fehler);
            return false;
        }
        const Wert* sp = w.feld("spuren");
        const Wert* ka = w.feld("kamera");
        const Wert* ab = ka == nullptr ? nullptr : ka->feld("abschnitte");
        bool tonMitDauer = false;
        for (const Wert& t : sp == nullptr ? std::vector<Wert>{} : sp->liste) {
            const Wert* evs = t.feld("ereignisse");
            for (const Wert& e : evs == nullptr ? std::vector<Wert>{} : evs->liste) {
                if (const Wert* ton = e.feld("ton"); ton != nullptr && ton->zahlVon("dauerMs") > 0.0) { tonMitDauer = true; }
            }
        }
        return sp != nullptr && !sp->liste.empty() && ab != nullptr && !ab->liste.empty() && tonMitDauer;
    }));
    // Kamerabilder: durch die Kamera, alle 2 s bis zum praktischen Ende der Kamerafahrt.
    add(tu("zeitleiste0: " + szene + " - Bilderfolge vorbereiten", [=] {
        std::vector<Schritt> folge;
        for (const double t : bildZeiten()) {
            char n[64];
            std::snprintf(n, sizeof(n), "%s_t%06d", szene.c_str(), static_cast<int>(t));
            folge.push_back(tu(std::string("zeitleiste0: zu ") + std::to_string(static_cast<int>(t)) + " ms", [=] {
                g_app->playing = false;
                g_app->throughCamera = true;
                g_app->playMs = t;
                g_app->mapDirty = true;
            }));
            folge.push_back(pause(0.5));
            folge.push_back(bild(n, [] { return hb::karte(); }, 256, zaehler));
        }
        folge.push_back(tu("zeitleiste0: " + szene + " - Bilder fertig", [=] {
            stand->bilder[szene] = *zaehler;
            g_app->throughCamera = false;
            g_app->playMs = 0.0;
            g_app->mapDirty = true;
            diag::detail("Knopftest: zeitleiste0 - " + szene + ": " + std::to_string(*zaehler) + " Bilder");
        }));
        folge.push_back(pruefe("zeitleiste0: " + szene + " - Kamerabilder und Bild der heutigen Leiste geschrieben",
                               [=] { return stand->bilder[szene] >= 2; }));
        schritteEinfuegen(std::move(folge));
    }));
    return s;
}

}  // namespace zl0

std::vector<Schritt> zeitleiste0Schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<zl0::Stand>();
    add(tu("zeitleiste0: Ausgangslage (reines JKA, eingerichtete Spielordner)", [=] {
        const char* e = std::getenv("BHED_JKA_BASIS");
        st->jka = e != nullptr ? e : "C:\\Program Files (x86)\\Steam\\steamapps\\common\\Jedi Academy\\GameData original\\base";
        for (const std::string& o : g_app->settings.gamePaths) {
            std::error_code ec;
            if (st->md.empty() && std::filesystem::is_regular_file(::bhed::utf8Pfad(o + "\\assets1.pk3"), ec)) { st->md = o + "\\assets1.pk3"; }
        }
        g_app->leftMode = 1;
        g_app->missionPickOpen = false;
        diag::detail("Knopftest: zeitleiste0 - reines JKA \"" + st->jka + "\", eingerichtet \"" + st->md + "\"");
    }));
    // academy1 / intro (reines JKA): Luke erklaert.
    add(tu("zeitleiste0: Load mission (JKA)", [=] { missionenAusArchiv(st->jka + "\\assets1.pk3"); }));
    add(warteBis("zeitleiste0: academy1 in der Auswahl", [] { return missionsliste::hat("academy1", true); }, 3500));
    add(waehleInListe("academy1", false, 0, "##missionpick"));
    add(warteBis("zeitleiste0: academy1 geladen", [] {
        return g_app->map.path == "maps/academy1.bsp" && g_app->camTrackValid && g_app->missionAktiv;
    }, 3000));
    add(tu("zeitleiste0: Reiter academy1/intro", [] { g_app->tabRequest = mission1::reiterMit("intro"); }));
    add(pause(1.0));
    add(warteBis("zeitleiste0: Szene von intro mit Figuren", [] {
        return g_app->camTrackValid && mission1::klein(g_app->shownName).find("intro") != std::string::npos &&
               jk2probe::figurenGeladen();
    }, 1500));
    for (Schritt& x : zl0::szeneSchritte("academy1_intro", "JKA", st)) { s.push_back(std::move(x)); }
    // md_am_sith (Movie Duels): das Schiff.
    add(tu("zeitleiste0: Load mission (Movie Duels)", [=] {
        mission1::ungespeicherteEntitiesVergessen();
        missionenAusArchiv(st->md);
    }));
    add(warteBis("zeitleiste0: md_am_sith in der Auswahl", [] { return missionsliste::hat("md_am_sith", true); }, 3500));
    add(waehleInListe("md_am_sith", false, 0, "##missionpick"));
    add(warteBis("zeitleiste0: md_am_sith geladen", [] {
        return g_app->map.path == "maps/md_am_sith.bsp" && g_app->camTrackValid && g_app->tabs.size() > 1;
    }, 3000));
    add(pause(1.0));
    add(warteBis("zeitleiste0: Szene von md_am_sith mit Figuren", [] { return g_app->camTrackValid && jk2probe::figurenGeladen(); }, 1500));
    for (Schritt& x : zl0::szeneSchritte("md_am_sith", "MD", st)) { s.push_back(std::move(x)); }
    return s;
}
