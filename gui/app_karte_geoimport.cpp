// app_karte_geoimport.cpp - Karten-Editor, Stufe 9 Punkt 2: Geometrie zu Brushes
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// File > Import geometry as brushes...: eine .obj/.fbx/.glb/.gltf lesen (karte::netzLesen) - ASEB: .ase, .md3 und jedes
// weitere Format, das q3map2 liest, genau wie q3map2 (karte::modellWieQ3map2, die Shadernamen von AssModelMesh), je Koerper ein
// Teil (karte::nachKoerpern); .glm mit Meldung (Ghoul2, keine Kartengeometrie); "Modell in Brushes umwandeln" nimmt die gewaehlten
// misc_model mit Lage, Skin und _remap und ersetzt sie beim Einfuegen -
// je Teil Auto / Brushes / Modell + Clip waehlen, die Vorschau zeigt je Teil, was entsteht
// (Brushes, detail), "Einfuegen" setzt alles als eine Undo-Stufe ein (karte::importEinfuegen),
// danach auf Wunsch der Optimierer. Teile als Modell + Clip kommen als .obj in den
// Import-Ordner des Programms (<Einstellungen>/import/models/import), den Editor und
// q3map2 (-fs_pakpath) als Suchordner kennen - nie in einen Spielordner. Eine .bsp oeffnet
// "Open .bsp" mit dem Leser ihres Spiels (karte::q3map2TypFuerBsp).
#include "karteneditor.h"

#include "app_internal.h"
#include "platform.h"
#include "textumbruch.h"

#include "bhed/karte/brushexport.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/modell.h"
#include "bhed/karte/modellimport.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/spielbeschreibung.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>

