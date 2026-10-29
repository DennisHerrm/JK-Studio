# Notizen vis1: Kompilieren so schnell wie möglich – zuerst VIS, bitgleich

Zweig `vis1` (Worktree `Y:\vis`), abgezweigt von karte9d (744c1e7a). Bau `Y:/vis/build-vis`
(Konfiguration wie karte8d, `-DBEHAVED_Q3MAP2_LLVM=Y:/werkzeuge/llvm-23.1.4/bin`). Messwerkzeuge im
Scratchpad `SP/vis1`: `vl.py` (ein Lauf bzw. abwechselnd A/B, Idle, 8-GB-Wächter, Wand/CPU/Speicher,
SHA-256 des Visibility-Lumps und aller Lumps ohne den Kopf mit NRCs Zeitstempel), `prep.py`
(Kartensatz: `-meta` je Karte), `ex.py` (Extreme Quality je Schritt), zeitweise Messfassungen
`stat_patch.py`, `prof_patch.py`, `sim_patch.py`, `sim2_patch.py` (nie eingecheckt). Rohdaten:
`SP/vis1/messung.txt`, `SP/vis1/laeufe/<karte>/<lauf>/`.

Maschine: i9-14900HX (8 P-Kerne mit HT + 16 E-Kerne = 32 Threads), 64 GB. Die anderen Agenten
bauten nebenher – alle Zeiten „unter Last“, A/B immer abwechselnd, Streuung ±10 %.

Kartensatz (`-meta` mit bhed_q3map2, `SP/vis1/karten`): ctf3 (2755 Portale), mb2_manaan (5051),
t3_byss (7284), taspir1 (12342), kejim_post (JK2, 23552), md_ch_ben (MD, 1759), duel_mos_espa (MD, 6756),
md_fh1_jedi (MD, 3477), md_rev_TarisUpper2 (MD, 38042).

## Commits

| Commit | Inhalt |
|---|---|
| 20c63bdc | Flussvektoren dünn, Passagen kompakt, `-bhedvisexact`, Fortschritt nach Arbeit, Thread-Tafel als Vektor, ctest q3map2_visgleich, q3map2_determinismus mit mehreren Threads |
| c733c29f | Wortmasken für Passagen und Rahmen (nur Wörter, die in beiden ≠ 0 sind) |
| 0ec977af | Editor: Speicherschätzung vor dem vollen VIS, Text KeVisLang, Handbuch (4 Sprachen) |
| 21d49493 | ctest q3map2_visgleich auch mit `-threads 100`, Handbuch-Zahlen |
| b36d2514 | Grenzen-Tafel: Prüfstelle vis.cpp:742 (ctest karte_grenzen) |
| (dieser) | Notizen vis1 |

## Messtabelle (Endstand c733c29f, `SP/vis1/ende.sh`)

32 Threads, NRC-Regel (Vorgabe), alt = 744c1e7a (NRCs Fluss-Code), A/B abwechselnd, Mittel der Läufe,
unter Last (die anderen Agenten bauten). „exakt“ = `-bhedvisexact`, Bytevergleich aller Lumps gegen den
alten Stand mit `-threads 1` (= NRCs Rechnung mit einem Thread).

