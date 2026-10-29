/* Handbuch BehavEd-Remake: Suche, Hell/Dunkel, Bild gross, Sprung zum Treffer.
 *
 * Die Suche liest window.HANDBUCH_INDEX (suche.js der Sprache): je Abschnitt
 * Seite, Anker, Titel und Text. Gesucht wird ohne Gross/Klein und ohne
 * Akzente ("animacion" findet "animación", "cestina" findet "čeština",
 * "uber" findet "über"), alle Woerter muessen vorkommen (Chinesisch/Japanisch:
 * als Zeichenfolge). Ein Klick springt auf die Seite an den Abschnitt und
 * markiert dort den Begriff.
 */
(function () {
  "use strict";
  var index = window.HANDBUCH_INDEX || [];
  var texte = window.HANDBUCH_TEXTE || { keine_treffer: "Nothing found.", treffer_auf: "on" };
  var feld = document.getElementById("suchfeld");
  var liste = document.getElementById("treffer");
  var wahl = -1;

  // Klein, NFKC und ohne Akzente (U+0300..U+036F nach NFD: é -> e, ñ -> n,
  // ě -> e, ü -> u). Japanische Dakuten liegen nicht in diesem Bereich und
  // bleiben (NFC setzt sie wieder zusammen).
  function norm(s) {
    return (s || "").normalize("NFKC").toLowerCase().normalize("NFD").replace(/[\u0300-\u036f]/g, "").normalize("NFC");
  }
  // Gefaltet und je gefaltetem Zeichen die Stelle im Original - Schnipsel und
  // Markierung schneiden das Original, auch wenn die Faltung Laengen aendert.
  function karte(s) {
    var t = "";
    var pos = [];
    for (var i = 0; i < s.length;) {
      var c = String.fromCodePoint(s.codePointAt(i));
      var f = norm(c);
      for (var k = 0; k < f.length; k++) { pos.push(i); }
      t += f;
      i += c.length;
    }
    pos.push(s.length);
    return { t: t, pos: pos };
  }
  // Die Stellen [von, bis) der Woerter im Original, sortiert, ohne Ueberlappung.
  function stellen(text, ws) {
    var k = karte(text);
    var aus = [];
    ws.forEach(function (w) {
      var p = k.t.indexOf(w);
      while (p >= 0) { aus.push([k.pos[p], k.pos[p + w.length]]); p = k.t.indexOf(w, p + w.length); }
    });
    aus.sort(function (a, b) { return a[0] - b[0]; });
    var rein = [];
    aus.forEach(function (st) { if (!rein.length || st[0] >= rein[rein.length - 1][1]) { rein.push(st); } });
    return rein;
  }
  function woerter(q) {
    return norm(q).split(/\s+/).filter(function (w) { return w.length > 0; });
  }
  function escHtml(s) {
    return s.replace(/[&<>"]/g, function (c) { return { "&": "&amp;", "<": "&lt;", ">": "&gt;", '"': "&quot;" }[c]; });
  }
  function markiere(text, ws) {
    var t = "";
    var pos = 0;
    stellen(text, ws).forEach(function (st) {
      t += escHtml(text.slice(pos, st[0])) + "<mark>" + escHtml(text.slice(st[0], st[1])) + "</mark>";
      pos = st[1];
    });
    return t + escHtml(text.slice(pos));
  }
  function schnipsel(text, ws) {
    var k = karte(text);
    var at = k.t.indexOf(ws[0]);
    if (at < 0) { return text.slice(0, 140); }
    var von = Math.max(0, k.pos[at] - 60);
    return (von > 0 ? "…" : "") + text.slice(von, von + 170) + (von + 170 < text.length ? "…" : "");
  }
  // Einmal gefaltet, nicht bei jedem Tastendruck.
  index.forEach(function (e) { e.ns = norm(e.s); e.nt = norm(e.t); e.nx = norm(e.x); });
  function suche(q) {
    var ws = woerter(q);
    if (!ws.length) { return []; }
    var aus = [];
    index.forEach(function (e) {
      var alles = e.ns + " " + e.nt + " " + e.nx;
      for (var i = 0; i < ws.length; i++) { if (alles.indexOf(ws[i]) < 0) { return; } }
      var punkte = 0;
      ws.forEach(function (w) {
        if (e.ns.indexOf(w) >= 0) { punkte += 10; }
        if (e.nt.indexOf(w) >= 0) { punkte += 4; }
        punkte += Math.min(5, e.nx.split(w).length - 1);
      });
      aus.push({ e: e, p: punkte });
    });
    aus.sort(function (a, b) { return b.p - a.p; });
    return aus.slice(0, 40);
  }
  function zeige() {
    var q = feld.value;
    var ws = woerter(q);
    var r = suche(q);
    wahl = -1;
    if (!ws.length) { liste.hidden = true; return; }
    liste.innerHTML = r.length ? r.map(function (x) {
      var e = x.e;
      var ziel = e.p + "?q=" + encodeURIComponent(q) + (e.a ? "#" + e.a : "");
      return '<a href="' + ziel + '"><b>' + markiere(e.s, ws) + "</b><small>" + escHtml(texte.treffer_auf) + " " +
        escHtml(e.t) + '</small><span class="schnipsel">' + markiere(schnipsel(e.x, ws), ws) + "</span></a>";
    }).join("") : '<div class="leer">' + escHtml(texte.keine_treffer) + "</div>";
    liste.hidden = false;
  }
  function waehle(d) {
    var a = liste.querySelectorAll("a");
    if (!a.length) { return; }
    if (wahl >= 0) { a[wahl].classList.remove("wahl"); }
    wahl = (wahl + d + a.length) % a.length;
    a[wahl].classList.add("wahl");
    a[wahl].scrollIntoView({ block: "nearest" });
  }
  if (feld) {
    feld.addEventListener("input", zeige);
    feld.addEventListener("focus", zeige);
    feld.addEventListener("keydown", function (ev) {
      if (ev.key === "ArrowDown") { waehle(1); ev.preventDefault(); }
      else if (ev.key === "ArrowUp") { waehle(-1); ev.preventDefault(); }
      else if (ev.key === "Enter") {
        var a = liste.querySelectorAll("a");
        if (a.length) { location.href = a[Math.max(0, wahl)].getAttribute("href"); }
      } else if (ev.key === "Escape") { liste.hidden = true; feld.blur(); }
    });
    document.addEventListener("click", function (ev) {
      if (!liste.contains(ev.target) && ev.target !== feld) { liste.hidden = true; }
    });
    document.addEventListener("keydown", function (ev) {
      var imFeld = document.activeElement && /input|textarea/i.test(document.activeElement.tagName);
      if (!imFeld && (ev.key === "/" || (ev.ctrlKey && ev.key.toLowerCase() === "k"))) {
        feld.focus(); feld.select(); ev.preventDefault();
      }
    });
  }

  // Auf der Zielseite: den Begriff markieren und den ersten Treffer zeigen.
  var q = new URLSearchParams(location.search).get("q");
  if (q) {
    var ws = woerter(q);
    var art = document.querySelector("article");
    var erster = null;
    var gang = document.createTreeWalker(art, NodeFilter.SHOW_TEXT, null);
    var knoten = [];
    while (gang.nextNode()) { knoten.push(gang.currentNode); }
    knoten.forEach(function (k) {
      var t = k.nodeValue;
      var st = stellen(t, ws);
      if (!st.length || k.parentNode.closest("mark, script, style")) { return; }
      var frag = document.createDocumentFragment();
      var pos = 0;
      st.forEach(function (s) {
        frag.appendChild(document.createTextNode(t.slice(pos, s[0])));
        var m = document.createElement("mark");
        m.textContent = t.slice(s[0], s[1]);
        frag.appendChild(m);
        if (!erster) { erster = m; }
        pos = s[1];
      });
      frag.appendChild(document.createTextNode(t.slice(pos)));
      k.parentNode.replaceChild(frag, k);
    });
    var ziel = location.hash ? document.getElementById(location.hash.slice(1)) : null;
    var zeigen = ziel || erster;
    if (zeigen) {
      setTimeout(function () {
        zeigen.scrollIntoView({ block: "start" });
        if (ziel) { ziel.classList.add("blink"); }
      }, 30);
    }
    if (feld) { feld.value = q; }
  }

  // Beim Scrollen: den Abschnitt, in dem man liest, in der Seitenleiste
  // hervorheben (shank: "das Unterthema gehighlighted, damit man sieht, bei
  // welchem Abschnitt man ist").
  var abschnitte = Array.prototype.slice.call(document.querySelectorAll("article h2[id]"));
  var unterLinks = {};
  document.querySelectorAll(".gruppe ul.unter a").forEach(function (a) {
    unterLinks[a.getAttribute("href").slice(1)] = a;
  });
  function hier() {
    var aktiv = null;
    abschnitte.forEach(function (h) { if (h.getBoundingClientRect().top <= 140) { aktiv = h.id; } });
    if (!aktiv && abschnitte.length && window.scrollY > 40) { aktiv = abschnitte[0].id; }
    if (window.innerHeight + window.scrollY >= document.body.scrollHeight - 4 && abschnitte.length) {
      aktiv = abschnitte[abschnitte.length - 1].id;   // ganz unten: der letzte
    }
    Object.keys(unterLinks).forEach(function (id) { unterLinks[id].classList.toggle("hier", id === aktiv); });
  }
  window.addEventListener("scroll", hier, { passive: true });
  hier();

  // Hell / dunkel, gemerkt.
  var knopf = document.getElementById("thema");
  if (knopf) {
    knopf.addEventListener("click", function () {
      var neu = document.documentElement.dataset.theme === "light" ? "dark" : "light";
      document.documentElement.dataset.theme = neu;
      try { localStorage.setItem("bhed-thema", neu); } catch (e) { /* ohne Speicher */ }
    });
  }

  // Bewegte Bilder anhalten (man kann ein GIF sonst nicht stoppen).
  document.querySelectorAll("button.anhalten").forEach(function (k) {
    k.addEventListener("click", function (ev) {
      ev.preventDefault();
      ev.stopPropagation();
      var img = k.parentNode.querySelector("img");
      var steht = k.classList.toggle("steht");
      img.src = steht ? k.dataset.stand : k.dataset.lauf + "?" + Date.now();
      k.innerHTML = steht ? "&#9654;" : "&#10074;&#10074;";
    });
  });

  // Classic / Graph / Tracks, gemerkt (rc601).
  document.querySelectorAll(".moduswahl button").forEach(function (k) {
    k.addEventListener("click", function () {
      document.documentElement.dataset.modus = k.dataset.modus;
      try { localStorage.setItem("bhed-modus", k.dataset.modus); } catch (e) { /* ohne Speicher */ }
    });
  });

  // Bilder gross.
  var gross = document.getElementById("gross");
  document.querySelectorAll("a.zoom").forEach(function (a) {
    a.addEventListener("click", function (ev) {
      ev.preventDefault();
      gross.querySelector("img").src = a.getAttribute("href");
      gross.hidden = false;
    });
  });
  if (gross) { gross.addEventListener("click", function () { gross.hidden = true; }); }
})();
