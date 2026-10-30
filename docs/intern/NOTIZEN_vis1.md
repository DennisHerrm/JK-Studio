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
| ca731f45, 44db51c5 | Notizen vis1, Stand bei der Pause |
| 8485603e | Licht: `-randomsamples` auf allen Kernen, größte Lightmaps/Flächen zuerst, SetupEnvelopes parallel; ctest q3map2_determinismus Stufe lichtzufall |
| 6a7da090 | Editor: Build > „Same VIS in every build“ (`-bhedvisexact`), Tooltip, Handbuch, Selbsttest stufe9d |
| ba0206c8 | GPU-Backen mit `-randomsamples`, Prozessor nebenher, Lichtgitter geteilt, Auto-Regel; ctest q3map2_gpubake 7e/7f, karte_gpuwahl; Handbuch |
| 51f0c4a6 | Lichtlisten über den Lichtbaum (IlluminateRawLightmap auf dem Prozessor, IlluminateVertexes), große Vertexlicht-Flächen auf allen Threads; ctest q3map2_gpubake 7g |
| 7eea1d3f | Vertexlicht: „groß“ gegen die Vertexe der Vertexlicht-Flächen |
| 25ebc69c | Auto: `-randomsamples` wieder unter Regel 5 (erst der Prozessor, dann das Schnellere) |
| (dieser) | Notizen vis1 nach der Pause |

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
| kejim_post (JK2) | 23552 | 550,5 → 214,1 s (1 × 2) | 12332 → 4324 s | 3527 → 927 | 550,7 s | ja (f500465d…, alle Lumps 28010407…; alt `-threads 1`: 7031 CPU-s, 2 h 17 min Wand mit Idle-Priorität) |
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
- (Stand vor der Pause, nach der Pause umgesetzt – siehe 6.) GPU-Backen (Auto) und das Teilen großer
  Lightmaps auf mehrere Kerne (Stufe 7) liefen nur ohne `-randomsamples`.
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

## 6. Licht bei „[Final] Extreme Quality“ (nach der Pause)

Auftrag nach der Pause: VIS-Vorgabe bleibt NRCs Regel (`-bhedvisexact` als Schalter, Menüpunkt, siehe
6a7da090), dann der Lichtengpass: GPU-Backen und Mehrkern auch mit `-randomsamples`, bitgleich. shank:
„Wenn es schneller geht, nimm das Schnellere.“ Messwerkzeug `SP/vis1/lt.py` (eine Karte `-meta`, `-vis -fast`
mit dem Referenzstand, dann `-light` mit mehreren exes, Hash aller 18 Lumps). „alt“ = 744c1e7a.

### 6a. Zufallsproben auf allen Kernen (8485603e)

- Seit Stufe 6 zieht jeder Arbeitsschritt (Raw-Lightmap, Fläche) aus einem eigenen Generator
  (`BhedWorkSeed(Durchgang, Nummer)`, MSVC-LCG). `RandomSubsampleRawLuxel` zieht je nachgetastetem Luxel
  immer 2 × samples Zahlen (GaussLikeRandom vor dem Abbildungstest); der n-te nachgetastete Luxel einer
  Lightmap beginnt also bei Zustand `BhedRandomSkip(seed, 2·samples·n)` (affine Abbildung, O(log n)).
- Große Lightmaps (mehr Luxel als der Anteil eines Threads) mit `-randomsamples`: Durchgang 1 (Erstabtastung)
  und 3 (Filtern/Aufsummieren) auf allen Threads (`BhedLichterParallel`), Durchgang 2 seriell in der Reihenfolge
  des seriellen Laufs (`BhedZufallsProben`, Lichter in Listenfolge, Stempel zeilenweise) – dieselben Zahlen.
  Ein Faden rechnet die großen mit dem Pool, während der Thread-Durchgang den Rest rechnet; jede weitere große
  nimmt, wer zuerst kommt (Anspruch je Lightmap).
- Größte zuerst (LPT) in IlluminateRawLightmap und IlluminateVertexes (je Schritt der Generator seiner Nummer,
  `BhedPassSeed` – unabhängig davon, wer ihn wann rechnet).
- SetupEnvelopes je Licht auf allen Threads (die gekappten danach in Listenfolge gelöscht): mos_espa mit Bounce
  452184 Lichter, 10,9 → 1,2 s je Bounce.
- **Verworfen**: Durchgang 2 vorab mit den Werten der Erstabtastung vorhersagen und parallel rechnen – die
  Vorhersage bricht nach 3–4 Nachtastungen (md_ch_ben: 99 echte gegen 145 vorhergesagte Ereignisse im ersten
  Licht), danach stimmen die Nummern der Zufallsfolge nicht mehr. Die großen vorab nacheinander mit dem Pool
  rechnen: duel_pit 2,2 → 5 s (Durchgang 2 seriell, die anderen Kerne warten) – ersetzt durch den Faden nebenher.
- ctest q3map2_determinismus: Stufe `lichtzufall` (`-fastbounce -patchshadows -samples 4 -randomsamples
  -bounce 2`) dreimal und `-threads 1` gleich, der parallele Weg muss laufen; rot mit dem alten bhed_q3map2.

### 6b. GPU-Backen mit `-randomsamples`

- Die GPU sagt mit den Werten der Erstabtastung je aktivem Licht den ersten Stempel voraus, der nachtasten
  würde (Kern `kandidaten`, Runde 0). Ohne vorhergesagten Stempel tastet auch der serielle Lauf nichts nach
  (er sieht bis zum ersten Nachtasten nur diese Werte) – die Lightmap ist mit Filtern und Aufsummieren auf der
  GPU fertig und zieht keine Zufallszahl. Jede Lightmap mit einem vorhergesagten Stempel rechnet der Prozessor
  ganz (mit dem Generator ihres Arbeitsschritts). mos_espa: 82 % der Bounce-Arbeit liegt in Lightmaps ohne
  Nachprobe (Messfassung `anteil_patch.py`: 57815 Lightmaps 16008 Thread-s gegen 6867 mit 3597 Thread-s).
- **Zwei Fehler gefunden (rot im neuen Fall 7e von ctest q3map2_gpubake, dann grün)**:
  1. `-samples 4 -randomsamples` setzt lightSamples auf 256 (NRCs Voreinstellung) – über der Grenze 7 der
     GPU-Unterabtastung, keine Lightmap kam auf die GPU. Mit `-randomsamples` tastet die GPU nicht nach, die
     Grenze gilt dort nicht.
  2. Das Lichtgitter auf der GPU zählte keinen Durchgang (die CPU: `RunThreadsOnIndividual(TraceGrid)`), die
     Durchgänge danach hatten andere Nummern und damit andere Zufallszahlen (Lumps 10 und 14 anders). Jetzt
     `BhedPassBegin()`.
- **Prozessor nebenher** (`BhedIlluminateGpu`): vorher rechnete erst die GPU alle Blöcke (der Prozessor
  wartete), dann der Prozessor seine Lightmaps (die GPU wartete). Jetzt rechnen numthreads eigene Fäden
  gleichzeitig: die GPU-Lightmaps fertigstellen (Rest von IlluminateRawLightmap mit einer Kopie der Luxel –
  im Pool des Hauptfadens neben den Fäden 57 s statt 7 s), die eigenen Lightmaps (die meiste Arbeit zuerst:
  FIFO ließ die größten eines Bounce erst nach 108 s anfangen) und – nur ohne `-randomsamples` – noch nicht
  vergebene GPU-Lightmaps vom Ende der Liste. Mit `-randomsamples` ist die Arbeit des Prozessors größer als die
  ganze GPU-Liste (mos_espa 120 gegen 33 s); jede übernommene machte das Ende später (Bounce 150 → 199 s).
- **Lichtgitter geteilt**: „[Final] Extreme Quality“ hat kein `-fast`, jedes Licht reicht weit – die GPU
  rechnete das Gitter langsamer als 32 Threads (md_am_sith 353 gegen 75 s, mos_espa 86 gegen 3 s). Jetzt nimmt
  die GPU Teile von vorn, die Fäden einzelne Punkte von hinten, am Ende auch die Punkte des Teils auf der GPU
  (die GPU hört dann auf, `BhedBakeGitter(..., halt)`); md_am_sith 353 → 63 s, mos_espa 86 → 3 s.
- `BHED_GPUBAKE_NUR_GPU`: ohne Prozessor nebenher (der ctest prüft damit die festen GPU-Zahlen).
- Auto (`bhed/gpuwahl.h`): in ba0206c8 galt Regel 5 (`-samples 4` groß → CPU) nicht mehr mit
  `-randomsamples`. Nach den Lichtbaum-Listen (6c) ist der Prozessor auf dem vollen Lichtschritt schneller
  (mos_espa `-bounce 8`: CPU 582 s, GPU 717 s, dieselben Bytes) – zurückgenommen (25ebc69c): zuerst der
  Prozessor, nach einem langen Lauf misst Auto einmal die GPU und nimmt danach das Schnellere (ctest
  karte_gpuwahl). GPU-Backen mit `-randomsamples` bleibt damit möglich und bitgleich, Auto entscheidet je
  Karte nach gemessener Zeit.
- Gemessene Engpässe, die Teil einer Messfassung waren (nicht eingecheckt): `nebendiag_patch.py`,
  `cpudiag_patch.py` (je Lightmap Start/Dauer). Achtung beim Messen: ein Bau nebenher machte einzelne
  Lightmaps 1,7-mal langsamer (d6) – alle Zahlen unten ohne Bau nebenher.

### 6c. Lichtlisten über den Lichtbaum, große Vertexflächen (51f0c4a6)

- Messfassung `vdiag_patch.py` (IlluminateVertexes, mos_espa, ein Bounce): 2382 Vertexlicht-Flächen,
  CreateTraceLightsForSurface 214 von 710 Thread-s, 55 s Wand (ideal 22 s – eine große Fläche am Ende);
  im ersten Durchgang dagegen 2,4 Mio. verschobene Proben (dunkle Vertexe bis zu 125-mal neu beleuchtet).
- CreateTraceLightsForBounds prüft jedes Licht für jede Raw-Lightmap und Fläche – mit 452184 Bounce-Lichtern
  etwa 90 ms je Liste. Das war der größte Teil eines Bounce auf dem Prozessor (und der Lightmaps mit
  Zufallsproben, die der Prozessor neben der GPU rechnet). Jetzt dieselben Tests auf den Kandidaten des
  Lichtbaums aus Stufe 8g (`BhedLichtBaum`, Kugeln mit Rand), in Listenfolge – dieselbe Liste;
  `BHED_LICHTLISTEN_PRUEFEN` vergleicht jede Liste mit CreateTraceLightsForBounds (ctest q3map2_gpubake 7g;
  md_ch_ben 48 + 10, duel_pit 369 + 6 Listen je Runde, alle gleich). Die `-v`-Zähler „lights … culled“
  zählen nur noch die Kandidaten (sie waren ohne Sperre über die Threads ohnehin nicht reproduzierbar).
- Eine Vertexlicht-Fläche mit mehr Vertexen als dem Anteil eines Threads rechnet ihre Vertexe vorab auf
  allen Threads (`VertexLicht`, je Faden eigener trace, Stile, Farben) – nur wenn jedes Licht der Liste
  schon seinen Stil-Platz hat (LightingAtSample vergibt Plätze in Vertexfolge) und kein Dirt Zufallszahlen
  zieht; Mittelwert und Rest in Vertexfolge wie immer. md_ch_ben: eine Fläche je Runde (`BHED-Vertex-Parallel`
  mit BHED_Q3_ZEITEN), Lumps gleich.
- mos_espa, ein Bounce: Bounce IlluminateRawLightmap Prozessor 613 → 93 s, mit GPU 129 → 72 s;
  IlluminateVertexes 59 → 31–37 s (die Vertexe auf allen Threads greifen auf mos_espa nicht, siehe 6e).

### 6d. Messungen Licht (32 Threads, RTX 4070 Laptop, ohne Bau nebenher)

