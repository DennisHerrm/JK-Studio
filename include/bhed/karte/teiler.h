// teiler.h - Karten-Editor: wie Qt 5 den Platz auf Fenster verteilt.
//
// NetRadiant-Custom baut sein Hauptfenster aus QSplitter (m_hSplit, m_vSplit,
// m_vSplit2), einer QStatusBar mit QHBoxLayout und QToolBars. Wie gross jedes
// Feld wird - beim Start, beim Vergroessern des Fensters und beim Ziehen eines
// Griffs - entscheidet Qt, nicht NRC. Damit die Felder genau so gross werden
// wie im Original, ist hier die Rechnung von Qt 5.15 nachgebaut, mit der
// NetRadiant-Custom 20260114 (Qt 5.15, MSYS2) ausgeliefert wird.
//
// Portiert aus NetRadiant-Custom und Qt 5.15:
//   radiant/mainframe.cpp:1717-1870 (Aufbau der QSplitter je Anordnung),
//   1891-1900 (RestoreGuiState: Startgroessen {384,576}, {377,20}, {250,150})
//
//   qGeomCalc          qtbase/src/widgets/kernel/qlayoutengine.cpp:75-334
//                      (QLayoutStruct: qlayoutengine_p.h:63-96)
//   QSplitter          qtbase/src/widgets/widgets/qsplitter.cpp:360-844,
//                      1091-1095, 1410-1450, 1610-1625 (getWidgetSize,
//                      doResize, storeSizes, addContribution, getRange,
//                      adjustPos, setGeo, doMove, setSizes_helper,
//                      moveSplitter, sizes)
//   QBoxLayout         qtbase/src/widgets/kernel/qboxlayout.cpp (setupGeom:
//                      Abstand nur zwischen nicht leeren Eintraegen)
//   QToolBarLayout     qtbase/src/widgets/widgets/qtoolbarlayout.cpp:257-347
//                      (updateGeomArray: minimumSize, sizeHint), 382-433
//                      (setGeometry: Erweiterungsknopf), 436-582
//                      (layoutActions, eine Reihe, nicht ausgeklappt)
//   QToolBarAreaLayoutLine::fitLayout
//                      qtbase/src/widgets/widgets/qtoolbararealayout.cpp
//                      (mehrere Leisten in einer Zeile eines Andockbereichs)
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors (NetRadiant-Custom).
//   Copyright (C) 2016 The Qt Company Ltd.
//   Qt 5.15 is licensed under the GNU General Public License version 2 or
//   (at your option) version 3 (alternatively LGPL v3). This file is part of
//   BehavEd-Remake and licensed under the GNU General Public License version
//   2 or later (LICENSE).
#ifndef BHED_KARTE_TEILER_H
#define BHED_KARTE_TEILER_H

#include <utility>
#include <string>
#include <vector>

namespace bhed::karte {

// QLAYOUTSIZE_MAX (qlayoutitem.h) und QWIDGETSIZE_MAX (qwidget.h).
inline constexpr int kLayoutMax = 2147483647 / 256 / 16;
inline constexpr int kWidgetMax = (1 << 24) - 1;

// QLayoutStruct
struct LayoutFeld {
    int dehnung = 0;          // stretch
    int hinweis = 0;          // sizeHint
    int maximum = kLayoutMax; // maximumSize
    int minimum = 0;          // minimumSize
    int abstand = 0;          // spacing (Abstand nach diesem Eintrag, QBoxLayout)
    bool dehnbar = false;     // expansive
    bool leer = true;         // empty
    bool fertig = false;      // done
    int pos = 0;              // Ergebnis
    int groesse = 0;          // Ergebnis

