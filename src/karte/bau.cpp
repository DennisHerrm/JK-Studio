// bau.cpp - Karten-Editor: das Build-Menue (Stufe 3 Punkt 3).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/build.cpp (siehe bau.h),
// libs/versionlib.h, libs/xml/xmlwriter.h.
//
//   Copyright (C) 2001-2006, William Joseph.
//   All Rights Reserved.
//   This file is part of GtkRadiant.
//
//   GtkRadiant is free software; you can redistribute it and/or modify it under
//   the terms of the GNU General Public License as published by the Free
//   Software Foundation; either version 2 of the License, or (at your option)
//   any later version. GtkRadiant is distributed in the hope that it will be
//   useful, but WITHOUT ANY WARRANTY; without even the implied warranty of
//   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General
//   Public License for more details (LICENSE).
#include "bhed/karte/bau.h"

#include "bhed/karte/xmlstrom.h"

#include <cstring>

namespace bhed::karte {

namespace {

void meldung(std::vector<std::string>* m, const std::string& text) {
    if (m != nullptr) { m->push_back(text); }
}

// version_parse / version_compatible (versionlib.h)
struct Fassung {
    int major = 0;
    int minor = 0;
};
int ganzzahl(const std::string& s, std::size_t a, std::size_t b) {
    int r = 0;
    for (std::size_t i = a; i < b && i < s.size(); ++i) {
        r = r * 10 + (s[i] - '0');
    }
    return r;
}
Fassung fassungLesen(const std::string& s) {
    Fassung f;
    const std::size_t endeMajor = s.find('.');
    if (endeMajor == std::string::npos) {
        f.major = ganzzahl(s, 0, s.size());
        return f;
    }
    std::size_t endeMinor = s.find('.', endeMajor + 1);
    if (endeMinor == std::string::npos) { endeMinor = s.size(); }
    f.minor = ganzzahl(s, endeMajor + 1, endeMinor);
    f.major = ganzzahl(s, 0, endeMajor);
    return f;
}
bool vertraeglich(const Fassung& code, const Fassung& daten) {
    return code.major == daten.major && !(code.minor < daten.minor);
}

// Die Leser von build.cpp als Rahmen auf einem Stapel (XMLParser::m_stack).
// pushElement gibt entweder einen neuen Leser zurueck oder sich selbst
// ("*this") - der zweite Fall ist hier ein Verweis (selbst = Index des
// Rahmens, an den geschrieben wird).
enum class Art { Wurzel, Projekt, Werkzeug, Bedingung, Eintrag, Befehl, Ueberspringen };
struct Rahmen {
    Art art = Art::Wurzel;
    int selbst = -1;               // >= 0: Verweis auf einen tieferen Rahmen
    BauWerkzeug* werkzeug = nullptr;
    std::size_t teilNr = 0;        // Bedingung: Index in werkzeug->teile
    std::size_t eintragNr = 0;     // Index in BauProjekt::eintraege (der Vektor waechst)
    std::size_t befehlNr = 0;
    std::string puffer;
};

class Leser {
public:
    Leser(BauProjekt& p, std::vector<std::string>* m, bool mitWurzel) : m_p(p), m_m(m) {
        Rahmen r;
        r.art = mitWurzel ? Art::Wurzel : Art::Projekt;
        m_stapel.push_back(r);
    }
    bool vertraeglich() const { return m_vertraeglich; }