`-light -fastbounce -patchshadows -samples 4 -randomsamples -bounce N` („[Final] Extreme Quality“ mit
kleinerem N), Wand des ganzen `-light`-Schritts. „alt“ = 744c1e7a, „CPU 8485603e“ = Zufallsproben auf allen
Kernen, „CPU jetzt“/„GPU jetzt“ = mit Lichtbaum-Listen (51f0c4a6/7eea1d3f), GPU = `-bhedgpubake` (Auto: zuerst der
Prozessor, nach einem langen Lauf einmal die GPU, dann das Schnellere – siehe 6b). Streuung zwischen Läufen bis ±15 % (Notebook, Wärme).

| Karte | N | alt | CPU 8485603e | CPU jetzt | GPU ba0206c8 | GPU jetzt | Lumps (alle gleich) |
|---|---|---|---|---|---|---|---|
| md_ch_ben (MD) | 2 | 4,2 s | 3,4 s | 3,6–5,2 s | 2,6 s | 3,6 s | 6f84669d794e2765 |
| mp__duel1 (JA) | 2 | 1,8 s | 1,2–1,4 s | 1,2–1,6 s | 1,2 s | 1,4 s | d7f8bc9615577d73 |
| t1_inter (JA) | 2 | 0,4 s | 0,4 s | 0,4 s | 0,4 s | 0,6 s | d948eb278beebe41 |
| duel_pit (JK2) | 2 | 2,2 s | 2,2 s | 2,2–3,0 s | 2,8 s | 3,0 s | 1bf84e5eac04f19d |
| md_am_sith (MD) | 2 | 826 s | 791 s | – | 758 s | 839 s | 448f3509fa84697b |
| duel_mos_espa (MD) | 1 | 828 s | 659 s | 237–271 s | 287 s | 210–227 s | 1a1f3e708921e3d9 |
| duel_mos_espa (MD) | 8 („[Final] Extreme Quality“) | nicht voll gerechnet (siehe unten) | – | 582 s | – | 717 s | 9b82e11ca6598c4e (CPU = GPU) |

Ohne Zufallsproben (`-light -fast -patchshadows -samples 2 -filter -bounce 2`) ebenfalls gleich: md_ch_ben
6552431bbafc6d79, duel_pit 04401b6d9323b064, mp__duel1 64095e6df6fbd3f8 (CPU und GPU); `-threads 1` gleich
(md_ch_ben, mp__duel1).

- **„[Final] Extreme Quality“ voll** (mos_espa, `-bounce 8`): Prozessor 582 s = 9,7 min, GPU 717 s = 12 min
  (dieselben Bytes; die GPU richtet je Runde Lichttafel, Baum und Blöcke ein, und der Prozessor rechnet die
  Lightmaps mit Zufallsproben ohnehin selbst – Auto nimmt zuerst den Prozessor, siehe 6b). Die späteren Bounces haben
  viel weniger Lichter (452184, 372993, 261736, 102269, 34562, 10342, 3578, 1110) und kosten fast nichts; die
  Schätzung „80 min“ vor der Pause nahm jeden Bounce so teuer wie den ersten. Mit derselben Annahme für den
  alten Stand (Bounce-Kosten etwa proportional zur Lichtzahl, da die Lichtlisten den Bounce bestimmten):
  etwa 120 s + 2,7 × 700 s ≈ 34 min.
- Kleine Karten: kein Gewinn mehr möglich (Laden, Gerät einrichten); GPU dort eher langsamer – Auto nimmt
  auf kleinen Karten den Prozessor (Regel 4).
- md_am_sith: fast nur Lightmaps mit Zufallsproben (13131 von 42438 Lightmaps in drei Runden, 16000 Thread-s) – die rechnet
  der Prozessor; die Listen sind dort kurz (erster Durchgang, 24577 Lichter). Ohne zufällige Nachproben auf
  der GPU (Offen) bleibt es bei etwa 13 min.
- mos_espa, ein Bounce (GPU jetzt): Laden 20–26 s, SetupTraceNodes 7–8 s, Gitter 4 s, erster Durchgang
  IlluminateRawLightmap 39–42 s, IlluminateVertexes 10–12 s; Bounce: IlluminateRawLightmap 65–76 s,
  IlluminateVertexes 31–37 s, Rest 10 s.

### 6e. Was bei „[Final] Extreme Quality“ sonst noch bremst

- **IlluminateVertexes** (31–37 s je Bounce auf mos_espa; ideal mit 32 Threads etwa 15 s; über alle
  8 Bounces 212 von 717 s): der Rest ist Ungleichgewicht – keine Vertexlicht-Fläche hat dort mehr Vertexe
  als der Anteil eines Threads (`BHED-Vertex-Parallel: 0`). Messfassung `vdiag2_patch.py`, erster Durchgang:
  die teuersten sind Patches mit 225 Vertexen und 114 Lichtern, je 10 s (311 Thread-s gesamt). **Verworfen**:
  „groß“ nach Vertexe × Listenlänge (Hälfte des Anteils eines Threads, ab 32 Vertexen) – 6–12 Flächen je
  Runde vorab auf allen Threads, der Bounce aber 31–37 → 45 s (die Kosten stecken im Verschieben dunkler
  Vertexe, nicht in der Listenlänge; vorab nacheinander lässt die Kerne zwischen den Flächen warten).
- **Lightmaps mit Zufallsproben** (der Prozessor rechnet sie, auch mit GPU): auf md_am_sith fast alles.
- Laden 20–26 s und SetupTraceNodes 7–8 s (einmal), `-meta` 29 s, `-vis` 91 s.

## Schlussrunde nach der Pause

Stand 355d179c (Code) und dieser Notizen-Commit, Werte unter Last (die anderen Agenten bauten und testeten gleichzeitig).

| Schritt | Ergebnis |
|---|---|
| Neubau (`--clean-first`, `--parallel 4`) | 0 Fehler, 0 Compiler-Warnungen (708 s); einmal MSB8064 an ZERO_CHECK wie vor der Pause |
| ctest | 158 / 159 (322 s): **karte_grenzen rot** – die Prüfstelle MAX_MAP_LIGHTGRID zeigte nach dem geteilten Lichtgitter auf light.cpp:1952 statt 2046; behoben (53b6f946: grenzen.cpp, grenzen.json mit `kartegrenzen --json`), danach grün; am Ende noch einmal ganz: 159 / 159 (275 s) |
| ctest q3map2_gpubake, q3map2_determinismus, karte_gpuwahl, karte_bau, handbuch | grün (auch einzeln mit dem Endstand) |
| Selbsttest karteneditor `BHED_KARTE_NUR=stufe9d` | 27 OK / 1 FEHLER („fx_runner spielt in der Echtzeit-Ansicht“, Effektsystem der Vorschau – nichts von vis1), Wiederholung 28 OK / 0 FEHLER |
| Selbsttest karteneditor kurz (`BHED_KARTE_NUR=fotos`) | **77 OK / 1 FEHLER**: „Menue Build: 264 x 772 wie NRC“ – der Menüpunkt aus 6a7da090 macht das Menü eine Zeile höher und (längster Text) 369 breit; Sollmaße im Selbsttest nachgezogen (355d179c), danach 78 OK / 0 FEHLER |
| Handbuchbild h_karte8_menue (4 Sprachen) | neu aus dem Selbsttest (`BHED_HB_NUR=karte8`), nur dieses Bild ersetzt (29ffdd06) |
| Selbsttest Modus 1 | 2645 OK / 0 FEHLER |
| Kartenprüfung (278 Karten, gegen rc606) | exit 0 (722 s), alle Summen gleich: geprüft 278, abgestürzt 0, nicht geladen 0, D3D 0, nicht gezeichnete Entities 1, NPC ohne Modell 2, fehlende .efx 27, fehlender Klang 29, fehlende Textur 139, fehlendes Modell 5 |
| Lints | lint_sources ok, lint_fassung ok (rc606), lint_headers ok, lint_i18n 705 (wie vorher), lint_portability 191 (wie vorher), lint_fremdprogramm 1 (wie vorher); lint_i18n_erzeugt scheitert wie auf 744c1e7a |
| Restprozesse/-dateien | keine eigenen Prozesse (die laufende Kartenprüfung aus Y:\sprache_cs gehört einem anderen Agenten); Läufe unter `Y:/vis/laeufe` (von git ausgeschlossen) und `SP/vis1`; Spielordner nur gelesen; Worktree sauber |


## Schlussrunde vor der Pause (Stand 0ec977af–b36d2514)

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
  md_fh1_jedi, mb2_manaan, taspir1, t3_byss, duel_mos_espa, kejim_post (siehe Messtabelle); der neue Stand mit `-threads 1` und mit
  `-bhedvisexact` und 8/32 Threads gibt dieselben Bytes; dazu `-nopassage` und `-passageOnly` auf ctf3.
- Die NRC-Regel mit mehreren Threads bleibt NRCs Regel (dieselbe Entscheidung „fertig: portalvis,
  sonst portalflood“ je Lesen) – jedes Ergebnis eines, das NRC mit derselben Thread-Zahl liefern kann.
- Speicher: gegen den Prozess gemessen (Arbeitsspeicher/Zusage) und gegen die gedruckte Zahl.
- Fortschritt: der gewichtete Pacifier gegen die gemessene Arbeit je Portal (stat-Läufe).
- Licht (nach der Pause): alle 18 Lumps gleich dem alten bhed_q3map2 (744c1e7a, Zufallsgenerator je
  Arbeitsschritt seit Stufe 6) auf md_ch_ben, mp__duel1, t1_inter, duel_pit, md_am_sith, duel_mos_espa –
  mit und ohne `-randomsamples`, Prozessor und GPU-Backen; `-threads 1` gleich (md_ch_ben, mp__duel1);
  jede Lichtliste des Lichtbaums gegen CreateTraceLightsForBounds (`BHED_LICHTLISTEN_PRUEFEN`).

Nicht gegen das Vorbild geprüft:
- Der volle Lichtschritt von „[Final] Extreme Quality“ (mos_espa `-bounce 8`) nur Prozessor gegen GPU (gleich),
  nicht gegen den alten Stand (der bräuchte geschätzt über eine halbe Stunde).
- Licht mit `-randomsamples` gegen NRCs q3map2.exe: seit Stufe 6 zieht bhed_q3map2 je Arbeitsschritt aus
  einem eigenen Generator (NRC mit mehreren Threads ist dort nicht reproduzierbar); Maßstab ist der eigene
  Lauf mit `-threads 1`.
- t3_byss, taspir1, kejim_post, duel_mos_espa gegen NRCs q3map2.exe mit `-threads 1` (nur gegen den alten
  bhed-Stand, der in 9d auf ctf3 NRC-gleich war; NRC mit einem Thread braucht auf mos_espa rund 40 min).
- Das volle VIS auf md_rev_TarisUpper2: läuft jetzt in 3,2 GB, aber Stunden bis Tage – nur die ersten
  15 min gemessen.
- Der Fortschritt im Editor-Fenster: die Gewichtung ist an drei Karten gemessen; im Fenster selbst nicht
  mit einem Bildschirmvergleich geprüft (der Parser liest dieselben 40 Zeichen wie vorher, ctest karte_bau
  und fortschritt_vis.bin unverändert grün).

## Offen

- **Zufällige Nachproben auf der GPU** (der Prozessor ist mit GPU die Engstelle von „[Final] Extreme
  Quality“): möglich und bitgleich, aber nicht begonnen. Weg: je Lightmap eine Gruppe; die Stempelkette
  seriell (ein Faden entscheidet, Lichter in Listenfolge), die 256 Proben eines Luxels parallel (Zustand je
  Probe mit `BhedRandomSkip`), danach in Probenfolge aufsummiert. GaussLikeRandom braucht cos/sin/sqrt in
  NRCs Rundung – `random()` liefert nur 32768 Werte, also Tafeln aus demselben CPU-Code (x-Wert vor dem
  Radius, Radius r je Wert), auf der GPU nur noch float-Multiplikation. Dazu Wiederaufnahme über mehrere
  Dispatches (TDR, 2 s).