namespace bhed::gui::karteneditor {

namespace {

namespace fs = std::filesystem;
using i18n::Str;

Editor& e() { return editor(); }

fs::path pfadVon(const std::string& s) { return fs::path(std::u8string(s.begin(), s.end())); }

std::string endungVon(const std::string& pfad) {
    const std::u8string u = pfadVon(pfad).extension().u8string();
    std::string x(u.begin(), u.end());
    if (!x.empty() && x[0] == '.') { x.erase(0, 1); }
    for (char& c : x) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return x;
}

std::string stammVon(const std::string& pfad) {
    const std::u8string u = pfadVon(pfad).stem().u8string();
    std::string s(u.begin(), u.end());
    for (char& c : s) {
        const unsigned char u8 = static_cast<unsigned char>(c);
        c = (std::isalnum(u8) != 0 || c == '_' || c == '-') ? static_cast<char>(std::tolower(u8)) : '_';
    }
    return s.empty() ? std::string("import") : s;
}

bool dateiLesen(const std::string& pfad, std::string& bytes) {
    std::ifstream f(pfadVon(pfad), std::ios::binary);
    if (!f) { return false; }
    std::ostringstream s;
    s << f.rdbuf();
    bytes = s.str();
    return true;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

// Die Formate von Blender (netzLesen: je Knoten ein Teil, Shader-Vorsatz, Y oben waehlbar)
bool netzFormat(const std::string& x) { return x == "obj" || x == "fbx" || x == "glb" || x == "gltf"; }

// ASEB: der Name, unter dem q3map2 die Datei als misc_model laede - er gibt die Wurzel der Shadernamen (AssModelMesh: rootPath):
// relativ zu einem Ordner des Suchpfads; sonst ab "models/" (ein Spielordner, der nicht eingerichtet ist); sonst der Dateiname
std::string vfsNameFuer(const std::string& pfad) {
    std::string p = pfad;
    std::replace(p.begin(), p.end(), '\\', '/');
    const std::string k = klein(p);
    if (const std::shared_ptr<Spielkontext>& sp = e().karteSpiel) {
        for (const std::string& a : sp->fs.archive(false)) {
            if (a.empty() || a.back() != '/') { continue; }   // nur Ordner, keine .pk3
            std::string ka = klein(a);
            std::replace(ka.begin(), ka.end(), '\\', '/');
            if (k.size() > ka.size() && k.compare(0, ka.size(), ka) == 0) { return p.substr(ka.size()); }
        }
    }
    if (const std::size_t m = k.rfind("/models/"); m != std::string::npos) { return p.substr(m + 1); }
    return p.substr(p.find_last_of('/') + 1);
}

// ASEB: eine Modelldatei von der Platte wie q3map2 - die Datei und ihre Nachbarn (.mtl) unter ihrem VFS-Namen, alles andere aus dem Spiel
bool modellDateiLesen(const std::string& pfad, karte::Modell& m, std::string& grund) {
    const std::string vfs = vfsNameFuer(pfad);
    const std::string vfsOrdner = vfs.substr(0, vfs.find_last_of('/') + 1);
    std::string platte = pfad;
    std::replace(platte.begin(), platte.end(), '\\', '/');
    const std::string platteOrdner = platte.substr(0, platte.find_last_of('/') + 1);
    const std::shared_ptr<Spielkontext> sp = e().karteSpiel;
    const karte::ModellDateiLeser leser = [&](const std::string& p, std::string& b) {
        if (klein(p) == klein(vfs)) { return dateiLesen(pfad, b); }
        if (klein(p).compare(0, vfsOrdner.size(), klein(vfsOrdner)) == 0 && dateiLesen(platteOrdner + p.substr(vfsOrdner.size()), b)) { return true; }
        return sp && sp->fs.dateiLesen(p, b);
    };
    return karte::modellWieQ3map2(vfs, leser, m, &grund);
}

karte::ZerlegOptionen optionen() {
    karte::ZerlegOptionen o;
    o.shaderVorsatz = e().geoImport.vorsatz;
    if (!o.shaderVorsatz.empty() && o.shaderVorsatz.back() != '/') { o.shaderVorsatz += '/'; }
    return o;
}

void vorschauRechnen() {
    GeoImportZustand& z = e().geoImport;
    const karte::ZerlegOptionen o = optionen();
    z.vorschau.clear();
    for (std::size_t i = 0; i < z.netz.teile.size(); ++i) {
        z.vorschau.push_back(karte::teilUmwandeln(z.netz.teile[i], static_cast<karte::TeilArt>(z.wahl[i]), o));
    }
    z.neu = false;
}

}  // namespace

std::string importOrdner() {
    const std::string o = platform::settingsDirectory() + "\\import";
    std::error_code ec;
    if (!fs::is_directory(pfadVon(o), ec)) { return std::string(); }
    return karte::ordnerBereinigt(o);
}

bool geoImportLaden(const std::string& pfad) {
    GeoImportZustand& z = e().geoImport;
    const std::string x = endungVon(pfad);
    char t[1400];
    if (x == "glm") {   // ASEB: Ghoul2 - weder q3map2 noch assimp lesen es als Geometrie
        std::snprintf(t, sizeof(t), trK(Str::KeGiGlm), pfad.c_str());
        fehler(t);
        return false;
    }
    karte::ImportNetz netz;
    std::string grund;
    bool ok = false;
    const bool modellNamen = !netzFormat(x) && karte::modellArtQ3map2(pfad);
    if (modellNamen) {
        // ASEB: wie q3map2 das Modell als misc_model laede; die Flaechen eines Koerpers bleiben ein Teil
        karte::Modell m;
        ok = modellDateiLesen(pfad, m, grund);
        if (ok) {
            netz = karte::nachKoerpern(karte::netzVonModell(m, nullptr, nullptr));
            for (karte::ImportTeil& teil : netz.teile) {
                for (karte::ImportPunkt& p : teil.punkte) { p = {p.x * z.skala, p.y * z.skala, p.z * z.skala}; }
            }
            ok = !netz.teile.empty();
            if (!ok) { grund = "no triangles"; }
        }
    } else {
        std::string bytes;
        (void)dateiLesen(pfad, bytes);
        karte::NetzOptionen no;
        no.skala = static_cast<double>(z.skala);
        no.yOben = z.yOben;
        ok = karte::netzLesen(bytes, x, no, netz, &grund);
    }
    if (!ok) {
        std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), pfad.c_str(), grund.c_str());
        fehler(t);
        return false;
    }
    z.datei = pfad;
    z.modellNamen = modellNamen;
    z.ersetzt.clear();
    z.netz = std::move(netz);
    z.wahl.assign(z.netz.teile.size(), static_cast<int>(karte::TeilArt::Auto));
    z.offen = true;
    vorschauRechnen();
    return true;
}