    [[nodiscard]] int klugerHinweis() const { return dehnung > 0 ? minimum : hinweis; }   // smartSizeHint
};

// qGeomCalc: verteilt platz ab pos auf kette[start, start+anzahl).
// abstand >= 0: ein Abstand fuer alle (QSplitter: 0), -1: der eigene
// Abstand jedes Eintrags (QBoxLayout).
void geomCalc(std::vector<LayoutFeld>& kette, int start, int anzahl, int pos, int platz, int abstand = -1);

// Ein Eintrag der Statuszeile (QStatusBar::addPermanentWidget(w, stretch)).
struct StatusFeld {
    int hinweis = 0;   // sizeHint().width()
    int minimum = 0;   // qSmartMinSize(w).width()
    int dehnung = 0;   // stretch
};
struct Spanne {
    int pos = 0;
    int groesse = 0;
};
// QStatusBar::reformat (qstatusbar.cpp:330-385): QHBoxLayout mit
// addSpacing(2), addStretch(0), den Eintraegen und setSpacing(6); verteilt
// mit QBoxLayout (setupGeom) und qGeomCalc auf breite.
[[nodiscard]] std::vector<Spanne> statuszeileVerteilen(const std::vector<StatusFeld>& felder, int breite);

// Eine Aktion einer QToolBar entlang ihrer Richtung: Knopf (31) oder
// Trenner (8); leer = versteckt (ToolbarItems: Trenner am Rand, versteckte
// Knoepfe).
struct LeistenEintrag {
    int groesse = 0;   // pick(o, sizeHint) = minimumSize (QToolButton, QToolBarSeparator)
    bool leer = false;
};
struct LeistenMasse {
    int rand = 4;          // PM_ToolBarItemMargin + PM_ToolBarFrameWidth (dark.qss: border 1 + padding 3)
    int griff = 9;         // PM_ToolBarHandleExtent (Fusion), beweglich
    int abstand = 0;       // PM_ToolBarItemSpacing (dark.qss spacing 0)
    int erweiterung = 12;  // PM_ToolBarExtensionExtent
};
struct LeistenErgebnis {
    std::vector<int> lage;        // Lage jedes Eintrags (-1: versteckt oder im Erweiterungsmenue)
    int ersterVersteckter = -1;   // ab diesem Eintrag im Erweiterungsmenue (-1: alles passt)
    bool erweiterung = false;     // Erweiterungsknopf sichtbar
    int erweiterungLage = 0;
    int minimum = 0;              // pick(o, minimumSize())
    int hinweis = 0;              // pick(o, sizeHint())
};
[[nodiscard]] LeistenErgebnis leisteAnordnen(const std::vector<LeistenEintrag>& eintraege, int laenge,
                                             const LeistenMasse& m = LeistenMasse{});
// QToolBarAreaLayoutLine::fitLayout: Leisten einer Zeile, je {minimum,
// hinweis}; die Mindestgroessen zuerst, der Rest der Reihe nach bis zum
// Hinweis, die letzte reicht bis zum Ende.
[[nodiscard]] std::vector<Spanne> leistenZeile(const std::vector<std::pair<int, int>>& minHinweis, int laenge);

// Stufe 9c: die Werkzeugleisten an den vier Raendern wie QMainWindows
// Toolbar-Bereiche (Qt::Top/Left/Right/BottomToolBarArea): oben und unten ueber
// die ganze Breite, links und rechts dazwischen; die Leisten eines Rands teilen
// sich eine Zeile (fitLayout, leistenZeile) in ihrer Reihenfolge. Was bleibt, ist
// die Mitte (die Ansichten mit ihren Teilern).
enum class LeistenRand { Oben = 0, Links = 1, Rechts = 2, Unten = 3 };
struct LeistenPlatz {
    LeistenRand rand = LeistenRand::Oben;
    int reihenfolge = 0;
    bool sichtbar = true;
    int minimum = 0;   // minimumSize entlang der Leiste
    int hinweis = 0;   // sizeHint entlang der Leiste
};
struct LeistenRahmen {
    bool da = false;
    bool waagerecht = true;
    int x = 0;
    int y = 0;
    int b = 0;
    int h = 0;
};
struct LeistenAnordnung {
    std::vector<LeistenRahmen> leisten;   // je LeistenPlatz
    int x = 0;                            // die Mitte
    int y = 0;
    int b = 0;
    int h = 0;
};
[[nodiscard]] LeistenAnordnung leistenAndocken(const std::vector<LeistenPlatz>& leisten, int x, int y, int b, int h, int dicke);
// "haupt=oben:0;filter=rechts:0;plugin=rechts:1" (Einstellung BhedToolbarDock)
[[nodiscard]] std::string leistenText(const std::vector<std::pair<std::string, LeistenPlatz>>& l);
bool leistenLesen(const std::string& text, std::vector<std::pair<std::string, LeistenPlatz>>& l);

// Ein QSplitter mit seinen Kindern, in logischen Qt-Bildpunkten.
class Teiler {
public:
    struct Kind {
        int sizer = -1;              // QSplitterLayoutStruct::sizer (-1: noch nicht bestimmt)
        int minimum = 0;             // pick(qSmartMinSize(widget))
        int maximum = kWidgetMax;    // pick(widget->maximumSize())
        int dehnfaktor = 0;          // sizePolicy().horizontalStretch()/verticalStretch()
        int hinweis = -1;            // pick(widget->sizeHint()), -1 = ungueltig
        int startGroesse = 0;        // pick(widget->size()) vor dem ersten Anordnen
        bool groesseGesetzt = false; // Qt::WA_Resized
        bool versteckt = false;      // widget->isHidden()
        bool eingeklappt = false;    // collapsed
        bool einklappbar = true;     // childrenCollapsible (Vorgabe true)
        // Ergebnis (rect)
        int pos = 0;
        int groesse = 0;
    };

