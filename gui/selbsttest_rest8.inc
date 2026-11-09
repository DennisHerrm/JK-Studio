// selbsttest_rest8.inc - REST8 (BHED_EDITORTEST=karteneditor, BHED_KARTE_NUR=rest8): doppelte Shadernamen im Karten-Editor.
//
//   Echtzeit-Licht: fuer jeden Shader, den bhed_q3map2 kennt (Spielkontext::q3map2, bhed/karte/q3map2shader.h), zeichnet
//         echtShaderInfo die Stufen des Eintrags, den q3map2 nimmt. Gezaehlt wird dazu, wie viele Shader der Weg bis REST8 (das
//         Inventar von NetRadiant-Custom, die Datei zerlegt wie dort) anders gezeichnet haette, und wie viele q3map2 und das Spiel
//         verschieden aufloesen.
//   Kartenpruefung: ein Kasten mit einem Shader, den Bau und Spiel verschieden aufloesen, und einer mit einem, den beide gleich
//         aufloesen - "shadervorrang" meldet genau den ersten.
// Eingebunden in gui/selbsttest.cpp nach selbsttest_rd.inc, aufgerufen aus selbsttest_handbuch.inc.
namespace rest8 {

namespace ke = karteneditor;

// Was das Echtzeit-Licht von einem Shader zeichnet: Zahl der Stufen, je Stufe Bild, Mischung und glow
std::string abdruck(const ShaderInfo* si) {
    if (si == nullptr) { return "-"; }
    std::string a = std::to_string(si->stages.size()) + ":" + si->image;
    for (const ShaderInfo::Stage& st : si->stages) {
        a += "|" + st.image + "," + std::to_string(static_cast<int>(st.srcFactor)) + "," + std::to_string(static_cast<int>(st.dstFactor)) +
             (st.glow ? ",glow" : "") + (st.lightmap ? ",lm" : "");
    }
    return a;
}

struct Stand {
    std::string vorrang;   // ein Shader unter textures/, den q3map2 und das Spiel verschieden aufloesen
    std::string gleich;    // einer, den beide gleich aufloesen
};

std::string karte() { return karteTestPfad("rest8_vorrang.map"); }

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&s](Schritt x) { s.push_back(std::move(x)); };
    auto st = std::make_shared<Stand>();
    add(pruefe("REST8: Echtzeit-Licht zeichnet bei doppelten Shadernamen den Eintrag, den bhed_q3map2 nimmt", [st] {
        const std::shared_ptr<ke::Spielkontext>& k = ke::spielWarten();
        if (!k) { return false; }
        long falsch = 0;
        long alt = 0;
        long q3Spiel = 0;
        std::string beispiele;
        std::string altBeispiele;
        std::map<std::string, ShaderMap> nrcDateien;   // der Weg bis REST8: die Datei des Inventars, ganz zerlegt
        for (const auto& [n, e] : k->q3map2.eintraege) {
            ShaderMap m;
            parseShaderScript(e.text, m);
            const auto it = m.find(n);
            const std::string soll = abdruck(it == m.end() ? nullptr : &it->second);
            if (abdruck(ke::echtShaderInfo(n)) != soll) {
                ++falsch;
                if (beispiele.size() < 400U) { beispiele += " " + n; }
            }
            if (const karte::ShaderEintrag* inv = k->inventar.shaderFinden(n); inv != nullptr && !inv->istStandard()) {
                auto d = nrcDateien.find(inv->datei);
                if (d == nrcDateien.end()) {
                    std::string t;
                    (void)k->fs.textLesen(inv->datei, t);
                    ShaderMap dm;
                    parseShaderScript(t, dm);
                    d = nrcDateien.emplace(inv->datei, std::move(dm)).first;
                }
                const auto a = d->second.find(n);
                if (abdruck(a == d->second.end() ? nullptr : &a->second) != soll) {
                    ++alt;
                    if (altBeispiele.size() < 400U) { altBeispiele += " " + n + " (" + inv->datei + " statt " + e.datei + " " + karte::pfadBlatt(e.quelle) + ")"; }
                }
            }
            const auto sp = k->spielShader.find(n);
            if (sp == k->spielShader.end() || n.rfind("textures/", 0) != 0) { continue; }
            if (!karte::shaderEintraegeGleich(e.text, sp->second)) {
                ++q3Spiel;
                if (st->vorrang.empty()) { st->vorrang = n; }
            } else if (st->gleich.empty()) {
                st->gleich = n;
            }
        }
        diag::detail("Knopftest: REST8 - q3map2 " + std::to_string(k->q3map2.eintraege.size()) + " Namen, " + std::to_string(k->q3map2.verdeckt) +
                     " verdeckt, " + std::to_string(k->q3map2.dateien.size()) + " Dateien" + (k->q3map2.shaderliste ? " (shaderlist.txt)" : "") +
                     ", Abbruch '" + k->q3map2.abbruch + "'; Spiel " + std::to_string(k->spielShader.size()) + " Namen; textures/ verschieden zum Spiel " +
                     std::to_string(q3Spiel) + " (z. B. " + st->vorrang + "), gleich z. B. " + st->gleich);
        diag::detail("Knopftest: REST8 - bis REST8 (Inventar von NetRadiant-Custom) anders gezeichnet: " + std::to_string(alt) + ":" + altBeispiele);
        diag::detail("Knopftest: REST8 - Echtzeit-Licht anders als q3map2: " + std::to_string(falsch) + ":" + beispiele);
        return falsch == 0 && alt > 0 && k->q3map2.abbruch.empty() && !st->vorrang.empty() && !st->gleich.empty();
    }));
    auto laeufe = std::make_shared<int>(0);
    add(tu("REST8: Karte mit zwei Kaesten (verschieden / gleich aufgeloest), Optimierer", [st, laeufe] {
        karte::Karte k;
        k.klassen = &ke::editor().spiel->klassen;
        auto welt = k.weltHolenOderAnlegen();
        r3::quader(k, welt, karte::Vektor3f(0, 0, 0), karte::Vektor3f(64, 64, 8), st->vorrang.c_str());
        r3::quader(k, welt, karte::Vektor3f(0, 0, 128), karte::Vektor3f(64, 64, 8), st->gleich.c_str());
        (void)r3::ent(k, "info_player_start", {{"origin", "0 0 64"}});
        (void)karte::karteSpeichern(karte(), k, false);
        (void)ke::karteOeffnen(karte());
        *laeufe = ke::optimiererBlick().laeufe;
        ke::optimiererStarten();
    }));
    add(stufe5::warteOptimierer("REST8: Optimierer fertig", laeufe));
    add(pruefe("REST8: Kartenpruefung \"shadervorrang\" fuer den verschieden aufgeloesten Shader, nicht fuer den gleichen", [st] {
        long long n = 0;
        const bool da = stufe5::hatBefund("shadervorrang", &n);
        diag::detail("Knopftest: REST8 - " + st->vorrang + " / " + st->gleich + ", Befunde " + stufe5::befundListe());
        return da && n == 1;
    }));
    add(tu("REST8: Optimierer zu, neue Karte", [] {
        ke::optimiererSchliessen();
        ke::editor().karte.undo.alsGespeichertMarkieren();
        ke::neueKarte();
    }));
    return s;
}

}  // namespace rest8