- **IlluminateVertexes**: Ungleichgewicht (siehe 6e), nur auf dem Prozessor.
- NRCs q3map2 stürzt ab, wenn man eine schon beleuchtete quake3-BSP mit Stilen noch einmal mit
  `-light ... -bounce` beleuchtet (auch NRCs q3map2.exe; gefunden beim ctest-Fall lichtzufall, der darum
  auf der Ausgabe von `-vis` läuft). Nicht behoben.
- ClipToSeperators (~50 % des Flusses) ist unverändert NRCs Rechnung; ein bytegleicher Weg, der schneller
  ist, fehlt (der Vorab-Test war langsamer).
- lint_i18n_erzeugt scheitert wie auf 744c1e7a (die Prüfkopie kopiert i18n_karte2.py ff. nicht),
  lint_fremdprogramm 1 (tools/kartenpruefung.py) wie vorher.

## vis2: Licht weiter beschleunigen, bitgleich (Nachauftrag, Stand 30.10. früh)

Auftrag `SP/auftrag_vis2.md`. Windows Update startete den Rechner am 30.10. um 00:29 neu – alle Läufe und
Messungen davor sind abgebrochen bzw. ungültig; alle Bytevergleiche unten sind nach dem Neustart neu
gelaufen. Harte Zeitgrenze 06:40 (Rechner um 07:00 aus), darum nur Teile des Auftrags. Messwerkzeuge
`SP/vis2`: `lz.py` (eine Karte `-light` mit mehreren exes, je Lauf eigene Schalter/Umgebung, Hash aller
18 Lumps, GPU-Bericht kurz), `klein.sh` (Bytevergleich vier kleine Karten), `absturz.py`, `rl.py`, `ibsp.py`
(Punkt 3), `teilen.py` (Hunks eines Diffs auf zwei Commits verteilen). „alt“ = `SP/vis2/bin/a0.exe` = Stand
24bbef3c (vis1 fertig).

### Commits vis2

| Commit | Inhalt |
|---|---|
| 48aba3bd | Punkt 3: zweites `-light` auf beleuchteter quake3-BSP mit Stilen stürzt nicht mehr ab; ctest q3map2_funde Fall `relicht`; Grenzen-Tafel light.cpp:2047 |
| c1259468 | Punkt 2: IlluminateVertexes teilt die Vertexe einer Fläche mit freien Threads – gleiche Bytes |
| 3c542830, f459fddf | WIP Punkt 1: zufällige Nachproben auf der GPU (Kern `zufall`), Übernahme durch die GPU hinter `BHED_GPUBAKE_ZUFALL` (ab Werk aus); ctest q3map2_gpubake 7e |
| (dieser) | Notizen vis2 |

### 1. Zufällige Nachproben auf der GPU – WIP (bitgleich, aber noch nicht schneller)

- **Kern `zufall`** (bhed_gpubacken.hlsl): eine Gruppe (64 Fäden) je Lightmap. Faden 0 führt den seriellen
  Lauf von BhedZufallsProben (aktive Lichter in Listenfolge, je Licht der Stempellauf in Zeilenfolge, nur
  vorhergesagte und „schmutzige“ Stempel wie `stempellauf`); jeder neu nachzutastende Luxel bekommt den
  Zustand des Generators der Lightmap (`seed` = BhedPassSeed wie BhedCpuLightmap) nach 2·lightSamples·n Zügen
  (Sprung je Luxel aus `zufallSprung[lightSamples]`). Die lightSamples Proben eines Luxels rechnen alle Fäden
  (Zustand je Probe `s·x_b + y_b`, Sprungtafel für 2·b Züge), aufsummiert in Probenfolge (je Farbkanal ein
  Faden), dann `/ mapped` mit der genau gerundeten Division.
- GaussLikeRandom auf der GPU ohne cos/sin/sqrt: `random()` hat nur 32768 Werte; GaussLikeRandom ist in
  `GaussLikeWinkel`/`GaussLikeRadius` geteilt (dieselben Rechnungen), die Tafeln (x, y vor dem Radius, Radius)
  entstehen auf der CPU aus genau diesem Code, auf der GPU nur noch zwei float-Multiplikationen.
- Wiederaufnahme: der Stand des Laufs je Lightmap liegt zwischen den Dispatches in `kandListe` (40 Wörter),
  höchstens `zRunden` Luxel je Gruppe und Dispatch (passt sich an, TDR); `zaehler[0]` zählt offene Lightmaps,
  `zaehler[1]` die nachgetasteten Luxel (Bericht „N with random subsamples on the GPU (M luxels)“).
- **Gefunden beim Bau (TDR)**: der Leiter-Faden mit `while (true)` und `return`/`continue` in der Schleife
  (innerhalb eines `if (tid == 0)` zwischen Gruppen-Syncs) hängte die NVIDIA-GPU (Gerät verloren nach 25 s,
  schon beim zweiten Stempel; mit einer zusätzlichen Zählvariable lief derselbe Code). Umgeschrieben ohne
  `return`/`continue` (Zustand in Registern, ein `suchen`-Merker) – seitdem stabil.
- Auf der CPU: die Nachprüfung einer GPU-Lightmap (`-bhedgpubakecheck`) rechnet jetzt mit dem Generator des
  Arbeitsschritts der Lightmap (vorher `rand()` – für Lightmaps mit Zufallsproben nötig).
- **Bytegleich** (nach dem Neustart, `klein.sh`, alle 18 Lumps gleich alt CPU): md_ch_ben (41 Lightmaps mit
  Nachproben, 1798 Luxel auf der GPU), duel_pit (414 / 43803), mp__duel1 (131 / 2776), t1_inter (keine), jeweils
  mit `BHED_GPUBAKE_NUR_GPU` (alle Nachproben auf der GPU), Vorgabe (wie bisher: Prozessor) und
  `BHED_GPUBAKE_ZUFALL=1` (duel_pit: 370 Lightmaps von der GPU übernommen, 16458 Luxel); jede nachgeprüfte
  GPU-Lightmap 100 % gleich.
- **Messung (vor dem Neustart, unter Last, kein A/B)**: md_am_sith `-bounce 2`, alle Nachproben nur auf der GPU:
  erster Durchgang IlluminateRawLightmap 1432 s gegen 722 s im alten Weg (Prozessor rechnet die 13131
  Lightmaps mit Nachproben neben der GPU) – **die GPU allein ist hier halb so schnell wie 32 Threads**.
  Vermutete Ursachen: je Block (Budget 40 Mio. Paare, md_am_sith etwa 275 Blöcke) nur etwa 50 Lightmaps mit
  Nachproben → 50 Gruppen, die GPU ist kaum ausgelastet, und alles wartet je Block; jede der 256 Proben eines
  Luxels rechnet bei Flächenlichtern ohne `-fast` den Formfaktor mit double-acos (Verbraucher-GPU: double 1/64).
- Darum der **Mischweg** (WIP, `BHED_GPUBAKE_ZUFALL=1`): die Lightmaps mit Nachproben gehen zuerst an den
  Prozessor (`nebenZufall`, die Threads nehmen die größten); hat die GPU keinen eigenen Block mehr, nimmt sie die
  kleinsten in Stapeln (je ein Drittel der Restarbeit, solange mehr übrig sind als Threads) – beide rechnen bis
  zum Ende. Bitgleich (oben), aber auf großen Karten **nicht gemessen** → ab Werk aus; ohne den Schalter ist
  der Weg genau der bisherige (dieselben Warteschlangen). Die Auto-Regel (gpuwahl.h) bleibt unverändert.
- **Messung nach dem Neustart** (duel_mos_espa `-bounce 1`, GPU-Backen, unter Last, je ein Lauf): ohne Schalter
  154,7 s, mit `BHED_GPUBAKE_ZUFALL=1` 187,4 s – langsamer; die GPU übernahm 524 Lightmaps mit 13577
  nachgetasteten Luxeln, Kern `zufall` 15,4 s, dazu die Erstabtastung dieser Lightmaps noch einmal (Kern `erst`
  35,7 → 48,4 s). Lumps gleich (1a1f3e708921e3d9). Etwa 1 ms je Luxel in der seriellen Kette einer Gruppe: ein
  GPU-Faden braucht für einen Strahl durch den BSP ein Vielfaches eines CPU-Kerns, und mit 64 Fäden rechnet jeder
  4 der 256 Proben nacheinander. Mit 256 Fäden je Gruppe (f459fddf, eine Probe je Faden): Kern `zufall` 4,8 s für
  14433 Luxel (3,4-mal schneller je Luxel; md_ch_ben 0,145 → 0,053 s), der ganze Schritt mit Schalter aber
  immer noch langsamer als ohne (210,9 gegen 173,6 s, unter Last; die Erstabtastung der übernommenen Lightmaps
  kommt doppelt dazu) – Schalter bleibt aus.
- **md_am_sith mit 256 Fäden, alles auf der GPU** (`BHED_GPUBAKE_NUR_GPU`, `-bounce 2`, nach dem Neustart, unter
  Last): **alle 18 Lumps 448f3509fa84697b – gleich dem Wert aus 6d (Prozessor = GPU)**, 13095 Lightmaps mit
  Nachproben und 1 987 071 nachgetastete Luxel auf der GPU; erster Durchgang IlluminateRawLightmap 840 s (64 Fäden
  vor dem Neustart: 1432 s; alter Weg vor dem Neustart: 722 s), Kern `zufall` 377 s (etwa 1,35 Mio. Proben/s),
  Kern `erst` 238 s; der ganze Schritt 1281 s, davon das Lichtgitter allein auf der GPU 401 s (mit NUR_GPU ohne
  den Prozessor nebenher). Gleich danach der Vorgabeweg (GPU-Backen, die Nachproben rechnet der Prozessor
  nebenher, ohne Schalter): ganzer Schritt **603 s**, erster Durchgang **529 s**, Lichtgitter geteilt 46 s, Lumps
  ebenfalls 448f3509fa84697b. Auf md_am_sith ist der Prozessor für die Nachproben also weiter schneller
  (529 gegen 840 s) – „nimm das Schnellere“: Vorgabe bleibt der Prozessor, die GPU-Nachproben bleiben WIP.
- **Nächster Schritt**: md_am_sith und duel_mos_espa mit und ohne `BHED_GPUBAKE_ZUFALL` A/B messen; den Kern
  auslasten (Lightmaps mit Nachproben aus mehreren Blöcken sammeln statt je Block, alle neuen Luxel eines
  Stempels in einem Durchgang, 128/256 Fäden je Gruppe), Kernzeit mit `BHED_GPUBAKE_ZEITEN` („zufall“) gegen
  die Luxelzahl; erst wenn schneller: Schalter weg, Auto-Regel und Handbuch.

### 2. IlluminateVertexes besser verteilen (c1259468)

- Vorher war eine Vertexlicht-Fläche ein Arbeitsschritt; große Flächen (mehr Vertexe als der Anteil eines
  Threads) liefen vorab mit allen Threads nacheinander, während sonst nichts lief, und auf duel_mos_espa
  hielten einzelne Patches (225 Vertexe, 114 Lichter, dunkle Vertexe bis 125-mal verschoben) je 10 s einen
  Thread allein.
- Jetzt (`BhedVertexArbeit`): der Thread einer Fläche stellt ihre Vertexe in eine Liste; wer keine eigene
  Arbeit hat, nimmt einzelne Vertexe (je Thread ein Hilfs-Arbeitsschritt hinter den Flächen in
  RunThreadsOnIndividual, und der Besitzer, solange er auf die letzten Vertexe seiner Fläche wartet). Nur Flächen,
  deren Vertexe unabhängig sind (wie bisher: jedes Licht hat seinen Stilplatz, kein Dirt mit Zufallszahlen);
  Mittelwert und Rest in Vertexfolge. Die Fortschrittsanzeige zählt die Hilfsschritte mit (am Ende).
