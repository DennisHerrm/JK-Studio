// rueckgaengig.cpp - Karten-Editor: Rueckgaengig/Wiederholen.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/undo.cpp,
// libs/maplib.h (UndoFileChangeTracker), libs/undolib.h (save -> changed).
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
#include "bhed/karte/rueckgaengig.h"

#include <atomic>

namespace bhed::karte {

namespace {
std::atomic<std::uint64_t> g_zerstoert{0};
}

std::uint64_t objekteZerstoert() { return g_zerstoert.load(std::memory_order_relaxed); }

Rueckgaengigbar::~Rueckgaengigbar() {
    abmelden();
    g_zerstoert.fetch_add(1, std::memory_order_relaxed);
}

void Rueckgaengigbar::anmelden(Rueckgaengig* undo) {
    if (m_undo == undo) { return; }
    abmelden();
    m_undo = undo;
    if (m_undo) { m_undo->anmelden(this); }
}

void Rueckgaengigbar::abmelden() {
    if (m_undo) {
        m_undo->abmelden(this);
        m_undo = nullptr;
    }
}

void Rueckgaengigbar::sichern() {
    if (m_undo) { m_undo->sichernVon(this); }
}

// UndoStack::start (undo.cpp:160-165)
void Rueckgaengig::Stapel::beginnen(const std::string& befehl) {
    m_offen = std::make_unique<Vorgang>();
    m_offen->befehl = befehl;
}

// UndoStack::finish (undo.cpp:166-177): false, wenn nichts gesichert wurde.
bool Rueckgaengig::Stapel::beenden(const std::string& befehl) {
    if (m_offen) {
        m_offen.reset();
        return false;
    }
    if (!m_stapel.empty()) {
        m_stapel.back()->befehl = befehl;
    }
    return true;
}

// UndoStack::save (undo.cpp:178-184)
void Rueckgaengig::Stapel::sichern(Rueckgaengigbar* objekt) {
    if (m_offen) {
        m_stapel.push_back(std::move(m_offen));
    }
    Zustand z;
    z.objekt = objekt->shared_from_this();
    z.daten = objekt->zustandSichern();
    m_stapel.back()->zustaende.push_front(std::move(z));
}

Rueckgaengig::Rueckgaengig() = default;

Rueckgaengig::~Rueckgaengig() {
    clear();
    // angemeldete Objekte vergessen ihr Undo-System
    for (auto& [objekt, stapel] : m_objekte) {
        (void)stapel;
        objekt->m_undo = nullptr;
    }
    m_objekte.clear();
}

// UndoFileChangeTracker::changed (maplib.h:102-107): die anstehende
// Zaehlung genau einmal ausfuehren.
void Rueckgaengig::verfolgerGeaendert() {
    switch (m_anstehend) {
    case Anstehend::Befehl:   // pushOperation (maplib.h:80-86)
        if (m_stand < m_gespeichertStand) {
            // redo queue has been flushed.. it is now impossible to get back
            // to the saved state via undo/redo
            m_gespeichertStand = kMaxAenderungen;
        }
        ++m_stand;
        break;
    case Anstehend::Zurueck: --m_stand; break;   // pop
    case Anstehend::Vor: ++m_stand; break;       // push
    case Anstehend::Nichts: return;
    }
    m_anstehend = Anstehend::Nichts;
}

// UndoStackFiller::save (undo.cpp:197-204): einmal je Befehl.
// ObservedUndoableObject::save (undolib.h:77-84): erst m_map->changed() fuer
// angemeldete Objekte, dann der Stapel.
void Rueckgaengig::sichernVon(Rueckgaengigbar* o) {
    ++m_aenderungen;
    auto it = m_objekte.find(o);
    if (it == m_objekte.end()) { return; }
    verfolgerGeaendert();
    if (it->second == nullptr) { return; }
    Stapel* s = it->second;
    it->second = nullptr;
    s->sichern(o);
}

// mark_undoables (undo.cpp:214-218)
void Rueckgaengig::markieren(Stapel* s) {
    for (auto& [objekt, stapel] : m_objekte) {
        (void)objekt;
        stapel = s;
    }
}

void Rueckgaengig::setzeStufen(std::size_t stufen) {   // setLevels (undo.cpp:241-251)
    if (stufen > kMaxStufen) { stufen = kMaxStufen; }
    while (m_undoStapel.groesse() > stufen) { m_undoStapel.vorneEntfernen(); }
    m_stufen = stufen;
}

void Rueckgaengig::startUndo() {
    m_undoStapel.beginnen("unnamedCommand");
    markieren(&m_undoStapel);
}
bool Rueckgaengig::finishUndo(const std::string& befehl) {
    const bool geaendert = m_undoStapel.beenden(befehl);
    markieren(nullptr);
    return geaendert;
}
void Rueckgaengig::startRedo() {
    m_redoStapel.beginnen("unnamedCommand");
    markieren(&m_redoStapel);
}
bool Rueckgaengig::finishRedo(const std::string& befehl) {
    const bool geaendert = m_redoStapel.beenden(befehl);
    markieren(nullptr);
    return geaendert;
}

// start (undo.cpp:284-291)
void Rueckgaengig::start() {
    m_redoStapel.leeren();
    if (m_undoStapel.groesse() == m_stufen && !m_undoStapel.leer()) {
        m_undoStapel.vorneEntfernen();
    }
    startUndo();
    m_anstehend = Anstehend::Befehl;   // trackersBegin
}

// finish (undo.cpp:292-301)
void Rueckgaengig::finish(const std::string& name) {
    ++m_abschluesse;
    if (finishUndo(name) && ausgabe) { ausgabe(name); }
}

// undo (undo.cpp:302-317)
bool Rueckgaengig::undo() {
    if (m_undoStapel.leer()) {
        if (ausgabe) { ausgabe("Undo: no undo available"); }
        return false;
    }
    Vorgang& v = m_undoStapel.hinten();
    const std::string befehl = v.befehl;
    if (ausgabe) { ausgabe("Undo: " + befehl); }
    startRedo();
    m_anstehend = Anstehend::Zurueck;   // trackersUndo
    for (auto& z : v.zustaende) {   // Snapshot::restore
        z.objekt->zustandLaden(*z.daten);
    }
    (void)finishRedo(befehl);
    m_undoStapel.hintenEntfernen();
    ++m_abschluesse;
    return true;
}

// redo (undo.cpp:318-333)
bool Rueckgaengig::redo() {
    if (m_redoStapel.leer()) {
        if (ausgabe) { ausgabe("Redo: no redo available"); }
        return false;
    }
    Vorgang& v = m_redoStapel.hinten();
    const std::string befehl = v.befehl;
    if (ausgabe) { ausgabe("Redo: " + befehl); }
    startUndo();
    m_anstehend = Anstehend::Vor;   // trackersRedo
    for (auto& z : v.zustaende) {
        z.objekt->zustandLaden(*z.daten);
    }
    (void)finishUndo(befehl);
    m_redoStapel.hintenEntfernen();
    ++m_abschluesse;
    return true;
}

// clear (undo.cpp:334-339)
void Rueckgaengig::clear() {
    markieren(nullptr);
    m_undoStapel.leeren();
    m_redoStapel.leeren();
    m_stand = 0;   // trackersClear -> UndoFileChangeTracker::clear
    ++m_abschluesse;
}

std::string Rueckgaengig::undoName() const { return m_undoStapel.leer() ? std::string() : m_undoStapel.hinten().befehl; }
std::string Rueckgaengig::redoName() const { return m_redoStapel.leer() ? std::string() : m_redoStapel.hinten().befehl; }

}  // namespace bhed::karte
