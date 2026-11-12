// selbsttest_rest9.inc - REST9 (shank 09.11.): die Update-Leiste oben, wie im Programm G2C
//
// Eingebunden in gui/selbsttest.cpp (im selben Namensraum, mit allen Helfern davor).
//
//   BHED_EDITORTEST=updateleiste    die Leiste gegen eine simulierte GitHub-Antwort (updater::testNetzSetzen,
//                                   wie tests/uebergang mit g_simApi - kein echtes Netz): erscheint bei einer
//                                   neueren Fassung, nicht bei gleicher oder aelterer; Spaeter und x blenden aus,
//                                   der Hinweis in der Statuszeile bleibt; "Diese Fassung ueberspringen" haelt und
//                                   weicht einer noch neueren; Jetzt aktualisieren laedt (Fortschritt in der Leiste),
//                                   ein kaputter Download zeigt den Fehler mit Erneut versuchen, danach installiert
//                                   und Jetzt neu starten merkt den Neustart vor; Was ist neu? oeffnet das Fenster;
//                                   jede Schaltflaeche hat einen Tooltip; in allen fuenf Tabs rueckt der Arbeitsbereich
//                                   um die Hoehe der Leiste nach unten und nach dem Schliessen zurueck. Installiert
//                                   wird wirklich (eine Datei r9_update_probe.txt neben die .exe) - nur im Laufordner.
//                                   REST10 (Abgleich mit G2C): beim Laden [Abbrechen] mitten im (langsam simulierten)
//                                   Download und "x.x / y.y MB" im Balken; nach dem Laden [Spaeter]; die Suche von Hand
//                                   zeigt "Suche ...", "aktuell" und Fehler mit [Releases-Seite] [Schliessen] in der Leiste
//                                   (kein Fenster); Symbole Info/Haken/Fehler, lesbar in hellem und dunklem Thema; Wortwahl
//                                   "Diese Version ueberspringen".
//   BHED_EDITORTEST=updateleiste2   der zweite Start mit den Einstellungen des ersten (CFG=<einst/behaved.cfg>):
//                                   die uebersprungene Fassung meldet sich nicht, eine neuere schon.
//   BHED_EDITORTEST=handbuch BHED_HB_NUR=updateleiste   das Bild h_update_leiste.
namespace rest9 {

namespace fs = std::filesystem;

// Die simulierte GitHub-Seite. Der Hintergrundfaden des Updaters liest sie, darum unter einer Sperre.
struct Sim {
    std::mutex m;
    std::string tag;              // das "neueste Release"
    bool keinNetz = false;        // keine Verbindung
    bool kaputt = false;          // das .zip passt nicht zu SHA256SUMS.txt
    std::atomic<bool> halt{false};   // der Download wartet (Laedt bleibt stehen)
    std::string zip;              // die Bytes des .zip
    std::string summen;           // SHA256SUMS.txt
    std::atomic<int> summenAnfragen{0};   // REST10: so oft wurde SHA256SUMS.txt geholt (erst nach dem ganzen .zip)
};
Sim& sim() {
    static Sim* s = new Sim();   // nie freigegeben: der Hintergrundfaden kann beim Beenden noch lesen
    return *s;
}
constexpr const char* kZipName = "JK-Studio-sim-windows-x64.zip";
constexpr const char* kProbe = "r9_update_probe.txt";
constexpr const char* kPolster = "r10_update_polster.bin";   // REST10: macht das .zip gross genug fuer "x.x / y.y MB"

int lokalNr() { return bhed::update::rcNummer(updater::lokaleFassung()); }
std::string tagNr(int n) { return "v1.0.0-rc" + std::to_string(n); }
std::string kurz(int n) { return "rc" + std::to_string(n); }

std::string releaseJson(const std::string& tag, const std::string& notizen) {
    const std::string basis = "https://github.com/DennisHerrm/JK-Studio/releases/download/" + tag + "/";
    const std::size_t zipGroesse = sim().zip.size();
    return "{\"tag_name\":\"" + tag + "\",\"name\":\"JK Studio " + tag + "\",\"body\":\"" + notizen +
           "\",\"html_url\":\"https://github.com/DennisHerrm/JK-Studio/releases/tag/" + tag + "\",\"assets\":["
           "{\"name\":\"" + kZipName + "\",\"url\":\"https://api.github.com/repos/DennisHerrm/JK-Studio/releases/assets/1\","
           "\"browser_download_url\":\"" + basis + kZipName + "\",\"size\":" + std::to_string(zipGroesse) + "},"
           "{\"name\":\"SHA256SUMS.txt\",\"url\":\"https://api.github.com/repos/DennisHerrm/JK-Studio/releases/assets/2\","
           "\"browser_download_url\":\"" + basis + "SHA256SUMS.txt\",\"size\":" + std::to_string(sim().summen.size()) + "}]}";
}

bool endetMit(const std::string& s, const std::string& e) { return s.size() >= e.size() && s.compare(s.size() - e.size(), e.size(), e) == 0; }

// Das .zip des simulierten Releases. `gross` (REST10): dazu 3 MB Zufallsbytes (lassen sich nicht packen) -
// fuer das Abbrechen mitten im Laden und die MB-Angabe im Balken.
void simZip(bool gross) {
    {
        std::vector<Pk3Datei> inhalt{{std::string("JK-Studio/") + kProbe, "REST9 update bar probe\n"}};
        if (gross) {
            std::string polster(3U * 1024U * 1024U, '\0');
            std::uint32_t x = 0x12345678U;
            for (char& c : polster) {
                x = x * 1664525U + 1013904223U;
                c = static_cast<char>(x >> 24);
            }
            inhalt.push_back({std::string("JK-Studio/") + kPolster, polster});
        }
        const std::string ordner = platform::executableDirectory() + "/r9_sim";
        std::error_code ec;
        fs::create_directories(::bhed::utf8Pfad(ordner), ec);
        const std::string pfad = ordner + "/" + kZipName;
        std::string fehler;
        if (!writePk3(pfad, inhalt, &fehler)) { meldeFehler("REST9: Test-.zip nicht geschrieben: " + fehler); }
        std::ifstream f(::bhed::utf8Pfad(pfad), std::ios::binary);
        std::lock_guard<std::mutex> l(sim().m);
        sim().zip.assign(std::istreambuf_iterator<char>(f), std::istreambuf_iterator<char>());
        sim().summen = bhed::update::sha256Hex(sim().zip) + "  " + kZipName + "\n";
    }
}

// Die Antworten: /releases/latest, das .zip, SHA256SUMS.txt - sonst 404.
void simEinschalten() {
    simZip(false);
    updater::testNetzSetzen([](const std::string& adresse, int& status, std::string& inhalt) {
        if (endetMit(adresse, kZipName)) {
            while (sim().halt.load()) { std::this_thread::sleep_for(std::chrono::milliseconds(10)); }
        }
        std::lock_guard<std::mutex> l(sim().m);
        if (sim().keinNetz) { return false; }
        status = 200;
        if (adresse.find("/releases/latest") != std::string::npos) {
            inhalt = releaseJson(sim().tag, "- New: an update bar under the menu bar\\n- Fixed: everything else");
        } else if (endetMit(adresse, kZipName)) {
            inhalt = sim().zip;
            if (sim().kaputt) { inhalt[inhalt.size() / 2] = static_cast<char>(inhalt[inhalt.size() / 2] ^ 0x5A); }
        } else if (endetMit(adresse, "SHA256SUMS.txt")) {
            inhalt = sim().summen;
            ++sim().summenAnfragen;
        } else {
            status = 404;
        }
        diag::detail("Knopftest: REST9 Sim - " + adresse + " -> " + std::to_string(status));
        return true;
    });
}

void simTag(int n) {
    std::lock_guard<std::mutex> l(sim().m);
    sim().tag = tagNr(n);
}

// Nachsehen wie beim Start (stumm, ohne Fenster) und warten, bis die Antwort da ist.
void nachsehen(std::vector<Schritt>& s, const std::string& was) {
    s.push_back(tu("REST9: nachsehen - " + was, [] { updater::pruefen(true); }));
    s.push_back(warteBis("REST9: Antwort - " + was, [] { return updater::zustand().stand != updater::Stand::Pruefe; }, 600));
    s.push_back(pause(0.2));
}

// Die Leiste ist sichtbar UND wurde im letzten Bild gezeichnet.
bool leisteDa() { return updater::leisteSichtbar() && updater::leisteBild().zeilen > 0; }

std::string standText() {
    const updater::Zustand z = updater::zustand();
    const updater::LeistenBild b = updater::leisteBild();
    return "Stand " + std::to_string(static_cast<int>(z.stand)) + ", Release " + z.release.tag + ", Leiste " +
           (updater::leisteSichtbar() ? "sichtbar" : "weg") + " (" + std::to_string(b.zeilen) + " Zeilen, \"" + b.text + "\"), Meldung " +
           z.meldung;
}

// Der Hinweis in der Statuszeile (unten, kleiner gruener Knopf).
bool statusHinweisDa(const std::string& tag) {
    char t[200];
    std::snprintf(t, sizeof(t), tr(Str::UpdStatusAvail), tag.c_str());
    return finde(t, "###main", 0, false) != nullptr;
}

ImGuiWindow* hauptfenster() {
    for (ImGuiWindow* w : GImGui->Windows) {
        if (std::strstr(w->Name, "###main") != nullptr && w->RootWindow == w && (w->Flags & ImGuiWindowFlags_ChildWindow) == 0) { return w; }
    }
    return nullptr;
}

// Unterkante der Menueleiste des Hauptfensters.
float menueUnten() {
    const ImGuiWindow* w = hauptfenster();
    return w != nullptr ? w->Pos.y + w->TitleBarHeight + w->MenuBarHeight : -1.0F;
}

// Wo der Arbeitsbereich beginnt: die oberste Kante aller Kindfenster und angedockten Fenster des Hauptfensters.
std::string g_obersterName;   // welches Fenster dort steht (fuers Protokoll)
float inhaltOben() {
    const ImGuiWindow* haupt = hauptfenster();
    float oben = FLT_MAX;
    for (ImGuiWindow* w : GImGui->Windows) {
        if (w == haupt || !w->WasActive || w->Hidden || w->RootWindowDockTree != haupt) { continue; }
        if (w->Pos.y < oben) {
            oben = w->Pos.y;
            g_obersterName = w->Name;
        }
    }
    return oben;
}

// Eine Schaltflaeche der Leiste: Maus darauf, bis ihr Tooltip steht.
Schritt tooltipDa(const std::string& label) {
    return {"REST9: Tooltip " + label, [label](int b) {
                const Element* e = finde(label, "###main", 0, false);
                if (e == nullptr) {
                    meldeFehler("REST9: " + label + " nicht gefunden");
                    return true;
                }
                if (b == 0) {
                    setzeMaus(e->rect.GetCenter());
                    return false;
                }
                for (ImGuiWindow* w : GImGui->Windows) {
                    if (std::strncmp(w->Name, "##Tooltip_", 10) == 0 && w->Active) {
                        meldeOk("REST9: " + label.substr(0, label.find("##")) + " hat einen Tooltip");
                        return true;
                    }
                }
                if (b > 90) {
                    meldeFehler("REST9: " + label + " ohne Tooltip");
                    return true;
                }
                return false;
            }};
}

Schritt mausWeg() {
    return tu("REST9: Maus weg von der Leiste", [] {
        if (const ImGuiWindow* w = hauptfenster()) { setzeMaus(ImVec2{w->Pos.x + w->Size.x * 0.5F, w->Pos.y + w->Size.y - 4.0F}); }
    });
}

// Das Update-Fenster ueber sein x schliessen.
Schritt fensterZu() {
    auto ziel = std::make_shared<ImVec2>();
    return {"REST9: Update-Fenster schliessen", [=](int b) {
                if (b == 0) {
                    ImGuiWindow* w = ImGui::FindWindowByName("###update");
                    const Element* e = w != nullptr ? findeKennung(ImHashStr("#CLOSE", 0, w->ID)) : nullptr;
                    if (e == nullptr) {
                        meldeFehler("REST9: Schliessknopf des Update-Fensters nicht gefunden");
                        return true;
                    }
                    *ziel = e->rect.GetCenter();
                    setzeMaus(*ziel);
                    return false;
                }
                if (b == 1) { ImGui::GetIO().AddMouseButtonEvent(0, true); return false; }
                if (b == 2) { ImGui::GetIO().AddMouseButtonEvent(0, false); return false; }
                return b >= 5;
            }};
}

std::string knopf(Str id, const char* kennung) { return std::string(tr(id)) + "###updleiste_" + kennung; }

// --- REST10 -------------------------------------------------------------------------------------
std::string mitText(Str id, const std::string& a, const std::string& b = {}) {
    char z[1024];
    std::snprintf(z, sizeof(z), tr(id), a.c_str(), b.c_str());
    return z;
}

const char* symbolName(updater::LeistenSymbol s) {
    switch (s) {
        case updater::LeistenSymbol::Info: return "Info";
        case updater::LeistenSymbol::Haken: return "Haken";
        case updater::LeistenSymbol::Fehler: return "Fehler";
        default: return "keins";
    }
}

// Kontrast nach WCAG 2.x (relative Leuchtdichte, sRGB) zwischen zwei ImU32-Farben.
double leuchtdichte(unsigned c) {
    const auto kanal = [](unsigned v) {
        const double x = static_cast<double>(v & 0xFFU) / 255.0;
        return x <= 0.04045 ? x / 12.92 : std::pow((x + 0.055) / 1.055, 2.4);
    };
    return 0.2126 * kanal(c >> IM_COL32_R_SHIFT) + 0.7152 * kanal(c >> IM_COL32_G_SHIFT) + 0.0722 * kanal(c >> IM_COL32_B_SHIFT);
}
double kontrast(unsigned a, unsigned b) {
    const double la = leuchtdichte(a), lb = leuchtdichte(b);
    return (std::max(la, lb) + 0.05) / (std::min(la, lb) + 0.05);
}

// Das Symbol steht links vom Text in der ersten Zeile der Leiste, in einer Farbe mit Kontrast >= 4.5 zum Grund.
bool symbolOk(updater::LeistenSymbol erwartet) {
    const updater::LeistenBild b = updater::leisteBild();
    const double k = kontrast(b.symbolFarbe, b.grundFarbe);
    char z[200];
    std::snprintf(z, sizeof(z), "Knopftest: REST10 Symbol %s (erwartet %s) bei %.1f,%.1f-%.1f,%.1f, Kontrast %.2f", symbolName(b.symbol),
                  symbolName(erwartet), static_cast<double>(b.sx0), static_cast<double>(b.sy0), static_cast<double>(b.sx1),
                  static_cast<double>(b.sy1), k);
    diag::detail(z);
    return b.symbol == erwartet && b.sx1 - b.sx0 > 8.0F && b.sx0 >= b.x0 && b.sy0 >= b.y0 && b.sy1 <= b.y1 && k >= 4.5;
}

bool da(const std::string& label) { return finde(label, "###main", 0, false) != nullptr; }

std::string mb(std::uint64_t geladen, std::uint64_t gesamt) {
    char z[64];
    std::snprintf(z, sizeof(z), "%.1f / %.1f MB", static_cast<double>(geladen) / (1024.0 * 1024.0), static_cast<double>(gesamt) / (1024.0 * 1024.0));
    return z;
}

// Liegt irgendwo in der Ablage des Updaters (%TEMP%/behaved_update) noch das .zip?
bool zipInAblage() {
    std::error_code ec;
    const fs::path ablage = fs::temp_directory_path(ec) / "behaved_update";
    for (fs::recursive_directory_iterator it(ablage, ec), ende; !ec && it != ende; it.increment(ec)) {
        if (it->path().filename() == kZipName) { return true; }
    }
    return false;
}

bool dateiNebenExe(const char* name) {
    std::error_code ec;
    return fs::exists(::bhed::utf8Pfad(platform::executableDirectory() + "/" + name), ec);
}

// Die Suche von Hand wie das Menue Hilfe > Nach Updates suchen.
void menueSuche(std::vector<Schritt>& s) {
    s.push_back(klick(tr(Str::MenuHelp), "###main"));
    s.push_back(pause(0.3));
    s.push_back(klick(tr(Str::UpdMenu), "##Menu"));
}

// Lage in einem Tab: mit Leiste, ohne (Schliessen = die uebersprungene Fassung), wieder mit.
struct Lage {
    float mit = 0.0F, ohne = 0.0F, wieder = 0.0F, hoehe = 0.0F, leisteOben = 0.0F, leisteUnten = 0.0F, menue = 0.0F;
    float editorMit = 0.0F, editorOhne = 0.0F;
};

void lageInTab(std::vector<Schritt>& s, int modus, const char* name, int uebersprungen) {
    auto l = std::make_shared<Lage>();
    s.push_back(tu(std::string("REST9: Tab ") + name, [modus] { g_app->leftMode = modus; }));
    s.push_back(mausWeg());   // REST10: kein Tooltip eines Knopfs der Leiste unter der stehenden Maus (wechselt die Elemente mitten in der Ruhepruefung)
    s.push_back(stabil(std::string("REST9 ") + name + " mit Leiste", 40));
    s.push_back(foto("r9_lage_" + std::to_string(modus)));
    s.push_back(tu(std::string("REST9: ") + name + " messen (mit Leiste)", [l] {
        const updater::LeistenBild b = updater::leisteBild();
        l->mit = inhaltOben();
        diag::detail("Knopftest: REST9 oberstes Fenster mit Leiste: " + g_obersterName);
        l->hoehe = b.y1 - b.y0;
        l->leisteOben = b.y0;
        l->leisteUnten = b.y1;
        l->menue = menueUnten();
        l->editorMit = karteneditor::editor().editorOben;
        g_app->settings.updateUebersprungen = tagNr(lokalNr() + 2);   // die Leiste geht weg
    }));
    s.push_back(stabil(std::string("REST9 ") + name + " ohne Leiste", 40));
    s.push_back(tu(std::string("REST9: ") + name + " messen (ohne Leiste)", [l, uebersprungen] {
        l->ohne = inhaltOben();
        diag::detail("Knopftest: REST9 oberstes Fenster ohne Leiste: " + g_obersterName);
        l->editorOhne = karteneditor::editor().editorOben;
        diag::detail("Knopftest: REST9 Leiste nach Schliessen: " + standText());
        g_app->settings.updateUebersprungen = tagNr(uebersprungen);   // wieder da
    }));
    s.push_back(stabil(std::string("REST9 ") + name + " wieder mit Leiste", 40));
    s.push_back(pruefe(std::string("REST9: ") + name + " - die Leiste steht direkt unter der Menueleiste, der Arbeitsbereich beginnt um ihre Hoehe tiefer "
                           "(nichts verdeckt) und nach dem Schliessen wieder oben",
                       [l, modus] {
                           l->wieder = inhaltOben();
                           char z[400];
                           std::snprintf(z, sizeof(z), "Knopftest: REST9 Lage Tab %d: Menue %.1f, Leiste %.1f-%.1f (%.1f), Inhalt mit %.1f, ohne %.1f, wieder %.1f, "
                                         "Editor mit %.1f ohne %.1f",
                                         modus, static_cast<double>(l->menue), static_cast<double>(l->leisteOben), static_cast<double>(l->leisteUnten),
                                         static_cast<double>(l->hoehe), static_cast<double>(l->mit), static_cast<double>(l->ohne),
                                         static_cast<double>(l->wieder), static_cast<double>(l->editorMit), static_cast<double>(l->editorOhne));
                           diag::detail(z);
                           bool ok = l->hoehe > 10.0F && std::fabs(l->leisteOben - l->menue) < 0.5F && l->mit >= l->leisteUnten - 0.5F &&
                                     std::fabs((l->mit - l->ohne) - l->hoehe) < 0.5F && std::fabs(l->wieder - l->mit) < 0.5F &&
                                     leisteDa();
                           if (modus == 3) {   // der Map Editor setzt sich an leisteUnten
                               ok = ok && std::fabs(l->editorMit - l->leisteUnten) < 0.5F && std::fabs((l->editorMit - l->editorOhne) - l->hoehe) < 0.5F;
                           }
                           return ok;
                       }));
}

std::vector<Schritt> schritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const int k = lokalNr();
    add(tu("REST9: simulierte GitHub-Antwort, Einstellung leer", [] {
        g_app->settings.updateUebersprungen.clear();
        std::error_code ec;
        fs::remove(::bhed::utf8Pfad(platform::executableDirectory() + "/" + kProbe), ec);
        simEinschalten();
        updater::testNeuerStart();
    }));
    add(pruefe("REST9: ohne Fund keine Leiste", [] { return !updater::leisteSichtbar() && updater::leisteBild().zeilen == 0; }));

