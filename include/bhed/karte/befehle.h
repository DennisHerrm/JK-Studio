// befehle.h - Karten-Editor: die Befehlstabelle von NetRadiant-Custom (Namen,
// Standardtasten, Menues, Werkzeugleisten) und Tastenkuerzel wie Qt.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/commands.cpp
// (g_shortcuts: ein Name, eine Taste; GlobalCommands_insert,
// GlobalToggles_insert, GlobalKeyEvents_insert, GlobalShortcuts_insert),
// radiant/mainframe.cpp (create_main_menu, create_main_toolbar),
// radiant/filterbar.cpp (create_filter_toolbar), radiant/gtkmisc.cpp
// (g_toolbarHiddenButtons, toolbar_action_set_tooltip). Die Tastenlogik folgt
// Qt 5 (LGPL), wie NRC sie benutzt: QKeySequence (Text <-> Taste,
// qkeysequence.cpp keyname[]), QWindowsKeyMapper::possibleKeys
// (qwindowskeymapper.cpp) und QShortcutMap::find (alle moeglichen Tasten,
// mehrdeutig = keine Ausloesung).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   Copyright (C) 2001-2006, William Joseph. All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
//
// Qt vergleicht beim Tastenkuerzel das ERZEUGTE Zeichen: "[" ist auf einer
// deutschen Tastatur AltGr+8, und "Ctrl+[" ist dort nicht erreichbar, weil
// AltGr selbst Strg+Alt ist (possibleKeys findet dann "["). So verhaelt sich
// NRC, so verhaelt sich dieser Nachbau.
#ifndef BHED_KARTE_BEFEHLE_H
#define BHED_KARTE_BEFEHLE_H

#include "bhed/i18n.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace bhed::karte::befehle {

// --- Qt-Tastencodes (Qt::Key, Qt::KeyboardModifier) ---------------------
inline constexpr std::uint32_t kShift = 0x02000000;
inline constexpr std::uint32_t kCtrl = 0x04000000;
inline constexpr std::uint32_t kAlt = 0x08000000;
inline constexpr std::uint32_t kMeta = 0x10000000;
inline constexpr std::uint32_t kNum = 0x20000000;   // KeypadModifier
inline constexpr std::uint32_t kModMaske = 0xfe000000;   // KeyboardModifierMask
inline constexpr std::uint32_t kKeyEscape = 0x01000000;
inline constexpr std::uint32_t kKeyTab = 0x01000001;
inline constexpr std::uint32_t kKeyBackspace = 0x01000003;
inline constexpr std::uint32_t kKeyReturn = 0x01000004;
inline constexpr std::uint32_t kKeyEnter = 0x01000005;
inline constexpr std::uint32_t kKeyInsert = 0x01000006;
inline constexpr std::uint32_t kKeyDelete = 0x01000007;
inline constexpr std::uint32_t kKeyHome = 0x01000010;
inline constexpr std::uint32_t kKeyEnd = 0x01000011;
inline constexpr std::uint32_t kKeyLeft = 0x01000012;
inline constexpr std::uint32_t kKeyUp = 0x01000013;
inline constexpr std::uint32_t kKeyRight = 0x01000014;
inline constexpr std::uint32_t kKeyDown = 0x01000015;
inline constexpr std::uint32_t kKeyPageUp = 0x01000016;
inline constexpr std::uint32_t kKeyPageDown = 0x01000017;
inline constexpr std::uint32_t kKeyF1 = 0x01000030;   // F1 .. F35 fortlaufend
inline constexpr std::uint32_t kKeyUnknown = 0x01ffffff;

// Eine Taste mit Zusatztasten, wie ein Element von QKeySequence (key | mods).
using Taste = std::uint32_t;

// QKeySequence( "Ctrl+Shift+P" ): 0 bei leerem oder ungueltigem Text.
[[nodiscard]] Taste tasteAusText(std::string_view text);
// QKeySequence::toString( PortableText ): "Ctrl+Shift+P", "Esc", "Del", "PgDown".
[[nodiscard]] std::string tasteAlsText(Taste t);

// KeyboardLayoutItem (qwindowskeymapper.cpp): die Zeichen, die eine Taste mit
// den Zusatztasten-Kombinationen von ModsTbl erzeugt (Grossbuchstaben bzw.
// Qt::Key_*), [8] ist der Rueckfall ohne Zeichen (winceKeyBend).
struct TastenBelegung {
    bool vorhanden = false;
    std::array<std::uint32_t, 9> qtKey{};
};
// QWindowsKeyMapper::possibleKeys: alle Tasten, als die ein Druck gelten kann.
// mods: die gedrueckten Zusatztasten (kShift|kCtrl|kAlt|kMeta, kNum bei
// Ziffernblock). Erweitertes Return (Ziffernblock-Enter) wird Key_Enter.
[[nodiscard]] std::vector<Taste> moeglicheTasten(const TastenBelegung& kb, std::uint32_t mods,
                                                bool erweitertesReturn = false);

// --- Befehle (g_shortcuts) --------------------------------------------------
enum ArtBits : unsigned { kCommand = 1, kToggle = 2, kKeyEvent = 4 };

struct Befehl {
    const char* name;      // NRC-Name ("NewMap")
    unsigned art;          // ArtBits
    const char* taste;     // NRC-Standardtaste in Qt-Schreibweise, "" = keine
    const char* quelle;    // "radiant/map.cpp:2429"
    int stufe;             // 1 = Stufe 1b, 2/3 = kommt spaeter (sichtbar, ausgegraut)
};
[[nodiscard]] const std::vector<Befehl>& alle();
[[nodiscard]] const Befehl* finden(std::string_view name);

