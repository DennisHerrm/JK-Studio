// bauteile.cpp - Menue-Editor M2 Punkt 3 und 4: Detailbereich in drei Ebenen
// und die Bauteil-Bibliothek.
//
// Eigener Code.
#include "bhed/menue/bauteile.h"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::menue {

namespace {
std::string z(float v) { return zahlText(v); }
std::string rect(float x, float y, float w, float h) { return z(x) + " " + z(y) + " " + z(w) + " " + z(h); }
std::string anf(const std::string& s) {
    std::string r = "\"";
    for (const char c : s) { r += c == '"' ? '\'' : c; }
    return r + "\"";
}

// Ein itemDef aus Zeilen "schluessel wert" (Tabulator-Einrueckung wie die Originale)
std::string item(const std::vector<std::pair<std::string, std::string>>& zeilen) {
    std::string s = "\t\titemDef\n\t\t{\n";
    for (const auto& [k, v] : zeilen) {
        s += "\t\t\t" + k;
        if (!v.empty()) { s += std::string(k.size() < 8 ? 2 : 1, '\t') + v; }
        s += "\n";
    }
    return s + "\t\t}\n";
}
}  // namespace

DetailOptionen beispielDetail() {
    // nach MovieDuels_Settings_Menu (Kategorien und Gruppen aus ingamesetup.menu)
    DetailOptionen o;
    o.reiter = {
        {"Gameplay", {{"Difficulty", "How hard enemies hit you and how much damage you can take."},
                      {"Movement and style", "Which movement set and animation style you play with."},
                      {"World", "Slow motion, dismemberment, blood and corpse removal."}}},
        {"Combat", {{"Lightsaber", "Stance set, swing speed, saber locks and breaking."},
                    {"Blaster and Force", "Weapon and Force behaviour."}}},
        {"Interface", {{"HUD", "What the heads-up display shows."}, {"Camera", "Third person camera distance and angle."}}},
        {"Display", {{"Window", "Resolution, window mode and refresh rate."}, {"Renderer", "Vanilla or Rend2 renderer."}}},
        {"Graphics", {{"Quality", "Texture and model detail."}, {"Effects", "Dynamic lights, glow and shadows."}}},
        {"Audio", {{"Volume", "Effects, music and voice volume."}, {"Options", "Subtitles and sound quality."}}},
    };
    return o;
}

std::string detailOnOpen(const DetailOptionen& o) {
    // Werte in "": der MP-Leser (botlib) zerlegt 0_0 sonst in 0 und _0
    return "setcvar " + o.cvarReiter + " \"0\" ; setcvar " + o.cvarSel + " \"0_0\" ; setcvar " + o.cvarOfs + " \"0\"";
}