- Bytegleich: siehe Punkt 1 (`klein.sh`, CPU und GPU, mit und ohne `-randomsamples`); geteilte Flächen je
  Durchgang md_ch_ben 10, duel_pit 6, mp__duel1 16 (vorher 1, 0, 0).
- Messung duel_mos_espa `-light -fastbounce -patchshadows -samples 4 -randomsamples -bounce 1`, Prozessor,
  32 Threads, nach dem Neustart, **unter Last** (die anderen Agenten arbeiteten), je ein Lauf alt/neu:

| | alt | neu |
|---|---|---|
| Bounce: IlluminateVertexes | 33,0 s | **9,9 s** |
| erster Durchgang: IlluminateVertexes | 9,6 s | 12,7 s |
| unveränderte Abschnitte (Last-Vergleich): SetupTraceNodes, TraceGrid, IlluminateRawLightmap erster Durchgang / Bounce | 8,2 s, 3,8 s, 42,0 s / 78,8 s | 10,8 s, 7,3 s, 78,3 s / 112,5 s |
| ganzer `-light`-Schritt (Wand) | 221,9 s | 280,5 s |
| Lumps | 1a1f3e708921e3d9 | gleich |

Der Lauf „neu“ lag unter deutlich mehr Last (die unveränderten Abschnitte 1,3- bis 1,9-mal langsamer); trotzdem
ist IlluminateVertexes im Bounce 3,3-mal schneller (2382 Flächen geteilt, in beiden Durchgängen), im ersten
Durchgang unter dieser Last etwa gleich (nicht weiter untersucht) – Endmessung bei niedriger Last offen.

### 3. NRC-Absturz bei zweitem `-light` (48aba3bd)

- Nachgestellt (`SP/vis2/absturz.py`, dann ctest): raum.map mit Stil 1/5, `-game quake3`, `-meta`, `-vis`,
  `-light …`, dann noch einmal `-light` – 0xc0000005 schon bei `-light -fast` (rot im neuen Fall `relicht` von
  ctest q3map2_funde, auch mit `-randomsamples`).
- Ursache: der quake3-Stiltrick schreibt externe Lightmaps in eigene Shader; WriteBSPFileAfterLight stellt dann
  für „nomipmaps“ Flare-Flächen (MST_FLARE, 0 Vertexe, Shader `maps/<karte>/nomipmaps<n>`) vor alle Flächen und
  verschiebt Blatt-Flächen, Pinselseiten und Modelle. Das zweite `-light` liest diese BSP, die Flächennummern
  passen nicht mehr zur .srf (Fläche 0 bekommt die Angaben der ersten echten Fläche) → Absturz.
- Behebung (`BhedVorigesLichtEntfernen` nach LoadBSPFile): die vorangestellten Flares samt Verschiebungen
  zurücknehmen; die Shader, die ein Licht angelegt hat (`<karte>/<md4>` von CustomShader und die
  nomipmaps-Shader – immer am Ende der Liste), durch den eigenen Shader jeder Fläche aus der .srf ersetzen und
  die Liste kürzen (nur wenn .srf und Liste passen; sonst nur die Flares). Ergebnis: alle 17 Lumps (Entities ohne
  `_q3map2_cmdline`, den Schrittverlauf), die Shader-Datei und die externen Lightmaps gleich einem einzigen
  `-light` auf der Ausgabe von `-vis` – grün, mit und ohne `-randomsamples`.

### 4.–6. Nicht erreicht (Zeitgrenze)

- Punkt 4 (Profil des vollen „[Final] Extreme Quality“ auf duel_mos_espa, md_am_sith, taspir1, kejim_post):
  nicht begonnen.
- Punkt 5: Messtabelle nur wie oben (Vertexe); keine Endmessung bei niedriger Last.
- Punkt 6 (Schlussrunde): nach dem Neustart: Bau bhed_q3map2 0 Fehler/0 Warnungen (nur das Ziel
  bhed_q3map2_clang und kartegrenzen neu gebaut, kein sauberer Neubau des Ganzen), ctest 159/159 (90 s, -j 4,
  auf dem Stand 3c542830), danach q3map2_gpubake/q3map2_determinismus/q3map2_funde mit f459fddf grün;
  lint_sources ok, lint_fassung ok (rc606), lint_cbuffer ok. Nicht gelaufen: sauberer Neubau, Selbsttest
  karteneditor (kurz, stufe9d), Modus 1, Kartenprüfung, Bytevergleiche auf kejim_post/taspir1/großen MD-Karten
  (nur md_ch_ben, duel_pit, mp__duel1, t1_inter, duel_mos_espa). Außerhalb von q3map2 geändert: nur der Text der
  Prüfstelle in grenzen.cpp/grenzen.json (ctest karte_grenzen grün).
- Handbuch: keine Bedienänderung (der GPU-Schalter ist nur eine Umgebungsvariable, WIP).

## vis3: Rest von vis2 (Nachauftrag, 30.10. morgens)

Auftrag `SP/auftrag_vis3.md`. Harte Zeitgrenze 08:40. Werkzeuge `SP/vis3`: `b1.sh` (Bytevergleich alt CPU gegen
neu CPU/GPU/GPU mit Schalter, Profil „[Final] Extreme Quality“ mit `-bounce 2` und das schnelle Profil, über
`SP/vis2/lz.py`), `st3.py`/`kp3.sh` (Selbsttest und Kartenprüfung mit den eingefrorenen Dateien des sauberen
Neubaus in `Y:/vis/laeufe/frei`), `sith.sh`, `stempel_patch.py`, `anteil_patch.py`, `warte.py`. „alt“ =
`SP/vis2/bin/a0.exe` (24bbef3c, vis1 fertig); n1 = sauberer Neubau von 214c4ab2 (Code f459fddf); s2 = 2e29643a; s3 = 874121ef.
**Alle Zeiten unter Last** (andere Agenten bauten und testeten, dazu eigene Läufe nebenher wie angegeben).

### Commits vis3

| Commit | Inhalt |
|---|---|
| 3f40c7e0 | WIP Punkt 2: Kern `zufall` rechnet alle neuen Luxel eines Stempels in einem Durchgang – gleiche Bytes |
| 2e29643a | WIP Punkt 2: Anteil der GPU an den Lightmaps mit Nachproben nach gemessenen Raten statt fest ein Drittel – gleiche Bytes |
| 874121ef | Punkt 3: SetupSurfaceLightmaps – der erschöpfende Merge geht nur die späteren Flächen mit denselben festen Merkmalen durch – gleiche Bytes |
| (dieser) | Notizen vis3 |

### 1. Schlussrunde von vis2 (Stand 214c4ab2 = Code f459fddf)

| Schritt | Ergebnis |
|---|---|
| Neubau (`--clean-first`, `--parallel 4`) | 0 Fehler, 0 Compiler-Warnungen (858 s, nebenher die Bytevergleiche taspir1/kejim_post); einmal MSB8064 an ZERO_CHECK wie bisher |
| ctest | **159 / 159** (119 s, `-j 4`, nebenher der Vergleich kejim_post); nach 3f40c7e0/2e29643a noch einmal q3map2_gpubake, q3map2_determinismus, q3map2_funde, karte_grenzen: grün |
| Selbsttest karteneditor kurz (`BHED_KARTE_NUR=fotos`) | **78 OK / 0 FEHLER** (38 s) |
| Selbsttest karteneditor `BHED_KARTE_NUR=stufe9d` | **28 OK / 0 FEHLER** (44 s) |
| Selbsttest Modus 1 | **2645 OK / 0 FEHLER** (467 s; dieselben 18 Protokollzeilen „Feld 0 zeigt Reiter …“ wie im Lauf vor der Pause) |
| Kartenprüfung (278 Karten, gegen rc606) | exit 0 (1360 s, unter Last), **alle Summen gleich**: geprüft 278, abgestürzt 0, nicht geladen 0, GPU-Fehler 0, D3D 0, Protokollfehler 0, nicht gezeichnete Entities 1, NPC ohne Modell 2, fehlende .efx 27, fehlender Klang 29, fehlende Textur 139, fehlendes Modell 5 |
| Lints | lint_sources ok, lint_fassung ok (rc606), lint_cbuffer ok |
| Bytevergleiche | siehe unten – alle 18 Lumps überall gleich |
| Restprozesse/-dateien | keine eigenen Prozesse; Läufe unter `Y:/vis/laeufe` (von git ausgeschlossen), `SP/vis2/lz`, `SP/vis3`; Spielordner nur gelesen |

Selbsttests und Kartenprüfung liefen mit den eingefrorenen Dateien des Neubaus (`laeufe/frei`), damit der
Weiterbau an bhed_q3map2 (Punkt 2) sie nicht ändert.

**Bytevergleiche** (Hash aller 18 Lumps; Eingabe je Karte dieselbe `-meta`/`-vis -fast`-BSP; „Extreme“ =
`-light -fastbounce -patchshadows -samples 4 -randomsamples -bounce N`, „schnell“ = `-light -fast -patchshadows
-samples 2 -filter -bounce 2`; GPU = `-bhedgpubake`, Schalter = `BHED_GPUBAKE_ZUFALL=1`, nur GPU =
`BHED_GPUBAKE_NUR_GPU=1`):

| Karte | Profil | Hash | gleich bei |
|---|---|---|---|
| taspir1 (MD) | Extreme, N = 2 | 89c381abf3d17268 | alt CPU, n0 CPU, n0 GPU, n0 GPU + Schalter (3536 Lightmaps / 340609 Luxel mit Nachproben auf der GPU), s2 GPU, s2 GPU + Schalter |
| taspir1 | schnell | 06c3eb7e6e0e6384 | alt CPU, n0 CPU, n0 GPU |
| kejim_post (JK2) | Extreme, N = 2 | de898d504b1c0795 | alt CPU, n0 CPU, n0 GPU, n0 GPU + Schalter (8804 / 523725), s2 GPU, s2 GPU + Schalter (3183 / 139625) |
| kejim_post | schnell | 2b3544f0b4661e3c | alt CPU, n0 CPU, n0 GPU |
| duel_mos_espa (MD) | Extreme, N = 1 | 1a1f3e708921e3d9 | n1 CPU, n1 GPU, n1 GPU + Schalter, s2 GPU + Schalter (gleich dem Wert aus 6d/vis2) |
| md_am_sith (MD) | Extreme, N = 2 | 448f3509fa84697b | s2 CPU, s2 GPU + Schalter (7033 Lightmaps / 693990 Luxel mit Nachproben auf der GPU), s2 GPU Vorgabe; gleich dem Wert aus 6d/vis2 (Vorgabe-GPU, nur GPU) |
| md_ch_ben, duel_pit, mp__duel1 | Extreme, N = 2 | 6f84669d794e2765, 1bf84e5eac04f19d, d7f8bc9615577d73 | alt CPU, n1/s1 nur GPU (mit dem neuen Kern), s1/s2 GPU + Schalter, s2 CPU (duel_pit, mp__duel1) |

n0 = die exe vor dem Neubau (gebaut 05:26 aus f459fddf, dieselben Quellen wie n1); jede nachgeprüfte
GPU-Lightmap 100 % gleich.

### 2. GPU-Nachproben besser auslasten (WIP, Schalter bleibt aus)