    void anfang(const std::string& name, const XmlAttribute& attribute) {
        const auto attr = [&](const char* n) {
            for (const auto& [k, w] : attribute) {
                if (k == n) { return w; }
            }
            return std::string();
        };
        const int obenIdx = ziel(static_cast<int>(m_stapel.size()) - 1);
        Rahmen& oben = m_stapel[static_cast<std::size_t>(obenIdx)];
        Rahmen neu;
        switch (oben.art) {
        case Art::Wurzel:   // RootXMLConstructor::pushElement
            if (name == "project") {
                if (bhed::karte::vertraeglich(fassungLesen(kBauMenueFassung), fassungLesen(attr("version")))) {
                    m_vertraeglich = true;
                    neu.art = Art::Projekt;
                } else {
                    neu.art = Art::Ueberspringen;
                }
            } else {
                neu.selbst = obenIdx;
            }
            break;
        case Art::Projekt:   // ProjectXMLConstructor::pushElement
            if (name == "var") {
                // m_tools.insert( ... ).first: ein vorhandenes Werkzeug bleibt und waechst
                BauWerkzeug& w = m_p.werkzeuge.try_emplace(attr("name")).first->second;
                neu.art = Art::Werkzeug;
                neu.werkzeug = &w;
            } else if (name == "build") {
                m_p.eintraege.push_back(BauEintrag{attr("name"), {}});
                neu.art = Art::Eintrag;
                neu.eintragNr = m_p.eintraege.size() - 1;
            } else if (name == "separator") {
                m_p.eintraege.push_back(BauEintrag{kBauTrenner, {}});
                neu.selbst = obenIdx;
            } else {
                meldung(m_m, "parse error: invalid element");
                neu.selbst = obenIdx;
            }
            break;
        case Art::Werkzeug:   // ToolXMLConstructor::pushElement
            if (name == "cond") {
                spuelen(oben);
                oben.werkzeug->teile.push_back(BauTeil{true, attr("value"), {}});
                neu.art = Art::Bedingung;
                neu.werkzeug = oben.werkzeug;
                neu.teilNr = oben.werkzeug->teile.size() - 1;
            } else {
                meldung(m_m, "parse error: invalid element \"" + name + "\"");
                neu.selbst = obenIdx;
            }
            break;
        case Art::Eintrag:   // BuildXMLConstructor::pushElement
            if (name == "command") {
                m_p.eintraege[oben.eintragNr].befehle.emplace_back();
                neu.art = Art::Befehl;
                neu.eintragNr = oben.eintragNr;
                neu.befehlNr = m_p.eintraege[oben.eintragNr].befehle.size() - 1;
            } else {
                meldung(m_m, "parse error: invalid element");
                neu.selbst = obenIdx;
            }
            break;
        case Art::Bedingung:
        case Art::Befehl:   // VariableString-/ConditionalXMLConstructor::pushElement
            meldung(m_m, "parse error: invalid element \"" + name + "\"");
            neu.selbst = obenIdx;
            break;
        case Art::Ueberspringen:
            neu.selbst = obenIdx;
            break;
        }
        m_stapel.push_back(neu);
    }

    void zeichen(const std::string& text) {
        Rahmen& r = m_stapel[static_cast<std::size_t>(ziel(static_cast<int>(m_stapel.size()) - 1))];
        if (r.art == Art::Werkzeug || r.art == Art::Bedingung || r.art == Art::Befehl) { r.puffer += text; }
    }

    void ende(const std::string& /*name*/) {
        if (m_stapel.size() <= 1) { return; }
        Rahmen r = m_stapel.back();
        m_stapel.pop_back();
        if (r.selbst >= 0) { return; }   // popElement eines Verweises: nichts zu tun
        abschliessen(r);
    }

    void fertig() {
        while (m_stapel.size() > 1) { ende(std::string()); }
        if (m_stapel.front().art == Art::Projekt) { abschliessen(m_stapel.front()); }
    }

private:
    int ziel(int i) const {
        while (m_stapel[static_cast<std::size_t>(i)].selbst >= 0) { i = m_stapel[static_cast<std::size_t>(i)].selbst; }
        return i;
    }
    // ToolXMLConstructor::flush mit dem Kniff fuer -fs_pakpath (ExtraResourcePaths)
    void spuelen(Rahmen& r) {
        if (r.puffer.empty()) { return; }
        r.werkzeug->teile.push_back(BauTeil{false, {}, r.puffer});
        // q3map2 ExtraResourcePaths hack: ist es q3map2 und steht die Variable noch nicht da?
        // Anpassung: auch bhed_q3map2 (unser Bau, data/karte/<spiel>/default_build_menu.xml).
        const bool istQ3map2 = r.puffer.find("[RadiantPath]q3map2.[ExecutableType]") != std::string::npos ||
                               r.puffer.find("[RadiantPath]bhed_q3map2.[ExecutableType]") != std::string::npos;
        if (istQ3map2 && r.puffer.find("[ExtraResourcePaths]") == std::string::npos) {
            r.werkzeug->teile.push_back(BauTeil{false, {}, "[ExtraResourcePaths]"});
        }
        r.puffer.clear();
    }
    void abschliessen(Rahmen& r) {
        switch (r.art) {
        case Art::Werkzeug: spuelen(r); break;                                   // ~ToolXMLConstructor
        case Art::Bedingung: r.werkzeug->teile[r.teilNr].text = r.puffer; break;   // ~ConditionalXMLConstructor
        case Art::Befehl: m_p.eintraege[r.eintragNr].befehle[r.befehlNr] = r.puffer; break;   // ~VariableStringXMLConstructor
        default: break;
        }
    }

