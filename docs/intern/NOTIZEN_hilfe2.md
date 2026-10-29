# Stufe H2 – mehr Touren, ein Helfer, der mehr versteht, ein KI-Chat mit Verlauf

Zweig `hilfe2` (Worktree `Y:\hilfe`, abgezweigt von `karte1` = rc606, 1274434d).

shank: „das ganze Programm hat Tutorials, ist leicht zu verstehen, man findet alles, es fühlt
sich interaktiv an; eine KI im Programm zeigt aktiv, wie man sein Ziel erreicht, und kann – nach
Bestätigung – für einen handeln.“ Sicherheit wie in H1 verbindlich (ab Werk aus, kleinste Rechte,
Bestätigung, Undo, Protokoll, keine Telemetrie); jede neue Fähigkeit hat einen Sicherheitstest.
Leitlinien wie in H1 (`docs/intern/NOTIZEN_hilfe1.md`, Diátaxis, NN/g, Plain Language).

## 1. Mehr interaktive Touren mit echten Aktionen

Neun neue Touren; jeder Schritt ist erst erledigt, wenn das Programm die Wirkung sieht
(`gui/tour_messwerte.cpp`, 29 neue Messwerte). Selbsttest `BHED_EDITORTEST=karteneditor
BHED_KARTE_NUR=hilfe2` (einzelne Teile mit `BHED_H2_NUR=mission,gangart,kamera,timen,wege,skins,
menue,tuer,update,spielordner,helfer,ki`): jede Tour mit echten Klicks/Tasten, nach jeder Aktion
muss die Tour selbst weitergeschaltet haben – und in jeder Tour mindestens eine Gegenprobe
(`wartetNoch`): ohne die Wirkung bleibt der Schritt eine halbe Sekunde lang offen (das Fenster
„Neue Mission“ nur öffnen, nur ein Element wählen, nur anhalten, nur zwei Entities wählen …).

| Tour (Seite) | Schritte | geprüft an | Selbsttest |
|---|---|---|---|
| `k-gangart` NPC laufen lassen | Figur wählen → Rechtsklick auf das Laufziel → Rennen → abspielen | vorschau.figurgewaehlt, vorschau.gangartmenue, aenderung+, vorschau.laeuft | Kyle + Laufziel gesetzt, Rechtsklick auf das hervorgehobene (selbst gezeichnete) Laufziel, „Rennen“, Abspielen |
| `k-wegpunkte` | Fenster → erzeugen → prüfen → von Hand setzen | vorschau.wegfenster, vorschau.wegpunkte+, vorschau.weggeprueft | auf taspir1, echtes Erzeugen |
| `k-neu-timen` (Skript in der Vorschau abspielen) | abspielen, Block, Dauer, Bildschritt, Zeile | vorschau.laeuft, aenderung+, vorschau.tlschritte+, auswahl | Abspielen und Pfeiltaste echt; Block/Dauer als Wertänderung (wie das Ziehen) |
| `tut-p1` Mission anlegen | Startseite → Neue Mission → ansehen → altes Intro löschen | vorschau.startseite, mission.name~, vorschau.ents- | Karte im Fenster suchen und wählen, Name tippen, Anlegen, t271 suchen, Entf; `BHED_MISSIONEN` = Laufordner (nie Spielordner) |
| `k-skins` | Haut → Flächenliste → Kappen → Skelett | modell.skin~, modell.flaechenliste+, modell.caps~, modell.skelett~ | Klick auf den (selbst gezeichneten) Flächenzähler; Skelett (Dateidialog) übersprungen |
| `k-spielordner` (neu) | Pfade → Ordner dazu → Liste → Einstellungen → Regelwerk | programm.pfade, programm.spielordner+, programm.einstellungen, programm.regelwerk~ | Menüs echt; der Ordnerdialog über die Funktion dahinter (leerer Ordner im Laufordner, danach entfernt) |
| `k-update` (neu) | suchen → Antwort → beim Start → Änderungen | update.fenster, update.ergebnis, update.auto~, offen:###changelog | aus dem Karten-Editor: erst Events (Vorbedingung), dann Help-Menü |
| `tut-karte-tuer` (neu, Karten-Editor) | Brush → func_door → angle → Trigger → verbinden → bauen | karte.pinsel+, karte.ent.func_door+, karte.schluessel~, karte.ent.trigger_multiple+, karte.verbindungen+ | Ziehen und Rechtsklick in der 2D-Ansicht, Strg+K; Bauen übersprungen (macht hilfe1) |
| `tut-menue` (neu, Menü-Editor) | Modus → aus dem Spiel → verschieben → Bedienen → Export | modus:4, menue.dokumente+, menue.text~, menue.bedienen, menue.exporte+ | Liste filtern und Menü wählen, Bedienen echt; Export über die Funktion dahinter in den Laufordner |

