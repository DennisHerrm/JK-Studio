// szene.cpp - AP5 (Abgleich P7): ModView-Skript (.mvs) lesen und schreiben, siehe bhed/modell/szene.h
//
// Quelle: JACoders/OpenJK-tools ModView/script.cpp und script.h (GPL v2): Script_Write, R_ModelContainer_CallBack_WriteBolts,
// R_ModelContainer_WriteOptional, Script_Read, Script_Parse_LoadModel, Script_Parse_BoltModel, Script_Parse_ReadOptionalArgs.
// Das Textformat ist GenericParser2 (bhed/efx/gp2.h - derselbe Parser, mit dem ModView liest).
#include "bhed/modell/szene.h"

#include "bhed/efx/gp2.h"

#include <cctype>
#include <cstdio>
#include <cstdlib>

namespace bhed::modell {
namespace {

// CGPGroup::FindPairValue: der erste Wert des Schluessels, sonst "".
std::string wert(const efx::gp2::Group& g, std::string_view key) {
    const efx::gp2::Property* p = g.findProperty(key);
    return p != nullptr && !p->values.empty() ? p->values.front() : std::string();
}

// Eine Liste "surfaces_on { name0 "a" name1 "b" }": alle Werte der Unterguppe in ihrer Reihenfolge (GetPairs/GetTopValue).
std::vector<std::string> liste(const efx::gp2::Group& g, std::string_view key) {
    std::vector<std::string> aus;
    if (const efx::gp2::Group* u = g.findSubGroup(key)) {
        for (const efx::gp2::Property& p : u->properties) {
            if (!p.values.empty()) { aus.push_back(p.values.front()); }
        }
    }
    return aus;
}

bool gleich(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) { return false; }
    for (std::size_t i = 0; i < a.size(); ++i) {
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) { return false; }
    }
    return true;
}

// Schluessel aus Script_Parse_ReadOptionalArgs, die JK Studio nicht abbildet - nur melden.
void nichtAbgebildet(const efx::gp2::Group& g, const std::string& wer, Szene& s, bool anbau) {
    static const char* const kImmer[] = {"multiseq_primary_lock", "multiseq_secondary_lock", "multiseq_primarylist", "multiseq_secondarylist",
                                          "ethnic", "skinfile"};
    for (const char* k : kImmer) {
        if (g.findProperty(k) != nullptr || g.findSubGroup(k) != nullptr) { s.nichtUebernommen.push_back(wer + ": " + k); }
    }
    if (!anbau) { return; }
    static const char* const kAnbau[] = {"locksequence", "locksequence_secondary", "bonename_secondarystart", "surfacename_rootoverride", "oldskinfile",
                                          "surfaces_on", "surfaces_off", "surfaces_offnochildren"};
    for (const char* k : kAnbau) {
        if (g.findProperty(k) != nullptr || g.findSubGroup(k) != nullptr) { s.nichtUebernommen.push_back(wer + ": " + k); }
    }
}

void zahlLesen(const efx::gp2::Group& g, std::string_view key, float& f, bool& da) {
    const std::string w = wert(g, key);
    da = false;
    if (w.empty()) { return; }
    char* ende = nullptr;
    const float v = std::strtof(w.c_str(), &ende);
    if (ende != w.c_str()) {
        f = v;
        da = true;
    }
}

std::string flaechen(const char* einzug, const char* key, const std::vector<std::string>& namen) {
    if (namen.empty()) { return {}; }
    std::string t = std::string(einzug) + key + "\n" + einzug + "{\n";
    for (std::size_t i = 0; i < namen.size(); ++i) { t += std::string(einzug) + "\tname" + std::to_string(i) + "\t\"" + namen[i] + "\"\n"; }
    return t + einzug + "}\n";
}

}  // namespace

