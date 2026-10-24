// ebenen.h - Karten-Editor: die Ebenen (Layers) und was der Ebenen-Browser
// mit ihnen tut.
//
// Portiert aus NetRadiant-Custom 20260114: libs/layers.h (Layer, Layers,
// forEach, update_ownIndices), radiant/layerswindow.cpp:64-198
// (LayerAssignVisitor, LayerCollectVisitor, LayerHideWalker, LayerShowWalker,
// LayerSelectWalker, LayerDeletetWalker), 336-352 (layer_new), 354-416
// (context_menu: Move Selection to This Layer, New Layer, Delete, Rename,
// Color), 418-430 (context_moveto_menu), 432-466 (LayersSetVisible,
// layers_setVisible_all), 468-491 (itemClicked: Select mit Strg/Alt,
// Umschalt = Move to layer), 270-301 (QTreeWidget_layers::rowsInserted:
// Ziehen und Ablegen), 566-576 (Scene_ExpandSelectionToLayers).
//
// Die Karte haelt die Ebenen flach in Tiefensuche (Karte::schichten); jede
// Aenderung am Baum baut ihn auf, aendert ihn und legt ihn neu ab, die
// Knoten (Node::m_layer = schicht) bekommen ihre neuen Nummern.
// NRC kennt fuer die Ebenen kein Rueckgaengig (layerswindow.cpp:61 "? undo").
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
#ifndef BHED_KARTE_EBENEN_H
#define BHED_KARTE_EBENEN_H

#include <array>
#include <string>
#include <vector>

namespace bhed::karte {

class Karte;

// Die direkten Kinder einer Ebene (eltern -1 = die Wurzel), in ihrer Reihenfolge.
[[nodiscard]] std::vector<int> ebenenKinder(const Karte& k, int eltern);
// item_getLayers: die Ebene und alle darunter (Tiefensuche)
[[nodiscard]] std::vector<int> ebenenUnterbaum(const Karte& k, int ebene);
// Die Tiefe einer Ebene (0 = oberste)
[[nodiscard]] int ebenenTiefe(const Karte& k, int ebene);

// Ergebnis einer Aenderung am Baum: alte Nummer -> neue Nummer (-1 = geloescht),
// fuer alles, was neben der Karte je Ebene etwas haelt (Sichtbarkeit im Browser).
using EbenenNummern = std::vector<int>;

// layer_new: als letztes Kind von eltern (-1 = Wurzel), wird die aktuelle
// Ebene (item_setCurrent). neu = die Nummer der neuen Ebene.
EbenenNummern ebeneNeu(Karte& k, int eltern, const std::string& name, int* neu = nullptr);
// context_menu "Delete": die Ebene samt Unterebenen; was darin lag, kommt auf
// die aktuelle Ebene (ist sie geloescht: die erste oberste). Nicht erlaubt
// fuer die einzige oberste Ebene (leeres Ergebnis).
EbenenNummern ebeneLoeschen(Karte& k, int ebene);
// rowsInserted nach dem Ablegen: die Ebene (mit Unterebenen) als Kind
// position unter neuerEltern (-1 = Wurzel).
EbenenNummern ebeneVerschieben(Karte& k, int ebene, int neuerEltern, int position);
// "Rename" (ohne '"') und "Color"
void ebeneUmbenennen(Karte& k, int ebene, const std::string& name);
void ebeneFarbe(Karte& k, int ebene, const std::array<int, 3>& farbe);
// QInputDialog-Text ohne '"' (text.remove( '"' ))
[[nodiscard]] std::string ebenenName(const std::string& text);

// LayerAssignVisitor: die gewaehlten Knoten (ausser Gruppenentitaeten) auf die Ebene
void auswahlAufEbene(Karte& k, int ebene);
// LayerSelectWalker: sichtbare Knoten der Ebenen waehlen bzw. abwaehlen
void ebenenWaehlen(Karte& k, const std::vector<int>& ebenen, bool waehlen);
// LayersSetVisible::setVisible: Knoten der Ebenen zeigen bzw. verstecken
// (Node::eLayerHidden), Gruppen mit; worldspawn bleibt sichtbar.
void ebenenSichtbar(Karte& k, const std::vector<int>& ebenen, bool sichtbar);
// Scene_ExpandSelectionToLayers (Strg+Q)
void auswahlAufEbenenErweitern(Karte& k);

}  // namespace bhed::karte

#endif