void geoImportOeffnen() {
    // ASEB: dazu .ase, .md3 und jedes weitere Format, das q3map2 liest (die Endungen des Modell-Browsers)
    std::string muster = "*.obj;*.fbx;*.glb;*.gltf";
    for (const std::string& x : karte::modellEndungenAssimp()) {
        if (!netzFormat(x)) { muster += ";*." + x; }
    }
    muster += ";*.bsp";
    const std::string pfad = kartenDateiDialog(true, "Import geometry", (muster + "|" + muster + "|*.*|*.*").c_str(), kartenStartOrdner());
    if (pfad.empty()) { return; }
    if (endungVon(pfad) == "bsp") {   // BSPs (auch anderer id-Tech-3-Spiele): wie Open .bsp
        (void)bspOeffnenDatei(pfad);
        return;
    }
    (void)geoImportLaden(pfad);
}

void geoImportEinfuegen() {
    if (bspRueckSperre()) { return; }   // Stufe 9d: erst nach der Rueckgewinnung (Open .bsp)
    Editor& ed = e();
    GeoImportZustand& z = ed.geoImport;
    if (z.neu) { vorschauRechnen(); }
    const karte::ZerlegOptionen o = optionen();
    std::vector<const karte::ImportTeil*> modellTeile;
    for (std::size_t i = 0; i < z.vorschau.size(); ++i) {
        if (z.vorschau[i].art == karte::TeilArt::ModellClip) { modellTeile.push_back(&z.netz.teile[i]); }
    }
    std::string modell;
    if (!modellTeile.empty()) {
        const std::string stamm = stammVon(z.datei);
        const fs::path ordner = pfadVon(platform::settingsDirectory() + "\\import") / "models" / "import";
        std::error_code ec;
        fs::create_directories(ordner, ec);
        std::ofstream f(ordner / (stamm + ".obj"), std::ios::binary);
        f << karte::modellObj(modellTeile, o);
        if (f) {
            modell = "models/import/" + stamm + ".obj";
            const std::u8string u = (ordner / (stamm + ".obj")).u8string();
            char t[1400];
            std::snprintf(t, sizeof(t), trK(Str::KeGiModellDatei), std::string(u.begin(), u.end()).c_str());
            ausgabe(t);
        }
    }
    std::vector<const karte::Entitaet*> weg;   // ASEB: die umgewandelten misc_model
    std::vector<std::shared_ptr<karte::Entitaet>> halten;
    for (const auto& w : z.ersetzt) {
        if (auto en = w.lock()) {
            weg.push_back(en.get());
            halten.push_back(std::move(en));
        }
    }
    z.letztes = karte::importEinfuegen(ed.karte, z.vorschau, modell, weg);
    z.ersetzt.clear();
    ++ed.sichtStand;
    auswahlGeaendert();
    char t[1400];
    std::snprintf(t, sizeof(t), trKN(Str::KeGiFertig, z.letztes.pinsel, z.letztes.modelle), z.datei.c_str(), static_cast<int>(z.letztes.pinsel), static_cast<int>(z.letztes.modelle));
    ausgabe(t);
    z.offen = false;
    if (z.optimierer) { optimiererOeffnen(); }
}

namespace {

// ASEB: die gewaehlten misc_model mit einem Modell (nicht "*n")
std::vector<std::shared_ptr<karte::Entitaet>> gewaehlteModelle() {
    std::vector<std::shared_ptr<karte::Entitaet>> v;
    for (const karte::AuswahlElement& a : e().karte.auswahl.elemente()) {
        const auto en = a.entitaet.lock();
        if (!en || !a.primitiv.expired() || klein(en->klassenname()) != "misc_model") { continue; }
        const std::string m = kartenModellSchluessel(*en);
        if (m.empty() || m[0] == '*' || std::find(v.begin(), v.end(), en) != v.end()) { continue; }
        v.push_back(en);
    }
    return v;
}

}  // namespace