Tour-Grammatik (`include/bhed/tour.h`, geprüft in `tests/tourtest.cpp`):
- `zahl:<name>-` (kleiner als beim Beginn), `modus:4` (Menü-Editor, auch als Aktion).
- **Selbst gezeichnete Ziele** `gezeichnet:<name>`: die Oberfläche meldet ihr Rechteck jedes Bild
  (`tourZielMelden`) – die Kamerazeile der Zeitleiste (`zeitleiste.kamera`, H1 offen), das erste
  Laufziel, das Menü eines Laufziels, der Flächenzähler, der Knopf „NPC-Wegpunkte“ (heißt wie ein
  anderes Element), die Seiten der Einstellungen des Karten-Editors, die Zeitleiste. Gemeldet wird
  nur der sichtbare Teil (eine weggerollte Zeile ist kein Ziel).
- **Verkettete Vorbedingungen** `<p1> < <z1> && <p2> < <z2>`: gezeigt wird der Öffner der ersten
  Stufe, die nicht gilt (siehe Punkt 3).
- k-kamera-ansicht, Schritt 2: ohne gewählte Kamera zeigt die Tour jetzt auf die Kamerazeile.

Neue Handbuchseiten (4 Sprachen): `k-spielordner`, `k-update`, `tut-karte-tuer`, `tut-menue`
(Titelbilder aus vorhandenen Fotos; drei neue Bilder je Sprache aus dem Selbsttest
`BHED_EDITORTEST=handbuch BHED_HB_NUR=hilfe2`: die Tour mit ihrer Hervorhebung im Fenster Pfade,
am Menü Hilfe und im Menü-Editor). Die Tutorial-Übersicht und die Startseite nennen sie.

Gefunden und behoben (erst im Selbsttest rot, dann grün):
- **Auswahllisten waren für Touren und Helfer unsichtbar** (seit H1): eine ImGui-Combo meldet
  ihre Beschriftung nur aufgeklappt, „Zeig es mir“ für *Regelwerk* oder *Schwierigkeit* hob also
  nie etwas hervor (rot: „Feld gefunden 0“). Jetzt melden die beiden Combos der Einstellungen und
  die Hautliste des Modell-Tabs ihre Beschriftung (`elemente::merkeText`, wie schon die
  Reiterauswahl; rot auch im Selbsttest k-skins: „Klick Skin: nicht gefunden“).
- Die Kamerazeile einer niedrigen Zeitleiste liegt außerhalb des sichtbaren Bereichs: ein
  gemeldetes Ziel zählt nur noch mit seinem sichtbaren Teil (rot: Klick landete im Fenster darüber).
- `tut-karte-tuer` Schritt 2 zählt Brushes ab dem Beginn des Schritts – im Test wird die leere
  Karte darum vor dem Start angelegt (sonst zählte der Brush der alten Karte mit).
- `k-skins` Schritt 1 erledigte sich schon beim Laden einer Figur (sie setzt ihre Haut selbst;
  rot in der Gegenprobe der Schlussrunde). Jetzt zählt nur eine vom Anwender gewählte Haut
  (`modell.skin` = Wechsel über die Liste oder eine Datei), im Test ein echter Klick in die Liste.

## 2. Trefferquote bei frei formulierten Fragen

- **Beugungen** (`beugungen()`): deutsche Verbformen (rennt, läuft, gesetzt, laufende), Umlaut
  im Stamm (läuft → lauf), englische (entities, tried, making, running, stopped).
