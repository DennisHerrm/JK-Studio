// rueckgaengig.h - Karten-Editor: Rueckgaengig/Wiederholen fuer den Kartenkern.
//
// Portiert aus NetRadiant-Custom (Stand master, 2026): radiant/undo.cpp,
// include/iundo.h, libs/undolib.h.
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
//
// Wie NRC: jedes aenderbare Objekt (Flaeche, Pinsel, Patch, Schluessel einer
// Entitaet, Kinderliste) ist "rueckgaengigbar". Vor einer Aenderung ruft es
// sichern() - das Undo-System legt dann EINMAL je Befehl eine Kopie des alten
// Zustands ab (UndoStackFiller merkt sich, ob schon gesichert wurde). Ein Befehl
// ist start() ... finish(name); ohne Aenderung dazwischen entsteht kein
// Eintrag. undo() stellt die Zustaende wieder her - dabei sichern die Objekte
// erneut, und zwar auf den Wiederholen-Stapel.
//
// Objekte melden sich an, wenn sie in die Karte kommen (NRC: instanceAttach),
// und ab, wenn sie herausgenommen werden. Erst angemeldete Objekte werden
// gesichert; ein Objekt, das erst waehrend des Befehls entsteht, wird nicht
// gesichert - seine Kinderliste haelt es fest (wie NRC).
#ifndef BHED_KARTE_RUECKGAENGIG_H
#define BHED_KARTE_RUECKGAENGIG_H

#include <cstddef>
#include <functional>
#include <list>
#include <map>
#include <memory>
#include <string>

namespace bhed::karte {

// UndoMemento
class Memento {
public:
    virtual ~Memento() = default;
};

class Rueckgaengig;

// Undoable
class Rueckgaengigbar : public std::enable_shared_from_this<Rueckgaengigbar> {
public:
    virtual ~Rueckgaengigbar();
    Rueckgaengigbar() = default;
    Rueckgaengigbar(const Rueckgaengigbar&) = delete;
    Rueckgaengigbar& operator=(const Rueckgaengigbar&) = delete;

    [[nodiscard]] virtual std::unique_ptr<Memento> zustandSichern() const = 0;   // exportState
    virtual void zustandLaden(const Memento& m) = 0;                             // importState

    // instanceAttach / instanceDetach
    void anmelden(Rueckgaengig* undo);
    void abmelden();
    [[nodiscard]] Rueckgaengig* undoSystem() const { return m_undo; }

protected:
    // Face::undoSave / Brush::undoSave: vor jeder Aenderung aufrufen.
    void sichern();

private:
    friend class Rueckgaengig;
    Rueckgaengig* m_undo = nullptr;
};

// RadiantUndoSystem
class Rueckgaengig {
public:
    static constexpr std::size_t kMaxStufen = 4096;   // MAX_UNDO_LEVELS

    Rueckgaengig();
    ~Rueckgaengig();
    Rueckgaengig(const Rueckgaengig&) = delete;
    Rueckgaengig& operator=(const Rueckgaengig&) = delete;

    void start();                          // UndoSystem::start - Befehl beginnt
    void finish(const std::string& name);  // UndoSystem::finish - Befehl endet
    bool undo();                           // false: nichts rueckgaengig zu machen
    bool redo();                           // false: nichts zu wiederholen
    void clear();
    void setzeStufen(std::size_t stufen);  // setLevels (Vorgabe 512)
    [[nodiscard]] std::size_t stufen() const { return m_stufen; }
    [[nodiscard]] std::size_t undoAnzahl() const { return m_undoStapel.groesse(); }
    [[nodiscard]] std::size_t redoAnzahl() const { return m_redoStapel.groesse(); }
    [[nodiscard]] std::string undoName() const;   // Name des naechsten Rueckgaengig-Schritts
    [[nodiscard]] std::string redoName() const;