// --- Menues und Werkzeugleisten ----------------------------------------------
enum class MenueArt { Menue, Unter, Ende, Punkt, Haken, Ruf, Trenner, Liste };
// Wann ein Eintrag da ist (mainframe.cpp:932-1050, Fensteranordnung).
enum Bedingung : unsigned { kImmer, kNurFloating, kNichtRegular, kTexturen, kNichtSplit, kNurSplit };
// NRCs Fensteranordnungen (MainFrame::EViewStyle, mainframe.h:51).
enum class Anordnung { Regular = 0, Floating = 1, Split = 2, RegularLeft = 3 };
[[nodiscard]] bool bedingungGilt(Bedingung b, Anordnung a, bool gruppenDialogEingebaut);

struct MenueEintrag {
    MenueArt art;
    const char* text;      // NRC-Beschriftung mit '&'
    const char* befehl;    // Befehl; bei Ruf der Name des Rueckrufs; bei Liste ihr Name
    i18n::Str str;         // Uebersetzung (Str::Count = keine)
    Bedingung bedingung;
};
[[nodiscard]] const std::vector<MenueEintrag>& menues();
// Die Menues des Texturbrowsers (View, Tags; texwindow.cpp:1238, :1281).
[[nodiscard]] const std::vector<MenueEintrag>& texturMenues();

struct Knopf {
    const char* befehl;        // "" = Trenner
    const char* beschreibung;  // NRCs Text (Zeilen mit '\n')
    const char* bild;          // bitmaps/<bild>.svg
    bool schalter;             // toolbar_append_toggle_button
    Bedingung bedingung;
    const char* rechtsklick;   // Filterleiste: Befehl, "func_group" oder "tex:a,b"
    bool trenner;
    std::array<i18n::Str, 5> zeilen;   // Uebersetzung je Zeile (Str::Count = unveraendert)
};
[[nodiscard]] const std::vector<Knopf>& werkzeugleiste();   // create_main_toolbar
[[nodiscard]] const std::vector<Knopf>& filterleiste();     // create_filter_toolbar
[[nodiscard]] const std::vector<Knopf>& texturKnoepfe();    // Texturbrowser (texwindow.cpp:1800)
[[nodiscard]] const std::vector<Knopf>& pluginleiste();     // create_plugin_toolbar (bobToolz)
// Ein Eintrag eines Plugin-Menues (PlugInMenu_Add): Menue = Modulname,
// Titel "" = Trenner.
struct PluginEintrag {
    const char* menue;
    const char* titel;
    const char* befehl;
};
[[nodiscard]] const std::vector<PluginEintrag>& pluginEintraege();
// g_toolbarHiddenButtons (gtkmisc.cpp:147): standardmaessig versteckte Knoepfe.
[[nodiscard]] const std::vector<std::string>& standardVersteckt();

// toolbar_action_set_tooltip (gtkmisc.cpp:305): erste Zeile + " (Taste)" +
// der Rest - mit den Zeilen, wie sie uebergeben werden.
[[nodiscard]] std::string knopfTooltip(const std::vector<std::string>& zeilen, const std::string& taste);

// --- Die Belegung zur Laufzeit -----------------------------------------------
//
// Je Befehl die aktuelle Taste (Vorgabe oder eigene Belegung aus der
// Einstellung, LoadCommandMap) und das Suchen wie QShortcutMap::find.
class Belegung {
public:
    Belegung();
    // Eigene Belegung (shortcuts.ini, ReadCommandMap): Name -> Text; leerer
    // Text heisst "keine Taste". Unbekannte Namen werden uebergangen,
    // unlesbare Tasten gemeldet ("WARNING: failed to parse user command ...").
    std::vector<std::string> laden(const std::vector<std::pair<std::string, std::string>>& eigene);
    // Fuer die Einstellung: nur die vom Standard abweichenden (SaveCommandMap
    // schreibt alle; wir nur die abweichenden, gleiche Wirkung).
    [[nodiscard]] std::vector<std::pair<std::string, std::string>> abweichende() const;

    [[nodiscard]] Taste taste(std::string_view name) const;
    [[nodiscard]] Taste standard(std::string_view name) const;
    void setzen(std::string_view name, Taste t);        // accelerator_alter
    void zuruecksetzen(std::string_view name);           // Reset
    void alleZuruecksetzen();                            // Reset All
    // Wer hat diese Taste schon (VerifyAcceleratorNotTaken)? "" = niemand.
    [[nodiscard]] std::string belegtVon(Taste t, std::string_view ausser) const;

    // QShortcutMap::find: alle Befehle, deren Taste einer der moeglichen
    // Tasten entspricht (ohne Doppelte). Mehr als einer = mehrdeutig, dann
    // loest Qt nichts aus ("Ambiguous shortcut overload").
    [[nodiscard]] std::vector<std::string> treffer(const std::vector<Taste>& moegliche) const;

    [[nodiscard]] std::size_t anzahl() const { return m_namen.size(); }
    [[nodiscard]] const std::vector<std::string>& namen() const { return m_namen; }

private:
    std::vector<std::string> m_namen;   // nach Namen sortiert (std::map in NRC)
    std::vector<Taste> m_tasten;
    std::vector<Taste> m_standard;
    [[nodiscard]] std::ptrdiff_t index(std::string_view name) const;
};

}  // namespace bhed::karte::befehle
#endif
