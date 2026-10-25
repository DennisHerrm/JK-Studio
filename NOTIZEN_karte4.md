# Karten-Editor, Stufe 4: fertige Karten oeffnen, Originallicht, alle Plugins

Stand: Y:\karte, Zweig `karte1` (auf Stufe 3, `f7334a9`). Vorbild ist
NetRadiant-Custom 20260114 (NRC, Git-Tag `20260114` in
`SP\nrc_src\netradiant-custom`, ausgepackt nach `SP\s4\tag`; Binaries in
`SP\nrc_dl\nrc`). Quellangaben in den Kommentaren: `Funktion (datei:zeile)`.

`SP` = `C:\Users\shank\AppData\Local\Temp\claude\c--Users-shank-Downloads-behaved-1-0-0-rc567-behaved\af9c6614-e078-4efa-a574-1fa901786464\scratchpad`

Regeln wie in Stufe 3: je Punkt ein gezielter Rot/Gruen-Test, ctest, ein
Commit je Punkt; radiant.exe nie gestartet; ein Testprozess zur Zeit mit
Prioritaet Idle (`SP\s2\idle_karte.sh`, 8-GB-Waechter).

## Punkt 0c: keine abgeschnittene Beschriftung (Einstellungen, 4 Sprachen)

Neue Pruefung im Selbsttest (`BHED_KARTE_NUR=stufe4`, `stufe4::reste0`): jede
der 23 Einstellungsseiten in Englisch, Deutsch, Chinesisch und Japanisch; aus
der Zeichenliste des Fensters jede Glyphe (Vierecke mit der Schrift-Textur,
nicht das weisse Pixel und nicht die UV der kantengeglaetteten Linien, ohne
den Fenstertitel) muss waagerecht im Rahmen der Seitengruppe liegen (164..586
- links davon verdeckt der Seitenbaum, was hinausragt), jede Glyphe des Baums
in dessen Ausschnitt. ImGui gibt Glyphen ganz ausserhalb des Ausschnitts gar
nicht aus; die eine, die ueber den Rand reicht, ist der Befund.

| Lauf | Ergebnis |
|---|---|
| s4p0rot4 (vorher) | Englisch 0, Deutsch 23, Japanisch 23 FEHLER: Baum "Oberflaechen-Inspektor" 4 Bildpunkte zu lang; Seite Brush: "Ebenenpunkte auf ganze Zahlen runden (kann Pinsel zerstoeren)" rechts hinaus; Japanisch: Seite Interface Beschriftung links hinaus (x 966 < 1000), Seite Textures (988), Seite Build ("リージョン枠のシェーダー", der Befund aus Stufe 3), Seite Brush rechts hinaus, Baum "サーフェスインスペクター" und "テクスチャブラウザー" |
| s4p0gruen5 (nachher) | 105 OK / 0 FEHLER (92 Seiten-Pruefungen + Surface Inspector) |

Ursache links: `labelBreite` mass mit Tahoma (DirectWrite, Qt-Regeln) -
Zeichen, die Tahoma nicht hat, misst DirectWrite mit seiner Ersatzschrift,
gezeichnet wird mit der CJK-Schrift des Programms (breiter). Jetzt gilt bei
Text mit Zeichen ab U+0800 das Groessere von beidem (Latein unveraendert,
also bleiben die gegen die NRC-Fotos gemessenen Lagen). Baum und
Kontrollkaestchen: zu lange Uebersetzungen gekuerzt (deutsch
"Flaecheninspektor" ueberall statt "Oberflaechen-Inspektor", Baum
"Autosicherung", "Ebenenpunkte ganzzahlig (kann Pinsel zerstoeren)";
japanisch "表面インスペクタ", "テクスチャ一覧", "モデルブラウザ", kuerzerer Text
fuer "Snap plane points"). Die englische Spalte (= NRC) ist unveraendert.

## Punkt 0d: Surface Inspector so breit wie das NRC-Foto

| Lauf | Innenraum | Texturing-Spalten (x/Breite) |
|---|---|---|
| NRC-Foto `dlg_View_Surface_Inspector_0.png` | 285 x 364 | 23/43, 72/44, 122/67, 195/67 |
| s4p0rot4 (vorher) | 292 x 364 | 23/43, 72/52, 130/67, 203/67 |
| s4p0gruen5 (nachher) | 285 x 364 | 23/43, 72/44, 122/67, 195/67 |

Unser Fenster war 7 Punkte (14 Bildpunkte bei Massstab 2) **breiter**, nicht
schmaler (die Notiz in Stufe 3 "12 Punkte schmaler" war verdreht; die Tabelle
dort zeigte 582 gegen 570 Bildpunkte). Ursache: wir rechneten die Knopfspalten
der Gruppe "Texturing" mit Text + 16. NRC zieht das Fenster auf 99 x 99
(BuildDialog, surfacedialog.cpp:637), also auf die Mindestgroesse des
QVBoxLayout; die Knoepfe haben `setMinimumWidth( 10 )` (surfacedialog.cpp:
762-830) und zaehlen dafuer nicht. Das Breiteste ist das obere Raster (285).
In der Gruppe verteilt QGridLayout 221 Punkte mit qGeomCalc zwischen Mindest-
(Beschriftung "Project:" 43, Knoepfe 10, Spinbox 67) und Wunschbreite
(QPushButton: Text + 8, wenigstens 80 - QCommonStyle/QFusionStyle
CT_PushButton). Nachgebaut mit `karte::geomCalc` (aus Stufe 1b).