bool modellZuBrushesMoeglich() { return !gewaehlteModelle().empty(); }

void modellZuBrushesOeffnen() {
    Editor& ed = e();
    const std::vector<std::shared_ptr<karte::Entitaet>> modelle = gewaehlteModelle();
    char t[1400];
    if (modelle.empty()) {
        fehler(trK(Str::KeMbKeins));
        return;
    }
    const std::shared_ptr<Spielkontext> sp = ed.karteSpiel;
    const karte::ModellDateiLeser leser = [sp](const std::string& p, std::string& b) { return sp && sp->fs.dateiLesen(p, b); };
    karte::ImportNetz netz;
    std::string namen;
    for (const auto& en : modelle) {
        const std::string pfad = kartenModellSchluessel(*en);
        karte::Modell m;
        std::string grund;
        // InsertModel: LoadModel, Skin und _remap je Flaeche, die Lage des misc_model (ModelGetTransform)
        if (!karte::modellWieQ3map2(pfad, leser, m, &grund)) {
            std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), pfad.c_str(), grund.c_str());
            fehler(t);
            return;
        }
        const std::vector<std::string> shader = kartenModellShader(*en, m);
        const karte::Matrix4 lage = karte::miscModellMatrix(*en);
        karte::ImportNetz teil = karte::nachKoerpern(karte::netzVonModell(m, &shader, &lage));
        for (karte::ImportTeil& x : teil.teile) {
            if (modelle.size() > 1) { x.name = pfad.substr(pfad.find_last_of('/') + 1) + " " + x.name; }
            netz.teile.push_back(std::move(x));
        }
        namen += (namen.empty() ? "" : ", ") + pfad;
    }
    if (netz.teile.empty()) {
        std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), namen.c_str(), "no triangles");
        fehler(t);
        return;
    }
    GeoImportZustand& z = ed.geoImport;
    z.datei = namen;
    z.modellNamen = true;
    z.netz = std::move(netz);
    z.ersetzt.assign(modelle.begin(), modelle.end());
    z.wahl.assign(z.netz.teile.size(), static_cast<int>(karte::TeilArt::Auto));
    z.offen = true;
    vorschauRechnen();
}

bool blenderExport(const std::string& pfadWahl) {
    Editor& ed = e();
    char t[1400];
    bool pinsel = false;
    for (const auto& a : ed.karte.auswahl.elemente()) {
        const auto p = a.primitiv.lock();
        pinsel = pinsel || (p && p->art() == karte::Primitiv::Art::Pinsel);
    }
    if (!pinsel) {
        fehler(trK(Str::KeBlNichts));
        return false;
    }
    const std::string pfad = pfadWahl.empty() ? kartenDateiDialog(false, "Export for Blender", "*.glb|*.glb|*.obj|*.obj", kartenStartOrdner(), "export.glb")
                                              : pfadWahl;
    if (pfad.empty()) { return false; }
    const bool glb = endungVon(pfad) == "glb";
    const std::string objPfad = glb ? pfad.substr(0, pfad.size() - 4) + ".obj" : pfad;
    karte::brushexport::Optionen o;   // je Brush eine Gruppe, mit Materialien
    const karte::brushexport::Ergebnis r = karte::brushexport::exportieren(ed.karte, o, objPfad, [](const std::string&) {
        return std::array<float, 3>{1.0F, 1.0F, 1.0F};
    });
    std::string inhalt;
    if (glb) {
        karte::ImportNetz netz;
        karte::NetzOptionen no;
        no.yOben = true;   // brushexport schreibt schon Y oben (x, z, -y; brushexport.cpp wie NRC)
        std::string grund;
        if (!karte::netzLesen(r.obj, "obj", no, netz, &grund)) {
            std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), pfad.c_str(), grund.c_str());
            fehler(t);
            return false;
        }
        inhalt = karte::glbSchreiben(netz, true);
    } else {
        inhalt = r.obj;   // NRCs brushexport schreibt die .obj schon mit Y oben
    }
    std::ofstream f(pfadVon(pfad), std::ios::binary);
    f.write(inhalt.data(), static_cast<std::streamsize>(inhalt.size()));
    if (!f) {
        std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), pfad.c_str(), "write");
        fehler(t);
        return false;
    }
    if (!glb && !r.mtl.empty()) {
        std::ofstream m(pfadVon(r.mtlPfad), std::ios::binary);
        m << r.mtl;
    }
    std::snprintf(t, sizeof(t), trK(Str::KeBlFertig), pfad.c_str());
    ausgabe(t);
    return true;
}