std::string szenenKennung(const std::string& pfad, const std::vector<std::string>& vergeben) {
    std::string n = pfad;
    const std::size_t schnitt = n.find_last_of("/\\");
    if (schnitt != std::string::npos) { n = n.substr(schnitt + 1); }
    const std::size_t punkt = n.rfind('.');
    if (punkt != std::string::npos) { n.resize(punkt); }
    for (char& c : n) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    std::string k = n;
    for (int alternative = 1;; ++alternative) {
        bool frei = true;
        for (const std::string& v : vergeben) { frei = frei && v != k; }
        if (frei) { return k; }
        k = n + "_" + std::to_string(alternative + 1);   // GenerateHandleName: va("_%d", iAlternative) nach dem ersten iAlternative++
    }
}

bool szeneLesen(std::string_view text, Szene& s, std::string* fehler, SzeneFehler* was) {
    s = Szene{};
    if (was != nullptr) { *was = SzeneFehler{}; }
    const efx::gp2::ParseResult r = efx::gp2::parse(text);
    // m: der Text von ModView (Protokoll); art, block, schluessel: dasselbe fuer eine Meldung in der Sprache der Oberflaeche
    const auto melde = [&](const std::string& m, SzeneFehlerArt art, const char* block = "", const std::string& schluessel = std::string(), int zeile = 0) {
        if (fehler != nullptr) { *fehler = m; }
        if (was != nullptr) { *was = SzeneFehler{art, zeile, block, schluessel}; }
        return false;
    };
    if (!r.ok()) {
        const int zeile = static_cast<int>(r.errors.front().line);
        return melde("{} - Brace mismatch error (line " + std::to_string(zeile) + ")", SzeneFehlerArt::Klammer, "", {}, zeile);
    }
    const efx::gp2::Group* lm = r.topLevel.findSubGroup("loadmodel");
    if (lm == nullptr) { return melde("Unable to find keyword 'loadmodel'", SzeneFehlerArt::KeinBlock, "loadmodel"); }
    s.basisOrdner = wert(*lm, "basedir");
    s.name = wert(*lm, "name");
    if (s.name.empty()) { return melde("Script_Parse_LoadModel(): No 'name' keyword found!", SzeneFehlerArt::KeinSchluessel, "loadmodel", "name"); }
    s.modell = wert(*lm, "modelfile");
    if (s.modell.empty()) { return melde("Script_Parse_LoadModel(): No 'modelfile' keyword found!", SzeneFehlerArt::KeinSchluessel, "loadmodel", "modelfile"); }
    s.wurzel = wert(*lm, "surfacename_rootoverride");
    s.sekundaerStart = wert(*lm, "bonename_secondarystart");
    s.primaer = wert(*lm, "locksequence");
    s.sekundaer = wert(*lm, "locksequence_secondary");
    s.haut = wert(*lm, "oldskinfile");
    s.flaechenAn = liste(*lm, "surfaces_on");
    s.flaechenAus = liste(*lm, "surfaces_off");
    s.flaechenOhneKinder = liste(*lm, "surfaces_offnochildren");
    nichtAbgebildet(*lm, s.name, s, false);
    // JK Studio
    bool da = false;
    float f = 0.0F;
    zahlLesen(*lm, "jks_lod", f, da);
    if (da) { s.lod = static_cast<int>(f); }
    zahlLesen(*lm, "jks_fov", f, da);
    if (da) { s.sichtfeld = f; }
    zahlLesen(*lm, "jks_floor", f, da);
    if (da) { s.boden = f != 0.0F ? 1 : 0; }
    zahlLesen(*lm, "jks_floorz", f, da);
    if (da) { s.bodenZ = f; }
    // Anbauten: jede Untergruppe "boltmodel" (GetSubGroups / GetNext), parent ist eine schon gelesene Kennung
    std::vector<std::string> kennungen{s.name};
    for (const efx::gp2::Group& u : lm->subGroups) {
        if (!gleich(u.name, "boltmodel")) { continue; }
        SzenenAnbau a;
        a.name = wert(u, "name");
        if (a.name.empty()) { return melde("Script_Parse_BoltModel(): No 'name' keyword found!", SzeneFehlerArt::KeinSchluessel, "boltmodel", "name"); }
        a.modell = wert(u, "modelfile");
        if (a.modell.empty()) { return melde("Script_Parse_BoltModel(): No 'modelfile' keyword found!", SzeneFehlerArt::KeinSchluessel, "boltmodel", "modelfile"); }
        a.eltern = wert(u, "parent");
        if (a.eltern.empty()) { return melde("Script_Parse_BoltModel(): No 'parent' keyword found!", SzeneFehlerArt::KeinSchluessel, "boltmodel", "parent"); }
        const std::string knochen = wert(u, "bolttobone");
        const std::string flaeche = wert(u, "bolttosurface");
        if (knochen.empty() && flaeche.empty()) {
            return melde("Script_Parse_BoltModel(): No 'bolttobone' or 'bolttosurface' keyword found!", SzeneFehlerArt::KeinSchluessel, "boltmodel", "bolttobone / bolttosurface");
        }
        a.knochen = !knochen.empty();
        a.ziel = a.knochen ? knochen : flaeche;
        bool bekannt = false;
        for (const std::string& k : kennungen) { bekannt = bekannt || k == a.eltern; }
        if (!bekannt) { return melde("Script_Parse_BoltModel(): Unknown parent handle \"" + a.eltern + "\"", SzeneFehlerArt::ElternUnbekannt, "boltmodel", a.eltern); }
        if (a.eltern != s.name) { s.nichtUebernommen.push_back(a.name + ": parent " + a.eltern); }   // Anbau an einem Anbau
        nichtAbgebildet(u, a.name, s, true);
        kennungen.push_back(a.name);
        s.anbauten.push_back(std::move(a));
    }
    return true;
}