- **Tippfehler**: Damerau-Levenshtein 1 (ab 9 Buchstaben 2), gleicher Anfangsbuchstabe oder die
  ersten zwei vertauscht; im Fenster „Meintest du …?“ bei 0 Treffern.
- **Inhalte von Untermenüs und Dialogen** als Suchwörter (`Eintrag::inhalt`, schwach gewichtet,
  zählt kaum zur Längennorm): Kinder eines Menüs, die Felder der Seiten der KE-Einstellungen, der
  Inhalt des Fensters Pfade.
- **Modus als Gewicht** (`Index::frage(…, modus)`): Einträge des aktuellen Bereichs ×1,35, Orte
  des Karten-/Menü-Editors außerhalb davon ×0,8; Seiten bekommen einen Bereich.
- Vielfalt: eine Tour und ihre Handbuchseite sind EIN Thema, davon höchstens zwei Treffer (der
  zweite Platz gehört der Tour selbst) – so bleibt Platz für den Ort (Menüpunkt, Knopf).
- Pfad-Wörter schwächer (0,5): sonst unterlagen Eltern ihren Kindern („Einstellungen“ fand die
  Seiten statt des Dialogs). Neue Begriffsgruppe @patrouille.

Messung (`hilfetest`, Top 3):

| Liste | vorher (rc606) | H2 | de / en / zh / ja |
|---|---|---|---|
| **H2-Liste** (neu, 48 Fragen, 12 Ziele × 4 Sprachen, 20 mit Modus; eingefroren, `tests/hilfe_fragen_h2.inc`) | 38/48 = 79,2 % | **42/48 = 87,5 %** (Schranke ≥ 85 %) | 12 / 10 / 10 / 10 von 12 |
| H1-Hauptliste (99, abhängig) | 99/99 | 99/99 = 100 % | 26 / 25 / 24 / 24 |
| H1-Zweitliste (24, unverändert) | 18/24 = 75 % | 18/24 = 75 % | 5 / 4 / 4 / 5 von 6 |

Die sechs Fehltreffer der H2-Liste: zweimal Update-Frage ohne Kontext (die neue Tour k-update
und die NetRadiant-Update-Prüfung des Karten-Editors – dieselbe Funktion – stehen vor dem
Menüpunkt), zweimal „Hauptmenü bearbeiten“ (neue Tour tut-menue zweimal vorn, die Liste erwartet
die Seite oder den Knopf des Menü-Editors), „Einstellungen“ auf Chinesisch/Japanisch im
Karten-Editor (die Tour k-spielordner mit „设置/設定“ im Titel schlägt den KE-Dialog).
Eine Formulierung der neuen Seite k-spielordner („das Hauptmenü steht dort“ → „dort stehen die
Menüs Datei, Bearbeiten und Ansicht“) wurde nach einem Fehltreffer klarer gefasst (+1 Frage).

Ehrlich: Die H2-Liste wurde VOR dem Abstimmen geschrieben und eingefroren (Commit 01ce9eb2;
einzige Korrektur davor: Anker als `seite:x.html#anker`, Grundlinie damit 38/48 = 79,2 %), danach
nicht verändert. Abgestimmt wurde danach mit Blick auf ihre Fehltreffer – sie ist also nach dem
Abstimmen keine ganz unabhängige Messung mehr. Die neuen Touren kannte sie nicht: wo jetzt eine
neue Tour derselben Sache vorne steht (k-update, tut-menue, tut-karte-tuer, k-spielordner), zählt
das streng als daneben. Die H1-Zweitliste (24) blieb unverändert; von ihren 6 Fehltreffern werden
2 inzwischen von den neuen Touren derselben Seite beantwortet (k-gangart, tut-karte-tuer), die
die Liste nicht annimmt. In der (abhängigen) Hauptliste zählen die neuen Touren dort mit, wo sie
die richtige Antwort sind (8 Fragen, im Code markiert).

## 3. Ortskatalog erweitert, Vorbedingungen verkettet