    // --- gleiche und aeltere Fassung: keine Leiste ---------------------------
    add(tu("REST9: Release aelter", [k] { simTag(k - 1); }));
    nachsehen(s, "aelter");
    add(pruefe("REST9: aelteres Release - aktuell, keine Leiste", [] {
        diag::detail("Knopftest: REST9 aelter: " + standText());
        return updater::zustand().stand == updater::Stand::Aktuell && !updater::leisteSichtbar() && updater::leisteBild().zeilen == 0;
    }));
    add(tu("REST9: Release gleich", [k] { simTag(k); }));
    nachsehen(s, "gleich");
    add(pruefe("REST9: gleiches Release - aktuell, keine Leiste", [] {
        diag::detail("Knopftest: REST9 gleich: " + standText());
        return updater::zustand().stand == updater::Stand::Aktuell && !updater::leisteSichtbar() && updater::leisteBild().zeilen == 0;
    }));

    // --- neuere Fassung: die Leiste ---------------------------------------------
    add(tu("REST9: Release neuer", [k] { simTag(k + 1); }));
    nachsehen(s, "neuer");
    add(mausWeg());
    add(pause(0.3));
    add(foto("r9_leiste"));
    add(pruefe("REST9: neueres Release - die Leiste steht unter der Menueleiste mit \"" + kurz(k + 1) + "\" und \"" + kurz(k) +
                   "\", ohne Fenster, der Hinweis unten ist auch da",
               [k] {
                   const updater::LeistenBild b = updater::leisteBild();
                   diag::detail("Knopftest: REST9 neuer: " + standText());
                   return updater::zustand().stand == updater::Stand::Verfuegbar && leisteDa() && b.zeilen == 1 && !b.gekuerzt &&
                          b.text.find(kurz(k + 1)) != std::string::npos && b.text.find(kurz(k)) != std::string::npos &&
                          std::fabs(b.y0 - menueUnten()) < 0.5F && !updater::fensterOffen() && statusHinweisDa(tagNr(k + 1));
               }));
    add(pruefe("REST10: verfuegbar - Info-Symbol vor dem Text, lesbar", [] { return symbolOk(updater::LeistenSymbol::Info); }));
    // REST10 Punkt 4: in hellem und dunklem Thema lesbar (Foto je Thema).
    {
        auto themaVorher = std::make_shared<std::string>();
        for (const char* id : {"light", "dark"}) {
            const std::string tid = id;
            add(tu(std::string("REST10: Farbschema ") + id, [tid, themaVorher] {
                if (themaVorher->empty() && g_app->activeTheme != nullptr) { *themaVorher = g_app->activeTheme->id; }
                if (const theme::Theme* t = theme::findTheme(tid)) {   // wie setThemeById (app.cpp)
                    g_app->activeTheme = t;
                    g_app->themePending = true;
                }
            }));
            add(pause(0.4));
            add(foto(std::string("r10_leiste_") + id));
            add(pruefe(std::string("REST10: Farbschema ") + id + " - Symbol und Text lesbar (Kontrast >= 4.5)", [] {
                return symbolOk(updater::LeistenSymbol::Info);
            }));
        }
        add(tu("REST10: Farbschema zurueck", [themaVorher] {
            if (const theme::Theme* t = theme::findTheme(*themaVorher)) {
                g_app->activeTheme = t;
                g_app->themePending = true;
            }
        }));
        add(pause(0.3));
    }
    // REST10 Punkt 5: "Diese Version ueberspringen" wie G2C - in keiner Sprache mehr "Fassung".
    add(pruefe("REST10: Wortwahl \"Diese Version ueberspringen\" (de), alle 6 Sprachen eigen, Tooltips ohne \"Fassung\"", [] {
        bool ok = std::string(i18n::trIn(i18n::Language::German, Str::UpdBarSkip)) == "Diese Version \xC3\xBC" "berspringen";
        for (const i18n::LanguageInfo& l : i18n::languages()) {
            for (Str id : {Str::UpdBarSkip, Str::UpdBarSkipHint, Str::UpdBarRestartHint}) {
                const std::string t = i18n::trIn(l.language, id);
                if (t.find("Fassung") != std::string::npos || (l.language != i18n::Language::English && !i18n::hatEigenenText(l.language, id))) {
                    diag::detail(std::string("Knopftest: REST10 Wortwahl ") + l.code + ": " + t);
                    ok = false;
                }
            }
        }
        return ok;
    }));
    for (const std::string& l : {knopf(Str::UpdBarNow, "jetzt"), knopf(Str::UpdBarWhatsNew, "neu"), knopf(Str::UpdLater, "spaeter"),
                                 knopf(Str::UpdBarSkip, "skip"), std::string("x###updleiste_zu")}) {
        add(tooltipDa(l));
    }
    add(mausWeg());
    // Schmal: die Knoepfe stehen unter dem Text, abgeschnitten wird nichts.
    add(tu("REST9: schmale Leiste", [] { updater::testLeistenBreite(ImGui::GetFontSize() * 22.0F); }));
    add(pause(0.2));
    add(foto("r9_leiste_schmal"));
    add(pruefe("REST9: schmal - zwei Zeilen, die Knoepfe unter dem Text, alle ganz sichtbar", [] {
        const updater::LeistenBild b = updater::leisteBild();
        const Element* jetzt = finde(knopf(Str::UpdBarNow, "jetzt"), "###main", 0, false);
        const Element* skip = finde(knopf(Str::UpdBarSkip, "skip"), "###main", 0, false);
        diag::detail("Knopftest: REST9 schmal: " + standText());
        return b.zeilen == 2 && jetzt != nullptr && skip != nullptr && ganzSichtbar(*jetzt) && ganzSichtbar(*skip) && jetzt->rect.Min.y > b.y0 + (b.y1 - b.y0) * 0.4F &&
               skip->rect.Max.y <= b.y1;
    }));
    add(tu("REST9: Leiste wieder breit", [] { updater::testLeistenBreite(0.0F); }));
    add(pause(0.2));