| Karte | Portale | Wand alt → neu | CPU alt → neu | Spitze MB alt → neu | exakt Wand | exakt = alt `-threads 1` |
|---|---|---|---|---|---|---|
| ctf3 | 2755 | 0,94 → 0,81 s (3 × 2) | 20,4 → 16,9 s | 93 → 62 | 1,5 s | ja (2c16bd92…) |
| md_ch_ben | 1759 | 2,79 → 2,48 s (3 × 2) | 64,7 → 59,9 s | 69 → 46 | 3,4 s | ja (fea78443…) |
| md_fh1_jedi | 3477 | 9,9 → 9,2 s (2 × 2) | 260 → 219 s | 135 → 98 | 9,2 s | ja (2c66f979…) |
| mb2_manaan | 5051 | 14,4 → 10,6 s (3 × 2) | 370 → 280 s | 205 → 129 | 14,0 s | ja (fa8ccd37…) |
| t3_byss | 7284 | 17,5 → 12,2 s (2 × 2) | 483 → 331 s | 358 → 155 | 30,4 s | ja (1c339af0…) |
| taspir1 | 12342 | 29,4 → 14,0 s (2 × 2) | 457 → 231 s | 981 → 310 | 26,8 s | ja (85d27f7f…) |
| duel_mos_espa | 6756 | 139,6 → 90,6 s (1 × 2) | 2037 → 1296 s | 386 → 201 | 296–417 s | ja (e1ea5af8…; alt `-threads 1`: 1675 s) |
| kejim_post (JK2) | 23552 | 550,5 → 214,1 s (1 × 2) | 12332 → 4324 s | 3527 → 927 | 550,7 s | f500465d… (alt `-threads 1` brauchte über 60 min, siehe unten) |
| md_rev_TarisUpper2 | 38042 | alt: > 8 GB bei 65 % CreatePassages; neu: BasePortalVis 11 s, CreatePassages 11 s, Fluss nach 15 min < 1/40 | | > 8 GB → 3172 | – | – |

Mit `-threads 1` gibt der neue Stand auf allen Karten dieselben Bytes wie der alte (ctf3, md_ch_ben,
md_fh1_jedi geprüft; alle anderen über „exakt = alt -threads 1“). md_ch_ben `-threads 1` A/B (3 × 2):
alt 32,5 s, neu (dünn) 32,0 s, mit Masken 5 % schneller – auf kleinen Karten bestimmt ClipToSeperators die Zeit.

## 1. Profil: wohin die Zeit geht

Messfassung `stat_patch.py` (rdtsc je Abschnitt, je Portal Zeit/Aufrufe/gelesene Vorgänger), NRCs Code:

| Karte | Bitfeld-Schleifen | ClipToSeperators + Cache | Rest | might je Rahmen |
|---|---|---|---|---|
| md_ch_ben, 1 Thread | 24 % | 59 % | 17 % | 8 von 55 Wörtern ≠ 0 |
| duel_mos_espa, 32 Threads | 43 % | 38 % | 19 % | 11,5 von 212 Wörtern ≠ 0 |

- Die Bitfeld-Schleifen rechnen jedes Wort (NRC: `long`, unter Windows 32 Bit) – auch die vielen
  Nullwörter – und kopieren den ganzen Vektor je Portal je Blatt (`memcpy`).
- Ein cansee hat im Mittel 30 von 212 Wörtern ≠ 0 (mos_espa), 21 von 55 (md_ch_ben).
- Schwanz mos_espa (32 Threads, NRC-Regel): das teuerste Portal 97 s, die fünf teuersten 11 % der
  ganzen Flusszeit; die letzten 10 % der Portale (sortiert) 29 % der Zeit. Mit 32 Threads 1,9-mal so
  viele Takte wie mit einem (HT/E-Kerne und weniger Abkürzung durch fertige Portale).
- Nach der dünnen Fassung (`prof_patch.py`): md_ch_ben 1 Thread Bitfeld 20 %, ClipToSeperators 57 %,
  Cache-Chops 9 %; mos_espa 32 Threads 30 % / 48 % / 13 %. ClipToSeperators läuft fast einmal je
  Rahmen und Flip (97,6 Mio. Aufrufe auf md_ch_ben bei 54 Mio. Rahmen) – meist erreicht nur ein
  Nachbar die Clip-Stelle.
- BasePortalVis und CreatePassages: auf allen Karten unter 1 s (Taris je 11 s gegen Stunden Fluss).

## 2. Umgesetzt (jeweils bytegleich geprüft)

### 2a. Dünne Flussvektoren (visflow.cpp `BhedFrameBuffers`, `BhedClearMight`, `BhedSetMight`)

Der might-Vektor jeder Rekursionstiefe liegt in einem Puffer je Thread und Tiefe, einmal genullt und
außerhalb seiner Wortliste immer null (pstack_t: `mightsee`, `mightwords`, `mightmask`). Ein Schritt
rechnet nur die Wörter, die im vorigen Vektor UND in der Passage ≠ 0 sind (Masken UND-verknüpft,
Rang je Maskenwort für das Bitwort der Passage). Gleiche Bits: `might[w] = prev[w] & cansee[w] & pv[w]`
ist für die übrigen Wörter ohnehin 0, `more` ebenso. 64-Bit-Wörter statt `long` – dieselben Bits.
Der Rahmen auf dem C-Stapel ist 16 KB kleiner (NRC: `byte mightsee[MAX_PORTALS/8]` je Tiefe).