    BauProjekt& m_p;
    std::vector<std::string>* m_m;
    std::vector<Rahmen> m_stapel;
    bool m_vertraeglich = false;
};

bool lesen(const std::string& xml, BauProjekt& p, std::vector<std::string>* m, bool mitWurzel) {
    Leser leser(p, m, mitWurzel);
    XmlStrom x;
    x.anfang = [&](const std::string& n, const XmlAttribute& a) { leser.anfang(n, a); };
    x.ende = [&](const std::string& n) { leser.ende(n); };
    x.zeichen = [&](const std::string& t) { leser.zeichen(t); };
    x.fehler = [&](const std::string& f) { meldung(m, "XML error: " + f); };
    x.einspeisen(xml);
    x.schliessen();
    leser.fertig();
    return !mitWurzel || leser.vertraeglich();
}

// XMLStreamWriter (xmlwriter.h): Elemente oeffnen sich mit "<name attr=..."
// und schliessen den Kopf erst beim ersten Inhalt; ein leeres Element wird "/>".
class Schreiber {
public:
    Schreiber() {
        m_offen.push_back(false);   // die Wurzel: schon "Inhalt"
        m_text = "<?xml version=\"1.0\"?>";
    }
    void anfang(const char* name, const char* attrName = nullptr, const std::string& attrWert = std::string()) {
        kopfSchliessen();
        m_offen.push_back(true);
        m_text += '<';
        m_text += xmlMaskieren(name);
        if (attrName != nullptr) {
            m_text += ' ';
            m_text += xmlMaskieren(attrName);
            m_text += "=\"";
            m_text += xmlMaskieren(attrWert);
            m_text += '"';
        }
    }
    void ende(const char* name) {
        if (m_offen.back()) {
            m_text += "/>";
        } else {
            m_text += "</";
            m_text += xmlMaskieren(name);
            m_text += '>';
        }
        m_offen.pop_back();
    }
    void text(const std::string& t) {
        kopfSchliessen();
        m_text += xmlMaskieren(t);
    }
    [[nodiscard]] const std::string& ergebnis() const { return m_text; }

private:
    void kopfSchliessen() {
        if (m_offen.back()) {
            m_offen.back() = false;
            m_text += '>';
        }
    }
    std::vector<bool> m_offen;   // true = Kopf noch offen (eStartElement)
    std::string m_text;
};

void werkzeugSchreiben(Schreiber& s, const BauWerkzeug& w) {   // Tool::exportXML
    for (const BauTeil& t : w.teile) {
        if (t.bedingt) {   // Conditional::exportXML
            s.anfang("cond", "value", t.test);
            if (!t.text.empty()) { s.text(t.text); }
            s.ende("cond");
        } else {
            if (!t.text.empty()) { s.text(t.text); }   // importer << c_str()
        }
    }
}

}  // namespace

bool bauIstTrenner(const BauEintrag& e) {
    if (e.name != kBauTrenner) { return false; }
    for (const std::string& b : e.befehle) {
        if (!b.empty()) { return false; }
    }
    return true;
}

std::string bauAuswerten(const std::string& text, const BauVariablen& v, std::vector<std::string>* meldungen) {
    std::string variable;
    std::string aus;
    bool inVariable = false;
    for (const char c : text) {
        if (!inVariable) {
            if (c == '[') {
                inVariable = true;
            } else {
                aus += c;
            }
        } else if (c == ']') {
            inVariable = false;
            const auto it = v.find(variable);
            if (it != v.end()) {
                aus += it->second;
            } else {
                meldung(meldungen, "undefined build variable: \"" + variable + "\"");   // build_get_variable
            }
            variable.clear();
        } else {
            variable += c;
        }
    }
    return aus;
}

std::string bauAuswerten(const BauWerkzeug& w, const BauVariablen& v, std::vector<std::string>* meldungen) {
    std::string aus;
    for (const BauTeil& t : w.teile) {
        if (t.bedingt) {
            if (!bauAuswerten(t.test, v, meldungen).empty()) { aus += bauAuswerten(t.text, v, meldungen); }
        } else {
            aus += bauAuswerten(t.text, v, meldungen);
        }
    }
    return aus;
}

bool bauMenueLesen(const std::string& xml, BauProjekt& p, std::vector<std::string>* meldungen) {
    const bool ok = lesen(xml, p, meldungen, true);
    if (!ok) { meldung(meldungen, "failed to parse build menu"); }
    return ok;
}

std::string bauMenueSchreiben(const BauProjekt& p) {
    Schreiber s;
    s.text("\n");
    s.anfang("project", "version", kBauMenueFassung);
    s.text("\n");
    for (const auto& [name, w] : p.werkzeuge) {
        s.anfang("var", "name", name);
        werkzeugSchreiben(s, w);
        s.ende("var");
        s.text("\n");
    }
    for (const BauEintrag& e : p.eintraege) {
        if (bauIstTrenner(e)) {
            s.anfang("separator");
            s.ende("separator");
            s.text("\n");
        } else {
            s.anfang("build", "name", e.name);
            s.text("\n");   // BuildXMLExporter: importer << '\n'
            for (const std::string& b : e.befehle) {
                s.anfang("command");
                if (!b.empty()) { s.text(b); }
                s.ende("command");
                s.text("\n");
            }
            s.ende("build");
            s.text("\n");
        }
    }
    s.ende("project");
    s.text("\n");
    return s.ergebnis();
}

void bauWerkzeugeAuswerten(const BauProjekt& p, BauVariablen& v, const std::string& bis) {
    for (const auto& [name, w] : p.werkzeuge) {
        if (!bis.empty() && name == bis) { break; }   // vorher definierte Werkzeuge sind im aktuellen nutzbar
        v[name] = bauAuswerten(w, v);
    }
}

std::vector<std::string> bauBefehle(const BauProjekt& p, std::size_t index, BauVariablen v, std::vector<std::string>* meldungen) {
    for (const auto& [name, w] : p.werkzeuge) { v[name] = bauAuswerten(w, v, meldungen); }   // build_init_tools
    std::vector<std::string> befehle;
    if (index < p.eintraege.size()) {
        for (const std::string& b : p.eintraege[index].befehle) {
            std::string aus = bauAuswerten(b, v, meldungen);
            if (!aus.empty()) { befehle.push_back(std::move(aus)); }
        }
    } else {
        meldung(meldungen, "build #" + std::to_string(index) + " not defined");
    }
    return befehle;
}

std::string bauWerkzeugXml(const BauWerkzeug& w) {
    Schreiber s;
    werkzeugSchreiben(s, w);
    const std::string& t = s.ergebnis();
    return t.substr(t.find('>') + 1);   // strchr( stream, '>' ) + 1: ohne den Kopf
}

bool bauWerkzeugAusXml(const std::string& text, BauWerkzeug& w) {
    BauProjekt p;
    lesen("<var name=\"DUMMY\">" + text + "</var>", p, nullptr, false);
    if (p.werkzeuge.empty()) { return false; }
    w = std::move(p.werkzeuge.begin()->second);
    return true;
}

std::string bauTooltip(const BauEintrag& e) {
    std::string s;
    for (const std::string& b : e.befehle) {
        s += b;
        s += '\n';
    }
    if (!s.empty()) { s.pop_back(); }
    return s;
}

}  // namespace bhed::karte