    // --- Was ist neu? --------------------------------------------------------------
    add(klick(knopf(Str::UpdBarWhatsNew, "neu"), "###main"));
    add(pause(0.3));
    add(pruefe("REST9: Was ist neu? oeffnet das Update-Fenster mit den Notizen", [] {
        bool notizen = false;
        for (ImGuiWindow* w : GImGui->Windows) {
            notizen = notizen || (w->WasActive && std::strstr(w->Name, "###update") != nullptr && std::strstr(w->Name, "##notizen") != nullptr);
        }
        diag::detail(std::string("Knopftest: REST9 Was ist neu: Fenster ") + (updater::fensterOffen() ? "offen" : "zu") + ", Notizen " + (notizen ? "ja" : "nein"));
        return updater::fensterOffen() && notizen;
    }));
    add(fensterZu());
    add(pause(0.3));
    add(pruefe("REST9: Update-Fenster zu, die Leiste bleibt", [] { return !updater::fensterOffen() && leisteDa(); }));

    // --- Spaeter: bis zum naechsten Start weg, der Hinweis unten bleibt --------------
    auto ohne = std::make_shared<float>(0.0F);
    auto mit = std::make_shared<float>(0.0F);
    add(tu("REST9: Lage mit Leiste merken", [mit] { *mit = inhaltOben(); }));
    add(klick(knopf(Str::UpdLater, "spaeter"), "###main"));
    add(pause(0.3));
    add(pruefe("REST9: Spaeter - die Leiste ist weg, der Hinweis in der Statuszeile bleibt, der Arbeitsbereich rueckt hoch", [k, mit, ohne] {
        *ohne = inhaltOben();
        diag::detail("Knopftest: REST9 Spaeter: " + standText() + ", Inhalt oben mit " + std::to_string(*mit) + " ohne " + std::to_string(*ohne));
        return !updater::leisteSichtbar() && updater::leisteBild().zeilen == 0 && statusHinweisDa(tagNr(k + 1)) && *ohne < *mit - 10.0F;
    }));
    nachsehen(s, "nach Spaeter, dieselbe Fassung");
    add(pruefe("REST9: Spaeter gilt bis zum naechsten Start - nochmal nachsehen zeigt die Leiste nicht", [] {
        return updater::zustand().stand == updater::Stand::Verfuegbar && !updater::leisteSichtbar();
    }));
    add(tu("REST10: nach Spaeter von Hand suchen", [] { updater::pruefen(false); }));
    add(warteBis("REST10: Antwort", [] { return updater::zustand().stand != updater::Stand::Pruefe; }, 600));
    add(pause(0.2));
    add(pruefe("REST10: von Hand gesucht zeigt die Leiste auch nach Spaeter wieder, kein Fenster", [] {
        diag::detail("Knopftest: REST10 von Hand nach Spaeter: " + standText());
        return updater::zustand().stand == updater::Stand::Verfuegbar && leisteDa() && !updater::fensterOffen();
    }));
    add(tu("REST9: wie ein neuer Start", [] { updater::testNeuerStart(); }));
    nachsehen(s, "neuer Start");
    add(pruefe("REST9: nach dem naechsten Start ist die Leiste wieder da", [] { return leisteDa(); }));