### 2b. Passagen kompakt (CreatePassages)

Eine Passage behält nur die Wörter von cansee, die nicht null sind (Maske je 64 Wörter, Rang, Bits),
alle Passagen eines Portals in einem Block; portalfront wird nach BasePortalVis freigegeben;
CreatePassages geht die gesetzten Bits beider Flood-Vektoren durch statt jedes Portal der Karte zu
prüfen (dieselben Portale in derselben Reihenfolge).

| Karte | Passagen voll | Passagen kompakt | VIS-Speicher jetzt (gedruckt) | Spitze Prozess alt → neu (32 Threads) |
|---|---|---|---|---|
| md_ch_ben | 8 MB | 4 MB | 7 MB | 69 → 46 MB |
| ctf3 | 21 MB | 6 MB | 13 MB | 93 → 62 MB |
| md_fh1_jedi | 33 MB | 14 MB | 26 MB | 135 → 98 MB |
| mb2_manaan | 66 MB | 20 MB | 45 MB | 205 → 129 MB |
| duel_mos_espa | 178 MB | 34 MB | 77 MB | 386 → 201 MB |
| t3_byss | | | | 358 → 155 MB |
| taspir1 | | | | 981 → 310 MB |
| kejim_post | | | | 3527 → 927 MB |
| md_rev_TarisUpper2 | 9053 MB | 1422 MB | 2802 MB (NRC-Rechnung 9d: 11116 MB) | > 8 GB (Abbruch bei 65 % CreatePassages) → 3172 MB |

Taris: BasePortalVis 11 s, CreatePassages 11 s (vorher in 30 s bis 65 %, dann 8-GB-Wächter).
PassagePortalFlow lief 15 min mit 32 Threads (22473 CPU-s) ohne das erste Vierzigstel der Arbeit –
auf 32 Threads geschätzt viele Stunden bis Tage (siehe 9d), jetzt aber in 3,2 GB statt über 11 GB.
`bhed_q3map2` druckt nach CreatePassages „BehavEd: passages take X MB, vis memory Y MB“; vorher
„… MB passage memory at most (full bit vectors)“. Der Editor-Hinweis (`visBedarfAusPrt`) schätzt
jetzt Vektoren (2 je Portalseite) + Passagen mit einem Drittel der vollen Vektoren (gemessen 16–30 %
auf Karten über 50 MB) – Taris: 4,5 GB statt 10,9 GB.

### 2c. Fortschritt nach Arbeit (Punkt 8)

Der Pacifier von PassagePortalFlow (und PortalFlow/PassageFlow) zählt die fertige Arbeit
(nummightsee² je Portal) statt der verteilten Portale – NRCs Text (`0...1...2 … 9... (s)`), 40 Zeichen.
Größter Abstand zwischen Pacifier und echtem Arbeitsanteil (`fit.py` auf den Messdaten):

| Karte | nach Portalen (NRC) | m¹ | m^1,5 | **m²** | m^2,5 | m³ |
|---|---|---|---|---|---|---|
| md_ch_ben | 47 % | 26 % | 17 % | **9 %** | 11 % | 17 % |
| mb2_manaan | 42 % | 13 % | 4 % | **10 %** | 17 % | 22 % |
| duel_mos_espa | 50 % | 21 % | 11 % | **5 %** | 9 % | 14 % |

Die Restzeit des Fortschrittsfensters beim ersten Bau (Hochrechnung aus dem Pacifier) ist damit
keine bloße Untergrenze mehr. Der Text KeVisLang und das Handbuch sagen das.

### 2d. Deterministisch: `-bhedvisexact` (Punkt 3)