`tools/gen_hilfe_orte.py` (6. Spalte `inhalt`): 166 statt 75 Orte aus dem Hauptprogramm –
42 Leistenknöpfe (Vorschau, Zeitleiste, Kamera mit Vorbedingung „Kamera gewählt“, Wegpunkte mit
„Fenster offen“, Modell), 24 Seiten der Einstellungen des Karten-Editors (Ziel: die Seite,
selbst gemeldet; ihre Felder als Inhalt), 26 Elemente des Menü-Editors (Speichern/Export erst
mit offenem Menü), die Knöpfe des Fensters Pfade. 55 Orte haben verkettete Vorbedingungen.
Eine Einstellung des Hauptprogramms, gesucht im Karten-Editor: erst **Events**, dann
**Bearbeiten > Einstellungen**, dann das Feld (Selbsttest: Stufe 1 → 2 → 3 mit echten Klicks).
`hilfetest` prüft Zahlen, Ketten und dass jede Kennung im Inhalt einen Text hat.

## 4. KI-Chat

- **Gestreamt** (Server-Sent Events, `ki::StromLeser`): Text erscheint Stück für Stück,
  Werkzeugaufrufe aus `input_json_delta`, thinking-Blöcke samt Signatur unverändert zurück;
  abgeschnittene Werkzeugeingaben gehen als `INVALID_JSON`-Fehler zurück (eager_input_streaming).
- **Verlauf** (`ki::chatStrom`): Nachrichten wachsen nur hinten; fester Systemtext, die
  Handbuch-Ausschnitte stehen als markierte Daten in der Frage (sonst gälten die thinking-Blöcke
  früherer Antworten nicht mehr – preserved thinking, Konten ab 31.08.2026 bekämen einen 400).
  Gespeichert DPAPI-verschlüsselt (`ki_verlauf.bin`), „Neues Gespräch“ löscht ihn; höchstens 60
  Nachrichten, dann ein neues Gespräch (kürzen hieße die Geschichte bearbeiten).
- **Abbrechen**: „Stopp“ bricht mitten im Empfang ab (WinHTTP-Handle zu); die Frage samt ihrer
  Runden wird hinten wieder entfernt – davor bleibt alles Byte für Byte gleich.
- **Schlüssel nullen**: die Kopfzeilen werden ohne Zwischenkopien gebaut (reservierter Puffer) und
  nach `WinHttpSendRequest` genullt; der Header-Vektor und der Schlüssel nach jeder Anfrage
  (`ki::nullen`, SecureZeroMemory). Bleibt: die Kopien in WinHTTP selbst (nicht in unserer Hand).
- **API-Fassung 2026-10-21** (die Anfragen senden weder temperature/top_p/top_k noch Dokumente in
  Antworten oder Zitate in Fragen; Doku gelesen: Migration 2026-10-21).
- Den Weg ohne Streaming (`ki::chat`, `Senden`, `systemText`) gibt es nicht mehr; die
  Prompt-Injection- und WinHTTP-Proben aus H1 laufen über `chatStrom` (`alsStrom` macht aus einer
  ganzen Antwort einen Strom).
- **Menü-Editor-Befehle** für `run_commands`: `menu:select:<n>`, `add:<n>`, `move:<dx>:<dy>`,
  `duplicate`, `delete`, `interact:on|off`, `insert:<typ>` – nie Öffnen/Speichern/Export (kein
  Pfad als Eingabe). Rückfrage in Worten („Menü-Editor: die Auswahl um 16, 0 verschieben“), ein
  Auftrag = ein Rückgängig-Schritt (`menueeditor::zuEinemSchritt`). Tour starten, Handbuch zeigen
  und hervorheben gab es seit H1 (start_tour, open_manual, show).
- Tests (`kitest`, alle grün): SSE in jeder Zerteilung (1–41 Bytes), thinking/Signatur,
  Werkzeug-JSON zusammengesetzt und abgeschnitten, error-Ereignis, ohne message_stop kein
  Ergebnis, 3000 Zufallsmutationen; Verlauf append-only mit festem Systemtext über zwei Fragen;
  Abbrechen und HTTP 529 lassen den Verlauf unverändert; Menü-Befehle (9 erlaubt, 10 abgewiesen);
  Prompt-Injection über den Text eines Menüs (Modell gehorcht: Export gibt es nicht, Löschen
  braucht Zustimmung – 0 ausgeführt, mit Zustimmung nur select/delete); Header genullt; gestreamt
  über WinHTTP gegen den lokalen Fake-Server (Fassung 2026-10-21, Schlüssel nie im Körper).
  Fuzzziel: erstes Byte ≥ 0xF0, Fall 3 = StromLeser. Selbsttest: Menü-Befehle über MCP –
  Ablehnen ändert nichts, Ausführen ändert, EIN Rückgängig nimmt den ganzen Auftrag zurück.

