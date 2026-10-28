// app_karte_geoimport.cpp - Karten-Editor, Stufe 9 Punkt 2: Geometrie zu Brushes
// (eigene Funktion, nicht in NetRadiant-Custom).
//
// File > Import geometry as brushes...: eine .obj/.fbx/.glb/.gltf lesen (karte::netzLesen),
// je Teil Auto / Brushes / Modell + Clip waehlen, die Vorschau zeigt je Teil, was entsteht
// (Brushes, detail), "Einfuegen" setzt alles als eine Undo-Stufe ein (karte::importEinfuegen),
// danach auf Wunsch der Optimierer. Teile als Modell + Clip kommen als .obj in den
// Import-Ordner des Programms (<Einstellungen>/import/models/import), den Editor und
// q3map2 (-fs_pakpath) als Suchordner kennen - nie in einen Spielordner. Eine .bsp oeffnet
// "Open .bsp" mit dem Leser ihres Spiels (karte::q3map2TypFuerBsp).
#include "karteneditor.h"

#include "app_internal.h"
#include "platform.h"

#include "bhed/karte/brushexport.h"
#include "bhed/karte/karte.h"
#include "bhed/karte/modellimport.h"
#include "bhed/karte/rueckgaengig.h"
#include "bhed/karte/spielbeschreibung.h"

#include <algorithm>
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
    std::string bytes;
    {
        std::ifstream f(pfadVon(pfad), std::ios::binary);
        std::ostringstream s;
        s << f.rdbuf();
        bytes = s.str();
    }
    karte::NetzOptionen no;
    no.skala = static_cast<double>(z.skala);
    no.yOben = z.yOben;
    karte::ImportNetz netz;
    std::string grund;
    if (!karte::netzLesen(bytes, endungVon(pfad), no, netz, &grund)) {
        char t[1400];
        std::snprintf(t, sizeof(t), trK(Str::KeGiFehler), pfad.c_str(), grund.c_str());
        fehler(t);
        return false;
    }
    z.datei = pfad;
    z.netz = std::move(netz);
    z.wahl.assign(z.netz.teile.size(), static_cast<int>(karte::TeilArt::Auto));
    z.offen = true;
    vorschauRechnen();
    return true;
}

void geoImportOeffnen() {
    const std::string pfad = kartenDateiDialog(true, "Import geometry", "*.obj;*.fbx;*.glb;*.gltf;*.bsp|*.obj;*.fbx;*.glb;*.gltf;*.bsp|*.*|*.*",
                                               kartenStartOrdner());
    if (pfad.empty()) { return; }
    if (endungVon(pfad) == "bsp") {   // BSPs (auch anderer id-Tech-3-Spiele): wie Open .bsp
        (void)bspOeffnenDatei(pfad);
        return;
    }
    (void)geoImportLaden(pfad);
}

void geoImportEinfuegen() {
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
    z.letztes = karte::importEinfuegen(ed.karte, z.vorschau, modell);
    ++ed.sichtStand;
    auswahlGeaendert();
    char t[1400];
    std::snprintf(t, sizeof(t), trK(Str::KeGiFertig), z.datei.c_str(), static_cast<int>(z.letztes.pinsel), static_cast<int>(z.letztes.modelle));
    ausgabe(t);
    z.offen = false;
    if (z.optimierer) { optimiererOeffnen(); }
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
    // Ort: unter dem Zeiger in einer 2D-Ansicht (XYWnd_MouseToPoint, dritte Achse = Mitte der
    // Workzone), sonst 128 vor der Kamera; auf das Raster
    karte::Vektor3f ort = ed.kamera.ursprung + (karte::Vektor3f(0, 0, 0) - ed.kamera.vpn) * 128.0F;
    karte::einrasten(ort, ed.raster.einrastGroesse());
    for (std::size_t i = 0; i < ed.xy.size(); ++i) {
        XyFenster& f = ed.xy[i];
        const float bx = punkt.x - f.bildX;
        const float by = punkt.y - f.bildY;
        if (bx < 0 || by < 0 || bx >= static_cast<float>(f.ansicht.breite) || by >= static_cast<float>(f.ansicht.hoehe)) { continue; }
        ort = f.ansicht.zuPunkt(static_cast<int>(bx), static_cast<int>(by), ed.raster.einrastGroesse());
        const auto achse = static_cast<std::size_t>(karte::blickachse(f.ansicht.typ));
        const karte::Kasten& wz = ed.workzone;
        ort[achse] = karte::zahlEingerastet(karte::zahlMitte(wz.origin[achse] - wz.extents[achse], wz.origin[achse] + wz.extents[achse]), ed.raster.groesse());
        break;
    }
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
    std::snprintf(t, sizeof(t), trK(Str::KeMiFertig), pfad.c_str(), paket.modellPfad.c_str(), static_cast<int>(paket.flaechen),
                  static_cast<int>(paket.dreiecke));
    ausgabe(t);
    return true;
}

