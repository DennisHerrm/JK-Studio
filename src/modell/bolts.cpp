// modell/bolts.cpp - Bolts anlegen, ausrichten, pruefen (CW-MODELL, 15.11.2026) - siehe bhed/modell/bolts.h
#include "bhed/modell/bolts.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <map>

#include "bhed/glm.h"
#include "bhed/modell/g2.h"

namespace bhed::modell {
namespace {

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

std::size_t uz(int i) { return static_cast<std::size_t>(i); }

bool istBolt(const Teil& t) { return t.istTag() || (!t.name.empty() && t.name[0] == '*'); }

// Ein Netz mit dem Dreieck des Bolts, alle Ecken mit Gewicht 1 an `knochen`
Netz boltNetz(const float p[3][3], int knochen) {
    std::vector<Ecke> e(3);
    for (std::size_t i = 0; i < 3; ++i) {
        for (std::size_t k = 0; k < 3; ++k) { e[i].ort[k] = p[i][k]; }
        e[i].normale[2] = 1.0F;
        e[i].knochen[0] = static_cast<std::uint8_t>(std::clamp(knochen, 0, 255));
        e[i].gewicht[0] = 1.0F;
    }
    Netz n;
    n.aufbauen(std::move(e), {0, 1, 2});
    return n;
}

// Die Flaeche (kein Bolt), deren Ecken in Stufe 0 das meiste Gewicht an `knochen` tragen
int elternFuer(const Inhalt& in, int knochen) {
    int best = -1;
    double bestSumme = 0.0;
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const Teil& t = in.teile[i];
        if (istBolt(t) || t.stufen.empty()) { continue; }
        double s = 0.0;
        for (const Ecke& e : t.stufen[0].ecken) {
            for (std::size_t k = 0; k < 4; ++k) {
                if (e.knochen[k] == knochen) { s += e.gewicht[k]; }
            }
        }
        if (s > bestSumme) {
            bestSumme = s;
            best = static_cast<int>(i);
        }
    }
    return best;
}

void kinderNeu(Inhalt& in) {
    for (Teil& t : in.teile) { t.kinder.clear(); }
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        const int p = in.teile[i].eltern;
        if (p >= 0 && uz(p) < in.teile.size()) { in.teile[uz(p)].kinder.push_back(static_cast<int>(i)); }
    }
}

}  // namespace

