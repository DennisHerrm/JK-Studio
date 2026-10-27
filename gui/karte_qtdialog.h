// karte_qtdialog.h - Karten-Editor, Stufe 4: Bausteine fuer die Dialoge der
// Plugins, nachgebaut nach Qt 5.15 (Fusion, dark.qss) wie NRC sie anlegt.
//
// Die Masse sind logische Punkte; die Layouts der Plugins (QFormLayout,
// QVBoxLayout, QHBoxLayout mit Rand 11 und Abstand 6) rechnet der Aufrufer
// mit den Wunschgroessen ("Hinweis") der Bausteine:
//   QCheckBox      Kaestchen 14 + 4 + PM_CheckBoxLabelSpacing 6 + Text + 1
//                  (QCommonStyle CT_CheckBox: "4 pixels for label margins"), 20 hoch
//   QRadioButton   14 + 4 + 6 + Text + 1 (CT_RadioButton, am Foto
//                  dlg_Curve_Thicken_0 gemessen), 20 hoch
//   QPushButton    Text + PM_ButtonMargin 6 + 2 x Rahmen 1, wenigstens 80
//                  (QCommonStyle/QFusionStyle CT_PushButton), 22 hoch
//   QDialogButtonBox  die Knoepfe nebeneinander, Abstand 6, rechtsbuendig
//   QSpinBox       max( Text von min, max ) + " " + 2 (Schreibmarke) + 16
//                  (Pfeile) + 2 x 2 (PM_SpinBoxFrameWidth) (QAbstractSpinBox::sizeHint,
//                  QFusionStyle CT_SpinBox), 22 hoch
//   QLineEdit      17 x "x" + 2 x 2 + 2 x 1 (QLineEdit::sizeHint, CT_LineEdit), 22 hoch
//   QComboBox      laengster Eintrag + 8 (CT_PushButton) + 30 (QFusionStyle
//                  CT_ComboBox), 22 hoch
//   QFrame HLine   3 hoch
// QFormLayout::addWidget legt das Element in die Feldspalte (QFormLayout::addItem
// -> FieldRole); die Feldspalte ist so breit wie das breiteste Feld (an den
// Fotos: OK/Cancel 166, drei Knoepfe 252).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors.
//   For a list of contributors, see the accompanying CONTRIBUTORS file.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#ifndef BHED_GUI_KARTE_QTDIALOG_H
#define BHED_GUI_KARTE_QTDIALOG_H

#include "imgui.h"

#include <string>
#include <vector>

namespace bhed::gui::karteneditor::qtd {

// Innenraum des gerade gezeichneten Dialogs (logisch -> Bildpunkt)
[[nodiscard]] ImVec2 bp(float x, float y);
[[nodiscard]] float px(float l);
[[nodiscard]] float sk();
// Breite eines Textes in Tahoma 8 pt (Qt-Punkte)
[[nodiscard]] float textBreite(const char* text);
[[nodiscard]] inline float textBreite(const std::string& text) { return textBreite(text.c_str()); }

// Wunschbreiten (sizeHint) wie oben beschrieben
[[nodiscard]] float hakenHinweis(const std::string& text);
[[nodiscard]] float radioHinweis(const std::string& text);
[[nodiscard]] float knopfHinweis(const std::string& text);
[[nodiscard]] float knoepfeHinweis(const std::vector<std::string>& texte);
[[nodiscard]] float zeileHinweis();
[[nodiscard]] float spinHinweis(double min, double max, int stellen, const std::string& vorsatz = "");
[[nodiscard]] float kombiHinweis(const std::vector<std::string>& eintraege);
[[nodiscard]] float radioReiheHinweis(const std::vector<std::string>& texte);

// Der Dialog: modal (QDialog::exec) mit Schliessknopf (Qt::WindowCloseButtonHint).
// Gibt true zurueck, solange er offen ist und gezeichnet werden soll.
bool beginnen(const std::string& id, bool& oeffnen, float innenB, float innenH, bool modal = true);
// schliessen: CloseCurrentPopup; danach immer beenden() rufen, wenn beginnen() true war
void beenden(bool schliessen);
// Wurde der Dialog ueber den Schliessknopf (Titelzeile) verlassen?
[[nodiscard]] bool geschlossen();

// Text senkrecht mittig in einer Zeile y .. y + h
void text(const std::string& t, float x, float y, float h, bool aktiv = true);
// Text in den Feldern des Selbsttests merken (merkeFeld) unter "Plugin:<name>"
void merke(const std::string& name, float x, float y, float b, float h);

bool haken(const std::string& text, const std::string& id, bool& wert, float x, float y, float h = 20.0F, bool aktiv = true);
bool radio(const std::string& text, const std::string& id, int& wert, int eigener, float x, float y, float h = 20.0F, bool aktiv = true);
// QHBoxLayout mit Radioknoepfen in der Breite b: jeder bekommt seinen Hinweis
// und einen gleichen Anteil am Rest; wert = Index (oder werte[Index])
void radioReihe(const std::vector<std::string>& texte, const std::string& id, int& wert, float x, float b, float y,
                const std::vector<int>* werte = nullptr);
// SpinBox mit SpinBoxLabel davor (Beschriftung ziehen: 20 Punkte je Schritt,
// Strg x 10; Pfeiltasten, Rad, Pfeilknoepfe). beschriftungX < 0: ohne Beschriftung.
bool zahlFeld(const std::string& beschriftung, const std::string& id, double& wert, double min, double max, int stellen, double schritt,
              float beschriftungX, float x, float b, float y, bool aktiv = true, const std::string& vorsatz = "", const char* tip = nullptr);
bool ganzFeld(const std::string& beschriftung, const std::string& id, int& wert, int min, int max, int schritt, float beschriftungX, float x, float b,
              float y, bool aktiv = true, const char* tip = nullptr);
bool zeileFeld(const std::string& id, std::string& wert, float x, float b, float y, int maxLaenge = 255, bool aktiv = true);
// editierbare QComboBox
bool kombiFeld(const std::string& id, std::string& wert, const std::vector<std::string>& liste, float x, float b, float y);
// Stufe 8b: nicht editierbare QComboBox (ComboBox des Terrain Generators): wert = Index
bool wahlFeld(const std::string& id, int& wert, const std::vector<std::string>& liste, float x, float b, float y, bool aktiv = true);
// ein einzelner QPushButton in der Breite b (Terrain Generator: "Pick", "Reset to Default")
bool knopf(const std::string& text, const std::string& id, float x, float b, float y, const char* tip = nullptr);
// QDialogButtonBox rechtsbuendig bis "rechts": der Index des gedrueckten Knopfs
// oder -1. Enter (ausser beim Schreiben) = standard, Escape = abbrechen (-2: keiner).
int knoepfe(const std::vector<std::string>& texte, const std::string& id, float rechts, float y, int standard, int abbrechen);
// QFrame HLine (Raised): eine helle und eine dunkle Linie
void hLinie(float x, float y, float b);
// Rahmen einer QGroupBox
void rahmen(float x, float y, float b, float h);

}  // namespace bhed::gui::karteneditor::qtd

#endif