## 5. Leicht verständlich: Tooltips und leere Zustände

Leere Zustände mit dem nächsten Schritt und einem Knopf „Zeig mir, wie“ (startet die Tour):
Skriptbaum ohne Befehle (vorher „Kein Skript geladen“ auch bei einem leeren neuen Skript),
Modell-Tab ohne Modell (vorher „File → Browse .pk3 → Models“, den Weg gibt es so nicht mehr),
Menü-Editor ohne Menü; NPC-Wegpunkte ohne Karte sagen, warum „Erzeugen“ grau ist.
Vorhanden und geprüft: Startseite der Vorschau, Zeitleiste ohne Kamera (mit Knopf), Graph/Spuren
leer, Helfer leer, Mission keine, Texturen ohne Spielordner, Rückgängig-Liste leer, KI-Protokoll leer.

Tooltips: Prüfskript über alle `Button/SmallButton/Checkbox/MenuItem/RadioButton(tr(...))` und
Leistenknöpfe (Heuristik: kein SetTooltip in den nächsten 4 Zeilen): vorher 223 von 311 ohne,
nachher 204 von 314. Ergänzt (26): Menü-Editor-Leiste (8), KI (Ausführen, Ablehnen, Trennen,
Einschalten, Schlüssel speichern, lokale Verbindung), Graph (4), Update (Installieren, Erneut),
Bearbeiten > Pfade. Der Rest ist überwiegend selbsterklärend (Ok/Abbrechen/Ja/Nein,
Kopieren/Einfügen, Rückgängig) oder hat Erklärungen im Hinweistext daneben. Offen (Liste):
app.cpp/app_anordnung.inc Menüpunkte der BehavEd-Menüs (Act*, Debug*), Kamera-Knöpfe (CamDeleteShot,
CamFlyToShot, CamNext/PrevShot), Mission (MissionChoose, MissionSaveNow, EntPickNpc, EntAddKey),
Gizmo-Knöpfe, Menü-Editor Suchen/Ersetzen/Gruppieren.

## 6. Handbuch, Tests, Schlussrunde

Handbuch: vier neue Seiten, Seite „Der Helfer und die Touren“ ergänzt (Schneller finden,
Nachfragen/Stopp/Neues Gespräch, Menü-Editor-Befehle der KI, Verlauf in „Ist das sicher?“ –
in `inhalt_hilfe2.py`, damit `inhalt_hilfe1.py` unverändert bleibt), Tutorial-Übersicht und
Startseite, drei neue Bilder je Sprache.

| Prüfung | Ergebnis |
|---|---|
| Bau Release, sauber neu (`--clean-first`, alle Ziele, /W4 /WX) | 0 Warnungen; danach eine Korrektur (k-skins) inkrementell, 0 Warnungen |
| ctest gesamt | 157/158 im ersten Lauf (`handbuch_sprachen`: die drei neuen Bilder fehlten noch), nach den Bildern und der letzten Korrektur 158/158 |
| tourtest / hilfetest / kitest | grün (H2-Liste 42/48 = 87,5 %, Hauptliste 99/99, Zweitliste 18/24) |
| lint_sources, lint_fassung, lint_headers, lint_gapp, lint_feldinit, lint_imgui_context | ok |
| lint_portability | 192 = Stand rc606 (keine neuen) |
| lint_i18n | 706 statt 705: zwei neue Texte werden nur in `.inc`-Dateien benutzt (der Lint liest nur `.cpp`), ein alter (ViewMenuEditor) ist jetzt benutzt |
| Handbuchbilder `BHED_EDITORTEST=handbuch BHED_HB_NUR=hilfe2`, 4 Sprachen | je 3 Bilder, 0 Fehler |
| Selbsttest `karteneditor` + `BHED_KARTE_NUR=hilfe2` | 44 OK, 0 FEHLER (nach der k-skins-Korrektur; davor 44 OK, 1 FEHLER: die Gegenprobe fand den Fehler) |
| Selbsttest `karteneditor` + `BHED_KARTE_NUR=hilfe1` | 59 OK, 0 FEHLER (wie H1; bhed_q3map2 über BHED_Q3MAP2 aus dem eigenen Bauordner) |
| Selbsttest `karteneditor` + `BHED_KARTE_NUR=fotos` (kurz) | 78 OK, 0 FEHLER |
| Selbsttest `menueeditor` (Leiste, Tooltips, leerer Zustand geändert) | 25 OK, 0 FEHLER |
| Selbsttest `1` | 2645 OK, 0 FEHLER |