- **Ganze Stempel je Durchgang (3f40c7e0)**: der Kern `zufall` rechnete die bis zu 4 neuen Luxel eines Stempels
  nacheinander (je Luxel 256 Proben auf 256 Fäden, zwei Gruppen-Syncs, die Summe je Farbkanal seriell auf einem
  Faden). Jetzt alle Proben des Stempels in einem Durchgang (Probe j: Luxel j / lightSamples, je Faden bis zu 4),
  die Summen je Luxel und Kanal auf eigenen Fäden, in Probenfolge (dieselbe float-Reihenfolge). Die Luxel eines
  Stempels sind unabhängig (ihre Generator-Zustände kommen aus dem Lauf, SubmapRawLuxel/LightContributionToSample
  lesen nur feste Daten). Kein `return`/`continue` im Leiter-Faden. Bytes gleich (md_ch_ben, duel_pit nur GPU,
  ctest q3map2_gpubake). **Kernzeit kaum besser**: duel_pit 1,611 → 1,587 s (43803 Luxel), md_ch_ben 0,051 →
  0,048 s. Der Kern ist latenzgebunden: je Gruppe etwa 10 ms je nachgetastetem Luxel (md_am_sith nur GPU, vis2:
  1,99 Mio. Luxel in 377 s bei etwa 50 Gruppen je Block; duel_pit: 414 Gruppen, 43803 Luxel in 1,6 s) – jeder
  Faden verfolgt seine Strahlen weiter nacheinander, nur die Syncs fallen weg.
- **Über Blöcke sammeln**: der Durchsatz wächst mit der Zahl gleichzeitiger Gruppen (duel_pit 27000 Luxel/s mit
  414 Gruppen gegen md_am_sith 5300 Luxel/s mit etwa 50). Lightmaps mehrerer Blöcke gemeinsam im Kern `zufall`
  hieße, die Paar-Puffer (P/F) mehrerer Blöcke auf der GPU zu halten und das Filtern dieser Lightmaps bis nach
  dem gemeinsamen Durchgang aufzuschieben (der Block-Ablauf `BhedBakeRest` filtert heute alle Lightmaps des Blocks
  sofort) – in der Zeit nicht begonnen. Der Weg mit Schalter sammelt schon über Blöcke: ein übernommener Stapel
  ist ein eigener Block nur aus Lightmaps mit Nachproben (taspir1 3536, kejim_post 8804 Lightmaps in wenigen
  Dispatches), kostet aber eine zweite Erstabtastung dieser Lightmaps auf der GPU.
- **Anteil nach Messung (2e29643a)**: mit Schalter nahm die GPU je Stapel fest ein Drittel der restlichen Arbeit;
  die Fäden liefen leer, während die GPU noch am Stapel rechnete. duel_mos_espa (`-bounce 1`, unter Last):
  Bounce IlluminateRawLightmap mit dem Drittel **331,8 s**, ganzer Schritt 447,6 s. Jetzt ist der erste Stapel
  klein (1/16), danach nimmt die GPU den Anteil, den beide Seiten nach den Raten des vorigen Stapels gleichzeitig
  fertig bekommen (Arbeit der GPU gegen die Arbeit, die die Fäden inzwischen genommen haben, 1/64 bis 1/2): Bounce
  **89,3 s**, ganzer Schritt 231,5 s. Nur die Verteilung ändert sich (jede Lightmap rechnet genau eine Seite,
  beide exakt) – Bytes gleich (duel_pit, mp__duel1, duel_mos_espa, kejim_post, taspir1).
- **Ergebnis**: mit Schalter auf duel_mos_espa weiter langsamer als die Vorgabe (Bounce 89,3 gegen 75,4 s, ganzer
  Schritt 231,5 gegen 219,3 s, Läufe direkt nacheinander unter Last) und auf md_am_sith nicht fair gemessen: mit Schalter 1144,2 s, erster Durchgang IlluminateRawLightmap 769,5 s
  (Kern `zufall` 108,7 s, `erst` 299,9 s) gegen die CPU allein 1155,8 / 864,8 s, beide unter schwerer Last;
  Vorgabe-GPU 998,4 s (erster Durchgang 913,3 s, Bounces 13,0 + 5,0 s), mit Schalter der erste Durchgang
  schneller (769,5 s), die Bounces aber 39,8 + 49,1 s – unter wechselnder schwerer Last kein Urteil. Auf den
  JA/JK2-Karten mit vielen kleinen Lightmaps war der Schalter (noch mit dem Drittel, n0) schneller als die
  Vorgabe-GPU: kejim_post 36,5 gegen 61,9 s (CPU 50,1 s), taspir1 96,1 gegen 120,1 s (CPU 89,0 s) – aber nur auf
  kejim_post schneller als die CPU, und die Läufe lagen unter wechselnder Last (Neubau nebenher).
  Mit s2 (Raten) unter schwerer Last, Paare direkt nacheinander: kejim_post Schalter 128,7 gegen Vorgabe-GPU 343,5 s,
  taspir1 202,1 gegen 258,2 s – die Richtung bleibt, die Zeiten sind nicht mit der CPU vergleichbar.
  **„Nur wenn schneller als die CPU“**: nicht auf den großen MD-Karten, für die „[Final] Extreme Quality“ zählt –
  der Schalter bleibt ab Werk aus, die Auto-Regel (gpuwahl.h) bleibt unverändert, Handbuch unverändert (keine
  Bedienänderung).

### 3. „[Final] Extreme Quality“ voll – größter verbliebener Abschnitt

- Profil des vollen Schritts (duel_mos_espa `-bounce 8`, CPU, aus vis1 `SP/vis1/lt/duel_mos_espa/cpu10b8`,
  Stand 51f0c4a6, Summe der `BHED_Q3_ZEITEN`-Abschnitte 551 s): IlluminateRawLightmap 270 s, IlluminateVertexes
  155 s, „BSP/Karte laden, Setup*“ 62 s, StoreSurfaceLightmaps 26 s, RadCreateDiffuseLights 14 s, SetupTraceNodes
  10 s. IlluminateVertexes ist seit c1259468 (vis2) je Bounce 33 → 10 s (Rest etwa 9 × 10 s). Größter
  verbliebener Abschnitt damit IlluminateRawLightmap, und darin die Lightmaps mit Nachproben auf dem Prozessor
  (mos_espa je Bounce 6700–7300, dazu etwa 3750 mit Alpha-Schatten/unsicher; md_am_sith 13131 Lightmaps im ersten
  Durchgang, 16000 Thread-s, auf 32 Threads gut verteilt – kein Gewinn durch Verteilung, nur durch die Kosten je
  Probe oder die GPU, siehe Punkt 2).
- duel_mos_espa `-bounce 1` mit GPU (s2, unter Last): Laden/Setup 49 s, SetupTraceNodes 7,6 s, Gitter 7 s,
  IlluminateRawLightmap 41 s, IlluminateVertexes 10 s, Bounce IlluminateRawLightmap 89 s (Vorgabe 75 s),
  IlluminateVertexes 10 s, Store 8 s; GPU insgesamt nur 53 s beschäftigt (166 Blöcke).
- **Laden/Setup aufgeschlüsselt** (Messfassungen m1/m2 mit weiteren `BhedZeit`-Marken, nicht eingecheckt;
  duel_mos_espa, unter Last): LoadSurfaceExtraFile (.srf lesen) 16 s, LoadBSPFile 0,3 s, LoadMapFile 1,6 s,
  SetupBrushes 0,2 s, **SetupSurfaceLightmaps 73 s** – darin die Flächeninfos samt Cluster 8,5 s (mit dem
  Laden davor), Sortieren 0,02 s, **erschöpfender Merge + FinishRawLightmap 22,8 s**. Der Merge gab für jede
  neue Raw-Lightmap jede spätere Fläche an AddSurfaceToRawLightmap (O(n²), wiederholt, solange etwas dazukam).
- **Umgesetzt (874121ef, bitgleich)**: eine Fläche, deren feste Merkmale (Samplegröße, Umgebungsfarbe,
  Entity, Schattenempfang, eigene Lightmap-Größe, Helligkeit, Filterradius, splotchFix) von denen der
  Raw-Lightmap abweichen, lehnt AddSurfaceToRawLightmap ab, bevor es etwas ändert (der Aufrufer gibt den
  Platz in lightSurfaces zurück); die Merkmale der Raw-Lightmap setzt nur ihr Start. Darum geht der Merge nur
  die späteren Flächen mit demselben Schlüssel durch (Tafel Schlüssel → Positionen in der Sortierung, aufsteigend)
  – dieselben Aufrufe in derselben Reihenfolge. floats im Schlüssel: 0 und -0 gleich (NaN passt nie und wird
  wie bisher abgelehnt). Einziger Unterschied: der Wert hinter dem letzten belegten Platz von lightSurfaces
  (nie gelesen – alle Leser bleiben in `firstLightSurface … + numLightSurfaces`).
- Laden/Setup duel_mos_espa (`BHED-Zeit light: BSP/Karte laden, Setup*`, abwechselnd, **unter schwerer Last** –
  die anderen Agenten übersetzten mit normaler Priorität, meine Läufe idle): s2 über 160 s (abgebrochen) /
  119,2 s, s3 93,6 / 48,5 s; **bei niedrigerer Last (08:16, abwechselnd) s2 25,0 / 22,8 s, s3 22,6 / 21,6 s** – ohne
  schwere Last also nur 1–2 s (5–10 %): die 22,8 s des Merge oben waren zum großen Teil Last. Bytes gleich (alle 18
  Lumps, s2 gegen s3): md_ch_ben 6552431bbafc6d79, duel_pit 04401b6d9323b064,
  mp__duel1 64095e6df6fbd3f8, t1_inter 7064907cdcf03b00 (schnelles Profil, CPU), duel_mos_espa e48a5464e2380376
  (schnelles Profil, GPU; 661,5† gegen 716,0† s); kejim_post 2b3544f0b4661e3c, taspir1 06c3eb7e6e0e6384 (schnelles Profil, CPU);
  ctest q3map2_* (14 Tests), karte_grenzen, karte_bau grün.
- **Nicht angegangen** (Zeitgrenze): ein bitgleicher schnellerer Weg für RandomSubsampleRawLuxel auf dem
  Prozessor (je Probe SubmapRawLuxel + LightContributionToSample mit Schattenstrahl, der größte Abschnitt),
  LoadSurfaceExtraFile (16 s unter Last, einfädig). Ein voller Lauf von „[Final] Extreme Quality“ auf md_am_sith
  wurde nicht gemessen.

### 4. Messtabelle vis3

Wand des ganzen `-light`-Schritts in s, 32 Threads, RTX 4070 Laptop. **Alles unter Last**; ab etwa 07:10
schwere Last (andere Agenten übersetzten mit normaler Priorität, meine Läufe liefen idle, dazu eigene Läufe
nebenher) – diese Werte (†) taugen nur für Bytes, nicht für Zeiten.

| Karte | Profil | alt CPU | neu CPU | GPU Vorgabe | GPU + Schalter, Drittel (n0/n1) | GPU + Schalter, Raten (s2) |
|---|---|---|---|---|---|---|
| taspir1 | Extreme, N = 2 | 94,0 | 89,0 | 120,1 (n0); 258,2† (s2) | 96,1 | 202,1† |
| kejim_post | Extreme, N = 2 | 61,9 | 50,1 | 61,9 (n0); 343,5† (s2) | 36,5 | 128,7† |
| duel_mos_espa | Extreme, N = 1 | – | 413,9† (n1) | 219,3 (n1) | 447,6 | 231,5 |
| md_am_sith | Extreme, N = 2 | – | 1155,8† (s2) | 998,4† (s2) | – | 1144,2† |
| taspir1 | schnell | 21,0 | 25,3 | 27,3 | – | – |
| kejim_post | schnell | 37,3 | 33,3 | 26,9 | – | – |

duel_mos_espa `-bounce 1`, Abschnitte (direkt nacheinander, unter Last): Bounce IlluminateRawLightmap Vorgabe
75,4 s, Schalter mit Raten 89,3 s, Schalter mit Drittel 331,8 s; erster Durchgang 40,9 / 41,1 / 36,3 s;
IlluminateVertexes je Durchgang 7–10 s (vis2: Bounce 33 s vor c1259468).

Kern `zufall` (nur GPU, `BHED_GPUBAKE_ZEITEN`): duel_pit 1,611 → 1,587 s, md_ch_ben 0,051 → 0,048 s (3f40c7e0).
Laden/Setup duel_mos_espa: s2 >160† / 119,2† s, s3 93,6† / 48,5† s; bei niedrigerer Last s2 25,0 / 22,8 s, s3 22,6 / 21,6 s (874121ef).

