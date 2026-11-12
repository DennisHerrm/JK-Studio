// nachplatz.h - TEMPO (12.11.2026): ein Speicher nach Platz (kleine, dichte Ganzzahlen ab 0) als Feld statt als Baum.
//
// Der Zeichner der Karte fragt je Zeichenaufruf zwei- bis dreimal nach dem Bild und der Lightmap eines Platzes
// (gpu::holeBild, gpu::holeLightmap). Im std::map war das ein Gang durch den Baum je Frage - im Spielbild des
// Karten-Editors rund 15 % des Hauptfadens. Die Plaetze sind die Nummern im Texturensatz bzw. der Lightmaps der Karte:
// dicht und ab 0. Hier liegen die Werte in einem Feld, die Frage ist ein Griff.
//
// Schnittstelle wie std::map<int, T>, soweit die Zeichenschicht sie braucht: find, end, operator[], erase(iterator),
// clear, size, empty, Durchlauf in aufsteigender Folge der Plaetze (Paare first/second). Statt rbegin()->first:
// groessterPlatz(). Plaetze unter 0 gibt es nicht (find liefert end, operator[] ist dafuer nicht erlaubt).
// Pruefung gegen std::map: tests/nachplatz.cpp.
#pragma once

#include <cassert>
#include <cstddef>
#include <type_traits>
#include <utility>
#include <vector>

namespace bhed {

template <typename T>
class NachPlatz {
public:
    using value_type = std::pair<int, T>;

    template <bool Konst>
    class Lauf {
    public:
        using Feld = std::conditional_t<Konst, const NachPlatz, NachPlatz>;
        using Wert = std::conditional_t<Konst, const value_type, value_type>;
        Lauf(Feld* feld, std::size_t stelle) : m_feld(feld), m_stelle(stelle) { weiter(); }
        Wert& operator*() const { return m_feld->m_werte[m_stelle]; }
        Wert* operator->() const { return &m_feld->m_werte[m_stelle]; }
        Lauf& operator++() {
            ++m_stelle;
            weiter();
            return *this;
        }
        bool operator==(const Lauf& o) const { return m_stelle == o.m_stelle; }
        bool operator!=(const Lauf& o) const { return m_stelle != o.m_stelle; }
        [[nodiscard]] std::size_t stelle() const { return m_stelle; }

    private:
        void weiter() {
            while (m_stelle < m_feld->m_da.size() && m_feld->m_da[m_stelle] == 0U) { ++m_stelle; }
        }
        Feld* m_feld;
        std::size_t m_stelle;
    };
    using iterator = Lauf<false>;
    using const_iterator = Lauf<true>;

    [[nodiscard]] iterator begin() { return iterator(this, 0); }
    [[nodiscard]] iterator end() { return iterator(this, m_da.size()); }
    [[nodiscard]] const_iterator begin() const { return const_iterator(this, 0); }
    [[nodiscard]] const_iterator end() const { return const_iterator(this, m_da.size()); }

    [[nodiscard]] iterator find(int platz) { return belegt(platz) ? iterator(this, static_cast<std::size_t>(platz)) : end(); }
    [[nodiscard]] const_iterator find(int platz) const { return belegt(platz) ? const_iterator(this, static_cast<std::size_t>(platz)) : end(); }

    // Wie std::map: legt den Platz mit T{} an, wenn es ihn noch nicht gibt.
    T& operator[](int platz) {
        assert(platz >= 0);
        const auto i = static_cast<std::size_t>(platz);
        if (i >= m_da.size()) {
            m_da.resize(i + 1U, 0U);
            m_werte.resize(i + 1U);
        }
        if (m_da[i] == 0U) {
            m_da[i] = 1U;
            m_werte[i] = value_type(platz, T{});
            ++m_zahl;
        }
        return m_werte[i].second;
    }

    void erase(iterator it) {
        const std::size_t i = it.stelle();
        if (i < m_da.size() && m_da[i] != 0U) {
            m_da[i] = 0U;
            m_werte[i] = value_type();
            --m_zahl;
        }
    }

    void clear() {
        m_werte.clear();
        m_da.clear();
        m_zahl = 0;
    }

    [[nodiscard]] std::size_t size() const { return m_zahl; }
    [[nodiscard]] bool empty() const { return m_zahl == 0; }
    // Der groesste belegte Platz (std::map: rbegin()->first); -1: leer
    [[nodiscard]] int groessterPlatz() const {
        for (std::size_t i = m_da.size(); i-- > 0U;) {
            if (m_da[i] != 0U) { return static_cast<int>(i); }
        }
        return -1;
    }

private:
    [[nodiscard]] bool belegt(int platz) const { return platz >= 0 && static_cast<std::size_t>(platz) < m_da.size() && m_da[static_cast<std::size_t>(platz)] != 0U; }

    std::vector<value_type> m_werte;
    std::vector<unsigned char> m_da;
    std::size_t m_zahl = 0;
};

}  // namespace bhed