    // Aenderungszaehler (NRC: MapFile::changed). Steigt bei jeder Sicherung.
    [[nodiscard]] std::size_t aenderungen() const { return m_aenderungen; }
    // Zaehlt jeden abgeschlossenen Befehl, Rueckgaengig, Wiederholen und
    // clear() - fuer Stellen, die erst nach einem Befehl rechnen (die Shader
    // in Gebrauch des Texturbrowsers), nicht bei jeder Sicherung mitten in
    // einer Bewegung.
    [[nodiscard]] std::size_t abschluesse() const { return m_abschluesse; }

    // UndoFileChangeTracker (libs/maplib.h:55-116), der Tracker, den NRC an
    // das Undo-System haengt (trackerAttach): zaehlt Befehle, Rueckgaengig und
    // Wiederholen und weiss, ob die Karte dem gespeicherten Stand entspricht
    // (Map_Modified). Stufe 1b.
    [[nodiscard]] bool gespeichert() const { return m_gespeichertStand == m_stand; }   // saved
    void alsGespeichertMarkieren() { m_gespeichertStand = m_stand; }                  // save
    // globalOutputStream: "<befehl>" nach jedem Befehl, "Undo: ..." und
    // "Redo: ..." (undo.cpp:292-333) - fuer die Konsole.
    std::function<void(const std::string&)> ausgabe;

private:
    friend class Rueckgaengigbar;

    struct Zustand {   // Snapshot::StateApplicator
        std::shared_ptr<Rueckgaengigbar> objekt;
        std::unique_ptr<Memento> daten;
    };
    struct Vorgang {   // Operation
        std::list<Zustand> zustaende;   // push_front wie Snapshot::save
        std::string befehl;
    };
    class Stapel {   // UndoStack
    public:
        [[nodiscard]] bool leer() const { return m_stapel.empty(); }
        [[nodiscard]] std::size_t groesse() const { return m_stapel.size(); }
        Vorgang& hinten() { return *m_stapel.back(); }
        [[nodiscard]] const Vorgang& hinten() const { return *m_stapel.back(); }
        void vorneEntfernen() { m_stapel.pop_front(); }
        void hintenEntfernen() { m_stapel.pop_back(); }
        void leeren() { m_stapel.clear(); }
        void beginnen(const std::string& befehl);
        bool beenden(const std::string& befehl);
        void sichern(Rueckgaengigbar* objekt);
    private:
        std::list<std::unique_ptr<Vorgang>> m_stapel;
        std::unique_ptr<Vorgang> m_offen;
    };

    void anmelden(Rueckgaengigbar* o) { m_objekte[o] = nullptr; }
    void abmelden(Rueckgaengigbar* o) { m_objekte.erase(o); }
    void sichernVon(Rueckgaengigbar* o);
    void markieren(Stapel* s);
    void startUndo();
    bool finishUndo(const std::string& befehl);
    void startRedo();
    bool finishRedo(const std::string& befehl);

    // UndoFileChangeTracker: m_size, m_saved, m_pending (MAPFILE_MAX_CHANGES)
    static constexpr std::size_t kMaxAenderungen = static_cast<std::size_t>(-1);
    enum class Anstehend { Nichts, Befehl, Zurueck, Vor };   // pushOperation, pop, push
    void verfolgerGeaendert();   // changed()
    std::size_t m_stand = 0;
    std::size_t m_gespeichertStand = kMaxAenderungen;
    Anstehend m_anstehend = Anstehend::Nichts;

    Stapel m_undoStapel;
    Stapel m_redoStapel;
    // Objekt -> Stapel, auf den es beim naechsten Sichern geht (UndoStackFiller).
    std::map<Rueckgaengigbar*, Stapel*> m_objekte;
    std::size_t m_stufen = 512;
    std::size_t m_aenderungen = 0;
    std::size_t m_abschluesse = 0;
};

// UndoableCommand: Befehlsklammer fuer einen Block.
class Befehl {
public:
    Befehl(Rueckgaengig& u, std::string name) : m_u(u), m_name(std::move(name)) { m_u.start(); }
    ~Befehl() { m_u.finish(m_name); }
    Befehl(const Befehl&) = delete;
    Befehl& operator=(const Befehl&) = delete;
private:
    Rueckgaengig& m_u;
    std::string m_name;
};

}  // namespace bhed::karte
#endif