Ein Fluss liest die Vektoren der Portale, die er passiert: portalvis eines fertigen, portalflood
eines unfertigen. Mit einem Thread sind genau die Portale vor ihm (sortierte Reihenfolge) fertig.
`-bhedvisexact`: jeder Fluss liest portalvis genau der Portale vor ihm (wartet auf laufende) und
portalflood aller danach, auch fertiger – das Ergebnis von `-threads 1`, Byte für Byte, auch der
Entity-Lump (der Schalter wird wie `-bhedvisreuse` vor argsToInject genommen).
- Warten: Streifen von 64 Bedingungsvariablen; wer wartet, gibt seinen „Zug“ ab: 4 × numthreads
  Threads, höchstens numthreads rechnen, ein freier Zug geht an den wartenden Fluss mit dem
  kleinsten Rang (mb2_manaan: 2/4/8/16-fach 21/19/19/30 s).
- Ohne Warten, wenn es nicht nötig ist: braucht der Fluss von einem laufenden Vorgänger p nur Bits,
  die p schon gefunden hat (Abfrage ∩ portalflood ⊆ bisheriges portalvis), ist das Ergebnis dasselbe
  (portalvis ⊆ portalflood) – md_ch_ben: 33353 Fälle ohne Warten, 6078 mit.
- Messung (32 Threads, unter Last):

| Karte | NRC-Regel neu | `-bhedvisexact` neu | alt (NRC-Code, NRC-Regel) |
|---|---|---|---|
| ctf3 | 0,8 s | 1,5 s | 0,9 s |
| md_ch_ben | 2,5 s | 3,4 s | 2,8 s |
| md_fh1_jedi | 9,2 s | 9,2 s | 9,9 s |
| mb2_manaan | 10,6 s | 14,0 s | 14,4 s |
| t3_byss | 12,2 s | 30,4 s | 17,5 s |
| taspir1 | 14,0 s | 26,8 s | 29,4 s |
| duel_mos_espa | 90,6 s | 296–417 s | 139,6 s |

  (Endmessung, siehe Messtabelle.) Gegen den alten Stand ist `-bhedvisexact` auf md_fh1_jedi, mb2_manaan
  und taspir1 gleich schnell oder schneller, auf ctf3/md_ch_ben etwas, auf t3_byss und mos_espa deutlich
  langsamer.

  Auf offenen Karten mit langem Schwanz (mos_espa) warten die teuren letzten Portale aufeinander –
  die CPU-Zeit ist kleiner (1067–1352 s statt 1296–1556 s, beste Abkürzung), aber im Mittel rechnen nur
  3–4 Kerne. **Nicht ohne Verlust → Vorgabe bleibt NRCs Regel; `-bhedvisexact` ist ein Schalter. shank
  entscheidet die Vorgabe.**

## 3. Verworfen (mit Zahlen)

- **Spekulation mit flood** (Stufe 7) und **mit dem Zwischenstand des Vorgängers** (`sim2_patch.py`):
  nimmt der Fluss beim Lesen eines unfertigen Vorgängers dessen bisheriges portalvis und prüft am
  Ende, ob der Vorgänger danach noch ein angenommenes Bit fand – auf md_ch_ben wäre das in 61 von
  6078 Fällen gutgegangen (1 %), auf mb2_manaan in 261 von 11743. Wartezeit gesamt 267 s bzw. 1554 s
  (über alle Threads). Nicht lohnend.
- **Einen teuren Fluss auf mehrere Threads teilen** (Punkt 2, Schwanz): das Ergebnis eines Portals
  ist keine reine Bit-Vereinigung – der Separator-Cache eines Rahmens (SEPERATORCACHE) bekommt seinen
  Inhalt vom ersten Nachbarn, der die Clip-Stelle erreicht; wird dieser beim Setzen weggeschnitten,
  bleibt der Cache ein Präfix, und ob ein Nachbar überhaupt hinkommt, hängt über „kann nichts Neues
  sehen“ vom bisherigen portalvis ab, also von der DFS-Reihenfolge. Was-wäre-wenn (`sim_patch.py`,
  md_ch_ben): Teilbäume ab Tiefe 1 – der größte trägt 99 % der Arbeit; ab Tiefe 2/3 lägen 74 %/88 %
  der Arbeit in Teilbäumen, deren Entscheidungen ohne die Marken der früheren Teilbäume anders ausfallen
  (kritische Entscheidungen 1,4 je Rahmen, davon 25 % „überspringen“). Ein Ergebnis, das NRC nie
  liefern kann – nicht erlaubt.
