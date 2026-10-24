// app_karte_modelle.cpp - Karten-Editor: die Modelle der misc_model-
// Entitaeten aus dem Dateisystem des Spiels (ModelCache, ModelResource).
//
// Portiert aus NetRadiant-Custom 20260114: radiant/referencecache.cpp:182-202
// (ModelResource_load: "Loaded Model:", "Model load failed:"), 280-305
// ("Model type not supported:"), 655-665 (RefreshReferences,
// FlushReferences: Map_Free leert die Ablage), plugins/entity/model.h:58-63
// (EModel::modelChanged: PathCleaned), plugins/entity/miscmodel.cpp:241-249
// (der Schluessel aus der Klasse, miscmodel_key), q3map2 model.cpp:1571-1572
// ("_skin" vor "skin") und 1227-1280 (die .skin-Datei).
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
#include "karteneditor.h"

#include "app_internal.h"

#include "bhed/karte/eklasse.h"

#include <algorithm>
#include <cctype>
#include <map>
#include <memory>
#include <mutex>
#include <string>

namespace bhed::gui::karteneditor {

namespace {

std::string bereinigt(std::string s) {   // PathCleaned
    std::replace(s.begin(), s.end(), '\\', '/');
    return s;
}

std::string klein(std::string s) {
    for (char& c : s) { c = static_cast<char>(std::tolower(static_cast<unsigned char>(c))); }
    return s;
}

class KartenModelle final : public karte::ModellQuelle {
public:
    [[nodiscard]] karte::ModellAnsicht ansicht(const karte::Entitaet& e) const override {
        const karte::EntitaetsKlasse* kl = e.klasse();
        const std::string schluessel = kl != nullptr && !kl->miscmodelKey.empty() ? kl->miscmodelKey : std::string("model");
        const std::string pfad = bereinigt(e.wert(schluessel));
        karte::ModellAnsicht a;
        a.modell = modell(pfad);
        // q3map2: e.read_keyvalue( skin, "_skin", "skin" )
        const std::string skin = !e.wert("_skin").empty() ? e.wert("_skin") : e.wert("skin");
        const karte::Umbenennungen* sk = skin.empty() || pfad.empty() ? nullptr : &skinFuer(karte::skinDateiName(pfad, skin));
        for (const karte::ModellFlaeche& f : a.modell->flaechen) {
            std::string name = f.shader;
            if (sk != nullptr && !sk->empty()) {   // handle .skin file
                const auto it = std::find_if(sk->begin(), sk->end(), [&](const auto& u) { return klein(u.first) == klein(f.shader); });
                name = it != sk->end() ? it->second : std::string();   // not mapping -> weg
            }
            if (!name.empty()) {   // handle shader remapping (getRemap)
                const std::string r = karte::remapFuer(e, name);
                if (!r.empty()) { name = r; }
            }
            a.shader.push_back(name);
        }
        return a;
    }

    // ModelNode::setModel im Modell-Browser: das Modell ohne Skin und _remap
    [[nodiscard]] karte::ModellAnsicht nachPfad(const std::string& pfad) const {
        karte::ModellAnsicht a;
        a.modell = modell(bereinigt(pfad));
        for (const karte::ModellFlaeche& f : a.modell->flaechen) { a.shader.push_back(f.shader); }
        return a;
    }

    void leeren() {   // FlushReferences / ModelCache_clear
        std::lock_guard<std::mutex> l(m_sperre);
        m_modelle.clear();
        m_skins.clear();
    }

private:
    // ModelResource_load ueber das Dateisystem des Spiels
    [[nodiscard]] std::shared_ptr<const karte::Modell> modell(const std::string& pfad) const {
        std::lock_guard<std::mutex> l(m_sperre);
        const auto it = m_modelle.find(pfad);
        if (it != m_modelle.end()) { return it->second; }
        const std::shared_ptr<Spielkontext>& spiel = editor().karteSpiel;
        auto m = std::make_shared<karte::Modell>(karte::nullModell());
        if (!spiel) { return m; }   // noch kein Spiel: nicht merken
        if (!pfad.empty()) {
            if (!karte::modellArtBekannt(pfad)) {
                fehler("Model type not supported: \"" + pfad + "\"");
            } else {
                std::string bytes;
                if (spiel->fs.dateiLesen(pfad, bytes)) {
                    ausgabe("Loaded Model: \"" + pfad + "\"");
                    karte::Modell neu;
                    const std::string ende = klein(pfad.substr(pfad.size() - 4));
                    bool ok = false;
                    if (ende == ".md3") {
                        ok = karte::md3Lesen(bytes, neu);
                    } else if (ende == ".ase") {
                        std::string fehlerText;
                        ok = karte::aseLesen(bytes, pfad, neu, &fehlerText);
                        if (!ok && !fehlerText.empty()) { fehler(fehlerText); }   // PICO_ERROR
                    } else {   // .glm in der Grundhaltung mit model_default.skin (wie im Spiel)
                        std::string skin;
                        (void)spiel->fs.textLesen(pfad.substr(0, pfad.find_last_of('/') + 1) + "model_default.skin", skin);
                        ok = karte::glmLesen(bytes, skin, neu);
                    }
                    if (ok) { *m = std::move(neu); }
                } else {
                    fehler("Model load failed: \"" + pfad + "\"");
                }
            }
        }
        m_modelle.emplace(pfad, m);
        return m;
    }

    [[nodiscard]] const karte::Umbenennungen& skinFuer(const std::string& datei) const {
        std::lock_guard<std::mutex> l(m_sperre);
        auto it = m_skins.find(datei);
        if (it != m_skins.end()) { return it->second; }
        karte::Umbenennungen u;
        const std::shared_ptr<Spielkontext>& spiel = editor().karteSpiel;
        std::string text;
        if (spiel && spiel->fs.textLesen(datei, text)) { u = karte::skinLesen(text); }
        return m_skins.emplace(datei, std::move(u)).first->second;
    }

    mutable std::mutex m_sperre;
    mutable std::map<std::string, std::shared_ptr<const karte::Modell>> m_modelle;
    mutable std::map<std::string, karte::Umbenennungen> m_skins;
};

KartenModelle& ablage() {
    static KartenModelle m;
    return m;
}

}  // namespace

const karte::ModellQuelle& kartenModelle() { return ablage(); }

karte::ModellAnsicht kartenModellPfad(const std::string& pfad) { return ablage().nachPfad(pfad); }

void modelleLeeren() {   // FlushReferences
    ablage().leeren();
    ++editor().sichtStand;
}

}  // namespace bhed::gui::karteneditor