    std::vector<Kind> kinder;
    int griff = 4;      // PM_SplitterWidth (Fusion: 4)
    int start = 0;      // contentsRect, Lage
    int platz = 0;      // contentsRect, Laenge

    // Griff i sitzt vor Kind i; der erste sichtbare ist versteckt
    // (recalc, qsplitter.cpp:394-409).
    [[nodiscard]] bool griffVersteckt(int i) const;
    [[nodiscard]] int griffPos(int i) const { return kinder[static_cast<std::size_t>(i)].pos - griff; }
    // doResize (qsplitter.cpp:483-563): beim ersten Zeigen und bei jeder
    // Groessenaenderung.
    void anordnen(int lage, int laenge);
    // setSizes (setSizes_helper, qsplitter.cpp:709-734)
    void setzeGroessen(const std::vector<int>& groessen);
    // sizes() (qsplitter.cpp:1590-1605): eingeklappte zaehlen 0.
    [[nodiscard]] std::vector<int> groessen() const;
    // moveSplitter (qsplitter.cpp:1410-1450): den Griff vor Kind index an
    // die Lage pos (Abstand vom Anfang des Teilers) ziehen.
    void ziehen(int pos, int index);
    // closestLegalPosition / getRange fuer den Griff index.
    void bereich(int index, int* farMin, int* min, int* max, int* farMax) const;

private:
    int widgetGroesse(Kind& k);   // getWidgetSize (qsplitter.cpp:360-377)
    [[nodiscard]] bool einklappbar(int index) const;
    void beitrag(int index, int* min, int* max, bool mayCollapse) const;   // addContribution
    int davorOderDanach(int index, int delta, int& einklappGroesse) const;   // findWidgetJustBeforeOrJustAfter
    int anpassen(int pos, int index, int* farMin, int* min, int* max, int* farMax) const;   // adjustPos
    void setzeLage(Kind& k, int p, int s, bool einklappenErlaubt);   // setGeo
    void verschieben(bool rueckwaerts, int hPos, int index, int delta, bool mayCollapse, std::vector<int>& lagen,
                     std::vector<int>& breiten) const;   // doMove
    void groessenMerken();   // storeSizes
};

}  // namespace bhed::karte

#endif