bool istModellDatei(const std::string& pfad) {
    const std::string x = endungVon(pfad);
    return x == "obj" || x == "fbx" || x == "glb" || x == "gltf";
}

// Wohin etwas Abgelegtes kommt: unter dem Zeiger in einer 2D-Ansicht (XYWnd_MouseToPoint, dritte
// Achse = Mitte der Workzone), in der Kamera auf die Flaeche unter dem Zeiger (Scene_Intersect),
// sonst 128 vor der Kamera; auf das Raster.
karte::Vektor3f ablageOrt(ImVec2 punkt) {
    Editor& ed = e();
    karte::Vektor3f ort = ed.kamera.ursprung + (karte::Vektor3f(0, 0, 0) - ed.kamera.vpn) * 128.0F;
    for (std::size_t i = 0; i < ed.xy.size(); ++i) {
        XyFenster& f = ed.xy[i];
        const float bx = punkt.x - f.bildX;
        const float by = punkt.y - f.bildY;
        if (bx < 0 || by < 0 || bx >= static_cast<float>(f.ansicht.breite) || by >= static_cast<float>(f.ansicht.hoehe)) { continue; }
        ort = f.ansicht.zuPunkt(static_cast<int>(bx), static_cast<int>(by), ed.raster.einrastGroesse());
        const auto achse = static_cast<std::size_t>(karte::blickachse(f.ansicht.typ));
        const karte::Kasten& wz = ed.workzone;
        ort[achse] = karte::zahlEingerastet(karte::zahlMitte(wz.origin[achse] - wz.extents[achse], wz.origin[achse] + wz.extents[achse]), ed.raster.groesse());
        return ort;
    }
    const KameraFenster& kf = ed.kamFenster;
    const float kx = punkt.x - kf.bildX, ky = punkt.y - kf.bildY;
    if (kx >= 0 && ky >= 0 && kx < static_cast<float>(kf.breite) && ky < static_cast<float>(kf.hoehe)) {
        karte::Kamera& k = ed.kamera;
        const karte::GeraeteVektor eps(8.F / static_cast<float>(k.breite), 8.F / static_cast<float>(k.hoehe));
        ort = karte::szeneSchnitt(ed.karte, ed.blicke[3], k.geraetePunkt(kx, ky), eps);
    }
    karte::einrasten(ort, ed.raster.einrastGroesse());
    return ort;
}