    // --- x wie Spaeter ----------------------------------------------------------------
    add(klick("x###updleiste_zu", "###main"));
    add(pause(0.3));
    add(pruefe("REST9: x - die Leiste ist weg, der Hinweis bleibt", [k] { return !updater::leisteSichtbar() && statusHinweisDa(tagNr(k + 1)); }));

    // --- Diese Fassung ueberspringen ------------------------------------------------------
    add(tu("REST9: neuer Start", [] { updater::testNeuerStart(); }));
    nachsehen(s, "vor Ueberspringen");
    add(klick(knopf(Str::UpdBarSkip, "skip"), "###main"));
    add(pause(0.3));
    add(pruefe("REST9: ueberspringen - die Leiste ist weg, die Fassung steht in den Einstellungen", [k] {
        diag::detail("Knopftest: REST9 ueberspringen: " + standText() + ", Einstellung " + g_app->settings.updateUebersprungen);
        return !updater::leisteSichtbar() && g_app->settings.updateUebersprungen == tagNr(k + 1);
    }));
    add(tu("REST9: neuer Start", [] { updater::testNeuerStart(); }));
    nachsehen(s, "uebersprungene Fassung");
    add(pruefe("REST9: die uebersprungene Fassung meldet sich auch nach einem neuen Start nicht (Hinweis unten bleibt)", [k] {
        return updater::zustand().stand == updater::Stand::Verfuegbar && !updater::leisteSichtbar() && statusHinweisDa(tagNr(k + 1));
    }));
    add(tu("REST10: die uebersprungene Version von Hand suchen", [] { updater::pruefen(false); }));
    add(warteBis("REST10: Antwort", [] { return updater::zustand().stand != updater::Stand::Pruefe; }, 600));
    add(pause(0.2));
    add(pruefe("REST10: von Hand gesucht zeigt auch die uebersprungene Version (wie G2C), die Einstellung bleibt", [k] {
        return leisteDa() && updater::leisteBild().text.find(kurz(k + 1)) != std::string::npos && g_app->settings.updateUebersprungen == tagNr(k + 1);
    }));
    // FIX612 R7: nach der Suche von Hand blendet "Diese Version ueberspringen" die Leiste ebenfalls aus (G2C drawUpdateBanner:
    // UpdSkip setzt die Einstellung und updateBannerHidden_); die naechste Suche von Hand zeigt sie wieder
    add(tu("FIX612 R7: die Einstellung leeren (der Klick muss sie setzen)", [] { g_app->settings.updateUebersprungen.clear(); }));
    add(pause(0.2));
    add(klick(knopf(Str::UpdBarSkip, "skip"), "###main"));
    add(pause(0.3));
    add(pruefe("FIX612 R7: nach einer Suche von Hand - ueberspringen blendet die Leiste aus, die Version steht in den Einstellungen", [k] {
        diag::detail("Knopftest: FIX612 R7 ueberspringen nach Suche von Hand: " + standText() + ", Leiste " + (updater::leisteSichtbar() ? "SICHTBAR" : "weg") + ", Einstellung " +
                     g_app->settings.updateUebersprungen);
        return !updater::leisteSichtbar() && g_app->settings.updateUebersprungen == tagNr(k + 1) && statusHinweisDa(tagNr(k + 1));
    }));
    add(tu("FIX612 R7: noch einmal von Hand suchen", [] { updater::pruefen(false); }));
    add(warteBis("FIX612 R7: Antwort", [] { return updater::zustand().stand != updater::Stand::Pruefe; }, 600));
    add(pause(0.2));
    add(pruefe("FIX612 R7: die naechste Suche von Hand zeigt die uebersprungene Version wieder (wie G2C)", [k] {
        return leisteDa() && updater::leisteBild().text.find(kurz(k + 1)) != std::string::npos && g_app->settings.updateUebersprungen == tagNr(k + 1);
    }));
    add(tu("REST9: Release noch neuer", [k] { simTag(k + 2); }));
    nachsehen(s, "noch neuer");
    add(pruefe("REST9: eine noch neuere Fassung zeigt die Leiste wieder", [k] {
        return leisteDa() && updater::leisteBild().text.find(kurz(k + 2)) != std::string::npos;
    }));

