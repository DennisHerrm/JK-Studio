// bhed_parallel.h - Stufe 7 Punkt 2: parallele Schleifen INNERHALB eines
// Arbeitsschritts von q3map2.
//
// q3map2 verteilt -light je Raw-Lightmap auf die Threads (RunThreadsOnIndividual).
// Auf mp__duel1 kostet eine einzige Lightmap 21 % der ganzen Radiosity-Zeit -
// mehr als 4,8-fach kann es mit beliebig vielen Kernen nicht schneller werden
// (gemessen: 3,9-fach mit 32 Threads). Die grossen Lightmaps rechnen darum
// vorab je Zeile und Lichtblock parallel (light_ydnar.cpp, BhedLichterParallel),
// ebenso der zufallsfreie Teil des Gitter-Vorlaufs (light.cpp). Jeder Luxel
// rechnet dabei genau dieselben Gleitkomma-Operationen in derselben Reihenfolge
// wie vorher; nur WER sie rechnet, aendert sich.
//
// Der Pool lebt so lange wie der Prozess (q3map2 lebt einen Bau lang):
// faeden - 1 Arbeiter, der aufrufende Faden hilft mit. Warten ueber
// std::mutex/std::condition_variable (kernel32, keine neuen Importe der .exe -
// ctest q3map2_importe), vorher kurz aktiv.
//
//   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
#pragma once

#include <immintrin.h>

#include <algorithm>
#include <atomic>
#include <condition_variable>
#include <functional>
#include <mutex>
#include <thread>
#include <vector>

namespace bhed_par {

class Pool {
public:
    explicit Pool(int faeden) {
        for (int i = 1; i < faeden; ++i) {
            m_arbeiter.emplace_back([this] { arbeiten(); });
        }
    }
    ~Pool() {
        {
            std::lock_guard<std::mutex> l(m_mutex);
            m_ende = true;
            ++m_auftrag;
        }
        m_start.notify_all();
        for (std::thread& t : m_arbeiter) { t.join(); }
    }
    Pool(const Pool&) = delete;
    Pool& operator=(const Pool&) = delete;

    [[nodiscard]] int faeden() const { return static_cast<int>(m_arbeiter.size()) + 1; }

    // f(0) ... f(anzahl - 1) auf allen Faeden (die Indizes in aufsteigender
    // Reihenfolge vergeben). Kehrt zurueck, wenn alle fertig sind.
    void fuerAlle(int anzahl, const std::function<void(int)>& f) {
        if (anzahl <= 0) { return; }
        if (m_arbeiter.empty() || anzahl == 1) {
            for (int i = 0; i < anzahl; ++i) { f(i); }
            return;
        }
        {
            std::lock_guard<std::mutex> l(m_mutex);
            m_f = &f;
            m_anzahl = anzahl;
            m_naechster.store(0);
            m_offen = static_cast<int>(m_arbeiter.size());
            m_offenAtomar.store(m_offen, std::memory_order_release);
            ++m_auftrag;
            m_auftragAtomar.store(m_auftrag, std::memory_order_release);
        }
        m_start.notify_all();
        abarbeiten();
        // auf die Arbeiter warten: kurz aktiv, dann schlafen
        for (int i = 0; i < 2000 && m_offenAtomar.load(std::memory_order_acquire) != 0; ++i) { _mm_pause(); }
        std::unique_lock<std::mutex> l(m_mutex);
        m_fertig.wait(l, [this] { return m_offen == 0; });
        m_f = nullptr;
    }

private:
    void abarbeiten() {
        const std::function<void(int)>& f = *m_f;
        while (true) {
            const int i = m_naechster.fetch_add(1);
            if (i >= m_anzahl) { break; }
            f(i);
        }
    }
    void arbeiten() {
        // 0 = der Stand beim Anlegen: ein Faden, der erst nach dem ersten Auftrag
        // startet, darf diesen nicht als schon gesehen nehmen
        unsigned gesehen = 0;
        while (true) {
            // kurz aktiv warten (der naechste Auftrag kommt oft nach Mikrosekunden)
            for (int i = 0; i < 2000 && m_auftragAtomar.load(std::memory_order_acquire) == gesehen; ++i) { _mm_pause(); }
            {
                std::unique_lock<std::mutex> l(m_mutex);
                m_start.wait(l, [&] { return m_auftrag != gesehen; });
                gesehen = m_auftrag;
                if (m_ende) { return; }
            }
            abarbeiten();
            bool letzter = false;
            {
                std::lock_guard<std::mutex> l(m_mutex);
                letzter = --m_offen == 0;
                m_offenAtomar.store(m_offen, std::memory_order_release);
            }
            if (letzter) { m_fertig.notify_one(); }
        }
    }

    std::vector<std::thread> m_arbeiter;
    std::mutex m_mutex;
    std::condition_variable m_start;
    std::condition_variable m_fertig;
    const std::function<void(int)>* m_f = nullptr;
    int m_anzahl = 0;
    int m_offen = 0;            // Arbeiter, die den Auftrag noch nicht beendet haben
    unsigned m_auftrag = 0;     // Nummer des Auftrags
    bool m_ende = false;
    std::atomic<int> m_naechster{0};
    std::atomic<unsigned> m_auftragAtomar{0};   // Kopien fuer das aktive Warten
    std::atomic<int> m_offenAtomar{0};
};

// Der Pool des Prozesses (q3map2: numthreads Faeden). Nie zerstoert - q3map2
// kann aus jedem Faden heraus exit() rufen; der Prozess endet mit dem Bau.
// faeden = -1 (q3map2 hat numthreads noch nicht gesetzt, -bsp ruft
// ThreadSetDefault erst beim ersten RunThreadsOn): so viele wie Kerne, wie
// ThreadSetDefault es setzen wird (ohne dessen Ausgabe vorzuziehen).
inline Pool& pool(int faeden) {
    if (faeden == -1) { faeden = static_cast<int>(std::max(std::thread::hardware_concurrency(), 1U)); }
    static Pool* p = new Pool(faeden < 1 ? 1 : faeden);
    return *p;
}

}  // namespace bhed_par
