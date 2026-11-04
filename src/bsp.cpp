#include "bhed/bsp.h"
#include "bhed/diag.h"
#include "bhed/regelwerk.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

namespace bhed {
namespace {

std::int32_t readInt(const std::string& b, std::size_t at) {
    std::uint32_t v = 0;
    for (int i = 3; i >= 0; --i) {
        v = (v << 8) | static_cast<unsigned char>(b[at + static_cast<std::size_t>(i)]);
    }
    std::int32_t out = 0;
    std::memcpy(&out, &v, 4);
    return out;
}

void addUnique(std::vector<std::string>& list, const std::string& value) {
    if (value.empty()) {
        return;
    }
    for (const std::string& v : list) {
        if (v == value) {
            return;
        }
    }
    list.push_back(value);
}

// Die Feldtabellen fields[] der Spiele (nur die Namen; auskommentierte Eintraege
// fehlen wie im Spiel). OJK code/game/g_spawn.cpp:223-371 (Movie Duels 9:
// code/game/g_spawn.cpp:321-471 dieselben und dazu fullName, icon,
// player_victoryscript), JK2 codeJK2/game/g_spawn.cpp:188-313.
constexpr const char* kFelderOjk[] = {
    "autobound", "groupname", "noBasicSounds", "noCombatSounds", "noExtraSounds", "classname", "origin", "mins",
    "maxs", "model", "model2", "model3", "model4", "model5", "model6", "spawnflags", "speed", "duration", "interest",
    "target", "target2", "target3", "target4", "targetJump", "targetname", "material", "message", "team", "mapname",
    "wait", "finaltime", "random", "FOV", "count", "bounceCount", "health", "friction", "light", "dmg", "angles",
    "angle", "modelAngles", "cameraGroup", "radius", "hiderange", "starttime", "turfrange", "type", "fxfile",
    "fxfile2", "noVisTime", "endFrame", "linear", "weapon", "parm1", "parm2", "parm3", "parm4", "parm5", "parm6",
    "parm7", "parm8", "parm9", "parm10", "parm11", "parm12", "parm13", "parm14", "parm15", "parm16", "delay",
    "sounds", "closetarget", "opentarget", "paintarget", "soundGroup", "backwardstarget", "splashDamage",
    "splashRadius", "spawnscript", "usescript", "awakescript", "angerscript", "attackscript", "victoryscript",
    "lostenemyscript", "painscript", "fleescript", "deathscript", "delayscript", "delayscripttime", "blockedscript",
    "ffirescript", "ffdeathscript", "mindtrickscript", "script_targetname", "NPC_targetname", "NPC_target",
    "NPC_target2", "NPC_target4", "NPC_type", "ownername", "saberType", "saberColor", "saberLeftHand", "saberSolo",
    "saberPitch", "startRGBA", "finalRGBA", "soundSet", "mass", "scale", "modelscale", "modelscale_vec", "style",
    "lip", "switch_style", "height", "noise", "gravity", "storyhead", "tier_storyinfo", "zoffset", "music",
    "forcevisible", "redcrosshair", "nodelay",
};
constexpr const char* kFelderJk2[] = {
    "autobound", "groupname", "noBasicSounds", "noCombatSounds", "noExtraSounds", "classname", "origin", "mins",
    "maxs", "model", "model2", "model3", "model4", "model5", "model6", "spawnflags", "speed", "duration", "interest",
    "target", "target2", "target3", "target4", "targetname", "material", "message", "team", "mapname", "wait",
    "finaltime", "random", "FOV", "count", "health", "friction", "light", "dmg", "angles", "angle", "cameraGroup",
    "radius", "hiderange", "starttime", "type", "fullName", "fxfile", "fxfile2", "endFrame", "linear", "parm1",
    "parm2", "parm3", "parm4", "parm5", "parm6", "parm7", "parm8", "parm9", "parm10", "parm11", "parm12", "parm13",
    "parm14", "parm15", "parm16", "noTED", "delay", "sounds", "closetarget", "opentarget", "paintarget", "soundGroup",
    "backwardstarget", "splashDamage", "splashRadius", "spawnscript", "usescript", "awakescript", "angerscript",
    "attackscript", "victoryscript", "lostenemyscript", "painscript", "fleescript", "deathscript", "delayscript",
    "delayscripttime", "blockedscript", "ffirescript", "ffdeathscript", "mindtrickscript", "script_targetname",
    "NPC_targetname", "NPC_target", "NPC_target2", "NPC_target4", "NPC_type", "ownername", "startRGBA", "finalRGBA",
    "soundSet", "scale", "_color", "style", "lip", "switch_style", "height",
};
constexpr const char* kFelderNurMd[] = {"fullName", "icon", "player_victoryscript"};

// Q_stricmpn (OJK code/qcommon/q_shared.cpp): nur 'a'..'z' werden zu Grossbuchstaben - ohne Locale.
inline unsigned char grossAscii(char c) {
    const auto u = static_cast<unsigned char>(c);
    return (u >= 'a' && u <= 'z') ? static_cast<unsigned char>(u - ('a' - 'A')) : u;
}

bool gleichOhneGross(const std::string& a, const char* b) {
    // Q_stricmp: Buchstaben ohne Ruecksicht auf Gross/Klein (q_shared.cpp Q_stricmpn).
    std::size_t i = 0;
    for (; i < a.size() && b[i] != '\0'; ++i) {
        if (grossAscii(a[i]) != grossAscii(b[i])) { return false; }
    }
    return i == a.size() && b[i] == '\0';
}

// Endrunde 1: find() fragt fuer jeden Schluessel die Feldtabelle (rund 140 Namen) - je Bild tausendfach, seit W2b
// gut 1 ms Prozessorzeit je Bild (Messung in NOTIZEN_rc609_endrunde.md). Die Namen nach ihrem ersten Buchstaben
// (ohne Gross/Klein) vorsortiert; verglichen wird wie vorher mit gleichOhneGross, nur mit den Namen desselben Anfangs.
// Gleiches Ergebnis, ein Bruchteil der Vergleiche.
class FeldTabelle {
public:
    template <std::size_t N>
    explicit FeldTabelle(const char* const (&t)[N]) {
        for (const char* f : t) { nachAnfang_[grossAscii(f[0])].push_back(f); }
    }
    [[nodiscard]] bool enthaelt(const std::string& k) const {
        if (k.empty()) { return false; }
        const std::vector<const char*>& kandidaten = nachAnfang_[grossAscii(k[0])];
        return std::any_of(kandidaten.begin(), kandidaten.end(), [&k](const char* f) { return gleichOhneGross(k, f); });
    }

private:
    std::array<std::vector<const char*>, 256> nachAnfang_;
};

const FeldTabelle& felderOjk() {
    static const FeldTabelle t(kFelderOjk);
    return t;
}
const FeldTabelle& felderJk2() {
    static const FeldTabelle t(kFelderJk2);
    return t;
}
const FeldTabelle& felderNurMd() {
    static const FeldTabelle t(kFelderNurMd);
    return t;
}

// COM_ParseExt(data, qtrue) - OJK code/qcommon/q_shared.cpp:281-376, MD9 :299-394.
struct Token {
    bool ende = true;
    bool offen = false;      // Anfuehrungszeichen ohne Ende (COM_Parse liest still bis zum NUL)
    std::string text;
    std::size_t anf = 0;
    std::size_t bis = 0;     // hinter dem Token
};

Token naechstesToken(const std::string& t, std::size_t n, std::size_t& i) {
    Token r;
    for (;;) {
        // SkipWhitespace: (unsigned char) <= ' ' (q_shared.cpp:188, "eurofix")
        while (i < n && static_cast<unsigned char>(t[i]) <= ' ') { ++i; }
        if (i >= n) { return r; }
        if (t[i] == '/' && i + 1 < n && t[i + 1] == '/') {
            i += 2;
            while (i < n && t[i] != '\n') { ++i; }
        } else if (t[i] == '/' && i + 1 < n && t[i + 1] == '*') {
            i += 2;
            while (i < n && (t[i] != '*' || i + 1 >= n || t[i + 1] != '/')) { ++i; }
            if (i < n) { i += 2; }
        } else {
            break;
        }
    }
    r.ende = false;
    r.anf = i;
    if (t[i] == '"') {
        const std::size_t von = ++i;
        while (i < n && t[i] != '"') { ++i; }
        r.text = t.substr(von, std::min(i - von, kMaxTokenZeichen));
        r.offen = i >= n;
        if (i < n) { ++i; }
        r.bis = i;
        return r;
    }
    // Ein Wort: do { ... } while (c > 32) mit int c = *data (char ist signed) -
    // ein Byte ab 0x80 beendet es (das erste Zeichen nimmt es immer).
    const std::size_t von = i;
    do {
        ++i;
    } while (i < n && static_cast<signed char>(t[i]) > 32);
    r.text = t.substr(von, std::min(i - von, kMaxTokenZeichen));
    r.bis = i;
    return r;
}

int zeileVon(const std::string& t, std::size_t pos) {
    return 1 + static_cast<int>(std::count(t.begin(), t.begin() + static_cast<std::ptrdiff_t>(std::min(pos, t.size())), '\n'));
}

// Die Kurzfelder - dieselbe Regel wie find (classname, targetname, origin und
// cameraGroup sind Felder in allen drei Tabellen: der letzte gewinnt).
void kurzfelder(MapEntity& e) {
    const auto wert = [&e](const char* k) {
        const std::string* v = e.find(k);
        return v != nullptr ? *v : std::string();
    };
    e.classname = wert("classname");
    e.targetname = wert("targetname");
    e.cameraGroup = wert("cameraGroup");
    e.origin = wert("origin");
}

}  // namespace


bool entFeldSchluessel(const std::string& key) {
    if (regelwerkJediOutcast()) { return felderJk2().enthaelt(key); }
    return felderOjk().enthaelt(key) || (regelwerkMovieDuels() && felderNurMd().enthaelt(key));
}

int entSchluesselIndex(const MapEntity& e, const std::string& key) {
    const int n = static_cast<int>(e.keys.size());
    if (entFeldSchluessel(key)) {
        // G_ParseField fuer jedes Paar der Reihe nach: der letzte Treffer bleibt stehen.
        for (int i = n - 1; i >= 0; --i) {
            if (gleichOhneGross(e.keys[static_cast<std::size_t>(i)].first, key.c_str())) { return i; }
        }
        return -1;
    }
    // G_SpawnString: der erste Treffer; JK2 vergleicht mit strcmp.
    const bool genau = regelwerkJediOutcast();
    for (int i = 0; i < n; ++i) {
        const std::string& k = e.keys[static_cast<std::size_t>(i)].first;
        if (genau ? k == key : gleichOhneGross(k, key.c_str())) { return i; }
    }
    return -1;
}

const std::string* MapEntity::find(const std::string& key) const {
    // Klassennamen (die WERTE) vergleicht die Engine mit strcmp, also genau -
    // das bleibt Sache der Aufrufer.
    const int i = entSchluesselIndex(*this, key);
    return i < 0 ? nullptr : &keys[static_cast<std::size_t>(i)].second;
}

void setKey(MapEntity& e, const std::string& key, const std::string& value) {
    const auto sauber = [](const std::string& t) {
        std::string r;
        r.reserve(t.size());
        for (const char c : t) {
            if (c != '"' && c != '\n' && c != '\r') { r += c; }
        }
        return r;
    };
    const std::string k = sauber(key);
    const std::string v = sauber(value);
    if (k.empty()) {
        return;
    }
    if (v.empty()) {
        std::erase_if(e.keys, [&k](const auto& kv) { return gleichOhneGross(kv.first, k.c_str()); });
    } else if (const int i = entSchluesselIndex(e, k); i >= 0) {
        e.keys[static_cast<std::size_t>(i)].second = v;
    } else {
        e.keys.emplace_back(k, v);
    }
    kurzfelder(e);
}

std::string writeEntities(const std::vector<MapEntity>& ents) {
    std::string out;
    for (const MapEntity& e : ents) {
        out += "{\n";
        for (const auto& [k, v] : e.keys) {
            out += '"';
            out += k;
            out += "\" \"";
            out += v;
            out += "\"\n";
        }
        out += "}\n";
    }
    return out;
}

bool parseEntitiesGenau(const std::string& text, std::vector<MapEntity>& out, std::vector<EntBlockSpanne>* spannen,
                        EntTextFehler* fehler) {
    out.clear();
    if (spannen != nullptr) { spannen->clear(); }
    const std::size_t n = std::min(text.find('\0'), text.size());
    std::size_t i = 0;
    std::size_t mehrzeilig = std::string::npos;   // in der laufenden Entity
    const auto merke = [&](const Token& tk) {
        if (mehrzeilig == std::string::npos && tk.anf < n && text[tk.anf] == '"' && tk.text.find('\n') != std::string::npos) { mehrzeilig = tk.anf; }
    };
    const auto fehl = [&](EntFehler art, std::size_t pos, const std::string& token) {
        if (fehler != nullptr) {
            fehler->art = art;
            fehler->zeile = zeileVon(text, pos);
            fehler->token = token;
            fehler->zeileMehrzeilig = mehrzeilig == std::string::npos ? 0 : zeileVon(text, mehrzeilig);
        }
        return false;
    };
    for (;;) {
        const Token auf = naechstesToken(text, n, i);
        if (auf.ende) { break; }   // Ende des Texts: fertig
        if (auf.offen) { return fehl(EntFehler::OffenesAnfuehrungszeichen, auf.anf, auf.text); }
        // com_token[0] != '{' - auch "{abc" gilt als Klammer (das Wort danach verschluckt).
        if (auf.text.empty() || auf.text[0] != '{') { return fehl(EntFehler::KlammerErwartet, auf.anf, auf.text); }
        MapEntity e;
        EntBlockSpanne sp;
        sp.anf = auf.anf;
        mehrzeilig = std::string::npos;
        std::size_t zeichen = 0;
        for (;;) {
            const Token k = naechstesToken(text, n, i);
            if (k.ende) { return fehl(EntFehler::OhneSchliessendeKlammer, auf.anf, std::string()); }
            if (k.offen) { return fehl(EntFehler::OffenesAnfuehrungszeichen, k.anf, k.text); }
            merke(k);
            if (!k.text.empty() && k.text[0] == '}') {
                sp.ende = k.bis;
                break;
            }
            const Token w = naechstesToken(text, n, i);
            if (w.ende) { return fehl(EntFehler::OhneSchliessendeKlammer, auf.anf, std::string()); }
            if (w.offen) { return fehl(EntFehler::OffenesAnfuehrungszeichen, w.anf, w.text); }
            merke(w);
            if (!w.text.empty() && w.text[0] == '}') { return fehl(EntFehler::KlammerOhneWert, w.anf, k.text); }
            if (static_cast<int>(e.keys.size()) == kMaxSpawnVars) { return fehl(EntFehler::ZuVielePaare, k.anf, k.text); }
            zeichen += k.text.size() + 1 + w.text.size() + 1;
            if (zeichen > static_cast<std::size_t>(kMaxSpawnVarsChars)) { return fehl(EntFehler::ZuVieleZeichen, k.anf, k.text); }
            e.keys.emplace_back(k.text, w.text);
            sp.paare.push_back({k.anf, k.bis, w.anf, w.bis});
        }
        if (!e.keys.empty()) {
            kurzfelder(e);
            out.push_back(std::move(e));
            if (spannen != nullptr) { spannen->push_back(std::move(sp)); }
        }
    }
    return true;
}

void parseEntities(const std::string& text, std::vector<MapEntity>& out) {
    // Fehler wie im Spiel erkannt, aber nicht abgewiesen: was davor steht, bleibt.
    (void)parseEntitiesGenau(text, out);
}

std::string bspFassungFalsch(const std::string& kennung, std::int32_t fassung) {
    std::string m = kennung + " has wrong version number (" + std::to_string(fassung) + " should be " +
                    std::to_string(kBspFassung) + ")";
    if (kennung == "IBSP" && fassung == 46) { m += " - Quake 3, not Jedi Academy / Jedi Outcast"; }
    return m;
}

bool readBspEntityLump(const std::string& bytes, std::string& text, std::string* error) {
    text.clear();
    // Kopf: Kennung (4) + Fassung (4) + Lumpverzeichnis. Lump 0 ist das
    // Entity-Lump, bei RBSP wie bei IBSP.
    if (bytes.size() < 16) {
        if (error != nullptr) { *error = "zu kurz fuer eine .bsp"; }
        return false;
    }
    const std::string magic = bytes.substr(0, 4);
    if (magic != "RBSP" && magic != "IBSP" && magic != "FBSP") {
        if (error != nullptr) { *error = "unbekannte Kennung: " + magic; }
        return false;
    }
    // rc609 F: nur Fassung 1 wie die Engine (BSP_VERSION; OpenJK code/qcommon/cm_load.cpp:764,
    // code/rd-vanilla/tr_bsp.cpp:1409 "has wrong version number"). Eine IBSP 46 (Quake 3)
    // hat andere Satzgroessen und laedt in JKA, JK2 und Movie Duels nicht.
    if (const std::int32_t fassung = readInt(bytes, 4); fassung != kBspFassung) {
        if (error != nullptr) { *error = bspFassungFalsch(magic, fassung); }
        return false;
    }
    const std::int32_t offset = readInt(bytes, 8);
    const std::int32_t length = readInt(bytes, 12);
    if (offset < 0 || length < 0 ||
        static_cast<std::size_t>(offset) + static_cast<std::size_t>(length) > bytes.size()) {
        if (error != nullptr) { *error = "Entity-Lump liegt ausserhalb der Datei"; }
        return false;
    }
    text = bytes.substr(static_cast<std::size_t>(offset), static_cast<std::size_t>(length));
    return true;
}

bool readBsp(const std::string& bytes, MapData& out, std::string* error) {
    out = MapData{};
    // rc603: Kennung, Lumpverzeichnis und Lump 0 liest readBspEntityLump -
    // dieselbe Pruefung fuer die Missionssuche in Arbeitsfaeden (ohne
    // Protokoll, das hier nicht fadensicher ist).
    // Kennung und Fassung stehen wie bisher auch nach einem Fehlschlag im
    // Ergebnis, soweit sie lesbar waren.
    std::string text;
    const bool lesbar = readBspEntityLump(bytes, text, error);
    if (bytes.size() >= 16) {
        out.magic = bytes.substr(0, 4);
        if (out.magic == "RBSP" || out.magic == "IBSP" || out.magic == "FBSP") {
            out.version = readInt(bytes, 4);
        }
    }
    if (!lesbar) {
        return false;
    }

    parseEntities(text, out.entities);

    collectNames(out);
    // Was wirklich in der Karte steht - nicht nur, dass das Lesen klappte.
    //
    // shank: "hast du ueberall fuer alles debug output hinzugefuegt? das es
    // bei mapping, Model laden usw. ueberall debug aufschreibt."
    //
    // Bisher protokollierte nur die Oberflaeche, WAS sie angestossen hat.
    // Die Leser selbst schwiegen ueber das, was sie gelesen haben - und
    // genau das braucht man, wenn eine Karte anders aussieht als erwartet.
    diag::detail("BSP gelesen: " + out.magic + " Fassung " +
                 std::to_string(out.version) + ", " +
                 std::to_string(bytes.size()) + " Bytes, " +
                 std::to_string(out.entities.size()) + " Einheiten, " +
                 std::to_string(out.targetNames.size()) + " Zielnamen, " +
                 std::to_string(out.cameraGroups.size()) + " Kameragruppen, " +
                 std::to_string(out.scripts.size()) + " Skriptverweise");
    return true;
}

void collectNames(MapData& out) {
    out.targetNames.clear();
    out.cameraGroups.clear();
    out.scripts.clear();

    // Die Schluessel, unter denen eine Entity ein Skript nennt. Alle fuenf
    // kommen in Movie Duels vor; usescript und deathscript am haeufigsten.
    // rc604 fix604 (JK2 Stufe 3): dazu "mindtrickscript" (behaviorSet
    // [BSET_MINDTRICK], codeJK2/game/g_spawn.cpp:287, ebenso in code/game) -
    // in JK2 an bespin_undercity, cairn_assembly und doom_detention.
    static const char* kScriptKeys[] = {"usescript", "spawnscript",
                                        "deathscript", "painscript",
                                        "awakescript", "angerscript",
                                        "lostenemyscript", "blockedscript",
                                        "mindtrickscript"};

    for (const MapEntity& e : out.entities) {
        addUnique(out.targetNames, e.targetname);
        addUnique(out.cameraGroups, e.cameraGroup);

        for (const char* key : kScriptKeys) {
            const std::string* v = e.find(key);
            if (v == nullptr || v->empty()) {
                continue;
            }
            // Dubletten weglassen: dasselbe deathscript steht oft an
            // mehreren Wachen. In md_tfoaj_jedi.ent dreimal.
            bool known = false;
            for (const MapData::ScriptRef& r : out.scripts) {
                if (r.path == *v) {
                    known = true;
                }
            }
            if (known) {
                continue;
            }
            MapData::ScriptRef ref;
            ref.path = *v;
            ref.key = key;
            ref.owner = e.targetname.empty() ? e.classname : e.targetname;
            out.scripts.push_back(std::move(ref));
        }
    }
    std::sort(out.targetNames.begin(), out.targetNames.end());
    std::sort(out.cameraGroups.begin(), out.cameraGroups.end());
}

}  // namespace bhed