    // --- Lage in allen fuenf Tabs ------------------------------------------------------------
    lageInTab(s, 0, "Events", k + 1);
    lageInTab(s, 1, "Mission", k + 1);
    lageInTab(s, 2, "Model", k + 1);
    lageInTab(s, 3, "Map Editor", k + 1);
    lageInTab(s, 4, "Menu Editor", k + 1);
    add(tu("REST9: zurueck zu Events", [] { g_app->leftMode = 0; }));
    add(stabil("REST9 Events", 40));

    // --- REST10 Punkt 1: [Abbrechen] mitten im Laden, "x.x / y.y MB" im Balken -----------------------
    add(tu("REST10: grosses .zip (3 MB), langsames Netz (400 ms je Sechzehntel)", [] {
        simZip(true);
        sim().summenAnfragen = 0;
        updater::testStueckPause(400);
    }));
    add(klick(knopf(Str::UpdBarNow, "jetzt"), "###main"));
    add(warteBis("REST10: ein Teil geladen", [] {
        const updater::Zustand z = updater::zustand();
        return z.stand == updater::Stand::Laedt && z.fortschritt >= 0.12;
    }, 600));
    add(pause(0.1));
    add(foto("r10_leiste_laedt_mb"));
    add(pruefe("REST10: beim Laden - Info-Symbol, \"x.x / y.y MB\" im Balken (Gesamt = Groesse des .zip), [Abbrechen] da", [] {
        const updater::Zustand z = updater::zustand();
        const updater::LeistenBild b = updater::leisteBild();
        std::size_t groesse = 0;
        {
            std::lock_guard<std::mutex> l(sim().m);
            groesse = sim().zip.size();
        }
        diag::detail("Knopftest: REST10 laedt: " + standText() + ", Balken \"" + b.balken + "\", erwartet \"" + mb(z.geladen, z.gesamt) + "\", .zip " +
                     std::to_string(groesse) + " Bytes, geladen " + std::to_string(z.geladen));
        return z.stand == updater::Stand::Laedt && z.gesamt == groesse && z.geladen > 0 && z.geladen < z.gesamt && b.balken.size() > 6 &&
               b.balken.compare(b.balken.size() - 6, 6, "3.0 MB") == 0 && b.balken.find(" / ") != std::string::npos &&
               da(knopf(Str::UpdBarCancel, "abbrechen")) && symbolOk(updater::LeistenSymbol::Info);
    }));
    add(tooltipDa(knopf(Str::UpdBarCancel, "abbrechen")));
    add(klick(knopf(Str::UpdBarCancel, "abbrechen"), "###main"));
    add(warteBis("REST10: abgebrochen", [] { return updater::zustand().stand != updater::Stand::Laedt; }, 900));
    add(pause(0.2));
    add(mausWeg());
    add(pruefe("REST10: Abbrechen - nichts eingespielt, SHA256SUMS nie geholt, kein .zip in der Ablage, die Leiste zeigt wieder \"verfuegbar\"", [k] {
        const updater::Zustand z = updater::zustand();
        diag::detail("Knopftest: REST10 abgebrochen: " + standText() + ", SHA256SUMS " + std::to_string(sim().summenAnfragen.load()) + "x");
        return z.stand == updater::Stand::Verfuegbar && !z.beimInstallieren && z.meldung.empty() &&
               updater::leisteBild().text == mitText(Str::UpdBarAvail, kurz(k + 2), kurz(k)) && da(knopf(Str::UpdBarNow, "jetzt")) &&
               !dateiNebenExe(kProbe) && !dateiNebenExe(kPolster) && sim().summenAnfragen.load() == 0 && !zipInAblage();
    }));
    add(tu("REST10: kleines .zip, schnelles Netz", [] {
        updater::testStueckPause(0);
        simZip(false);
    }));