### Offen nach vis3

- GPU-Nachproben: über Blöcke sammeln (Paar-Puffer mehrerer Blöcke halten, Filtern aufschieben) – der Kern ist
  latenzgebunden, nur mehr gleichzeitige Gruppen helfen; danach A/B ohne Last auf md_am_sith, duel_mos_espa,
  kejim_post, taspir1. Auf kejim_post war der Schalter vor der schweren Last schneller als CPU und GPU – ein
  dritter Kandidat für Auto (CPU, GPU, GPU + Nachproben, das Schnellste nach Messung) wäre der Weg, ihn ohne
  Risiko für die großen MD-Karten zu nutzen; nicht begonnen.
- RandomSubsampleRawLuxel auf dem Prozessor (größter Abschnitt von „[Final] Extreme Quality“), LoadSurfaceExtraFile.
- Zeiten ohne Last für alle vis3-Zahlen (die Läufe nach 07:10 lagen unter schwerer Last).
- Die Punkte aus „Offen“ und „vis2 → Nächster Schritt“ oben, soweit hier nicht erledigt.

## vis4: Licht schneller, bitgleich (Nachauftrag, 30.10. mittags und nachmittags) – angehalten (16:30)

Auftrag `SP/auftrag_vis4.md`. Um 13:10 vom Koordinator angehalten (höchstens 4 Agenten), ab 14:00 fortgesetzt.
Werkzeuge `SP/vis4`: `lz.py` (wie vis2, Bericht mit „in N runs“, `+budget<N>`, `+E_<NAME>`, `+auto`, Ausgabe nach
`LZ_AUS`), `laden.py` (nur Laden/Setup von `-light`), `tabelle.py` (Tabelle aus den lz-Zeilen, mit Auto-Wahl),
`messung.sh` (ruhige Endmessung, siehe unten), `st4.py` (Selbsttest wie `SP/vis3/st3.py`, Quelle aus `ST_QUELLE`),
`rep.py`, Messfassungen `bin/m1..m3.exe` (rdtsc-Zähler, nicht eingecheckt). „v0“ = `SP/vis4/bin/v0.exe` = s3
(874121ef, vor vis4), „c1“ = da96abce, „u1“ = dd8737f7 mit einem Test-Schalter für Punkt 3c.
**Alle Zeiten unter Last** (andere Agenten bauten und testeten, 15–45 % Last) – nur Tendenz; A/B wo angegeben
direkt nacheinander.

### Commits vis4

| Commit | Inhalt |
|---|---|
| 642dd15a | Punkt 1: Lightmaps mit Nachproben über mehrere Blöcke gesammelt (Sammelpuffer auf der GPU, `BhedBakeSammeln`/`BhedBakeSammelRechnen`), S mit `-randomsamples` nicht mehr angelegt – gleiche Bytes |
| 08b08e5f | Punkt 3: TraceLine testet vier Dreiecke auf einmal (SSE2), Nachproben enden am ersten undurchsichtigen Dreieck – gleiche Bytes |
| 4083fce7 | Punkt 3: FinishShader sucht stFlat vom letzten Pixel rückwärts; Punkt 1: Stapel aus dem Sammelpuffer nach gemessenen Raten – gleiche Bytes |
| db35d51e | Notizen beim Anhalten |
| da96abce | Punkt 3: ClusterForPointExt prüft nur die Brushes, deren Box (aus ihren achsparallelen Seiten) den Punkt hält – CPU vier auf einmal, GPU-Kern ebenso – gleiche Bytes |
| dd8737f7 | Punkt 2: Auto mit drittem Kandidaten GPU + Nachproben; Punkt 3c: mit `-randomsamples` nehmen freie Threads wieder GPU-Lightmaps; Tests, i18n, Handbuch |
| d79bbd25 | Test q3map2_gpubake: mit Threads nebenher bekommt der Sammelpuffer auf der kleinen Karte oft nichts |
| (dieser) | Notizen vis4, Changelog |

### 1. GPU-Nachproben über Blöcke sammeln (Sammelpuffer)

- Umsetzung (642dd15a, 4083fce7, vor der Pause): die Farben der Erstabtastung der aktiven Lichter jeder Lightmap mit
  Nachproben gehen per CopySubresourceRegion in einen Sammelpuffer (größte Zweierpotenz bis zum Blockbudget, 16 Byte
  je Paar – so viel, wie das nicht mehr angelegte S frei macht); ein Lauf baut daraus einen eigenen Block (nur die
  aktiven Lichter, `paarBase` = Stelle im Puffer), Vorhersage, Kern `zufall`, Filtern wie bisher. Mit Schalter steht
  jede gesammelte auch in `nebenZufall` (wer zuerst nimmt, rechnet); kein zweites Erst mehr. Voller Puffer und die
  Stapel nach den Blöcken nehmen den Anteil nach Raten (`sammelStapel`, erst 1/16), der Rest bleibt bei den Threads.
- Kern `zufall` (nur GPU, `BHED_GPUBAKE_ZEITEN`): kejim_post 41,3 → 23,9 s (45 Blöcke → 5 Läufe), md_am_sith
  377 (vis2) → 208 s (26 Läufe).
- Test (rot/grün): ctest q3map2_gpubake prüft jetzt den Sammelpuffer – nur GPU mit `BHED_GPUBAKE_BUDGET_MB=1`:
  20 Lightmaps mit Nachproben (998 Luxel) in 3 Läufen, Lumps gleich der CPU; mit Schalter neben den Threads: Lumps
  gleich (die Threads nehmen auf der kleinen Karte oft alle selbst). Rot mit v0 (kein „in N runs“), grün mit dd8737f7.
- Ergebnis (c1, unter Last, ganzer `-light`-Schritt): GPU + Schalter schneller als GPU auf taspir1 (61,9 gegen 68,2 s)
  und kejim_post (33,3 gegen 36,1 s), langsamer auf duel_mos_espa `-bounce 1` (179,3 gegen 150,4 s); auf md_am_sith
  `-bounce 2` schneller als die GPU (437,2 gegen 493,4 s), aber die CPU allein ist dort schneller (399,2 s).
  **Der Schalter bleibt ab Werk aus** (Punkt 5: auf duel_mos_espa langsamer) – Auto entscheidet (Punkt 2).

### 2. Auto mit drittem Kandidaten (dd8737f7)

- `bhed/gpuwahl.h`: mit `-randomsamples` gibt es „GPU + Nachproben“. Regel 3a: sind CPU- und GPU-Zeit gemerkt, die GPU
  schneller und die Zeit mit Nachproben unbekannt, misst Auto sie einmal (`measuregpuz`); 3b: sind alle drei gemerkt,
  nimmt Auto sie nur, wenn sie die schnellste ist (`learnedgpuz`), sonst wie bisher die schnellere von CPU und GPU.
  Wo die CPU gewinnt (duel_mos_espa, md_am_sith mit dem schnelleren Prozessor-Weg), wird sie nie gemessen – kein Lauf
  kostet dort etwas; auf taspir1/kejim_post kostet das Lernen nichts (sie ist dort schneller).
- bhed_q3map2: `-bhedgpubakezeitenz <ms>`, `g_bhedBakeZufall` (Umgebung `BHED_GPUBAKE_ZUFALL` oder Auto), Zeile
  „BehavEd GPU bake auto: GPU+subsamples (measuregpuz) … last GPU+subsamples N ms“.
- Editor: dritte gemerkte Zeit (`GpuZeiten::gpuzMs`, fünftes Feld in `karte_gpuzeiten.txt`, alte Dateien lesen sich
  weiter), `GpuWeg` statt bool, Wahltext „Grafikkarte mit Nachproben“ mit den Gründen (`tools/i18n_vis1.py`),
  Handbuch „Karten-Editor: GPU-Backen“ ein Absatz in vier Sprachen (keine Bildänderung: nur die Konsole zeigt es).
- Tests: ctest karte_gpuwahl (Tabelle mit den vis4-Zahlen, Speicher, Befehl, Ausgabezeile – rot: kompiliert gegen den
  alten Stand nicht), q3map2_gpubake (Auto mit `-randomsamples`: measuregpuz, learnedgpuz, learnedgpu, Lumps gleich
  der CPU – rot mit v0: 3 Fehler), Selbsttest stufe8g (Build > [Final] Extreme Quality zweimal: erst CPU, dann mit der
  GPU als schneller gemerkt „Grafikkarte mit Nachproben“, Lumps gleich dem CPU-Lauf, gemerkt 801/401/692 ms).

### 3. CPU-Seite

- 3a (vor der Pause, 08b08e5f, 4083fce7): TraceLine vier Dreiecke auf einmal, Nachproben enden am ersten
  undurchsichtigen Dreieck; FinishShader rückwärts (LoadSurfaceExtraFile: 99 % in ShaderInfoForShader). CPU-Sekunden
  duel_pit Extreme 45 → 28,5 s, taspir1 Wand 143 → 102 s.
- 3b ClusterForPointExt (da96abce): je Blatt die undurchsichtigen Brushes, die es prüft (b ≤ maxOpaqueBrush,
  opaqueBrushes), mit einer Box aus ihren achsparallelen Seiten. Für eine Seite mit Normale ±e_a ist der Abstand eines
  endlichen Punkts genau ±p[a] − dist (Produkte mit 0 und 1 exakt, die double-Differenz zweier floats auch), also
  verwirft auch die genaue Prüfung jeden Brush, dessen Box (dist + 0,5) den Punkt nicht hält, für jedes ε ≤ 0,25
  (größtes der Aufrufer). Ohne achsparallele Seite bleibt die Achse offen; nicht endliche Punkte und größeres ε gehen
  den alten Weg. CPU: vier Boxen je SSE2-Vergleich, die genaue Prüfung nur für die übrigen in Blattfolge; GPU-Kern
  `clusterExt` dieselbe Box je Brush. Gezählt (Messfassung, taspir1 Extreme): 234 Mio. Aufrufe, 13,1 Mrd.
  Blatt-Einträge (56 je Aufruf), nur 105 Mio. genaue Prüfungen (0,45 je Aufruf). Zeit: taspir1 CPU-s 2035 → 2003
  (je ein Lauf, unter Last) – die 532 Gtick der Messfassung m3 waren zum großen Teil Messaufwand; kleiner Gewinn.
- 3c Verteilung (dd8737f7): mit `-randomsamples` nahmen die Threads keine GPU-Lightmap (vis1: die Nachproben auf dem
  Prozessor waren mehr Arbeit als die ganze GPU-Liste). Mit dem schnelleren Prozessor-Weg warteten sie auf die GPU,
  die ihre Lightmaps erst finden muss – der GPU-Weg war langsamer als die CPU allein (md_am_sith 493,4 gegen 399,2 s,
  duel_mos_espa 150,4 gegen 121,9 s). Jetzt nehmen freie Threads wieder GPU-Lightmaps vom Ende der Liste (weiter
  zuletzt, nach den eigenen). A/B (u1, Test-Schalter, direkt nacheinander, unter Last): taspir1 76,0 → 65,5 s,
  kejim_post 41,6 → 40,5 s, duel_mos_espa 124,5/125,7 → 130,7/113,3 s (Reihenfolge getauscht, im Mittel 125,1 →
  122,0 s), md_am_sith 493,4 (c1) → 409,5 s. Bytes überall gleich.
- LoadSurfaceExtraFile: nach 3a nicht weiter angegangen – Laden/Setup duel_mos_espa jetzt 23,7–25,8 s (c1, unter
  Last; darin SetupSurfaceLightmaps, siehe vis3), kejim_post 1,8 s.

### 4. Bytevergleiche (Hash aller 18 Lumps)

Profile wie vis3: X2 = `-light -fastbounce -patchshadows -samples 4 -randomsamples -bounce 2`, X1 dasselbe mit
`-bounce 1`, F2 = `-light -fast -patchshadows -samples 2 -filter -bounce 2`.