std::vector<BoltVorlage> boltVorlagen(Zielspiel s) {
    const bool md = s == Zielspiel::MdSp || s == Zielspiel::MdMp;
    const bool mp = s == Zielspiel::JkaMp || s == Zielspiel::MdMp;
    // MAX_VEHICLE_MUZZLES / MAX_VEHICLE_EXHAUSTS: OpenJK code/game/g_vehicles.h:99/102 (10 / 4), Movie Duels Update 9
    // code/game/g_vehicles.h:103-104 (12 / 12), MP codemp/game/bg_vehicles.h:104-105 (Update 9: 12 / 12)
    const int muendungen = md ? 12 : 10;
    const int auspuffe = md ? 12 : 4;
    std::vector<BoltVorlage> v;
    v.push_back({"*driver", BoltGruppe::Fahrzeug,
                 md ? "MD9 code/game/g_client.cpp:1728" : "OpenJK code/game/g_client.cpp:1210, g_vehicles.cpp:298, codemp/game/g_vehicles.c:135",
                 true, false});
    v.push_back({"*droidunit", BoltGruppe::Fahrzeug, md ? "MD9 code/game/g_client.cpp:1731" : "OpenJK code/game/g_client.cpp:1213, codemp/game/g_client.c:1857", false, false});
    for (int i = 1; i <= muendungen; ++i) {
        v.push_back({"*muzzle" + std::to_string(i), BoltGruppe::Fahrzeug,
                     md ? "MD9 code/game/g_client.cpp:1743-1752 (MAX_VEHICLE_MUZZLES 12)" : "OpenJK code/game/g_client.cpp:1224-1234, codemp/game/g_client.c:1869 (MAX_VEHICLE_MUZZLES 10)",
                     false, true});
    }
    for (int i = 1; i <= auspuffe; ++i) {
        v.push_back({"*exhaust" + std::to_string(i), BoltGruppe::Fahrzeug,
                     md ? "MD9 code/game/g_client.cpp:1736-1740 (MAX_VEHICLE_EXHAUSTS 12)" : "OpenJK code/game/g_client.cpp:1217-1222, codemp/game/g_client.c:1862 (MAX_VEHICLE_EXHAUSTS 4)",
                     false, false});
    }
    if (mp) {
        for (int i = 1; i <= 4; ++i) {
            v.push_back({"*trail" + std::to_string(i), BoltGruppe::Fahrzeug,
                         md ? "MD9 codemp/cgame/cg_players.c:17269-17273" : "OpenJK codemp/cgame/cg_players.c:8229-8232", false, false});
        }
    }
    // Rueckfall der Muendungen: *flash1.. wenn *muzzle fehlt
    for (int i = 1; i <= muendungen; ++i) {
        v.push_back({"*flash" + std::to_string(i), BoltGruppe::Fahrzeug, md ? "MD9 code/game/g_client.cpp:1747-1751" : "OpenJK code/game/g_client.cpp:1228-1232", false, true});
    }
    // misc_emplaced_gun / Kanone (OpenJK code/game/g_emplaced.cpp:837-839, :472; MD9 :860)
    v.push_back({"*seat", BoltGruppe::Geschuetz, md ? "MD9 code/game/g_emplaced.cpp:860" : "OpenJK code/game/g_emplaced.cpp:837", true, false});
    v.push_back({"*flash01", BoltGruppe::Geschuetz, md ? "MD9 code/game/g_emplaced.cpp:861" : "OpenJK code/game/g_emplaced.cpp:838", false, true});
    v.push_back({"*flash02", BoltGruppe::Geschuetz, md ? "MD9 code/game/g_emplaced.cpp:862" : "OpenJK code/game/g_emplaced.cpp:839", false, true});
    v.push_back({"*cannonflash", BoltGruppe::Geschuetz, md ? "MD9 code/game/g_emplaced.cpp:499" : "OpenJK code/game/g_emplaced.cpp:472", false, true});
    // Waffen (Muendung des Weltmodells, NPC_spawn.cpp:668-684, cg_players.cpp:1072)
    v.push_back({"*flash", BoltGruppe::Waffe, md ? "MD9 code/game/NPC_spawn.cpp:1314-1334, code/cgame/cg_players.cpp:1277" : "OpenJK code/game/NPC_spawn.cpp:668-684, code/cgame/cg_players.cpp:1072",
                 false, true});
    // Figuren: was der Spielcode am Spielermodell sucht (bhed/modell/g2.h gesuchteTags)
    for (const std::string& n : gesuchteTags(s, "models/players/_humanoid/_humanoid", "models/players/kyle/model.glm")) {
        if (std::none_of(v.begin(), v.end(), [&](const BoltVorlage& x) { return x.name == n; })) {
            v.push_back({n, BoltGruppe::Figur, mp ? "codemp/cgame/cg_players.c CG_RegisterClientModelname" : "code/game/g_client.cpp G_SetG2PlayerModelInfo", false, false});
        }
    }
    return v;
}

void boltDreieck(const BoneMatrix& lage, float groesse, float aus[3][3]) {
    const float a = std::max(groesse, 0.01F);
    const float l = 2.0F * a;
    for (std::size_t k = 0; k < 3; ++k) {
        const float o = lage.m[k][3];
        aus[2][k] = o;                       // Ecke 2: der Ursprung (MDX_TAG_ORIGIN)
        aus[0][k] = o + a * lage.m[k][0];    // Ecke 0: Ursprung + a X (Seite 2 = Ecke 2 -> 0, die kuerzeste)
        aus[1][k] = o + l * lage.m[k][1];    // Ecke 1: Ursprung + L Y (Seite 0 = Ecke 0 -> 1, die laengste)
    }
}

bool boltLage(const Teil& t, BoneMatrix& lage) {
    if (!istBolt(t) || t.stufen.empty() || t.stufen[0].ecken.size() < 3U) { return false; }
    GlmModel m;
    GlmSurface s;
    s.name = t.name;
    s.flags = GlmSurface::kFlagBolt;
    for (std::size_t i = 0; i < 3; ++i) {
        GlmVertex v;
        for (std::size_t k = 0; k < 3; ++k) { v.xyz[k] = t.stufen[0].ecken[i].ort[k]; }
        s.verts.push_back(v);
    }
    s.indexes = {0, 1, 2};
    m.surfaces.push_back(std::move(s));
    return boltMatrixRigid(m, 0, lage);
}

bool boltNameFrei(const Inhalt& in, const std::string& name, int ausser) {
    if (name.size() < 2U || name.size() > 63U || name[0] != '*') { return false; }
    for (const char c : name) {
        if (std::isspace(static_cast<unsigned char>(c)) != 0 || c == '"' || c == ',') { return false; }
    }
    const std::string k = klein(name);
    for (std::size_t i = 0; i < in.teile.size(); ++i) {
        if (static_cast<int>(i) != ausser && klein(in.teile[i].name) == k) { return false; }
    }
    return true;
}

