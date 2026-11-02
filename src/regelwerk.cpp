// regelwerk.cpp - rc601 integ: der zentrale Schalter "OpenJK / Movie Duels".
// Siehe bhed/regelwerk.h.
#include "bhed/regelwerk.h"
#include "bhed/glm.h"

#include "bhed/navlauf.h"
#include "bhed/saberkontakt.h"

#include <atomic>
#include <cctype>
#include <string>
#include <string_view>

namespace bhed {

namespace {
// Ohne regelwerkAnwenden: Movie Duels, g_spskill 2 - so rechneten die Pakete
// bisher, und ihre Unit-Tests setzen das voraus.
std::atomic<bool> gMd{true};
std::atomic<int> gStufe{2};
std::atomic<bool> gAnimStil{false};   // g_ActivateAnimationStyle
std::atomic<bool> gJk2{false};
}  // namespace

const char* regelwerkSchluessel(int wahl) noexcept {
    switch (wahl) {
        case kRegelwerkOpenJK: return "openjk";
        case kRegelwerkMovieDuels: return "movieduels";
        case kRegelwerkJediOutcast: return "jedioutcast";
        default: return "auto";
    }
}

bool regelwerkAusSchluessel(const std::string& text, int& wahl) noexcept {
    if (text == "auto") {
        wahl = kRegelwerkAuto;
    } else if (text == "openjk") {
        wahl = kRegelwerkOpenJK;
    } else if (text == "movieduels") {
        wahl = kRegelwerkMovieDuels;
    } else if (text == "jedioutcast") {
        wahl = kRegelwerkJediOutcast;
    } else {
        return false;
    }
    return true;
}

bool regelwerkAufloesen(int wahl, bool movieDuelsErkannt) noexcept {
    if (wahl == kRegelwerkOpenJK) { return false; }
    if (wahl == kRegelwerkMovieDuels) { return true; }
    // Automatisch: der Betreuer - "wenn der Ordner unklar ist, OpenJK".
    return movieDuelsErkannt;
}

int regelwerkSpiel(int wahl, bool movieDuelsErkannt, bool jediOutcastErkannt) noexcept {
    if (wahl == kRegelwerkOpenJK || wahl == kRegelwerkMovieDuels || wahl == kRegelwerkJediOutcast) { return wahl; }
    // Automatisch: "wenn der Ordner unklar ist, OpenJK".
    if (movieDuelsErkannt) { return kRegelwerkMovieDuels; }
    return jediOutcastErkannt ? kRegelwerkJediOutcast : kRegelwerkOpenJK;
}
bool jediOutcastHinweis(const std::vector<std::string>& dateien, bool jk2Karte) {
    const auto beginnt = [](const std::string& s, const char* v) { return s.rfind(v, 0) == 0; };
    bool jka = false;
    for (const std::string& d : dateien) {
        // jk2sp.exe/jk2mp.exe (Raven), openjo_sp.x86(_64).exe (OpenJK-Ziel
        // "openjo_sp", codeJK2/CMakeLists.txt).
        if (d == "jk2sp.exe" || d == "jk2mp.exe" || beginnt(d, "openjo_sp.")) { return true; }
        if (d == "jasp.exe" || d == "jamp.exe" || beginnt(d, "openjk_sp.") || beginnt(d, "openjk.")) { jka = true; }
    }
    return !jka && jk2Karte;
}
bool regelwerkMovieDuels() noexcept { return gMd.load(std::memory_order_relaxed); }
bool regelwerkJediOutcast() noexcept { return gJk2.load(std::memory_order_relaxed); }
void regelwerkAnwendenSpiel(int spiel, int schwierigkeitEinstellung) noexcept {
    regelwerkAnwenden(spiel == kRegelwerkMovieDuels, schwierigkeitEinstellung);
    const bool jk2 = spiel == kRegelwerkJediOutcast;
    gJk2.store(jk2, std::memory_order_relaxed);
    // R_LoadMDXM unter JK2_MODE (tr_ghoul2.cpp:3663-3667, 3879-3893).
    setzeGhoul2Jk2(jk2);
    // rc604 fix604 (JK2 Stufe 3): NPC_MoveToGoal von Jedi Outcast (bhed/navlauf.h).
    navJk2Regeln(jk2);
}
void regelwerkAnwenden(bool movieDuels, int schwierigkeitEinstellung) noexcept {
    const int stufe = schwierigkeitWirksam(schwierigkeitEinstellung, movieDuels);
    gMd.store(movieDuels, std::memory_order_relaxed);
    gJk2.store(false, std::memory_order_relaxed);
    setzeGhoul2Jk2(false);
    gStufe.store(stufe, std::memory_order_relaxed);
    // Die Schalter der Pakete - sie bleiben die Stellen, die deren Code fragt.
    setzeSaberMovieDuelsRegeln(movieDuels);
    setzeSaberSpielstufe(stufe);
    navMovieDuelsRegeln(movieDuels);
    navJk2Regeln(false);
}

void animStilSetzen(bool an) noexcept { gAnimStil.store(an, std::memory_order_relaxed); }
bool animStilAktiv() noexcept { return gAnimStil.load(std::memory_order_relaxed) && regelwerkMovieDuels() && !regelwerkJediOutcast(); }

bool animStilMeistersatz(const std::string& gla) {
    // md_masterHumanoidSets (qcommon/md_animsets.h)
    static const char* const kSaetze[] = {
        "models/players/JK2anims",        "models/players/_humanoid_ani",   "models/players/_humanoid_bdroid",
        "models/players/_humanoid_ben",   "models/players/_humanoid_cal",   "models/players/_humanoid_clo",
        "models/players/_humanoid_df2",   "models/players/_humanoid_dooku", "models/players/_humanoid_galen",
        "models/players/_humanoid_gon",   "models/players/_humanoid_grievous", "models/players/_humanoid_jabba",
        "models/players/_humanoid_jango", "models/players/_humanoid_kotor", "models/players/_humanoid_luke",
        "models/players/_humanoid_mace",  "models/players/_humanoid_maul",  "models/players/_humanoid_md",
        "models/players/_humanoid_melee", "models/players/_humanoid_obi",   "models/players/_humanoid_obi3",
        "models/players/_humanoid_pal",   "models/players/_humanoid_reb",   "models/players/_humanoid_ren",
        "models/players/_humanoid_rey",   "models/players/_humanoid_vader", "models/players/_humanoid_yoda",
    };
    const auto klein = [](char c) { return static_cast<char>(std::tolower(static_cast<unsigned char>(c))); };
    for (const char* satz : kSaetze) {
        const std::string_view sv(satz);
        if (gla.size() <= sv.size()) { continue; }
        bool gleich = gla[sv.size()] == '/' || gla[sv.size()] == '\\';
        for (std::size_t i = 0; gleich && i < sv.size(); ++i) { gleich = klein(gla[i]) == klein(sv[i]); }
        if (gleich) { return true; }
    }
    return false;
}

bool animStilSkelettPasst(int netzKnochen, int meisterKnochen, const std::string& eigenerSatz) {
    return netzKnochen == meisterKnochen || (netzKnochen == 72 && eigenerSatz.find("_humanoid") != std::string::npos);
}

int regelwerkSpielstufe() noexcept { return gStufe.load(std::memory_order_relaxed); }

int multisampleWirksam(int einstellung, bool movieDuels) noexcept {
    if (einstellung == 0 || einstellung == 2 || einstellung == 4 || einstellung == 8) { return einstellung; }
    return movieDuels ? 8 : 0;
}

int schwierigkeitWirksam(int einstellung, bool movieDuels) noexcept {
    if (einstellung >= 0 && einstellung <= 3) { return einstellung; }
    // g_spskill: MD gi.cvar("g_spskill", "2", ...) (g_main.cpp:752),
    // OpenJK gi.cvar("g_spskill", "0", ...) (g_main.cpp:631), JK2 ebenso
    // "0" (codeJK2/game/g_main.cpp:571).
    return movieDuels ? 2 : 0;
}

bool fovAspectAdjustWirksam(int einstellung, bool movieDuels) noexcept {
    if (einstellung == 0 || einstellung == 1) { return einstellung == 1; }
    // cg_fovAspectAdjust: MD "1" (cg_main.cpp:555), OpenJK "0" (cg_main.cpp:355).
    return movieDuels;
}

float spielSeitenWirksam(float einstellung, bool movieDuels, float bildschirm) noexcept {
    if (einstellung > 0.0F) { return einstellung; }
    if (movieDuels) {
        // r_mode "-2" (sdl_window.cpp:779 MD): die Desktopaufloesung.
        return bildschirm > 0.0F ? bildschirm : 16.0F / 9.0F;
    }
    // r_mode "4" (sdl_window.cpp:746 OpenJK): 800x600.
    return 4.0F / 3.0F;
}

bool roffKlangFolgtEntity(bool movieDuels, bool mover) noexcept {
    // MD g_roff.cpp:262-266: immer cgi_S_StartSound(cent->s.pos.trBase, ...).
    // OpenJK g_roff.cpp:297-306: ET_MOVER fest am Ort, sonst G_SoundOnEnt.
    return !movieDuels && !mover;
}

bool roffNotizenOpenJk() noexcept { return !regelwerkMovieDuels() && !regelwerkJediOutcast(); }

}  // namespace bhed