std::string erzeugeDetailbereich(const DetailOptionen& o, Dialekt d) {
    (void)d;   // Namen (ITEM_TYPE_*, WINDOW_STYLE_*) gelten in SP direkt und in MP ueber menudef.h
    const Rechteck& b = o.bereich;
    const int n = static_cast<int>(o.reiter.size());
    const int sichtbar = std::max(1, std::min(o.sichtbar, std::max(1, n)));
    const int ofsAnzahl = std::max(1, n - sichtbar + 1);
    const float pfeil = n > sichtbar ? 20.0F : 0.0F;
    const float reiterH = 26.0F;
    const float reiterW = (b.w - 2 * pfeil) / static_cast<float>(sichtbar);
    const float listeW = std::min(180.0F, b.w * 0.32F);
    const float innenY = b.y + reiterH + 8;
    const float innenH = b.h - reiterH - 8;
    const std::string& p = o.praefix;
    std::string s;
    // Rahmen der beiden unteren Ebenen
    s += item({{"name", p + "_rahmen_liste"}, {"rect", rect(b.x, innenY, listeW, innenH)}, {"style", "WINDOW_STYLE_FILLED"},
               {"backcolor", "0 0 0 0.45"}, {"border", "1"}, {"bordercolor", "0.5 0.6 0.8 0.6"}, {"visible", "1"}, {"decoration", ""}});
    s += item({{"name", p + "_rahmen_detail"}, {"rect", rect(b.x + listeW + 8, innenY, b.w - listeW - 8, innenH)},
               {"style", "WINDOW_STYLE_FILLED"}, {"backcolor", "0 0 0 0.45"}, {"border", "1"}, {"bordercolor", "0.5 0.6 0.8 0.6"},
               {"visible", "1"}, {"decoration", ""}});
    // Ebene 1: Reiter je Blaetterstellung (cvarTest ui_tabofs)
    for (int ofs = 0; ofs < ofsAnzahl; ++ofs) {
        for (int k = 0; k < sichtbar && ofs + k < n; ++k) {
            const int t = ofs + k;
            s += item({{"name", p + "_reiter_" + std::to_string(t) + "_" + std::to_string(ofs)},
                       {"group", p + "_reiter"},
                       {"type", "ITEM_TYPE_BUTTON"},
                       {"rect", rect(b.x + pfeil + static_cast<float>(k) * reiterW, b.y, reiterW - 2, reiterH)},
                       {"style", "WINDOW_STYLE_FILLED"},
                       {"backcolor", "0.12 0.14 0.2 0.85"},
                       {"text", anf(o.reiter[static_cast<std::size_t>(t)].titel)},
                       {"textalign", "ITEM_ALIGN_CENTER"},
                       {"textalignx", z((reiterW - 2) / 2)},
                       {"textaligny", "2"},
                       {"textscale", "0.7"},
                       {"forecolor", "0.85 0.9 1 1"},
                       {"cvarTest", anf(o.cvarOfs)},
                       {"showCvar", "{ " + anf(std::to_string(ofs)) + " }"},
                       {"visible", "1"},
                       {"action", "{ setcvar " + o.cvarReiter + " " + anf(std::to_string(t)) + " ; setcvar " + o.cvarSel + " " +
                                      anf(std::to_string(t) + "_0") + " }"}});
        }
        if (n > sichtbar) {   // Blaettern: je Stellung ein Pfeil links und rechts
            if (ofs > 0) {
                s += item({{"name", p + "_links_" + std::to_string(ofs)}, {"type", "ITEM_TYPE_BUTTON"}, {"rect", rect(b.x, b.y, pfeil - 2, reiterH)},
                           {"text", anf("<")}, {"textalign", "ITEM_ALIGN_CENTER"}, {"textalignx", z((pfeil - 2) / 2)}, {"textaligny", "2"},
                           {"textscale", "0.7"}, {"forecolor", "1 1 1 1"}, {"cvarTest", anf(o.cvarOfs)},
                           {"showCvar", "{ " + anf(std::to_string(ofs)) + " }"}, {"visible", "1"},
                           {"action", "{ setcvar " + o.cvarOfs + " " + anf(std::to_string(ofs - 1)) + " }"}});
            }
            if (ofs < ofsAnzahl - 1) {
                s += item({{"name", p + "_rechts_" + std::to_string(ofs)}, {"type", "ITEM_TYPE_BUTTON"},
                           {"rect", rect(b.x + b.w - pfeil + 2, b.y, pfeil - 2, reiterH)}, {"text", anf(">")}, {"textalign", "ITEM_ALIGN_CENTER"},
                           {"textalignx", z((pfeil - 2) / 2)}, {"textaligny", "2"}, {"textscale", "0.7"}, {"forecolor", "1 1 1 1"},
                           {"cvarTest", anf(o.cvarOfs)}, {"showCvar", "{ " + anf(std::to_string(ofs)) + " }"}, {"visible", "1"},
                           {"action", "{ setcvar " + o.cvarOfs + " " + anf(std::to_string(ofs + 1)) + " }"}});
            }
        }
    }
    // Ebene 2: Ueberschrift und Unterreiter des offenen Reiters (cvarTest ui_tab)
    const float zeileH = 24.0F;
    for (int t = 0; t < n; ++t) {
        const DetailReiter& r = o.reiter[static_cast<std::size_t>(t)];
        const std::string tt = std::to_string(t);
        s += item({{"name", p + "_titel_" + tt}, {"rect", rect(b.x + 6, innenY + 4, listeW - 12, zeileH)}, {"text", anf(r.titel)},
                   {"textscale", "0.8"}, {"textaligny", "2"}, {"forecolor", "1 0.85 0.4 1"}, {"cvarTest", anf(o.cvarReiter)},
                   {"showCvar", "{ " + anf(tt) + " }"}, {"visible", "1"}, {"decoration", ""}});
        for (std::size_t u = 0; u < r.unter.size(); ++u) {
            const std::string sel = tt + "_" + std::to_string(u);
            const float y = innenY + 8 + zeileH * static_cast<float>(u + 1);
            // Markierung des gewaehlten Unterreiters (cvarTest ui_sel = "t_u")
            s += item({{"name", p + "_wahl_" + sel}, {"rect", rect(b.x + 4, y, listeW - 8, zeileH - 2)}, {"style", "WINDOW_STYLE_FILLED"},
                       {"backcolor", "0.3 0.45 0.8 0.5"}, {"cvarTest", anf(o.cvarSel)}, {"showCvar", "{ " + anf(sel) + " }"},
                       {"visible", "1"}, {"decoration", ""}});
            s += item({{"name", p + "_unter_" + sel}, {"type", "ITEM_TYPE_BUTTON"}, {"rect", rect(b.x + 4, y, listeW - 8, zeileH - 2)},
                       {"text", anf(r.unter[u].titel)}, {"textalignx", "6"}, {"textaligny", "2"}, {"textscale", "0.6"},
                       {"forecolor", "0.9 0.9 0.9 1"}, {"cvarTest", anf(o.cvarReiter)}, {"showCvar", "{ " + anf(tt) + " }"},
                       {"visible", "1"}, {"action", "{ setcvar " + o.cvarSel + " " + anf(sel) + " }"}});
            // Ebene 3: Detail (cvarTest ui_sel = "t_u")
            const float dx = b.x + listeW + 16;
            const float dw = b.w - listeW - 24;
            s += item({{"name", p + "_kopf_" + sel}, {"rect", rect(dx, innenY + 6, dw, 28)}, {"text", anf(r.unter[u].titel)},
                       {"textscale", "0.9"}, {"textaligny", "2"}, {"forecolor", "1 1 1 1"}, {"cvarTest", anf(o.cvarSel)},
                       {"showCvar", "{ " + anf(sel) + " }"}, {"visible", "1"}, {"decoration", ""}});
            s += item({{"name", p + "_text_" + sel}, {"rect", rect(dx, innenY + 40, dw, innenH - 50)}, {"text", anf(r.unter[u].text)},
                       {"textscale", "0.6"}, {"textaligny", "2"}, {"forecolor", "0.8 0.85 0.9 1"}, {"autowrapped", ""},
                       {"cvarTest", anf(o.cvarSel)}, {"showCvar", "{ " + anf(sel) + " }"}, {"visible", "1"}, {"decoration", ""}});
        }
    }
    return s;
}