- **Vorab-Test der Separator-Kandidaten** (vor dem Normieren, mit Fehlerschranke): bytegleich, aber
  20 % (ctf3) bis 40 % (mb2_manaan) langsamer – die meisten Kandidaten sind gültig, der Test kommt
  dazu. Nicht behalten.
- **GPU für BasePortalVis/CreatePassages** (Punkt 6): beide Phasen unter 1 s auf allen Karten, auf
  Taris je 11 s gegen Stunden PassagePortalFlow – mehr als 1 % kann die GPU nicht bringen; der Fluss
  selbst ist rekursiv mit Separator-Cache (siehe oben) und nicht als Compute-Shader zu rechnen.
  Kein Versuch.
- **AVX2 für die Bitfeld-Schleifen** (Punkt 4): nach 2a rechnet ein Schritt im Mittel nur die rund
  10 Wörter, die im vorigen Vektor und in der Passage ≠ 0 sind (statt 212 bzw. 1189 auf Taris) – breite
  Schleifen über alle Wörter wären langsamer. Nicht gemacht.
- **Arbeit-Stehlen/teure Portale zuerst** in der NRC-Regel: ändert nur, welches der gültigen
  NRC-Ergebnisse herauskommt, nicht den Schwanz (ein Portal = ein Thread); mit `-bhedvisexact`
  verlängert es die Warteketten.

## 4. Extreme Quality (Punkt 7)

duel_mos_espa, „[Final] Extreme Quality“ (`ex.py`, 32 Threads, unter Last):

| Schritt | Wand | CPU |
|---|---|---|
| `-meta` | 29 s (davon FixTJunctions 6 s) | 133 s |
| `-vis` | 96 s | 1290 s |
| `-light -fastbounce -patchshadows -samples 4 -randomsamples -bounce 8` | erster Durchgang: Laden 29 s, SetupTraceNodes 12 s, IlluminateRawLightmap 67 s, IlluminateVertexes 9 s; je Bounce: SetupEnvelopes 10–12 s, IlluminateRawLightmap 533 s, IlluminateVertexes 60 s → 8 Bounces etwa 80 min (nach Bounce 2 beendet) | in der Mitte der Phase 25,6 Kerne |

- **Licht ist die Engstelle** (etwa 95 % des Baus), VIS jetzt 2 %, BSP unter 1 %.
- **GPU-Backen (Auto) deckt diese Optionen nicht ab**: light_ydnar.cpp `BhedIlluminateRawLightmaps` nimmt
  die GPU nur ohne `-randomsamples` (und ohne deluxemap/Debug-Füllungen); das Teilen großer Lightmaps auf
  mehrere Kerne (Stufe 7) ebenso nur ohne `-randomsamples` – mit Extreme Quality rechnet jede Lightmap auf
  einem Kern, die großen am Ende der Phase allein. `RandomSubsampleRawLuxel` zieht je nachgerechnetem Luxel
  2 × samples Zufallszahlen aus der Folge des Arbeitsschritts; ein bitgleicher paralleler oder GPU-Weg muss
  diese Folge je Luxel reproduzieren (siehe Offen). In dieser Runde nicht erweitert.
- `-bsp`: in Stufe 7 bereits parallel (FixTJunctions); kein neuer Engpass gefunden.

## 5. Tests

- ctest **q3map2_visgleich** (neu): `tools/karte/q3map2_visgleich.py` erzeugt ein Gitter aus 9 × 9
  Räumen (391 Cluster, 619 Portale), `-meta`, dann `-vis`, `-vis -nopassage`, `-vis -passageOnly` mit
  `-threads 1` und mit `-bhedvisexact` und 8/32 Threads: alle Lumps gleich dem 1-Thread-Lauf, der
  Visibility-Lump gleich NRCs q3map2.exe `-threads 1` (a1a1f1d839d4599f, 388b016abb0a4fdb,
  f8f2997bcdc35b36). Rot mit dem alten bhed_q3map2 (5 FEHLER: Lump 16 schwankt), grün mit dem neuen.
  NRC-Regel mit 32 Threads gab auf der Karte in 5 Läufen 5 verschiedene Ergebnisse.