void geoImportDialogZeichnen() {
    GeoImportZustand& z = e().geoImport;
    if (!z.offen) { return; }
    const std::string titel = std::string(trK(Str::KeGiTitel)) + "###geoimport";
    ImGui::SetNextWindowSize(ImVec2(760, 460), ImGuiCond_Appearing);
    ImGui::SetNextWindowPos(ImGui::GetMainViewport()->GetCenter(), ImGuiCond_Appearing, ImVec2(0.5F, 0.5F));
    bool offen = true;
    if (ImGui::Begin(titel.c_str(), &offen)) {
        ImGui::TextUnformatted(z.datei.c_str());
        bool neuLesen = false;
        ImGui::SetNextItemWidth(120);
        neuLesen |= ImGui::InputFloat((std::string(trK(Str::KeGiSkala)) + "##giskala").c_str(), &z.skala, 0.0F, 0.0F, "%.3f", ImGuiInputTextFlags_EnterReturnsTrue);
        ImGui::SameLine();
        neuLesen |= ImGui::Checkbox((std::string(trK(Str::KeGiYOben)) + "##giyoben").c_str(), &z.yOben);
        ImGui::SetNextItemWidth(260);
        if (ImGui::InputText((std::string(trK(Str::KeGiVorsatz)) + "##givorsatz").c_str(), z.vorsatz, sizeof(z.vorsatz))) { z.neu = true; }
        ImGui::SameLine();
        ImGui::Checkbox((std::string(trK(Str::KeGiOptimierer)) + "##gioptimierer").c_str(), &z.optimierer);
        if (neuLesen && z.skala > 0.0F) {
            const std::vector<int> alt = z.wahl;
            if (geoImportLaden(z.datei) && alt.size() == z.wahl.size()) {
                z.wahl = alt;
                z.neu = true;
            }
        }
        if (ImGui::BeginTable("##giteile", 5, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg | ImGuiTableFlags_ScrollY, ImVec2(0, 220))) {
            ImGui::TableSetupColumn(trK(Str::KeGiTeil));
            ImGui::TableSetupColumn(trK(Str::KeGiDreiecke));
            ImGui::TableSetupColumn(trK(Str::KeGiGeschlossen));
            ImGui::TableSetupColumn(trK(Str::KeGiArt));
            ImGui::TableSetupColumn(trK(Str::KeGiErgebnis));
            ImGui::TableHeadersRow();
            const char* arten[3] = {trK(Str::KeGiAuto), trK(Str::KeGiPinsel), trK(Str::KeGiModellClip)};
            for (std::size_t i = 0; i < z.netz.teile.size() && i < z.vorschau.size(); ++i) {
                const karte::TeilErgebnis& r = z.vorschau[i];
                ImGui::TableNextRow();
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(z.netz.teile[i].name.c_str());
                ImGui::TableNextColumn();
                ImGui::Text("%d", static_cast<int>(r.dreiecke));
                ImGui::TableNextColumn();
                ImGui::TextUnformatted(r.geschlossen ? trK(Str::KeGiJa) : trK(Str::KeGiNein));
                ImGui::TableNextColumn();
                ImGui::SetNextItemWidth(-1);
                const std::string id = "##giart" + std::to_string(i);
                if (ImGui::Combo(id.c_str(), &z.wahl[i], arten, 3)) { z.neu = true; }
                ImGui::TableNextColumn();
                char t[256];
                std::snprintf(t, sizeof(t), trK(Str::KeGiVorschauText), static_cast<int>(r.pinsel.size()),
                              r.detail ? trK(Str::KeGiDetail) : trK(Str::KeGiStruktur));
                std::string text = t;
                if (r.art == karte::TeilArt::ModellClip) { text += std::string(" + ") + trK(Str::KeGiModellClip); }
                ImGui::TextUnformatted(text.c_str());
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