std::vector<Vorlage> standardVorlagen(Dialekt d) {
    std::vector<Vorlage> v;
    v.push_back({"knopf", "Button",
                 item({{"name", "knopf"}, {"type", "ITEM_TYPE_BUTTON"}, {"rect", "260 200 120 28"}, {"style", "WINDOW_STYLE_FILLED"},
                       {"backcolor", "0.15 0.18 0.26 0.9"}, {"text", anf("Button")}, {"textalign", "ITEM_ALIGN_CENTER"}, {"textalignx", "60"},
                       {"textaligny", "3"}, {"textscale", "0.7"}, {"forecolor", "1 1 1 1"}, {"visible", "1"},
                       {"action", "{ play \"sound/interface/button1.wav\" }"}}),
                 false});
    v.push_back({"schieber", "Slider with cvar",
                 item({{"name", "schieber"}, {"type", "ITEM_TYPE_SLIDER"}, {"rect", "200 240 240 24"}, {"text", anf("Volume:")},
                       {"cvarfloat", "\"s_volume\" 0.8 0 1"}, {"textalign", "ITEM_ALIGN_RIGHT"}, {"textalignx", "90"}, {"textaligny", "2"},
                       {"textscale", "0.7"}, {"forecolor", "1 1 1 1"}, {"visible", "1"}}),
                 false});
    v.push_back({"janein", "Yes/No",
                 item({{"name", "janein"}, {"type", "ITEM_TYPE_YESNO"}, {"rect", "200 280 240 24"}, {"text", anf("Option:")},
                       {"cvar", anf("ui_option")}, {"textalign", "ITEM_ALIGN_RIGHT"}, {"textalignx", "90"}, {"textaligny", "2"},
                       {"textscale", "0.7"}, {"forecolor", "1 1 1 1"}, {"visible", "1"}}),
                 false});
    // Reiterleiste: drei Reiter, setzen ui_tab
    std::string reiter;
    for (int i = 0; i < 3; ++i) {
        reiter += item({{"name", "reiter" + std::to_string(i)}, {"group", "reiterleiste"}, {"type", "ITEM_TYPE_BUTTON"},
                        {"rect", rect(120 + static_cast<float>(i) * 135, 60, 130, 26)}, {"style", "WINDOW_STYLE_FILLED"},
                        {"backcolor", "0.12 0.14 0.2 0.85"}, {"text", anf("Tab " + std::to_string(i + 1))}, {"textalign", "ITEM_ALIGN_CENTER"},
                        {"textalignx", "65"}, {"textaligny", "2"}, {"textscale", "0.7"}, {"forecolor", "1 1 1 1"}, {"visible", "1"},
                        {"action", "{ setcvar ui_tab " + std::to_string(i) + " }"}});
    }
    v.push_back({"reiter", "Tab bar", reiter, false});
    DetailOptionen o = beispielDetail();
    v.push_back({"detail", "Detail area (3 levels)", erzeugeDetailbereich(o, d), false});
    v.push_back({"liste", "List",
                 item({{"name", "liste"}, {"type", "ITEM_TYPE_LISTBOX"}, {"rect", "200 120 240 200"}, {"style", "WINDOW_STYLE_FILLED"},
                       {"backcolor", "0 0 0 0.5"}, {"border", "1"}, {"bordercolor", "0.5 0.5 0.5 1"},
                       {"feeder", d == Dialekt::SP ? "16" : "19"},   // FEEDER_PLAYER_SPECIES (SP 0x10, MP 0x13)
                       {"elementwidth", "220"}, {"elementheight", "20"}, {"textscale", "0.6"}, {"forecolor", "1 1 1 1"},
                       {"outlinecolor", "0.3 0.45 0.8 0.5"}, {"visible", "1"}}),
                 false});
    return v;
}