bool modellAbgelegt(const std::string& pfad, ImVec2 punkt) {
    Editor& ed = e();
    karte::NetzOptionen no;
    no.yOben = ed.geoImport.yOben;
    no.skala = static_cast<double>(ed.geoImport.skala);
    karte::ImportNetz netz;
    std::string grund;
    char t[1400];
    if (!karte::netzLesenDatei(pfad, no, netz, &grund)) {
        std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), pfad.c_str(), grund.c_str());
        fehler(t);
        return false;
    }
    const karte::Md3Paket paket = karte::md3Paket(netz, stammVon(pfad));
    if (!paket.fehler.empty()) {
        std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), pfad.c_str(), paket.fehler.c_str());
        fehler(t);
        return false;
    }
    // in den Import-Ordner (eigener Suchordner, nie der Spielordner)
    const fs::path wurzel = pfadVon(platform::settingsDirectory() + "\\import");
    for (const karte::Md3Paket::Datei& d : paket.dateien) {
        const fs::path ziel = wurzel / pfadVon(d.pfad);
        std::error_code ec;
        fs::create_directories(ziel.parent_path(), ec);
        std::ofstream f(ziel, std::ios::binary);
        f.write(d.inhalt.data(), static_cast<std::streamsize>(d.inhalt.size()));
        if (!f) {
            std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), pfad.c_str(), "write");
            fehler(t);
            return false;
        }
    }
    const karte::Vektor3f ort = ablageOrt(punkt);
    {
        karte::Befehl undo(ed.karte.undo, "entityCreate -class misc_model");
        ed.karte.auswahl.leeren();
        auto ent = ed.karte.neueEntitaet("misc_model", false);
        char o[96];
        std::snprintf(o, sizeof(o), "%g %g %g", static_cast<double>(ort.x()), static_cast<double>(ort.y()), static_cast<double>(ort.z()));
        ent->setzen("origin", o);
        ent->setzen("model", paket.modellPfad);
        if (paket.modelscale != 1.0) {
            std::snprintf(o, sizeof(o), "%g", paket.modelscale);
            ent->setzen("modelscale", o);
        }
        ed.karte.entitaetEinfuegen(ent);
        ed.karte.auswahl.waehlen(ent);
    }
    ed.spielKennung.clear();   // den Suchpfad neu lesen: die neuen Dateien (EnginePath_Realise)
    ++ed.sichtStand;
    auswahlGeaendert();
    std::snprintf(t, sizeof(t), trKN(Str::KeMiFertig, paket.flaechen, paket.dreiecke), pfad.c_str(), paket.modellPfad.c_str(), static_cast<int>(paket.flaechen),
                  static_cast<int>(paket.dreiecke));
    ausgabe(t);
    return true;
}

bool modellUmwandelnNoetig(const std::string& pfad, bool nurMd3) {
    if (!karte::modellArtBekannt(pfad)) { return false; }
    const std::string x = endungVon(pfad);
    return x != "md3" && (nurMd3 || x != "glm");
}

ModellUmwandlung modellUmwandeln(const std::string& vfsPfad) {
    Editor& ed = e();
    char t[1400];
    const karte::ModellAnsicht a = kartenModellPfad(vfsPfad);
    if (!a.modell || a.modell->null) {
        std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), vfsPfad.c_str(), "model load failed");
        fehler(t);
        return {};
    }
    const std::shared_ptr<Spielkontext> sp = ed.karteSpiel;
    // Je Shader: ein Skript des Spiels bleibt; sonst das Bild, das das Spiel naehme (R_LoadImage ohne Endung: die Lader
    // in der Reihenfolge von R_ImageLoader_Init, rd-common/tr_image_load.cpp:95-97 - jpg, png, tga)
    const karte::ModellBildHolen holen = [sp](const std::string& shader, std::string& bild, std::string& bytes) {
        if (!sp) { return false; }
        if (const karte::ShaderEintrag* s = sp->inventar.shaderFinden(shader); s != nullptr && !s->istStandard()) { return false; }
        for (const char* x : {"jpg", "png", "tga"}) {
            if (sp->fs.dateiLesen(shader + "." + x, bytes)) {
                bild = shader + "." + x;
                return true;
            }
        }
        return false;
    };
    const karte::Md3Paket paket = karte::md3PaketAusModell(*a.modell, karte::importStamm(vfsPfad), holen);
    if (!paket.fehler.empty()) {
        std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), vfsPfad.c_str(), paket.fehler.c_str());
        fehler(t);
        return {};
    }
    const bool ersterOrdner = importOrdner().empty();
    const fs::path wurzel = pfadVon(platform::settingsDirectory() + "\\import");
    for (const karte::Md3Paket::Datei& d : paket.dateien) {
        const fs::path ziel = wurzel / pfadVon(d.pfad);
        std::string alt;
        {
            std::ifstream f(ziel, std::ios::binary);
            std::ostringstream o;
            o << f.rdbuf();
            alt = o.str();
        }
        if (alt == d.inhalt) { continue; }   // gleich: kein neuer Stand (die Live-Wache laedt sonst neu)
        std::error_code ec;
        fs::create_directories(ziel.parent_path(), ec);
        std::ofstream f(ziel, std::ios::binary);
        f.write(d.inhalt.data(), static_cast<std::streamsize>(d.inhalt.size()));
        if (!f) {
            std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), vfsPfad.c_str(), "write");
            fehler(t);
            return {};
        }
    }
    if (ersterOrdner) { ed.spielKennung.clear(); }   // EnginePath_Realise: der neue Import-Ordner kommt in den Suchpfad
    std::snprintf(t, sizeof(t), trKN(Str::KeMoUmgewandelt, paket.flaechen, paket.dreiecke), vfsPfad.c_str(), paket.modellPfad.c_str(),
                  static_cast<int>(paket.flaechen), static_cast<int>(paket.dreiecke));
    ausgabe(t);
    return {paket.modellPfad, paket.modelscale};
}

