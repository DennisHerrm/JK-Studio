// teiler.cpp - Karten-Editor: wie Qt 5 den Platz auf Fenster verteilt.
// Portiert aus NetRadiant-Custom und Qt 5.15: qGeomCalc und QSplitter, wie
// NRCs Hauptfenster sie benutzt (Quellen und Zeilen in teiler.h).
//
//   Copyright (C) 1999-2006 Id Software, Inc. and contributors (NetRadiant-Custom).
//   Copyright (C) 2016 The Qt Company Ltd.
//   Qt 5.15 is licensed under the GNU General Public License version 2 or
//   (at your option) version 3 (alternatively LGPL v3). This file is part of
//   BehavEd-Remake and licensed under the GNU General Public License version
//   2 or later (LICENSE).
#include "bhed/karte/teiler.h"

#include <algorithm>
#include <cstdint>

namespace bhed::karte {

namespace {

// Festkomma mit 8 Nachkommabits (qlayoutengine.cpp:55-60).
using Fest = std::int64_t;
Fest fest(int i) { return static_cast<Fest>(i) * 256; }
int fRunden(Fest i) { return (i % 256 < 128) ? static_cast<int>(i / 256) : static_cast<int>(1 + i / 256); }

}  // namespace

// qGeomCalc (qlayoutengine.cpp:75-334)
void geomCalc(std::vector<LayoutFeld>& kette, int start, int anzahl, int pos, int platz, int abstand) {
    int cHint = 0;
    int cMin = 0;
    int sumStretch = 0;
    int sumSpacing = 0;
    int expandingCount = 0;
    bool allEmptyNonstretch = true;
    int pendingSpacing = -1;
    int spacerCount = 0;
    const auto wirksam = [abstand](const LayoutFeld& d) { return abstand >= 0 ? abstand : d.abstand; };
    for (int i = start; i < start + anzahl; ++i) {
        LayoutFeld& d = kette[static_cast<std::size_t>(i)];
        d.fertig = false;
        cHint += d.klugerHinweis();
        cMin += d.minimum;
        sumStretch += d.dehnung;
        if (!d.leer) {
            // Der Abstand nach dem letzten nicht leeren Eintrag zaehlt nicht.
            if (pendingSpacing >= 0) {
                sumSpacing += pendingSpacing;
                ++spacerCount;
            }
            pendingSpacing = wirksam(d);
        }
        if (d.dehnbar) { ++expandingCount; }
        allEmptyNonstretch = allEmptyNonstretch && d.leer && !d.dehnbar && d.dehnung <= 0;
    }

    int extraspace = 0;
    if (platz < cMin + sumSpacing) {
        // Weniger Platz als die Mindestgroessen: vom Groessten zuerst nehmen.
        const int minSize = cMin + sumSpacing;
        if (abstand >= 0) {
            abstand = minSize > 0 ? abstand * platz / minSize : 0;
            sumSpacing = abstand * spacerCount;
        }
        std::vector<int> minimumSizes;
        minimumSizes.reserve(static_cast<std::size_t>(anzahl));
        for (int i = start; i < start + anzahl; ++i) { minimumSizes.push_back(kette[static_cast<std::size_t>(i)].minimum); }
        std::sort(minimumSizes.begin(), minimumSizes.end());
        const int space_left = platz - sumSpacing;
        int sum = 0;
        int idx = 0;
        int space_used = 0;
        int current = 0;
        while (idx < anzahl && space_used < space_left) {
            current = minimumSizes[static_cast<std::size_t>(idx)];
            space_used = sum + current * (anzahl - idx);
            sum += current;
            ++idx;
        }
        --idx;
        const int deficit = space_used - space_left;
        const int items = anzahl - idx;
        const int deficitPerItem = deficit / items;
        const int remainder = deficit % items;
        const int maxval = current - deficitPerItem;
        int rest = 0;
        for (int i = start; i < start + anzahl; ++i) {
            int maxv = maxval;
            rest += remainder;
            if (rest >= items) {
                --maxv;
                rest -= items;
            }
            LayoutFeld& d = kette[static_cast<std::size_t>(i)];
            d.groesse = std::min(d.minimum, maxv);
            d.fertig = true;
        }
    } else if (platz < cHint + sumSpacing) {
        // Weniger als die Hinweise, mehr als die Mindestgroessen: von jedem
        // gleich viel nehmen (wie Qt 2.x).
        int n = anzahl;
        int space_left = platz - sumSpacing;
        int overdraft = cHint - space_left;
        for (int i = start; i < start + anzahl; ++i) {
            LayoutFeld& d = kette[static_cast<std::size_t>(i)];
            if (!d.fertig && d.minimum >= d.klugerHinweis()) {
                d.groesse = d.klugerHinweis();
                d.fertig = true;
                space_left -= d.klugerHinweis();
                --n;
            }
        }
        bool finished = n == 0;
        while (!finished) {
            finished = true;
            const Fest fp_over = fest(overdraft);
            Fest fp_w = 0;
            for (int i = start; i < start + anzahl; ++i) {
                LayoutFeld& d = kette[static_cast<std::size_t>(i)];
                if (d.fertig) { continue; }
                fp_w += fp_over / n;
                const int w = fRunden(fp_w);
                d.groesse = d.klugerHinweis() - w;
                fp_w -= fest(w);
                if (d.groesse < d.minimum) {
                    d.fertig = true;
                    d.groesse = d.minimum;
                    finished = false;
                    overdraft -= d.klugerHinweis() - d.minimum;
                    --n;
                    break;
                }
            }
        }
    } else {
        // Mehr Platz als noetig.
        int n = anzahl;
        int space_left = platz - sumSpacing;
        // Zuerst die festen und die nicht dehnbaren.
        for (int i = start; i < start + anzahl; ++i) {
            LayoutFeld& d = kette[static_cast<std::size_t>(i)];
            if (!d.fertig && (d.maximum <= d.klugerHinweis() || (!allEmptyNonstretch && d.leer && !d.dehnbar && d.dehnung == 0))) {
                d.groesse = d.klugerHinweis();
                d.fertig = true;
                space_left -= d.groesse;
                sumStretch -= d.dehnung;
                if (d.dehnbar) { --expandingCount; }
                --n;
            }
        }
        extraspace = space_left;
        // Probeverteilung; wer zu wenig hat, bekommt sein Mindestmass, wer zu
        // viel hat, sein Hoechstmass - und von vorn.
        int surplus = 0;
        int deficit = 0;
        do {
            surplus = 0;
            deficit = 0;
            const Fest fp_space = fest(space_left);
            Fest fp_w = 0;
            for (int i = start; i < start + anzahl; ++i) {
                LayoutFeld& d = kette[static_cast<std::size_t>(i)];
                if (d.fertig) { continue; }
                extraspace = 0;
                if (sumStretch > 0) {
                    fp_w += (fp_space * d.dehnung) / sumStretch;
                } else if (expandingCount > 0) {
                    fp_w += (fp_space * (d.dehnbar ? 1 : 0)) / expandingCount;
                } else {
                    fp_w += fp_space * 1 / n;
                }
                const int w = fRunden(fp_w);
                d.groesse = w;
                fp_w -= fest(w);
                if (w < d.klugerHinweis()) {
                    deficit += d.klugerHinweis() - w;
                } else if (w > d.maximum) {
                    surplus += w - d.maximum;
                }
            }
            if (deficit > 0 && surplus <= deficit) {
                for (int i = start; i < start + anzahl; ++i) {
                    LayoutFeld& d = kette[static_cast<std::size_t>(i)];
                    if (!d.fertig && d.groesse < d.klugerHinweis()) {
                        d.groesse = d.klugerHinweis();
                        d.fertig = true;
                        space_left -= d.klugerHinweis();
                        sumStretch -= d.dehnung;
                        if (d.dehnbar) { --expandingCount; }
                        --n;
                    }
                }
            }
            if (surplus > 0 && surplus >= deficit) {
                for (int i = start; i < start + anzahl; ++i) {
                    LayoutFeld& d = kette[static_cast<std::size_t>(i)];
                    if (!d.fertig && d.groesse > d.maximum) {
                        d.groesse = d.maximum;
                        d.fertig = true;
                        space_left -= d.maximum;
                        sumStretch -= d.dehnung;
                        if (d.dehnbar) { --expandingCount; }
                        --n;
                    }
                }
            }
        } while (n > 0 && surplus != deficit);
        if (n == 0) { extraspace = space_left; }
    }

    // Uebrigen Platz gleichmaessig auf die Abstaende (Anfang und Ende
    // mitgezaehlt).
    const int extra = extraspace / (spacerCount + 2);
    int p = pos + extra;
    for (int i = start; i < start + anzahl; ++i) {
        LayoutFeld& d = kette[static_cast<std::size_t>(i)];
        d.pos = p;
        p += d.groesse;
        if (!d.leer) { p += wirksam(d) + extra; }
    }
}

// --- QStatusBar ----------------------------------------------------------------

std::vector<Spanne> statuszeileVerteilen(const std::vector<StatusFeld>& felder, int breite) {
    // QBoxLayoutPrivate::setupGeom (qboxlayout.cpp): Abstand 6 haengt am
    // vorigen nicht leeren Eintrag; Abstandshalter sind leer.
    std::vector<LayoutFeld> a;
    LayoutFeld zwei;   // l->addSpacing(2): QSpacerItem(2, 0, Fixed, Minimum)
    zwei.hinweis = zwei.minimum = zwei.maximum = 2;
    a.push_back(zwei);
    LayoutFeld dehnung;   // l->addStretch(0): QSpacerItem(0, 0, Expanding, Minimum)
    dehnung.dehnbar = true;
    a.push_back(dehnung);
    int vorher = -1;
    for (const StatusFeld& f : felder) {
        LayoutFeld w;
        w.hinweis = std::max(f.hinweis, f.minimum);
        w.minimum = f.minimum;
        w.maximum = kLayoutMax;
        w.dehnung = f.dehnung;
        w.dehnbar = f.dehnung > 0;   // exp & Qt::Horizontal || box->stretch > 0 (QLabel: Preferred)
        w.leer = false;
        if (vorher >= 0) { a[static_cast<std::size_t>(vorher)].abstand = 6; }   // l->setSpacing(6)
        vorher = static_cast<int>(a.size());
        a.push_back(w);
    }
    geomCalc(a, 0, static_cast<int>(a.size()), 0, breite, -1);
    std::vector<Spanne> v;
    for (std::size_t i = 2; i < a.size(); ++i) { v.push_back({a[i].pos, a[i].groesse}); }
    return v;
}

// --- QToolBarLayout -------------------------------------------------------------

LeistenErgebnis leisteAnordnen(const std::vector<LeistenEintrag>& eintraege, int laenge, const LeistenMasse& m) {
    LeistenErgebnis r;
    const int n = static_cast<int>(eintraege.size());
    r.lage.assign(eintraege.size(), -1);
    // updateGeomArray: Mindestmass = erster sichtbarer Eintrag (+ Erweiterung,
    // sobald es mehr als einen Eintrag gibt), Hinweis = alle sichtbaren.
    std::vector<LayoutFeld> a(eintraege.size() + 1);
    int anzahl = 0;
    for (int i = 0; i < n; ++i) {
        const LeistenEintrag& e = eintraege[static_cast<std::size_t>(i)];
        if (!e.leer) {
            if (anzahl == 0) { r.minimum += e.groesse; }
            r.hinweis += (anzahl == 0 ? 0 : m.abstand) + e.groesse;
            ++anzahl;
        }
        LayoutFeld& f = a[static_cast<std::size_t>(i)];
        // QWidgetItem: versteckt = leer, alle Masse 0; sonst feste Groesse
        // (QToolButton, QToolBarSeparator).
        f.hinweis = e.leer ? 0 : e.groesse;
        f.minimum = f.hinweis;
        f.maximum = f.hinweis;
        f.dehnbar = false;
        f.dehnung = 0;
        f.leer = e.leer;
    }
    r.minimum += m.griff + 2 * m.rand;
    if (n > 1) { r.minimum += m.abstand + m.erweiterung; }
    r.hinweis += m.griff + 2 * m.rand;

    // layoutActions (nicht ausgeklappt: eine Reihe, der Rest ins Menue)
    const int space = laenge - 2 * m.rand - m.griff;
    if (space <= 0) { return r; }
    bool ranOutOfSpace = false;
    int i = 0;
    const int start = 0;
    int size = 0;
    int prev = -1;
    int count = 0;
    for (; i < n; ++i) {
        if (a[static_cast<std::size_t>(i)].leer) { continue; }
        const int newSize = size + (count == 0 ? 0 : m.abstand) + a[static_cast<std::size_t>(i)].minimum;
        if (prev != -1 && newSize > space) {
            ranOutOfSpace = true;
            // Muss der vorige Eintrag fuer den Erweiterungsknopf weichen?
            if (count > 1 && size + m.abstand + m.erweiterung > space) { i = prev; }
            break;
        }
        size = newSize;
        prev = i;
        ++count;
    }
    // Dehnung am Ende
    LayoutFeld& dehn = a[static_cast<std::size_t>(i)];
    dehn = LayoutFeld{};
    dehn.hinweis = 0;
    dehn.maximum = kWidgetMax;
    dehn.minimum = 0;
    dehn.dehnbar = true;
    dehn.dehnung = 0;
    dehn.leer = true;
    geomCalc(a, start, i - start + 1, 0, space - (ranOutOfSpace ? (m.erweiterung + m.abstand) : 0), m.abstand);
    for (int j = start; j < i; ++j) {
        if (a[static_cast<std::size_t>(j)].leer) { continue; }
        r.lage[static_cast<std::size_t>(j)] = m.rand + m.griff + a[static_cast<std::size_t>(j)].pos;
    }
    if (ranOutOfSpace) {
        r.ersterVersteckter = i;
        r.erweiterung = true;
        // setGeometry: rect.bottomRight() - Rand - Erweiterung + 2
        r.erweiterungLage = (laenge - 1) - m.rand - m.erweiterung + 2;
    }
    return r;
}

std::vector<Spanne> leistenZeile(const std::vector<std::pair<int, int>>& minHinweis, int laenge) {
    std::vector<Spanne> v(minHinweis.size());
    int min = 0;
    for (const auto& [mi, hi] : minHinweis) {
        (void)hi;
        min += mi;
    }
    int extra = std::max(0, laenge - min);
    for (std::size_t i = 0; i < minHinweis.size(); ++i) {
        const int itemMin = minHinweis[i].first;
        const int groesse = minHinweis[i].second;
        const int extraSpace = std::min(groesse - itemMin, extra);
        v[i].groesse = itemMin + extraSpace;
        extra -= extraSpace;
    }
    int pos = 0;
    for (std::size_t i = 0; i < v.size(); ++i) {
        v[i].pos = pos;
        if (i + 1 == v.size()) { v[i].groesse = std::max(0, laenge - v[i].pos); }   // die letzte bis zum Ende
        pos += v[i].groesse;
    }
    return v;
}

// --- QSplitter ---------------------------------------------------------------

bool Teiler::griffVersteckt(int i) const {
    // recalc: Griffe vor dem ersten sichtbaren Kind und vor versteckten.
    if (kinder[static_cast<std::size_t>(i)].versteckt) { return true; }
    for (int k = 0; k < i; ++k) {
        if (!kinder[static_cast<std::size_t>(k)].versteckt) { return false; }
    }
    return true;
}

int Teiler::widgetGroesse(Kind& k) {
    if (k.sizer == -1) {
        // Ohne gueltigen Hinweis (oder groesser gesetzt) zaehlt die
        // tatsaechliche Groesse.
        if (k.hinweis < 0 || (k.groesseGesetzt && k.startGroesse > k.hinweis)) {
            k.sizer = k.startGroesse;
        } else {
            k.sizer = k.hinweis;
        }
        if (k.dehnfaktor > 1) { k.sizer *= k.dehnfaktor; }
    }
    return k.sizer;
}

void Teiler::anordnen(int lage, int laenge) {
    start = lage;
    platz = laenge;
    const int n = static_cast<int>(kinder.size());
    std::vector<LayoutFeld> a(static_cast<std::size_t>(n) * 2);
    bool noStretchFactorsSet = true;
    for (const Kind& k : kinder) {
        if (k.dehnfaktor != 0) {
            noStretchFactorsSet = false;
            break;
        }
    }
    int j = 0;
    for (int i = 0; i < n; ++i) {
        Kind& s = kinder[static_cast<std::size_t>(i)];
        LayoutFeld& h = a[static_cast<std::size_t>(j)];
        h = LayoutFeld{};
        if (griffVersteckt(i)) {
            h.maximum = 0;
        } else {
            h.hinweis = h.minimum = h.maximum = griff;
            h.leer = false;
        }
        ++j;
        LayoutFeld& w = a[static_cast<std::size_t>(j)];
        w = LayoutFeld{};
        if (s.versteckt || s.eingeklappt) {
            w.maximum = 0;
        } else {
            w.minimum = s.minimum;
            w.maximum = s.maximum;
            w.leer = false;
            bool dehnen = noStretchFactorsSet;
            if (!dehnen) { dehnen = s.dehnfaktor != 0; }
            if (dehnen) {
                w.dehnung = widgetGroesse(s);
                w.hinweis = w.minimum;
                w.dehnbar = true;
            } else {
                w.hinweis = std::max(widgetGroesse(s), w.minimum);
            }
        }
        ++j;
    }
    geomCalc(a, 0, n * 2, lage, laenge, 0);
    for (int i = 0; i < n; ++i) {
        const LayoutFeld& w = a[static_cast<std::size_t>(i) * 2 + 1];
        setzeLage(kinder[static_cast<std::size_t>(i)], w.pos, w.groesse, false);
    }
}

void Teiler::setzeLage(Kind& k, int p, int s, bool einklappenErlaubt) {
    k.pos = p;
    k.groesse = s;
    if (einklappenErlaubt) { k.eingeklappt = s <= 0 && k.minimum > 0 && !k.versteckt; }
}

void Teiler::setzeGroessen(const std::vector<int>& groessen) {
    std::size_t j = 0;
    for (Kind& s : kinder) {
        s.eingeklappt = false;
        s.sizer = j < groessen.size() ? groessen[j] : 0;
        ++j;
        s.sizer = std::max(s.sizer, 0);   // clampNegativeSize
        if (s.sizer == 0) {
            if (s.einklappbar && s.minimum > 0) {
                s.eingeklappt = true;
            } else {
                s.sizer = s.minimum;
            }
        } else if (s.sizer < s.minimum) {
            s.sizer = s.minimum;
        }
    }
    anordnen(start, platz);
}

std::vector<int> Teiler::groessen() const {
    std::vector<int> v;
    v.reserve(kinder.size());
    for (const Kind& k : kinder) { v.push_back(k.groesse); }
    return v;
}

void Teiler::groessenMerken() {
    for (Kind& k : kinder) { k.sizer = k.groesse; }
}

bool Teiler::einklappbar(int index) const {
    if (index < 0 || index >= static_cast<int>(kinder.size())) { return false; }
    return kinder[static_cast<std::size_t>(index)].einklappbar;
}

void Teiler::beitrag(int index, int* min, int* max, bool mayCollapse) const {
    const Kind& s = kinder[static_cast<std::size_t>(index)];
    if (!s.versteckt) {
        if (!griffVersteckt(index)) {
            *min += griff;
            *max += griff;
        }
        if (mayCollapse || !s.eingeklappt) { *min += s.minimum; }
        *max += s.maximum;
    }
}

int Teiler::davorOderDanach(int index, int delta, int& einklappGroesse) const {
    if (delta < 0) { index += delta; }
    do {
        const Kind& k = kinder[static_cast<std::size_t>(index)];
        if (!k.versteckt) {
            if (k.einklappbar) { einklappGroesse = k.minimum; }
            return index;
        }
        index += delta;
    } while (index >= 0 && index < static_cast<int>(kinder.size()));
    return -1;
}

void Teiler::bereich(int index, int* farMin, int* min, int* max, int* farMax) const {
    const int n = static_cast<int>(kinder.size());
    if (index <= 0 || index >= n) { return; }
    int collapsibleSizeBefore = 0;
    const int idJustBefore = davorOderDanach(index, -1, collapsibleSizeBefore);
    int collapsibleSizeAfter = 0;
    const int idJustAfter = davorOderDanach(index, +1, collapsibleSizeAfter);
    int minBefore = 0;
    int minAfter = 0;
    int maxBefore = 0;
    int maxAfter = 0;
    for (int i = 0; i < index; ++i) { beitrag(i, &minBefore, &maxBefore, i == idJustBefore); }
    for (int i = index; i < n; ++i) { beitrag(i, &minAfter, &maxAfter, i == idJustAfter); }
    const int smartMinBefore = std::max(minBefore, platz - maxAfter);
    const int smartMaxBefore = std::min(maxBefore, platz - minAfter);
    const int minVal = start + smartMinBefore;
    const int maxVal = start + smartMaxBefore;
    int farMinVal = minVal;
    if (minBefore - collapsibleSizeBefore >= platz - maxAfter) { farMinVal -= collapsibleSizeBefore; }
    int farMaxVal = maxVal;
    if (platz - (minAfter - collapsibleSizeAfter) <= maxBefore) { farMaxVal += collapsibleSizeAfter; }
    *farMin = farMinVal;
    *min = minVal;
    *max = maxVal;
    *farMax = farMaxVal;
}

int Teiler::anpassen(int pos, int index, int* farMin, int* min, int* max, int* farMax) const {
    const int kSchwelle = 40;   // Threshold
    bereich(index, farMin, min, max, farMax);
    if (pos >= *min) {
        if (pos <= *max) { return pos; }
        const int delta = pos - *max;
        const int width = *farMax - *max;
        return (delta > width / 2 && delta >= std::min(kSchwelle, width)) ? *farMax : *max;
    }
    const int delta = *min - pos;
    const int width = *min - *farMin;
    return (delta > width / 2 && delta >= std::min(kSchwelle, width)) ? *farMin : *min;
}

void Teiler::verschieben(bool rueckwaerts, int hPos, int index, int delta, bool mayCollapse, std::vector<int>& lagen,
                         std::vector<int>& breiten) const {
    if (index < 0 || index >= static_cast<int>(kinder.size())) { return; }
    const Kind& s = kinder[static_cast<std::size_t>(index)];
    const int nextId = rueckwaerts ? index - delta : index + delta;
    if (s.versteckt) {
        verschieben(rueckwaerts, hPos, nextId, delta, einklappbar(nextId), lagen, breiten);
        return;
    }
    const int hs = griffVersteckt(index) ? 0 : griff;
    // rect.bottomRight() = pos + groesse - 1
    int ws = rueckwaerts ? hPos - s.pos : (s.pos + s.groesse - 1) - hPos - hs + 1;
    if (ws > 0 || (!s.eingeklappt && !mayCollapse)) {
        ws = std::min(ws, s.maximum);
        ws = std::max(ws, s.minimum);
    } else {
        ws = 0;
    }
    lagen[static_cast<std::size_t>(index)] = rueckwaerts ? hPos - ws : hPos + hs;
    breiten[static_cast<std::size_t>(index)] = ws;
    verschieben(rueckwaerts, rueckwaerts ? hPos - ws - hs : hPos + hs + ws, nextId, delta, einklappbar(nextId), lagen,
                breiten);
}

void Teiler::ziehen(int pos, int index) {
    if (index <= 0 || index >= static_cast<int>(kinder.size())) { return; }
    const Kind& s = kinder[static_cast<std::size_t>(index)];
    int farMin = 0;
    int min = 0;
    int max = 0;
    int farMax = 0;
    pos = anpassen(pos, index, &farMin, &min, &max, &farMax);
    const int oldP = s.pos;
    std::vector<int> lagen(kinder.size(), 0);
    std::vector<int> breiten(kinder.size(), 0);
    verschieben(false, pos, index, +1, einklappbar(index) && pos > max, lagen, breiten);
    verschieben(true, pos, index - 1, +1, einklappbar(index - 1) && pos < min, lagen, breiten);
    const bool upLeft = pos < oldP;
    const int count = static_cast<int>(kinder.size());
    const int startI = upLeft ? 0 : count - 1;
    const int schritt = upLeft ? 1 : -1;
    for (int wid = startI; wid >= 0 && wid < count; wid += schritt) {
        Kind& k = kinder[static_cast<std::size_t>(wid)];
        if (!k.versteckt) {
            setzeLage(k, lagen[static_cast<std::size_t>(wid)], breiten[static_cast<std::size_t>(wid)], true);
        }
    }
    groessenMerken();
}

}  // namespace bhed::karte