    // --- Jetzt aktualisieren: Fortschritt, Fehler, Erneut versuchen, Neu starten ------------------
    add(tu("REST9: der Download wartet, das .zip ist kaputt", [] {
        sim().halt = true;
        std::lock_guard<std::mutex> l(sim().m);
        sim().kaputt = true;
    }));
    add(klick(knopf(Str::UpdBarNow, "jetzt"), "###main"));
    add(warteBis("REST9: laedt", [] { return updater::zustand().stand == updater::Stand::Laedt; }, 300));
    add(pause(0.2));
    add(pruefe("REST9: Jetzt aktualisieren - die Leiste zeigt das Laden", [k] {
        char t[300];
        std::snprintf(t, sizeof(t), tr(Str::UpdBarLoading), kurz(k + 2).c_str());
        diag::detail("Knopftest: REST9 laedt: " + standText());
        return leisteDa() && updater::leisteBild().text == t;
    }));
    add(foto("r9_leiste_laedt"));
    add(tu("REST9: Download weiter", [] { sim().halt = false; }));
    add(warteBis("REST9: Fehler", [] { return updater::zustand().stand != updater::Stand::Laedt; }, 1200));
    add(pause(0.2));
    add(foto("r9_leiste_fehler"));
    add(pruefe("REST9: kaputtes .zip - die Leiste zeigt den Fehler (Pruefsumme) mit Erneut versuchen, nichts wurde eingespielt", [] {
        const updater::Zustand z = updater::zustand();
        diag::detail("Knopftest: REST9 Fehler: " + standText());
        std::error_code ec;
        return z.stand == updater::Stand::Fehler && z.beimInstallieren && leisteDa() && updater::leisteBild().text.find(kZipName) != std::string::npos &&
               finde(knopf(Str::UpdBarRetry, "nochmal"), "###main", 0, false) != nullptr &&
               !fs::exists(::bhed::utf8Pfad(platform::executableDirectory() + "/" + kProbe), ec);
    }));
    add(pruefe("REST10: Fehler beim Installieren - Fehler-Symbol", [] { return symbolOk(updater::LeistenSymbol::Fehler); }));
    add(tooltipDa(knopf(Str::UpdBarRetry, "nochmal")));
    add(mausWeg());
    add(tu("REST9: das .zip ist heil", [] {
        std::lock_guard<std::mutex> l(sim().m);
        sim().kaputt = false;
    }));
    add(klick(knopf(Str::UpdBarRetry, "nochmal"), "###main"));
    add(warteBis("REST9: installiert", [] {
        const updater::Stand st = updater::zustand().stand;
        return st == updater::Stand::Fertig || st == updater::Stand::Fehler;
    }, 1200));
    add(pause(0.2));
    add(mausWeg());
    add(foto("r9_leiste_fertig"));
    add(pruefe("REST9: Erneut versuchen - installiert, die Datei des Releases liegt neben der .exe, die Leiste bietet Jetzt neu starten", [k] {
        std::error_code ec;
        const std::string probe = platform::executableDirectory() + "/" + kProbe;
        std::ifstream f(::bhed::utf8Pfad(probe), std::ios::binary);
        const std::string inhalt((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
        char t[300];
        std::snprintf(t, sizeof(t), tr(Str::UpdBarDone), kurz(k + 2).c_str());
        diag::detail("Knopftest: REST9 fertig: " + standText() + ", Probe \"" + inhalt + "\"");
        return updater::zustand().stand == updater::Stand::Fertig && updater::leisteBild().text == t && inhalt == "REST9 update bar probe\n" &&
               finde(knopf(Str::UpdRestart, "neustart"), "###main", 0, false) != nullptr;
    }));
    add(tooltipDa(knopf(Str::UpdRestart, "neustart")));
    add(mausWeg());
    // REST10 Punkt 2: nach dem Laden auch [Spaeter] - Leiste weg, der Neustart-Hinweis unten bleibt, wirksam beim naechsten Start.
    add(pruefe("REST10: installiert - Haken-Symbol, [Spaeter] neben [Jetzt neu starten]", [] {
        const Element* neu = finde(knopf(Str::UpdRestart, "neustart"), "###main", 0, false);
        const Element* sp = finde(knopf(Str::UpdLater, "spaeter"), "###main", 0, false);
        return symbolOk(updater::LeistenSymbol::Haken) && neu != nullptr && sp != nullptr && sp->rect.Min.x > neu->rect.Max.x &&
               std::fabs(sp->rect.Min.y - neu->rect.Min.y) < 0.5F;
    }));
    add(tooltipDa(knopf(Str::UpdLater, "spaeter")));
    add(mausWeg());
    add(klick(knopf(Str::UpdLater, "spaeter"), "###main"));
    add(pause(0.3));
    add(pruefe("REST10: Spaeter nach dem Laden - Leiste weg, \"" + std::string(tr(Str::UpdStatusDone)) + "\" bleibt unten, kein Neustart vorgemerkt, installiert bleibt", [] {
        diag::detail("Knopftest: REST10 Spaeter nach dem Laden: " + standText());
        return !updater::leisteSichtbar() && updater::leisteBild().zeilen == 0 && finde(tr(Str::UpdStatusDone), "###main", 0, false) != nullptr &&
               !updater::neustartVorgemerkt() && !g_app->wantQuit && updater::zustand().stand == updater::Stand::Fertig && dateiNebenExe(kProbe);
    }));
    add(tu("REST10: von Hand suchen nach Spaeter (installiert)", [] { updater::pruefen(false); }));
    add(pause(0.3));
    add(pruefe("REST10: von Hand gesucht - die Leiste bietet wieder Jetzt neu starten", [] {
        return leisteDa() && updater::zustand().stand == updater::Stand::Fertig && da(knopf(Str::UpdRestart, "neustart"));
    }));
    // Jetzt neu starten: der vorhandene Ablauf (neuStarten: Neustart vormerken, Beenden wie Exit). Im Bild des Klicks
    // nimmt der Test beides zurueck, bevor die Fensterschicht wantQuit auswertet (wie beendenAngestossen).
    auto neustart = std::make_shared<bool>(false);
    {
        Schritt k2 = klick(knopf(Str::UpdRestart, "neustart"), "###main");
        auto innen = k2.tun;
        add(Schritt{"REST9: Klick Jetzt neu starten", [innen, neustart](int b) {
                        const bool fertig = innen(b);
                        if (updater::neustartVorgemerkt() || g_app->wantQuit) {
                            diag::detail(std::string("Knopftest: REST9 Neu starten: vorgemerkt ") + (updater::neustartVorgemerkt() ? "ja" : "nein") +
                                         ", wantQuit " + (g_app->wantQuit ? "ja" : "nein"));
                            *neustart = updater::neustartVorgemerkt() && g_app->wantQuit;
                            updater::neustartVergessen();
                            g_app->wantQuit = false;
                        }
                        return fertig;
                    }});
    }
    add(pruefe("REST9: Jetzt neu starten merkt den Neustart vor und beendet wie Exit", [neustart] { return *neustart; }));

    // --- REST10 Punkt 3: die Suche von Hand zeigt ihr Ergebnis in der Leiste ---------------------------
    add(tu("REST10: neuer Start, Release gleich, langsames Netz (100 ms je Sechzehntel)", [k] {
        updater::testNeuerStart();
        simTag(k);
        updater::testStueckPause(100);
    }));
    menueSuche(s);
    add(pause(0.1));
    add(foto("r10_leiste_suche"));
    add(pruefe("REST10: Hilfe > Nach Updates suchen - die Leiste zeigt \"" + std::string(tr(Str::UpdBarChecking)) + "\" mit Info-Symbol und x, kein Fenster", [] {
        diag::detail("Knopftest: REST10 Suche: " + standText());
        return updater::zustand().stand == updater::Stand::Pruefe && leisteDa() && updater::leisteBild().text == tr(Str::UpdBarChecking) &&
               symbolOk(updater::LeistenSymbol::Info) && da("x###updleiste_zu") && !updater::fensterOffen();
    }));
    add(warteBis("REST10: Antwort (aktuell)", [] { return updater::zustand().stand != updater::Stand::Pruefe; }, 900));
    add(pause(0.2));
    add(mausWeg());
    add(foto("r10_leiste_aktuell"));
    add(pruefe("REST10: aktuell - \"JK Studio ist aktuell (rcNNN)\" mit Haken und [Schliessen], ohne x, kein Fenster", [k] {
        diag::detail("Knopftest: REST10 aktuell: " + standText());
        return updater::zustand().stand == updater::Stand::Aktuell && leisteDa() &&
               updater::leisteBild().text == mitText(Str::UpdBarCurrent, kurz(k)) && symbolOk(updater::LeistenSymbol::Haken) &&
               da(knopf(Str::UpdBarClose, "schliessen")) && !da("x###updleiste_zu") && !updater::fensterOffen();
    }));
    add(tooltipDa(knopf(Str::UpdBarClose, "schliessen")));
    add(mausWeg());
    add(klick(knopf(Str::UpdBarClose, "schliessen"), "###main"));
    add(pause(0.3));
    add(pruefe("REST10: Schliessen - die Leiste ist weg", [] { return !updater::leisteSichtbar() && updater::leisteBild().zeilen == 0; }));
    // x waehrend der Suche: die Leiste geht, die Suche endet still (wie beim Start).
    add(tu("REST10: von Hand suchen, x waehrend der Suche", [] { updater::pruefen(false); }));
    add(pause(0.2));
    add(klick("x###updleiste_zu", "###main"));
    add(warteBis("REST10: Antwort nach x", [] { return updater::zustand().stand != updater::Stand::Pruefe; }, 900));
    add(pause(0.2));
    add(pruefe("REST10: x waehrend der Suche - \"aktuell\" kommt still, keine Leiste", [] {
        return updater::zustand().stand == updater::Stand::Aktuell && !updater::leisteSichtbar();
    }));
    add(tu("REST10: keine Verbindung, von Hand suchen", [] {
        {
            std::lock_guard<std::mutex> l(sim().m);
            sim().keinNetz = true;
        }
        updater::testStueckPause(0);
        updater::pruefen(false);
    }));
    add(warteBis("REST10: Antwort (Fehler)", [] { return updater::zustand().stand != updater::Stand::Pruefe; }, 900));
    add(pause(0.2));
    add(foto("r10_leiste_suchfehler"));
    add(pruefe("REST10: Fehler beim Suchen - Grund mit Fehler-Symbol, [Releases-Seite] [Schliessen], kein Fenster", [] {
        const updater::Zustand z = updater::zustand();
        diag::detail("Knopftest: REST10 Suchfehler: " + standText());
        return z.stand == updater::Stand::Fehler && !z.beimInstallieren && leisteDa() && updater::leisteBild().text == mitText(Str::UpdBarCheckFailed, z.meldung) &&
               symbolOk(updater::LeistenSymbol::Fehler) && da(knopf(Str::UpdBarReleases, "seite")) && da(knopf(Str::UpdBarClose, "schliessen")) &&
               !da(knopf(Str::UpdBarRetry, "nochmal")) && !updater::fensterOffen();
    }));
    add(tooltipDa(knopf(Str::UpdBarReleases, "seite")));
    add(mausWeg());
    add(klick(knopf(Str::UpdBarReleases, "seite"), "###main"));
    add(pause(0.2));
    add(pruefe("REST10: Releases-Seite - die Seite der Releases (im Selbsttest nur gemerkt, kein Browser), die Leiste bleibt", [] {
        diag::detail("Knopftest: REST10 Seite: " + updater::testLetzteSeite());
        return updater::testLetzteSeite() == "https://github.com/DennisHerrm/JK-Studio/releases" && leisteDa();
    }));
    add(klick(knopf(Str::UpdBarClose, "schliessen"), "###main"));
    add(pause(0.3));
    add(pruefe("REST10: Schliessen nach dem Fehler - die Leiste ist weg", [] { return !updater::leisteSichtbar(); }));

    // --- ein gescheitertes Nachsehen meldet die Leiste nicht ------------------------------------
    add(tu("REST9: neuer Start, keine Verbindung", [] {
        updater::testNeuerStart();
        std::lock_guard<std::mutex> l(sim().m);
        sim().keinNetz = true;
    }));
    nachsehen(s, "ohne Netz");
    add(pruefe("REST9: ohne Verbindung beim Start - Fehler, aber keine Leiste (still wie G2C)", [] {
        diag::detail("Knopftest: REST9 ohne Netz: " + standText());
        return updater::zustand().stand == updater::Stand::Fehler && !updater::leisteSichtbar();
    }));

    // --- Aktionssuche ------------------------------------------------------------------------------
    add(pruefe("REST9: der Helfer findet \"" + std::string(tr(Str::UpdMenu)) + "\"", [] {
        std::string frage = tr(Str::UpdMenu);
        while (!frage.empty() && frage.back() == '.') { frage.pop_back(); }
        for (const HilfeTreffer& t : hilfeFrage(frage)) {
            if (t.id == "ort:menue:MenuHelp>UpdMenu") { return true; }
        }
        return false;
    }));

    add(tu("REST9: aufraeumen (die uebersprungene Fassung bleibt fuer updateleiste2)", [k] {
        updater::testStueckPause(0);
        updater::testNetzSetzen({});
        updater::testNeuerStart();
        g_app->settings.updateUebersprungen = tagNr(k + 1);
        std::error_code ec;
        fs::remove_all(::bhed::utf8Pfad(platform::executableDirectory() + "/r9_sim"), ec);
        fs::remove(::bhed::utf8Pfad(platform::executableDirectory() + "/" + kProbe), ec);
        fs::remove(::bhed::utf8Pfad(platform::executableDirectory() + "/" + kPolster), ec);
    }));
    return s;
}

// Der zweite Start mit den Einstellungen des ersten.
std::vector<Schritt> neustartSchritte() {
    std::vector<Schritt> s;
    auto add = [&](Schritt x) { s.push_back(std::move(x)); };
    const int k = lokalNr();
    add(pruefe("REST9 zweiter Start: die uebersprungene Fassung steht in den Einstellungen", [k] {
        diag::detail("Knopftest: REST9 zweiter Start: Einstellung " + g_app->settings.updateUebersprungen);
        return g_app->settings.updateUebersprungen == tagNr(k + 1);
    }));
    add(tu("REST9 zweiter Start: simulierte GitHub-Antwort mit der uebersprungenen Fassung", [k] {
        simEinschalten();
        simTag(k + 1);
    }));
    nachsehen(s, "zweiter Start, uebersprungen");
    add(pruefe("REST9 zweiter Start: die uebersprungene Fassung zeigt keine Leiste, der Hinweis unten steht", [k] {
        diag::detail("Knopftest: REST9 zweiter Start: " + standText());
        return updater::zustand().stand == updater::Stand::Verfuegbar && !updater::leisteSichtbar() && statusHinweisDa(tagNr(k + 1));
    }));
    add(tu("REST9 zweiter Start: eine neuere Fassung", [k] { simTag(k + 2); }));
    nachsehen(s, "zweiter Start, neuer");
    add(pruefe("REST9 zweiter Start: eine neuere Fassung zeigt die Leiste", [] { return leisteDa(); }));
    add(tu("REST9 zweiter Start: aufraeumen", [] {
        updater::testNetzSetzen({});
        updater::testNeuerStart();
        g_app->settings.updateUebersprungen.clear();
        std::error_code ec;
        fs::remove_all(::bhed::utf8Pfad(platform::executableDirectory() + "/r9_sim"), ec);
    }));
    return s;
}

// Handbuch (BHED_EDITORTEST=handbuch, BHED_HB_NUR=updateleiste): die Leiste mit einem neueren Release.
std::vector<Schritt> handbuchSchritte() {
    std::vector<Schritt> s;
    const int k = lokalNr();
    s.push_back(tu("Handbuch REST9: simulierte GitHub-Antwort", [k] {
        g_app->settings.updateUebersprungen.clear();
        simEinschalten();
        simTag(k + 1);
        updater::testNeuerStart();
    }));
    nachsehen(s, "Handbuch");
    s.push_back(mausWeg());
    s.push_back(stabil("Handbuch REST9", 40));
    s.push_back(pause(0.5));
    s.push_back(pruefe("Handbuch REST9: die Leiste steht in einer Zeile, ungekuerzt", [] {
        const updater::LeistenBild b = updater::leisteBild();
        return leisteDa() && b.zeilen == 1 && !b.gekuerzt;
    }));
    hb::markiere("h_update_leiste", [] {
        const updater::LeistenBild b = updater::leisteBild();
        return std::vector<hb::Marke>{ImRect{b.x0 + 2.0F, b.y0 + 1.0F, b.x1 - 2.0F, b.y1 - 1.0F}};
    });
    s.push_back(hb::fotoTeil("h_update_leiste", [] {
        const ImGuiWindow* w = hauptfenster();
        const updater::LeistenBild b = updater::leisteBild();
        if (w == nullptr) { return ImRect{}; }
        // Menueleiste, Leiste und der Anfang des Arbeitsbereichs darunter
        return ImRect{w->Pos.x, w->Pos.y, w->Pos.x + w->Size.x, std::min(w->Pos.y + w->Size.y, b.y1 + (b.y1 - b.y0) * 5.0F)};
    }));
    s.push_back(tu("Handbuch REST9: Ende", [] {
        updater::testNetzSetzen({});
        updater::testNeuerStart();
        std::error_code ec;
        fs::remove_all(::bhed::utf8Pfad(platform::executableDirectory() + "/r9_sim"), ec);
    }));
    return s;
}

}  // namespace rest9