- Dazu `-threads 100`: der alte bhed_q3map2 (wie NRCs q3map2) stürzt mit 0xC0000409 ab (threads.cpp
  `std::thread threads[MAX_THREADS]`, 64 Plätze) – rot; neu (Vektor) gleich dem 1-Thread-Lauf – grün.
- ctest q3map2_determinismus: `-vis` jetzt mit 32 Threads und `-bhedvisexact` (vorher immer 1 Thread).
- ctest karte_bau: `visBedarfAusPrt` mit der neuen Schätzung.
- Bytevergleich gegen den alten Stand (`-threads 1`, alle Lumps): siehe Messtabelle.

## Schlussrunde

Werte unter Last (die anderen Agenten bauten und testeten gleichzeitig).

| Schritt | Ergebnis |
|---|---|
| Bau Release | ok, 0 Warnungen (/W4 /WX) |
| Neubau (`--clean-first`) | 0 Fehler, 0 Compiler-Warnungen (727 s); dazu einmal MSB8064 an ZERO_CHECK (die Stempel-Regel nach `--clean-first`, keine Übersetzung) |
| ctest | 158 / 159 (300 s): **karte_grenzen rot** – die Prüfstelle MAX_MAP_VISIBILITY zeigte nach den Änderungen in vis.cpp auf Zeile 733 statt 742; behoben (grenzen.cpp, grenzen.json mit `kartegrenzen --json`), danach grün |
| ctest q3map2_* (visgleich, determinismus, teilbau, vergleich, importe), karte_bau, handbuch_sprachen | 7/7 vorab grün |
| Selbsttest karteneditor `BHED_KARTE_NUR=stufe9d` (VIS-Hinweis aus der .prt) | 25 OK / 0 FEHLER |
| Selbsttest karteneditor kurz (`BHED_KARTE_NUR=fotos`) | 78 OK / 0 FEHLER |
| Selbsttest Modus 1 | zweimal: 2638 OK / 8 FEHLER (Zwischenablage: Copy/Paste, danach Strg+Z/Un-REM) und 2640 OK / 7 FEHLER (Tour-Prüfung im Editor-Dialog, „Do it for me“ nicht gefunden) – jede Prüfung, die in einem Lauf fehlschlug, war im anderen OK; die anderen Agenten testeten gleichzeitig (Zwischenablage und Fokus sind geteilt, wie in 9d). Nichts davon berührt VIS/q3map2/den Bau-Hinweis. |
| Kartenprüfung (278 Karten, gegen rc606) | exit 0 (998 s), alle Summen gleich: geprüft 278, abgestürzt 0, nicht geladen 0, D3D 0, nicht gezeichnete Entities 1, NPC ohne Modell 2, fehlende .efx 27, fehlender Klang 29, fehlende Textur 139, fehlendes Modell 5 |
| Lints | lint_sources ok, lint_fassung ok (rc606), lint_i18n 705 (744c1e7a: 705), lint_portability 191 wie 9d, lint_fremdprogramm 1 wie vorher; lint_i18n_erzeugt scheitert wie auf 744c1e7a |
| Restprozesse/-dateien | keine eigenen Prozesse; Läufe unter `Y:/vis/laeufe` (ausgeschlossen) und `SP/vis1`; Spielordner nur gelesen |

## Was gegen das Vorbild geprüft wurde

Geprüft gegen das Vorbild:
- Bytegleich zu NRCs q3map2.exe `-threads 1`: der Visibility-Lump der Testkarte des ctest
  q3map2_visgleich für `-vis`, `-vis -nopassage`, `-vis -passageOnly` (NRC-Lauf von Hand, Hashes im Skript);
  ctf3 bhed `-threads 1` = NRC `-threads 1` war schon in 9d nachgeprüft – unverändert.