std::string szeneSchreiben(const Szene& s) {
    std::string t = "loadmodel\n{\n";
    // R_ModelContainer_CallBack_WriteBolts, Hauptmodell
    t += "\tname\t\t\"" + s.name + "\"\n";
    t += "\tmodelfile\t\"" + s.modell + "\"\n";
    // R_ModelContainer_WriteOptional
    if (!s.wurzel.empty()) { t += "\tsurfacename_rootoverride\t\"" + s.wurzel + "\"\n"; }
    if (!s.sekundaerStart.empty()) { t += "\tbonename_secondarystart\t\"" + s.sekundaerStart + "\"\n"; }
    if (!s.primaer.empty()) { t += "\tlocksequence\t\"" + s.primaer + "\"\n"; }
    if (!s.sekundaer.empty()) { t += "\tlocksequence_secondary\t\"" + s.sekundaer + "\"\n"; }
    if (!s.haut.empty()) { t += "\toldskinfile\t\"" + s.haut + "\"\n"; }
    t += flaechen("\t", "surfaces_on", s.flaechenAn);
    t += flaechen("\t", "surfaces_off", s.flaechenAus);
    t += flaechen("\t", "surfaces_offnochildren", s.flaechenOhneKinder);
    // JK Studio: ModView fragt nur seine eigenen Schluessel ab (FindPairValue) und uebergeht diese
    char z[64];
    if (s.lod >= 0) { t += "\tjks_lod\t\"" + std::to_string(s.lod) + "\"\n"; }
    if (s.sichtfeld > 0.0F) {
        std::snprintf(z, sizeof z, "%g", static_cast<double>(s.sichtfeld));
        t += std::string("\tjks_fov\t\"") + z + "\"\n";
    }
    if (s.boden >= 0) {
        std::snprintf(z, sizeof z, "%g", static_cast<double>(s.bodenZ));
        t += "\tjks_floor\t\"" + std::to_string(s.boden) + "\"\n";
        t += std::string("\tjks_floorz\t\"") + z + "\"\n";
    }
    t += "\n";
    // die Anbauten
    for (const SzenenAnbau& a : s.anbauten) {
        t += "\tboltmodel\n\t{\n";
        t += "\t\tname\t\"" + a.name + "\"\n";
        t += "\t\tmodelfile\t\"" + a.modell + "\"\n";
        t += "\t\tparent\t\"" + (a.eltern.empty() ? s.name : a.eltern) + "\"\n";
        t += std::string("\t\t") + (a.knochen ? "bolttobone" : "bolttosurface") + "\t\"" + a.ziel + "\"\n";
        t += "\t}\n\n";
    }
    t += "}\n\n";
    return t;
}

}  // namespace bhed::modell