void geoImportDialogZeichnen() {
    GeoImportZustand& z = e().geoImport;
    if (!z.offen) { return; }
    const std::string titel = std::string(trK(Str::KeGiTitel)) + "###geoimport";
    // rc610 TUT-GEO: Groesse mit der Schrift des Karten-Editors (13 Punkte = 1, wie die Ansichten; sie waechst mit der
    // Oberflaechengroesse und den DPI), hoechstens 60 x 80 % des Fensters (daneben bleibt Platz fuer das Tutorial); die Tabelle fuellt es bis ueber die Knoepfe - bei
    // 150 % lag die dritte Zeile unter dem festen Tabellenrand von 220 Punkten, und die Vorschau war abgeschnitten
    const float sk = std::max(1.0F, ImGui::GetFontSize() / 13.0F);
    const ImVec2 platz = ImGui::GetMainViewport()->WorkSize;
    ImGui::SetNextWindowSize(ImVec2(std::min(std::round(760.0F * sk), platz.x * 0.6F), std::min(std::round(460.0F * sk), platz.y * 0.8F)),
                             ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen)) {
        ImGui::TextUnformatted(z.datei.c_str());
        bool neuLesen = false;
        // ASEB: ein misc_model hat seine Lage schon (modelscale); ein Modell wie q3map2 hat Z oben und seine Shader
        if (z.ersetzt.empty()) {
            ImGui::SetNextItemWidth(std::round(120.0F * sk));
            neuLesen |= ImGui::InputFloat((std::string(trK(Str::KeGiSkala)) + "##giskala").c_str(), &z.skala, 0.0F, 0.0F, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        }
        if (!z.modellNamen) {
            ImGui::SameLine();
            neuLesen |= ImGui::Checkbox((std::string(trK(Str::KeGiYOben)) + "##giyoben").c_str(), &z.yOben);
            ImGui::SetNextItemWidth(std::round(260.0F * sk));
            if (ImGui::InputText((std::string(trK(Str::KeGiVorsatz)) + "##givorsatz").c_str(), z.vorsatz, sizeof(z.vorsatz))) { z.neu = true; }
        } else {
            ImGui::TextDisabled("%s", trK(Str::KeGiModellNamen));
            if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled)) { ImGui::SetTooltip("%s", trK(Str::KeGiModellNamenTip)); }
        }
        ImGui::Checkbox((std::string(trK(Str::KeGiOptimierer)) + "##gioptimierer").c_str(), &z.optimierer);
        if (neuLesen && z.skala > 0.0F && z.ersetzt.empty()) {
            const std::vector<int> alt = z.wahl;
            if (geoImportLaden(z.datei) && alt.size() == z.wahl.size()) {
                z.wahl = alt;
                z.neu = true;
            }
        }
        const char* arten[3] = {trK(Str::KeGiAuto), trK(Str::KeGiPinsel), trK(Str::KeGiModellClip)};
        float artBreite = 0.0F;   // die Klappliste so breit wie ihr laengster Eintrag (in jeder Sprache)
        for (const char* a : arten) { artBreite = std::max(artBreite, ImGui::CalcTextSize(a).x); }
        artBreite += ImGui::GetFrameHeight() + ImGui::GetStyle().FramePadding.x * 2.0F;
        // ASEB: der Teil so breit wie sein laengster Name (Koerper "1: holz (+1)"; in Japanisch schnitt die alte Dehnung ab),
        // hoechstens 40 % der Tabelle - laengere zeigt der Tooltip ganz
        float teilBreite = ImGui::CalcTextSize(trK(Str::KeGiTeil)).x;
        for (const karte::ImportTeil& t : z.netz.teile) { teilBreite = std::max(teilBreite, ImGui::CalcTextSize(t.name.c_str()).x); }
        teilBreite = std::min(teilBreite + ImGui::GetStyle().CellPadding.x * 2.0F, ImGui::GetContentRegionAvail().x * 0.4F);
        // die Tabelle fuellt das Fenster bis ueber die Knoepfe; Dreiecke und Geschlossen so breit wie ihr Inhalt
        if (ImGui::BeginTable("##giteile", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY,
                              ImVec2(0.0F, -ImGui::GetFrameHeightWithSpacing()))) {
            ImGui::TableSetupScrollFreeze(0, 1);
            ImGui::TableSetupColumn(trK(Str::KeGiTeil), ImGuiTableColumnFlags_WidthFixed, teilBreite);
            ImGui::TableSetupColumn(trK(Str::KeGiDreiecke), ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(trK(Str::KeGiGeschlossen), ImGuiTableColumnFlags_WidthFixed);
            ImGui::TableSetupColumn(trK(Str::KeGiArt), ImGuiTableColumnFlags_WidthFixed, artBreite);
            ImGui::TableSetupColumn(trK(Str::KeGiErgebnis), ImGuiTableColumnFlags_WidthStretch, 2.0F);
            ImGui::TableHeadersRow();
            for (std::size_t i = 0; i < z.netz.teile.size() && i < z.vorschau.size(); ++i) {
                const karte::TeilErgebnis& r = z.vorschau[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(z.netz.teile[i].name.c_str());
                if (ImGui::IsItemHovered()) { ImGui::SetTooltip("%s", z.netz.teile[i].name.c_str()); }
                ImGui::TableNextColumn();
                ImGui::Text("%d", static_cast<int>(r.dreiecke));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(r.geschlossen ? trK(Str::KeGiJa) : trK(Str::KeGiNein));
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1);
                const std::string id = "##giart" + std::to_string(i);
                if (ImGui::Combo(id.c_str(), &z.wahl[i], arten, 3)) { z.neu = true; }
                // die Lage fuer den Selbsttest (ImGui meldet eine Klappliste nicht unter ihrer Kennung, wie klappe())
                e().klappOrte.push_back({id, ImGui::GetItemRectMin().x, ImGui::GetItemRectMin().y, ImGui::GetItemRectMax().x, ImGui::GetItemRectMax().y});
                ImGui::TableNextColumn();
                char t[256];
                std::snprintf(t, sizeof(t), trKN(Str::KeGiVorschauText, r.pinsel.size()), static_cast<int>(r.pinsel.size()),
                              r.detail ? trK(Str::KeGiDetail) : trK(Str::KeGiStruktur));
                std::string text = t;
                if (r.art == karte::TeilArt::ModellClip) { text += std::string(" + ") + trK(Str::KeGiModellClip); }
                // umbrechen statt abschneiden (p_karte: "1 個(detail) + モデル + クリップ" lief im Japanischen ueber den Rand)
                cjkTextWrapped("%s", text.c_str());
            }
            ImGui::EndTable();
        }
        if (z.neu) { vorschauRechnen(); }
        if (ImGui::Button((std::string(trK(Str::KeGiEinfuegen)) + "##gieinfuegen").c_str())) { geoImportEinfuegen(); }
        ImGui::SameLine();
        if (ImGui::Button((std::string(trK(Str::KeGiAbbrechen)) + "##giabbrechen").c_str())) { z.offen = false; }
    }
    ImGui::End();
    if (!offen) { z.offen = false; }
}

}  // namespace bhed::gui::karteneditor