## 7. Offen / Ideen für H3
- Eigene Bilder für die Schritte der vier neuen Seiten (Selbsttest-Fotos).
- Ziehen eines Blocks in der Zeitleiste im Selbsttest als echte Mausbewegung (heute als
  Wertänderung, wie es das Ziehen bewirkt).
- Gezeichnete Ziele außerhalb des sichtbaren Bereichs automatisch hineinrollen.
- Tooltips der übrigen Bedienelemente (Liste oben).
- Restrisiko: Kopien des API-Schlüssels innerhalb von WinHTTP.

## English
- **Nine more interactive tutorials that wait for your real actions:** make a figure run instead of walk, give NPCs paths, play and retime a cutscene in the timeline, create a mission, switch skins, set up the game folder and preferences, check for updates, a door with a trigger in the map editor, and change a game menu in the menu editor. Self-drawn things like the camera row of the timeline, a walk target on the map or a page of the map editor's preferences can now be highlighted too.
- **The helper understands more:** other word forms ("runs", "läuft") and small typos ("Did you mean ...?"), the contents of submenus and dialogs, and the area you work in - in the map editor its own results come first. It now knows the toolbar buttons, the pages of the map editor's preferences and the menu editor.
- **"Show me" step by step:** if a control is hidden, the helper first points at what opens it - for example from the map editor: first Events, then Edit > Preferences, then the field.
- **AI chat with follow-up questions:** the answer appears while it is written, Stop cancels at once, the conversation is kept encrypted on your PC only and "New conversation" deletes it. In the menu editor the AI can select, move, duplicate or delete elements - only after you confirm, one undo step, never opening, saving or exporting files. The API key is wiped from memory after each request.
- Clearer empty areas with a "Show me how" button (empty script, model tab, menu editor, waypoints without a map) and 23 new tooltips (menu editor, AI, graph, updates, Edit > Paths).

## Deutsch
- **Neun weitere interaktive Touren, die auf deine echten Aktionen warten:** eine Figur rennen statt gehen lassen, NPCs Wege geben, eine Zwischensequenz in der Zeitleiste abspielen und neu timen, eine Mission anlegen, Skins wechseln, Spielordner und Einstellungen einrichten, nach Updates suchen, eine Tür mit Trigger im Karten-Editor und ein Menü des Spiels im Menü-Editor ändern. Auch selbst gezeichnete Dinge wie die Kamerazeile der Zeitleiste, ein Laufziel auf der Karte oder eine Seite der Einstellungen des Karten-Editors lassen sich jetzt hervorheben.
- **Der Helfer versteht mehr:** andere Wortformen („rennt“, „läuft“, „running“) und kleine Tippfehler („Meintest du …?“), die Inhalte von Untermenüs und Dialogen und den Bereich, in dem du arbeitest – im Karten-Editor stehen dessen Treffer oben. Er kennt jetzt die Knöpfe der Leisten, die Seiten der Einstellungen des Karten-Editors und den Menü-Editor.
- **„Zeig es mir“ Schritt für Schritt:** Ist ein Bedienelement verborgen, zeigt der Helfer zuerst auf das, was es öffnet – etwa aus dem Karten-Editor: erst Events, dann Bearbeiten > Einstellungen, dann das Feld.
- **KI-Chat mit Nachfragen:** Die Antwort erscheint, während sie geschrieben wird, Stopp bricht sofort ab, das Gespräch bleibt verschlüsselt nur auf deinem PC und „Neues Gespräch“ löscht es. Im Menü-Editor kann die KI Elemente wählen, verschieben, verdoppeln oder löschen – nur nach deiner Bestätigung, als ein Rückgängig-Schritt, nie Dateien öffnen, speichern oder exportieren. Der API-Schlüssel wird nach jeder Anfrage aus dem Speicher gelöscht.
- Verständlichere leere Bereiche mit einem Knopf „Zeig mir, wie“ (leeres Skript, Modell-Tab, Menü-Editor, Wegpunkte ohne Karte) und 23 neue Tooltips (Menü-Editor, KI, Graph, Updates, Bearbeiten > Pfade).