- Bytegleich zum alten bhed_q3map2 (744c1e7a, NRCs Fluss-Code) mit `-threads 1`, alle Lumps: ctf3, md_ch_ben,
  md_fh1_jedi, mb2_manaan, taspir1, t3_byss, duel_mos_espa (siehe Messtabelle); der neue Stand mit `-threads 1` und mit
  `-bhedvisexact` und 8/32 Threads gibt dieselben Bytes; dazu `-nopassage` und `-passageOnly` auf ctf3.
- Die NRC-Regel mit mehreren Threads bleibt NRCs Regel (dieselbe Entscheidung „fertig: portalvis,
  sonst portalflood“ je Lesen) – jedes Ergebnis eines, das NRC mit derselben Thread-Zahl liefern kann.
- Speicher: gegen den Prozess gemessen (Arbeitsspeicher/Zusage) und gegen die gedruckte Zahl.
- Fortschritt: der gewichtete Pacifier gegen die gemessene Arbeit je Portal (stat-Läufe).

Nicht gegen das Vorbild geprüft:
- t3_byss, taspir1, kejim_post, duel_mos_espa gegen NRCs q3map2.exe mit `-threads 1` (nur gegen den alten
  bhed-Stand, der in 9d auf ctf3 NRC-gleich war; NRC mit einem Thread braucht auf mos_espa rund 40 min).
- Das volle VIS auf md_rev_TarisUpper2: läuft jetzt in 3,2 GB, aber Stunden bis Tage – nur die ersten
  15 min gemessen.
- Der Fortschritt im Editor-Fenster: die Gewichtung ist an drei Karten gemessen; im Fenster selbst nicht
  mit einem Bildschirmvergleich geprüft (der Parser liest dieselben 40 Zeichen wie vorher, ctest karte_bau
  und fortschritt_vis.bin unverändert grün).

## Offen

- Licht ist bei „[Final] Extreme Quality“ die Engstelle (siehe 4): `-randomsamples` schaltet sowohl das
  GPU-Backen (Auto) als auch das Teilen großer Lightmaps auf mehrere Kerne ab. Weg: die Zufallsfolge je
  Arbeitsschritt (LCG wie MSVC rand) lässt sich vorspulen (affine Abbildung, O(log n)); mit der seriellen
  Liste der nachgerechneten Luxel bekäme jeder parallele Teil seinen Startzustand – bitgleich. Nicht
  begonnen.
- `-bhedvisexact` als Vorgabe oder als Menüpunkt im Editor: entscheidet shank (Zahlen oben).
- ClipToSeperators (~50 % des Flusses) ist unverändert NRCs Rechnung; ein bytegleicher Weg, der schneller
  ist, fehlt (der Vorab-Test war langsamer).
- lint_i18n_erzeugt scheitert wie auf 744c1e7a (die Prüfkopie kopiert i18n_karte2.py ff. nicht),
  lint_fremdprogramm 1 (tools/kartenpruefung.py) wie vorher.

## Stand bei der Pause (29.10., 17:00)

Fertig und committet: alles oben (Code, Tests, Handbuch, Notizen), Schlussrunde gelaufen.
Fehlt nur noch: der Bytevergleich kejim_post `-bhedvisexact` (Visibility f500465d…, alle Lumps
2801040796d85225) gegen den alten Stand mit `-threads 1` – der Lauf `alt_t1b` wurde für die Pause bei
70 % beendet (der erste brauchte mehr als 60 min). Weiter mit (Scratchpad, ein Thread, etwa 70 min):

    cd $SP/vis1 && python -I vl.py lauf "$(cygpath -w $SP/vis1/bin/alt.exe)" kejim_post alt_t1c --threads 1 --grenze 7200

Ist `vis` gleich f500465df5797d28 und `bsp` gleich 2801040796d85225, in der Messtabelle „ja“ eintragen.
Danach nur noch der Bericht; keine offenen Änderungen im Worktree.

## Changelog

## English