| Karte | Profil | Hash | gleich bei (alle Läufe dieses Abschnitts) |
|---|---|---|---|
| taspir1 | X2 | 89c381abf3d17268 | t4 (db35d51e) CPU, c1 CPU/GPU/GPU + Schalter, u1 GPU/GPU + Threads nehmen/dazu Schalter (= vis3) |
| kejim_post | X2 | de898d504b1c0795 | c1 CPU/GPU/GPU + Schalter (auch mit `BHED_GPUBAKE_ZEITEN`), u1 GPU/Threads nehmen/dazu Schalter (= vis3) |
| duel_mos_espa | X1 | 1a1f3e708921e3d9 | c1 CPU/GPU/GPU + Schalter (580 Lightmaps / 15902 Luxel in 3 Läufen auf der GPU), u1 GPU/Threads nehmen (zweimal) – **neu in vis4: CPU und Schalter** (= vis3) |
| md_am_sith | X2 | 448f3509fa84697b | c1 CPU, GPU, GPU + Schalter (6335 Lightmaps / 756358 Luxel in 18 Läufen auf der GPU), u1 Threads nehmen – **neu in vis4: CPU und Schalter** (= vis3) |
| duel_pit, md_ch_ben, mp__duel1 | X2 | 1bf84e5eac04f19d, 6f84669d794e2765, d7f8bc9615577d73 | t4 CPU, c1 CPU, nur GPU, GPU + Schalter (= vis3) |

Mit dem Endstand (e1 = Neubau von d79bbd25) noch nicht wiederholt – siehe Nächster Schritt.

### 5. Messtabelle vis4 (Wand des ganzen `-light`-Schritts in s, unter Last – nur Tendenz)

| Karte | Profil | CPU | GPU | GPU + Schalter | GPU, Threads nehmen GPU-Lightmaps (u1) |
|---|---|---|---|---|---|
| taspir1 | X2 | 86,6 | 68,2 / 76,0 | 61,9 | 65,5 (Schalter dazu 67,6) |
| kejim_post | X2 | 39,1 / 45,9 | 33,7 / 36,1 / 41,6 | 33,3 / 34,5† | 40,5 (Schalter dazu 37,3) |
| duel_mos_espa | X1 | 121,9 | 150,4 / 124,5 / 125,7 | 179,3 | 130,7 / 113,3 |
| md_am_sith | X2 | 399,2 | 493,4 | 437,2 | 409,5 |

† mit `BHED_GPUBAKE_ZEITEN`. vis3 zum Vergleich (schwerere Last): md_am_sith CPU 1155,8†, GPU 998,4†;
duel_mos_espa CPU 413,9†, GPU 219,3. Abschnitte c1 duel_mos_espa X1: erster Durchgang IlluminateRawLightmap CPU
12,8 / GPU 17,1 / Schalter 19,1 s, Bounce 48,9 / 72,1 / 92,8 s; md_am_sith X2 erster Durchgang CPU 327,1 / GPU 410,0 s
(GPU 216 s beschäftigt).

**Ruhige Endmessung** (eigener Schritt): `sh SP/vis4/messung.sh <neu.exe> [alt.exe] [ausgabe.txt]` – je Karte und
Weg eine Befehlszeile (`lz.py`: cpu, gpu, gpuz = GPU + Schalter, auto = `-bhedgpubakeauto` ohne gemerkte Zeiten,
alt = v0 auf dem Prozessor), am Ende die Tabelle (`tabelle.py`: Wand, CPU, Spitze, Hash, gleich der ersten Zeile der
Gruppe, Nachproben auf der GPU, Auto-Wahl). Dauer unter Last etwa 1 h (md_am_sith 5 × 400–500 s).

### 6. Schlussrunde vis4

Um 16:30 vom Koordinator erneut angehalten (shank: erst Karten- und Menü-Editor veröffentlichen; rc607 nimmt vis1
bis 6236b69f). Bis dahin:

| Schritt | Ergebnis |
|---|---|
| Neubau (`--clean-first`, `--parallel 4`) | 0 Fehler, 0 Compiler-Warnungen (9:38 min; einmal MSB8064 an ZERO_CHECK wie bisher). Der innere Ninja-Bau `build-vis/q3map2_clang` wird davon nicht gesäubert – eigens `--target clean` und neu: 461 Schritte samt fxc, 0 Warnungen (5:21 min) |
| ctest | **159 / 159** (79 s, `-j 4`); vorher gezielt q3map2_*, karte_gpuwahl, karte_grenzen, karte_bau 15 / 15 |
| q3map2_gpubake | grün (0 Fehler); mit v0 rot: 5 Fehler (Sammelpuffer zweimal, Auto mit `-randomsamples` dreimal) |
| Selbsttest stufe8g | 42 OK / 3 FEHLER – die neuen Auto-Schritte grün; die 3 FEHLER („Pack map as .pk3“ 1–2 Pixel außerhalb) ebenso mit `laeufe/frei` (vis3) |
| Lints | lint_sources ok, lint_fassung ok (rc606), lint_cbuffer ok, lint_feldinit ok, lint_headers ok, lint_i18n ohne Beanstandung der neuen Texte; lint_i18n_erzeugt scheitert schon vorher am Import der i18n_*.py in seiner Kopie – die Generatoren erzeugen die eingecheckten Dateien unverändert |
| Nicht gelaufen | Selbsttest karteneditor kurz (`BHED_KARTE_NUR=fotos`), stufe9d, Modus 1, Kartenprüfung, Bytevergleich mit dem Endstand |

### Offen nach vis4

- Ruhige Endmessung mit `SP/vis4/messung.sh` (alle Zeiten oben unter Last).
- GPU + Schalter auf duel_mos_espa langsamer: die GPU bleibt dort der Engpass, jeder Stapel des Sammelpuffers nimmt ihr
  Zeit für die eigene Liste; Auto misst den Schalter dort nie (die CPU ist schneller).
- LoadSurfaceExtraFile/SetupSurfaceLightmaps (Laden 24 s auf duel_mos_espa) und PointInLeafNum je Nachprobe
  (ab dem gemeinsamen Knoten der Probenbox) nicht weiter angegangen.
- Selbsttest stufe8g: „Pack map as .pk3“ im Build-Menü 1–2 Pixel außerhalb des Fensters (3 FEHLER im Tutorial der
  Stufe 8c) – ebenso mit dem eingefrorenen Stand von vis3 (`laeufe/frei`), also nicht aus vis4; vermutlich die
  Bildschirmhöhe dieser Sitzung.

### Nächster Schritt

1. Schlussrunde fertig: mit den eingefrorenen Dateien `Y:/vis/laeufe/frei4` (Neubau von d79bbd25, q3map2 auch als
   `SP/vis4/bin/e1.exe`): `ST_QUELLE='Y:/vis/laeufe/frei4' python -I SP/vis4/st4.py karteneditor <name> 1500
   BHED_KARTE_NUR=fotos` und `=stufe9d`, Modus 1 (`st4.py 1 <name> 1500`), Kartenprüfung `sh SP/vis4/kp4.sh` (gegen
   rc606). Vorher prüfen, ob seit d79bbd25 etwas geändert wurde – sonst neu bauen und einfrieren.
2. Bytevergleich mit dem Endstand: `sh SP/vis4/messung.sh <e1.exe>` (liefert zugleich alle Hashes je Weg); unter Last
   nur die Bytes werten, die ruhige Zeitmessung ist ein eigener Schritt.
3. Danach Notizen (Abschnitt 4–6 oben) ergänzen; Schalter bleibt ab Werk aus (auf duel_mos_espa langsamer).

## Changelog

## English