std::string vorlagenDateiname(const std::string& name) {
    std::string r;
    for (const char c : name) {
        const auto u = static_cast<unsigned char>(c);
        r += (std::isalnum(u) != 0 || c == '_' || c == '-') ? c : '_';
    }
    return r.empty() ? std::string("vorlage") : r;
}

std::vector<Vorlage> ladeEigeneVorlagen(const std::string& ordner) {
    std::vector<Vorlage> v;
    std::error_code ec;
    if (ordner.empty() || !std::filesystem::is_directory(ordner, ec)) { return v; }
    for (const auto& e : std::filesystem::directory_iterator(ordner, ec)) {
        if (!e.is_regular_file(ec) || e.path().extension() != ".vorlage") { continue; }
        std::ifstream f(e.path(), std::ios::binary);
        std::ostringstream s;
        s << f.rdbuf();
        const std::u8string stem = e.path().stem().u8string();
        Vorlage x;
        x.id = std::string(stem.begin(), stem.end());
        x.name = x.id;
        x.text = s.str();
        x.eigen = true;
        if (x.text.find("itemDef") != std::string::npos) { v.push_back(x); }
    }
    std::sort(v.begin(), v.end(), [](const Vorlage& a, const Vorlage& b) { return a.name < b.name; });
    return v;
}

bool speichereVorlage(const std::string& ordner, const std::string& name, const std::string& text) {
    if (ordner.empty() || text.find("itemDef") == std::string::npos) { return false; }
    std::error_code ec;
    std::filesystem::create_directories(ordner, ec);
    std::ofstream f(std::filesystem::path(ordner) / (vorlagenDateiname(name) + ".vorlage"), std::ios::binary);
    f << text;
    return static_cast<bool>(f);
}

}  // namespace bhed::menue