## 中文
- **又新增九个会等待你真正操作的交互式教程:** 让角色跑而不是走、为 NPC 设置路线、在时间轴中播放并重新调整过场动画的时间、新建任务、更换皮肤、设置游戏文件夹和首选项、检查更新、在地图编辑器中制作带触发器的门,以及在菜单编辑器中修改游戏菜单。时间轴的镜头行、地图上的行走目标或地图编辑器首选项的某一页等自绘元素现在也可以高亮显示。
- **助手理解得更多:** 不同的词形和小的拼写错误(“你是不是要找……?”)、子菜单和对话框中的内容,以及你当前工作的区域——在地图编辑器中,它自己的结果排在前面。它现在还认识工具栏按钮、地图编辑器首选项的各个页面以及菜单编辑器。
- **一步一步“演示给我看”:** 如果某个控件被隐藏,助手会先指向打开它的地方——例如在地图编辑器中:先是 Events,然后是“编辑 > 首选项”,最后是该字段。
- **可以追问的 AI 聊天:** 回答会边写边显示,“停止”会立即取消,对话只加密保存在你的电脑上,“新对话”会删除它。在菜单编辑器中,AI 可以选择、移动、复制或删除元素——只有在你确认后才执行,作为一个撤销步骤,绝不打开、保存或导出文件。每次请求后都会从内存中清除 API 密钥。
- 更易懂的空白区域,带有“演示给我看”按钮(空脚本、模型标签页、菜单编辑器、没有地图时的路径点),以及 23 条新的工具提示(菜单编辑器、AI、图、更新、编辑 > 路径)。

## 日本語
- **実際の操作を待つインタラクティブ チュートリアルがさらに 9 つ:** キャラクターを歩かせずに走らせる、NPC に経路を与える、タイムラインでカットシーンを再生してタイミングを調整する、ミッションを作る、スキンを切り替える、ゲームフォルダーと設定を整える、アップデートを確認する、マップエディターでトリガー付きのドアを作る、メニューエディターでゲームのメニューを変える。タイムラインのカメラ行、マップ上の歩く目標、マップエディターの設定のページなど、独自に描かれた要素も強調表示できるようになりました。
- **ヘルパーがより多くを理解:** 語形の違いや小さな打ち間違い(「もしかして…?」)、サブメニューやダイアログの中身、作業中の領域を考慮します。マップエディターではその結果が上に来ます。ツールバーのボタン、マップエディターの設定の各ページ、メニューエディターも知っています。
- **「やって見せて」を一歩ずつ:** 操作部分が隠れているときは、まずそれを開く場所を示します。例えばマップエディターからなら、まず Events、次に 編集 > 設定、最後にその項目です。
- **続けて質問できる AI チャット:** 回答は書かれながら表示され、停止ですぐに中止できます。会話はあなたの PC にだけ暗号化して保存され、「新しい会話」で削除されます。メニューエディターでは AI が要素の選択・移動・複製・削除を行えます(確認後のみ、1 つの元に戻す手順として、ファイルを開く・保存する・エクスポートすることはありません)。API キーはリクエストのたびにメモリーから消去されます。
- 「やり方を見る」ボタン付きの分かりやすい空の画面(空のスクリプト、モデル タブ、メニューエディター、マップなしのウェイポイント)と、23 個の新しいツールチップ(メニューエディター、AI、グラフ、アップデート、編集 > パス)。