- bhed_q3map2 `-vis`, same result, faster and much leaner: the portal flow works only on the parts of its bit vectors that are not empty, and the passages keep only their non-empty part - 32 threads: kejim_post 551 → 214 s, taspir1 29 → 14 s, duel_mos_espa 140 → 91 s, with a third to a quarter of the memory; md_rev_TarisUpper2 needs 3.2 GB instead of more than 11 GB.
- New switch `-bhedvisexact` for `-vis`: with several threads exactly the visibility of `-threads 1`, byte for byte, in every build - for reproducible builds. It waits where a portal needs one that is not finished yet - on open maps much slower, so it is off by default; in the map editor Build > Same VIS in every build adds it to every `-vis` step.
- bhed_q3map2 `-light`, same result, much faster with bounce rounds and with `-randomsamples` ("[Final] Extreme Quality"): the culled light lists of the lightmaps and vertex lit surfaces come from a light tree instead of testing every light (a bounce of duel_mos_espa has 452184 lights), big lightmaps use all cores also with random samples, the threaded passes start with the biggest lightmaps and surfaces, the light envelopes are computed on all threads - duel_mos_espa (one bounce) 828 → about 250 s on the processor; its whole "[Final] Extreme Quality" light step (8 bounces) takes under 10 minutes.
- GPU bake also with `-randomsamples`: the graphics card computes every lightmap that needs no random supersampling, the processor the others at the same time; the processor also finishes the graphics card's lightmaps and shares the light grid with it. The same bytes as on the processor - duel_mos_espa (one bounce) 828 → about 220 s, light grid of md_am_sith 353 → 63 s. GPU bake Auto learns here too which is faster (on duel_mos_espa the processor).
- Map editor, build progress: the VIS progress counts the expected work of the portals instead of their number, so the remaining time of a first build is close to the real one; the memory hint before a full VIS estimates the compact passages.
- Fixed: bhed_q3map2 with more than 64 threads (`-threads`) wrote past its thread table.
- Fixed: a second `-light` on a quake3 BSP that was already lit with light styles crashed (also in NRC's q3map2): the light now takes back the "nomipmaps" surfaces and the shaders of the former light, and gives the same result as lighting the BSP once.
- bhed_q3map2 `-light`: the vertexes of a vertex lit surface are shared with idle threads instead of keeping one thread busy (same result).
- bhed_q3map2 `-light`: setting up the raw lightmaps compares a surface only with the later surfaces that can share its lightmap, instead of with all of them (same result) - on duel_mos_espa every light step starts 1-2 s earlier (much more when the computer is busy).
- bhed_q3map2 `-light`, same result, faster shadow tests: four triangles are tested at once, and a random subsample stops at the first opaque triangle; loading the shader images no longer walks every pixel of every image.
- bhed_q3map2 `-light`, same result: a point is tested only against the opaque brushes whose box holds it (four boxes at a time, also on the graphics card); with the graphics card and `-randomsamples`, idle processor threads take lightmaps from the graphics card again (taspir1 and md_am_sith clearly faster).
- GPU bake Auto with `-randomsamples` ("[Final] Extreme Quality"): a third choice, the graphics card computing the random subsamples too (gathered over several blocks). Auto measures it once only where the graphics card was faster than the processor and keeps it only where it was the fastest - faster on maps with many small lightmaps (taspir1, kejim_post), never taken where it is slower.

## Deutsch

- bhed_q3map2 `-vis`, gleiches Ergebnis, schneller und viel sparsamer: der Portalfluss rechnet nur auf den Teilen seiner Bitfelder, die nicht leer sind, und die Passagen behalten nur ihren nicht leeren Teil - 32 Threads: kejim_post 551 → 214 s, taspir1 29 → 14 s, duel_mos_espa 140 → 91 s, mit einem Drittel bis Viertel des Speichers; md_rev_TarisUpper2 braucht 3,2 GB statt über 11 GB.
- Neuer Schalter `-bhedvisexact` für `-vis`: mit mehreren Threads genau die Sichtbarkeit von `-threads 1`, Byte für Byte, in jedem Bau - für reproduzierbare Bauten. Er wartet, wo ein Portal ein noch nicht fertiges braucht - auf offenen Karten deutlich langsamer, darum ist er aus; im Karten-Editor hängt Build > Dasselbe VIS in jedem Bau ihn an jeden `-vis`-Schritt.
- bhed_q3map2 `-light`, gleiches Ergebnis, viel schneller mit Bounce-Runden und mit `-randomsamples` („[Final] Extreme Quality“): die Lichtlisten der Lightmaps und Vertexlicht-Flächen kommen aus einem Lichtbaum, statt jedes Licht zu prüfen (ein Bounce von duel_mos_espa hat 452184 Lichter), große Lightmaps nutzen auch mit Zufallsproben alle Kerne, die Thread-Durchgänge beginnen mit den größten Lightmaps und Flächen, die Licht-Hüllen rechnen alle Threads - duel_mos_espa (ein Bounce) 828 → etwa 250 s auf dem Prozessor; sein ganzer Lichtschritt von „[Final] Extreme Quality“ (8 Bounces) dauert unter 10 Minuten.
- GPU-Backen auch mit `-randomsamples`: die Grafikkarte rechnet jede Lightmap ohne zufällige Nachabtastung, der Prozessor gleichzeitig die anderen; außerdem stellt der Prozessor die Lightmaps der Grafikkarte fertig und teilt sich mit ihr das Lichtgitter. Dieselben Bytes wie auf dem Prozessor - duel_mos_espa (ein Bounce) 828 → etwa 220 s, Lichtgitter von md_am_sith 353 → 63 s. GPU-Backen Auto lernt auch hier, was schneller ist (auf duel_mos_espa der Prozessor).
- Karten-Editor, Bau-Fortschritt: der VIS-Fortschritt zählt die erwartete Arbeit der Portale statt ihrer Zahl, die Restzeit eines ersten Baus liegt damit nahe an der echten; der Speicherhinweis vor einem vollen VIS schätzt die kompakten Passagen.
- Behoben: bhed_q3map2 mit mehr als 64 Threads (`-threads`) schrieb über seine Thread-Tafel hinaus.
- Behoben: ein zweites `-light` auf einer schon mit Lichtstilen beleuchteten quake3-BSP stürzte ab (auch in NRCs q3map2): das Licht nimmt jetzt die „nomipmaps“-Flächen und die Shader des vorigen Lichts zurück und ergibt dasselbe wie einmal beleuchten.
- bhed_q3map2 `-light`: die Vertexe einer Vertexlicht-Fläche teilen sich freie Threads, statt einen Thread allein zu beschäftigen (gleiches Ergebnis).
- bhed_q3map2 `-light`: beim Anlegen der Raw-Lightmaps wird eine Fläche nur noch mit den späteren Flächen verglichen, die ihre Lightmap teilen können, statt mit allen (gleiches Ergebnis) - auf duel_mos_espa beginnt jeder Lichtschritt 1-2 s früher (bei ausgelastetem Rechner deutlich mehr).
- bhed_q3map2 `-light`, gleiches Ergebnis, schnellere Schattentests: vier Dreiecke werden auf einmal geprüft, und eine zufällige Nachprobe endet am ersten undurchsichtigen Dreieck; das Laden der Shader-Bilder geht nicht mehr jedes Pixel jedes Bildes durch.
- bhed_q3map2 `-light`, gleiches Ergebnis: ein Punkt wird nur gegen die undurchsichtigen Brushes geprüft, deren Box ihn hält (vier Boxen auf einmal, auch auf der Grafikkarte); mit der Grafikkarte und `-randomsamples` nehmen freie Prozessor-Threads wieder Lightmaps der Grafikkarte (taspir1 und md_am_sith deutlich schneller).
- GPU-Backen Auto mit `-randomsamples` („[Final] Extreme Quality“): eine dritte Wahl, die Grafikkarte rechnet auch die zufälligen Nachproben (über mehrere Blöcke gesammelt). Auto misst sie nur dort einmal, wo die Grafikkarte schneller als der Prozessor war, und behält sie nur, wo sie die schnellste war - schneller auf Karten mit vielen kleinen Lightmaps (taspir1, kejim_post), nie genommen, wo sie langsamer ist.

## 中文

- bhed_q3map2 `-vis`:结果相同,速度更快,内存占用大幅减少:门户流只处理位向量中非空的部分,通道只保留非空部分——32 线程:kejim_post 551 → 214 秒,taspir1 29 → 14 秒,duel_mos_espa 140 → 91 秒,内存只需原来的三分之一到四分之一;md_rev_TarisUpper2 只需 3.2 GB,而不是 11 GB 以上。
- `-vis` 的新开关 `-bhedvisexact`:使用多个线程时,每次编译都精确得到与 `-threads 1` 逐字节相同的可见性——用于可重复编译。当某个门户需要一个尚未完成的门户时它会等待——在开阔的地图上会慢很多,因此默认关闭;在地图编辑器中,Build > 每次编译得到相同的 VIS 会把它加到每个 `-vis` 步骤。
- bhed_q3map2 `-light`:结果相同,有反弹轮次和使用 `-randomsamples`(“[Final] Extreme Quality”)时快很多:光照贴图和顶点光照面的灯光列表来自灯光树,不再逐个测试所有灯光(duel_mos_espa 的一次反弹有 452184 盏灯),使用随机采样时大型光照贴图也使用所有核心,多线程阶段先处理最大的光照贴图和面,光照包络在所有线程上计算——duel_mos_espa(一次反弹)在处理器上 828 → 约 250 秒;它完整的“[Final] Extreme Quality”光照步骤(8 次反弹)不到 10 分钟。
- GPU 烘焙也支持 `-randomsamples`:显卡计算所有不需要随机超采样的光照贴图,处理器同时计算其余的;处理器还负责完成显卡的光照贴图,并与显卡分担光照网格。结果与处理器逐字节相同——duel_mos_espa(一次反弹)828 → 约 220 秒,md_am_sith 的光照网格 353 → 63 秒。GPU 烘焙“自动”在这里同样会学习哪一方更快(在 duel_mos_espa 上是处理器)。
- 地图编辑器,编译进度:VIS 进度按门户的预计工作量计算,而不是按门户数,因此第一次编译的剩余时间也接近实际时间;完整 VIS 之前的内存提示按紧凑通道估算。
- 修复:bhed_q3map2 使用超过 64 个线程(`-threads`)时会写出线程表的范围。
- 修复:对已用灯光样式照明过的 quake3 BSP 再执行一次 `-light` 会崩溃(NRC 的 q3map2 也是):现在照明会撤销上一次照明添加的“nomipmaps”面和着色器,结果与只照明一次相同。
- bhed_q3map2 `-light`:顶点光照面的顶点由空闲线程分担,不再让一个线程单独忙碌(结果相同)。
- bhed_q3map2 `-light`:建立原始光照贴图时,一个面只与之后可能共用其光照贴图的面比较,而不是与所有面比较(结果相同)——在 duel_mos_espa 上每个光照步骤提前 1-2 秒开始(电脑繁忙时提前得更多)。
- bhed_q3map2 `-light`:结果相同,阴影测试更快:一次测试四个三角形,随机子采样在遇到第一个不透明三角形时即停止;加载着色器图像时不再遍历每张图像的每个像素。
- bhed_q3map2 `-light`:结果相同:一个点只与包围盒包含它的不透明画刷比较(一次四个包围盒,显卡上也一样);使用显卡和 `-randomsamples` 时,空闲的处理器线程会再次接手显卡的光照贴图(taspir1 和 md_am_sith 明显更快)。
- 使用 `-randomsamples`(“[Final] Extreme Quality”)时 GPU 烘焙自动模式有第三种选择:显卡也计算随机子采样(跨多个块收集)。自动模式只在显卡比处理器快的地图上测量一次,并只在它最快的地图上继续使用——在光照贴图多而小的地图(taspir1、kejim_post)上更快,在更慢的地图上从不选用。

## 日本語

- bhed_q3map2 `-vis`:結果は同じで、より速く、メモリも大幅に少なくなりました。ポータルフローはビット列の空でない部分だけを処理し、パッセージは空でない部分だけを保持します。32 スレッド:kejim_post 551 → 214 秒、taspir1 29 → 14 秒、duel_mos_espa 140 → 91 秒で、メモリは 3 分の 1 から 4 分の 1 です。md_rev_TarisUpper2 は 11 GB 超ではなく 3.2 GB で済みます。
- `-vis` の新しいスイッチ `-bhedvisexact`:複数のスレッドでも、毎回のビルドで `-threads 1` とバイト単位で同じ可視性を計算します(再現可能なビルド用)。まだ完了していないポータルが必要なところでは待つため、開けたマップではかなり遅くなり、既定ではオフです。マップエディターの Build > 毎回のビルドで同じ VIS で、すべての `-vis` ステップに追加されます。
- bhed_q3map2 `-light`:結果は同じで、バウンスや `-randomsamples`(「[Final] Extreme Quality」)ではずっと速くなりました。ライトマップと頂点ライティングの面のライトのリストは、すべてのライトを調べる代わりにライトのツリーから作ります(duel_mos_espa のバウンス 1 回は 452184 個のライト)。ランダムサンプルでも大きなライトマップはすべてのコアを使い、スレッド処理は最大のライトマップと面から始まり、ライトのエンベロープは全スレッドで計算します。duel_mos_espa(バウンス 1 回)はプロセッサーで 828 → 約 250 秒、「[Final] Extreme Quality」のライト処理全体(バウンス 8 回)は 10 分未満です。
- GPU ベイクも `-randomsamples` に対応しました。ランダムなスーパーサンプリングが不要なライトマップはグラフィックカードが、それ以外は同時にプロセッサーが計算します。さらにプロセッサーはグラフィックカードのライトマップを仕上げ、ライトグリッドもグラフィックカードと分け合います。結果はプロセッサーとバイト単位で同じです。duel_mos_espa(バウンス 1 回)828 → 約 220 秒、md_am_sith のライトグリッド 353 → 63 秒。GPU ベイクの「自動」はここでもどちらが速いかを学習します(duel_mos_espa ではプロセッサー)。
- マップエディター、ビルドの進行状況:VIS の進行状況はポータルの数ではなく見込み作業量で数えるため、初回ビルドの残り時間も実際に近くなります。完全な VIS の前のメモリのヒントはコンパクトなパッセージで見積もります。
- 修正:bhed_q3map2 で 64 を超えるスレッド(`-threads`)を使うと、スレッド表の範囲外に書き込んでいました。
- 修正:ライトスタイル付きで照明済みの quake3 BSP にもう一度 `-light` を実行するとクラッシュしていました(NRC の q3map2 でも同じ)。前回の照明が追加した「nomipmaps」の面とシェーダーを取り消し、1 回照明した場合と同じ結果になります。
- bhed_q3map2 `-light`:頂点ライティングの面の頂点を空いているスレッドで分担し、1 つのスレッドだけが忙しくなることがなくなりました(結果は同じ)。
- bhed_q3map2 `-light`:raw ライトマップの作成時に、面をすべての後続の面ではなく、そのライトマップを共有できる後続の面とだけ比較します(結果は同じ)。duel_mos_espa では各ライト処理が 1-2 秒早く始まります(コンピューターが混んでいるときはさらに早く)。
- bhed_q3map2 `-light`:結果は同じで、影のテストが速くなりました。三角形を 4 つずつまとめてテストし、ランダムなサブサンプルは最初の不透明な三角形で止まります。シェーダー画像の読み込みで、すべての画像のすべてのピクセルを調べることはなくなりました。
- bhed_q3map2 `-light`:結果は同じで、点はボックスがその点を含む不透明なブラシとだけ比較されます(ボックスを 4 つずつ、グラフィックカードでも同様)。グラフィックカードと `-randomsamples` では、空いているプロセッサーのスレッドが再びグラフィックカードのライトマップを引き受けます(taspir1 と md_am_sith がはっきり速くなりました)。
- `-randomsamples`(「[Final] Extreme Quality」)での GPU ベイクの自動に 3 つ目の選択肢:グラフィックカードがランダムなサブサンプルも計算します(複数のブロックにまたがって集めます)。自動はグラフィックカードがプロセッサーより速かったマップでだけ一度計測し、最も速かったマップでだけ使い続けます。小さなライトマップが多いマップ(taspir1、kejim_post)で速く、遅くなるマップでは選ばれません。