- bhed_q3map2 `-vis`, same result, faster and much leaner: the portal flow works only on the parts of its bit vectors that are not empty, and the passages keep only their non-empty part - 32 threads: kejim_post 551 → 214 s, taspir1 29 → 14 s, duel_mos_espa 140 → 91 s, with a third to a quarter of the memory; md_rev_TarisUpper2 needs 3.2 GB instead of more than 11 GB.
- New switch `-bhedvisexact` for `-vis`: with several threads exactly the visibility of `-threads 1`, byte for byte, in every build. It waits where a portal needs one that is not finished yet - on open maps much slower; add it to the `-vis` line in Build > Customize....
- Map editor, build progress: the VIS progress counts the expected work of the portals instead of their number, so the remaining time of a first build is close to the real one; the memory hint before a full VIS estimates the compact passages.
- Fixed: bhed_q3map2 with more than 64 threads (`-threads`) wrote past its thread table.

## Deutsch

- bhed_q3map2 `-vis`, gleiches Ergebnis, schneller und viel sparsamer: der Portalfluss rechnet nur auf den Teilen seiner Bitfelder, die nicht leer sind, und die Passagen behalten nur ihren nicht leeren Teil - 32 Threads: kejim_post 551 → 214 s, taspir1 29 → 14 s, duel_mos_espa 140 → 91 s, mit einem Drittel bis Viertel des Speichers; md_rev_TarisUpper2 braucht 3,2 GB statt über 11 GB.
- Neuer Schalter `-bhedvisexact` für `-vis`: mit mehreren Threads genau die Sichtbarkeit von `-threads 1`, Byte für Byte, in jedem Bau. Er wartet, wo ein Portal ein noch nicht fertiges braucht - auf offenen Karten deutlich langsamer; in Build > Customize... an die `-vis`-Zeile anhängen.
- Karten-Editor, Bau-Fortschritt: der VIS-Fortschritt zählt die erwartete Arbeit der Portale statt ihrer Zahl, die Restzeit eines ersten Baus liegt damit nahe an der echten; der Speicherhinweis vor einem vollen VIS schätzt die kompakten Passagen.
- Behoben: bhed_q3map2 mit mehr als 64 Threads (`-threads`) schrieb über seine Thread-Tafel hinaus.

## 中文

- bhed_q3map2 `-vis`:结果相同,速度更快,内存占用大幅减少:门户流只处理位向量中非空的部分,通道只保留非空部分——32 线程:kejim_post 551 → 214 秒,taspir1 29 → 14 秒,duel_mos_espa 140 → 91 秒,内存只需原来的三分之一到四分之一;md_rev_TarisUpper2 只需 3.2 GB,而不是 11 GB 以上。
- `-vis` 的新开关 `-bhedvisexact`:使用多个线程时,每次编译都精确得到与 `-threads 1` 逐字节相同的可见性。当某个门户需要一个尚未完成的门户时它会等待——在开阔的地图上会慢很多;在 Build > Customize... 中把它加到 `-vis` 行。
- 地图编辑器,编译进度:VIS 进度按门户的预计工作量计算,而不是按门户数,因此第一次编译的剩余时间也接近实际时间;完整 VIS 之前的内存提示按紧凑通道估算。
- 修复:bhed_q3map2 使用超过 64 个线程(`-threads`)时会写出线程表的范围。

## 日本語

- bhed_q3map2 `-vis`:結果は同じで、より速く、メモリも大幅に少なくなりました。ポータルフローはビット列の空でない部分だけを処理し、パッセージは空でない部分だけを保持します。32 スレッド:kejim_post 551 → 214 秒、taspir1 29 → 14 秒、duel_mos_espa 140 → 91 秒で、メモリは 3 分の 1 から 4 分の 1 です。md_rev_TarisUpper2 は 11 GB 超ではなく 3.2 GB で済みます。
- `-vis` の新しいスイッチ `-bhedvisexact`:複数のスレッドでも、毎回のビルドで `-threads 1` とバイト単位で同じ可視性を計算します。まだ完了していないポータルが必要なところでは待つため、開けたマップではかなり遅くなります。Build > Customize... で `-vis` の行に追加します。
- マップエディター、ビルドの進行状況:VIS の進行状況はポータルの数ではなく見込み作業量で数えるため、初回ビルドの残り時間も実際に近くなります。完全な VIS の前のメモリのヒントはコンパクトなパッセージで見積もります。
- 修正:bhed_q3map2 で 64 を超えるスレッド(`-threads`)を使うと、スレッド表の範囲外に書き込んでいました。