int boltAnlegen(Inhalt& in, const std::string& name, int knochen, const BoneMatrix& lage, float groesse) {
    if (in.herkunft != Herkunft::Glm || !boltNameFrei(in, name)) { return -1; }
    float p[3][3];
    boltDreieck(lage, groesse, p);
    Teil t;
    t.name = name;
    t.flaggen = GlmSurface::kFlagBolt;
    t.eltern = elternFuer(in, knochen);
    const Netz n = boltNetz(p, knochen);
    for (int s = 0; s < std::max(in.stufenZahl, 1); ++s) { t.stufen.push_back(n); }
    in.teile.push_back(std::move(t));
    kinderNeu(in);
    ++in.netzStand;
    return static_cast<int>(in.teile.size()) - 1;
}

int boltKnochen(const Teil& t) {
    if (t.stufen.empty() || t.stufen[0].ecken.empty()) { return -1; }
    const Ecke& e = t.stufen[0].ecken[0];
    int best = -1;
    float g = 0.0F;
    for (std::size_t k = 0; k < 4; ++k) {
        if (e.gewicht[k] > g) {
            g = e.gewicht[k];
            best = e.knochen[k];
        }
    }
    return best;
}

bool boltLageSetzen(Inhalt& in, int teil, const BoneMatrix& lage) {
    if (teil < 0 || uz(teil) >= in.teile.size() || !istBolt(in.teile[uz(teil)])) { return false; }
    Teil& t = in.teile[uz(teil)];
    // die Groesse bleibt: Laenge der kuerzesten Seite (Ecke 2 -> 0)
    float groesse = 1.0F;
    if (!t.stufen.empty() && t.stufen[0].ecken.size() >= 3U) {
        const Ecke& a = t.stufen[0].ecken[2];
        const Ecke& b = t.stufen[0].ecken[0];
        const float d = std::sqrt((a.ort[0] - b.ort[0]) * (a.ort[0] - b.ort[0]) + (a.ort[1] - b.ort[1]) * (a.ort[1] - b.ort[1]) +
                                  (a.ort[2] - b.ort[2]) * (a.ort[2] - b.ort[2]));
        if (d > 0.01F) { groesse = d; }
    }
    const int knochen = std::max(boltKnochen(t), 0);
    float p[3][3];
    boltDreieck(lage, groesse, p);
    const Netz n = boltNetz(p, knochen);
    for (Netz& s : t.stufen) { s = n; }
    if (t.stufen.empty()) { t.stufen.push_back(n); }
    ++in.netzStand;
    return true;
}

bool boltKnochenSetzen(Inhalt& in, int teil, int knochen) {
    if (teil < 0 || uz(teil) >= in.teile.size() || !istBolt(in.teile[uz(teil)]) || knochen < 0) { return false; }
    Teil& t = in.teile[uz(teil)];
    for (Netz& s : t.stufen) {
        std::vector<Ecke> e = s.ecken;
        for (Ecke& x : e) {
            for (std::size_t k = 0; k < 4; ++k) {
                x.knochen[k] = 0;
                x.gewicht[k] = 0.0F;
            }
            x.knochen[0] = static_cast<std::uint8_t>(std::clamp(knochen, 0, 255));
            x.gewicht[0] = 1.0F;
            x.ausDatei = false;
        }
        const std::vector<std::uint32_t> idx = s.indizes();
        s = Netz{};
        s.aufbauen(std::move(e), idx);
    }
    t.eltern = elternFuer(in, knochen);
    kinderNeu(in);
    ++in.netzStand;
    return true;
}

bool boltUmbenennen(Inhalt& in, int teil, const std::string& name) {
    if (teil < 0 || uz(teil) >= in.teile.size() || !istBolt(in.teile[uz(teil)]) || !boltNameFrei(in, name, teil)) { return false; }
    in.teile[uz(teil)].name = name;
    in.teile[uz(teil)].dateiName.clear();
    ++in.netzStand;
    return true;
}

std::vector<BoltBefundArt> fahrzeugBoltsPruefen(const Inhalt& in) {
    bool fahrer = false;
    bool muendung = false;
    for (const Teil& t : in.teile) {
        const std::string n = klein(t.name);
        fahrer = fahrer || n == "*driver";
        // m_iMuzzleTag: "*muzzle%d", sonst "*flash%d" (Nummer ab 1)
        for (const char* v : {"*muzzle", "*flash"}) {
            const std::string p(v);
            if (n.size() > p.size() && n.compare(0, p.size(), p) == 0 &&
                std::all_of(n.begin() + static_cast<std::ptrdiff_t>(p.size()), n.end(), [](char c) { return std::isdigit(static_cast<unsigned char>(c)) != 0; })) {
                muendung = true;
            }
        }
    }
    std::vector<BoltBefundArt> out;
    if (!fahrer) { out.push_back(BoltBefundArt::KeinFahrer); }
    if (!muendung) { out.push_back(BoltBefundArt::KeineMuendung); }
    return out;
}

}  // namespace bhed::modell
