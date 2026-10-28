// hilfetest.cpp - der Helfer ohne Oberflaeche (Stufe H1, bhed/hilfe.h)
//
//   a) Woerter: klein, Umlaute gefaltet, gestutzt, CJK als Ein- und
//      Doppelzeichen, Begriffe ueber die Sprachen (@kamera)
//   b) Ortskatalog: Hauptmenue, Modusleiste, Einstellungen und die Menues des
//      Karten-Editors; jede Kennung hat einen Text
//   c) die haeufigen Ziele (src/hilfe_daten.inc) stehen in jeder Sprache im Index
//   d) Trefferquote: typische Fragen in vier Sprachen, Top 3 - Ziel >= 85 %
//      (Argument 1 = Handbuchordner mit <sprache>/touren.json und suche.js)
#include <cstddef>
#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "bhed/hilfe.h"
#include "bhed/i18n.h"
#include "bhed/tour.h"

namespace {

int fehler = 0;

void expect(const char* was, bool ok) {
    std::printf("  %s %s\n", ok ? "ok  " : "FEHL", was);
    if (!ok) { ++fehler; }
}

std::string datei(const std::string& p) {
    std::ifstream f(p, std::ios::binary);
    std::ostringstream o;
    o << f.rdbuf();
    return o.str();
}

bool hat(const std::vector<std::string>& v, const std::string& w) {
    for (const std::string& x : v) {
        if (x == w) { return true; }
    }
    return false;
}

void woerterProben() {
    std::printf("Woerter\n");
    const std::vector<std::string> a = bhed::hilfe::woerter("Wie mache ich eine Kamerafahrt mit Texturen?");
    expect("Stoppwoerter fallen weg (wie, ich, eine, mit)", !hat(a, "wie") && !hat(a, "ich") && !hat(a, "eine") && !hat(a, "mit"));
    expect("Texturen -> textur", hat(a, "textur"));
    expect("Kamerafahrt traegt @kamera und @fahrt", hat(a, "@kamera") && hat(a, "@fahrt"));
    expect("Texturen traegt @textur", hat(a, "@textur"));
    const std::vector<std::string> b = bhed::hilfe::woerter("Größe ändern für Türen");
    expect("Umlaute gefaltet und gestutzt: groess, aend, tuer", hat(b, "groess") && hat(b, "aend") && hat(b, "tuer"));
    expect("Tueren -> @tuer", hat(b, "@tuer"));
    const std::vector<std::string> c = bhed::hilfe::woerter("制作镜头运动");
    expect("CJK: Einzel- und Doppelzeichen", hat(c, "镜") && hat(c, "镜头") && hat(c, "运动"));
    expect("CJK: 镜头 -> @kamera", hat(c, "@kamera"));
    const std::vector<std::string> d = bhed::hilfe::woerter("カメラワークを作る");
    expect("Katakana: カメラ -> @kamera, 作る -> @neu", hat(d, "@kamera") && hat(d, "@neu"));
    const std::vector<std::string> e = bhed::hilfe::woerter("camera path");
    expect("camera path -> @fahrt (zwei Woerter)", hat(e, "@fahrt"));
    expect("leer -> nichts", bhed::hilfe::woerter("   ").empty());
    expect("schrittTitel: der fette Anfang", bhed::hilfe::schrittTitel("**Texture.** Double-click a folder") == "Texture.");
}

void orteProben() {
    std::printf("Ortskatalog\n");
    const std::vector<bhed::hilfe::Ort>& o = bhed::hilfe::orte();
    int menue = 0;
    int karte = 0;
    int einst = 0;
    int ohneText = 0;
    for (const bhed::hilfe::Ort& x : o) {
        const std::string z = x.ziel;
        if (z.rfind("menue:", 0) == 0) { ++menue; }
        if (std::string(x.bereich) == "karte") { ++karte; }
        if (std::string(x.bereich) == "einstellungen") { ++einst; }
        std::string k = x.kette;
        std::size_t a = 0;
        while (a <= k.size()) {
            std::size_t e = k.find('>', a);
            if (e == std::string::npos) { e = k.size(); }
            bhed::i18n::Str id{};
            if (!bhed::i18n::strVonName(k.substr(a, e - a), &id)) { ++ohneText; }
            a = e + 1;
        }
    }
    std::printf("    %zu Orte: %d Menuepunkte, %d im Karten-Editor, %d Einstellungen\n", o.size(), menue, karte, einst);
    expect("mindestens 50 Menuepunkte des Hauptprogramms", menue - karte >= 50);
    expect("mindestens 150 Menuepunkte des Karten-Editors", karte >= 150);
    expect("mindestens 10 Einstellungen", einst >= 10);
    expect("jede Kennung der Ketten hat einen Text", ohneText == 0);
}

struct Frage {
    const char* sprache;
    const char* text;
    const char* erwartet;   // Vorsilben der Kennungen, "|" getrennt
};

// Typische Fragen - wie Leute in Foren und JKHub-Tutorials fragen, nicht nur
// die Wortlisten der Ziele.
const Frage kFragen[] = {
    // --- Deutsch ---
    {"de", "Wie mache ich meine erste eigene Karte?", "tour:tut-karte"},
    {"de", "Wie bekomme ich eine Textur auf die Wand?", "tour:tut-karte@4|tour:tut-karte@6"},
    {"de", "Licht in den Raum setzen", "tour:tut-karte@7"},
    {"de", "Wo setze ich den Spielerstart?", "tour:tut-karte@8"},
    {"de", "Karte kompilieren", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"de", "Wie packe ich meine Karte als pk3 ins Spiel?", "tour:tut-karte@14|ort:menue:KeLBuild>KePkMenue"},
    {"de", "Optimierer benutzen", "tour:tut-karte@11|seite:karte_optimierer|ort:menue:KeLBuild>KeOptMenue"},
    {"de", "Im Editor durch die Karte laufen", "tour:tut-karte@13|ort:menue:KeLView>KeSpMenue"},
    {"de", "Erstes Skript schreiben", "tour:k-neues-skript"},
    {"de", "Skript in eine ibi übersetzen", "tour:k-uebersetzen|seite:k-ibi"},
    {"de", "Befehl einfügen", "tour:k-befehl-einfuegen"},
    {"de", "Kamerafahrt für eine Zwischensequenz", "tour:k-kamera-ansicht@|seite:k-kamera|seite:kamera|seite:cmd-cam|seite:tut-p4|seite:tut-projekt"},
    {"de", "NPC zu einem Wegpunkt laufen lassen", "seite:k-gangart|seite:k-wegpunkte|seite:k-patrouille|seite:wegpunkte"},
    {"de", "Wo finde ich die Einstellungen?", "ort:menue:MenuEdit>AppPrefs|seite:einstellungen"},
    {"de", "Sprache auf Englisch umstellen", "ort:menue:MenuView>ViewLanguage"},
    {"de", "Gibt es eine neue Version?", "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck"},
    {"de", "Modell öffnen", "ort:menue:MenuFile>OpenGlm|ort:knopf:ViewModel@modebar|seite:modell"},
    {"de", "Mission laden", "ort:menue:MenuFile>LoadMission|seite:missionen|seite:tut-p1|seite:tut-projekt"},
    {"de", "Tastenkürzel ändern", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {"de", "Raum aushöhlen", "tour:tut-karte@5"},
    {"de", "Dunkles Design einschalten", "ort:menue:MenuView>ViewTheme"},
    {"de", "Rückgängig machen", "seite:k-rueckgaengig|ort:menue:MenuEdit|ort:menue:KeLEdit>KeLUndo"},
    // --- English ---
    {"en", "How do I make my first map?", "tour:tut-karte"},
    {"en", "apply a texture to a brush", "tour:tut-karte@4|tour:tut-karte@6"},
    {"en", "add a light to my room", "tour:tut-karte@7"},
    {"en", "where does the player spawn", "tour:tut-karte@8"},
    {"en", "compile my map", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"en", "pack the map into a pk3", "tour:tut-karte@14|ort:menue:KeLBuild>KePkMenue"},
    {"en", "check the map limits with the optimizer", "tour:tut-karte@11|seite:karte_optimierer|ort:menue:KeLBuild>KeOptMenue"},
    {"en", "walk around my map in the editor", "tour:tut-karte@13|ort:menue:KeLView>KeSpMenue"},
    {"en", "write my first script", "tour:k-neues-skript"},
    {"en", "compile a script to ibi", "tour:k-uebersetzen|seite:k-ibi"},
    {"en", "insert a command", "tour:k-befehl-einfuegen"},
    {"en", "make a camera move for a cutscene", "tour:k-kamera-ansicht@|seite:k-kamera|seite:kamera|seite:cmd-cam|seite:tut-p4|seite:tut-projekt"},
    {"en", "make an NPC walk to a waypoint", "seite:k-gangart|seite:k-wegpunkte|seite:k-patrouille|seite:wegpunkte"},
    {"en", "open the settings", "ort:menue:MenuEdit>AppPrefs|seite:einstellungen"},
    {"en", "change the language", "ort:menue:MenuView>ViewLanguage"},
    {"en", "check for updates", "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck"},
    {"en", "open a model", "ort:menue:MenuFile>OpenGlm|ort:knopf:ViewModel@modebar|seite:modell"},
    {"en", "load a mission", "ort:menue:MenuFile>LoadMission|seite:missionen|seite:tut-p1|seite:tut-projekt"},
    {"en", "change keyboard shortcuts", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {"en", "hollow out a room", "tour:tut-karte@5"},
    {"en", "the text is too small", "ort:menue:MenuView>ViewDpiScale"},
    // --- 中文 ---
    {"zh", "怎么制作第一张地图", "tour:tut-karte"},
    {"zh", "给画刷应用纹理", "tour:tut-karte@4|tour:tut-karte@6"},
    {"zh", "添加灯光", "tour:tut-karte@7"},
    {"zh", "设置玩家出生点", "tour:tut-karte@8"},
    {"zh", "编译地图", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"zh", "把地图打包成pk3", "tour:tut-karte@14|ort:menue:KeLBuild>KePkMenue"},
    {"zh", "使用优化器", "tour:tut-karte@11|seite:karte_optimierer|ort:menue:KeLBuild>KeOptMenue"},
    {"zh", "在编辑器中试玩地图", "tour:tut-karte@13|ort:menue:KeLView>KeSpMenue"},
    {"zh", "写第一个脚本", "tour:k-neues-skript"},
    {"zh", "编译脚本", "tour:k-uebersetzen|seite:k-ibi"},
    {"zh", "插入命令", "tour:k-befehl-einfuegen"},
    {"zh", "制作过场动画的镜头运动", "tour:k-kamera-ansicht@|seite:k-kamera|seite:kamera|seite:cmd-cam|seite:tut-p4|seite:tut-projekt"},
    {"zh", "让NPC走到路径点", "seite:k-gangart|seite:k-wegpunkte|seite:k-patrouille|seite:wegpunkte"},
    {"zh", "打开设置", "ort:menue:MenuEdit>AppPrefs|seite:einstellungen"},
    {"zh", "更改语言", "ort:menue:MenuView>ViewLanguage"},
    {"zh", "检查更新", "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck"},
    {"zh", "打开模型", "ort:menue:MenuFile>OpenGlm|ort:knopf:ViewModel@modebar|seite:modell"},
    {"zh", "载入任务", "ort:menue:MenuFile>LoadMission|seite:missionen|seite:tut-p1|seite:tut-projekt"},
    {"zh", "修改快捷键", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {"zh", "挖空房间", "tour:tut-karte@5"},
    // --- 日本語 ---
    {"ja", "初めてのマップを作るには", "tour:tut-karte"},
    {"ja", "ブラシにテクスチャを貼る", "tour:tut-karte@4|tour:tut-karte@6"},
    {"ja", "ライトを置く", "tour:tut-karte@7"},
    {"ja", "スポーン地点を設定", "tour:tut-karte@8"},
    {"ja", "マップをビルドする", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"ja", "マップをpk3にパックする", "tour:tut-karte@14|ort:menue:KeLBuild>KePkMenue"},
    {"ja", "最適化ツールを使う", "tour:tut-karte@11|seite:karte_optimierer|ort:menue:KeLBuild>KeOptMenue"},
    {"ja", "エディターでマップを歩き回る", "tour:tut-karte@13|ort:menue:KeLView>KeSpMenue"},
    {"ja", "最初のスクリプトを書く", "tour:k-neues-skript"},
    {"ja", "スクリプトをコンパイル", "tour:k-uebersetzen|seite:k-ibi"},
    {"ja", "コマンドを挿入する", "tour:k-befehl-einfuegen"},
    {"ja", "カットシーンのカメラワークを作る", "tour:k-kamera-ansicht@|seite:k-kamera|seite:kamera|seite:cmd-cam|seite:tut-p4|seite:tut-projekt"},
    {"ja", "NPCをウェイポイントまで歩かせる", "seite:k-gangart|seite:k-wegpunkte|seite:k-patrouille|seite:wegpunkte"},
    {"ja", "設定を開く", "ort:menue:MenuEdit>AppPrefs|seite:einstellungen"},
    {"ja", "言語を変更する", "ort:menue:MenuView>ViewLanguage"},
    {"ja", "アップデートを確認", "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck"},
    {"ja", "モデルを開く", "ort:menue:MenuFile>OpenGlm|ort:knopf:ViewModel@modebar|seite:modell"},
    {"ja", "ミッションを読み込む", "ort:menue:MenuFile>LoadMission|seite:missionen|seite:tut-p1|seite:tut-projekt"},
    {"ja", "ショートカットを変更", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {"ja", "部屋を中空にする", "tour:tut-karte@5"},
    // --- frei formuliert (keine Woerter aus den Wortlisten der Ziele) ---
    {"de", "Mein Raum ist viel zu dunkel", "tour:tut-karte@7|seite:k-hell"},
    {"de", "Wo stelle ich den Ordner vom Spiel ein?", "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|tour:tut-karte@0|seite:einstellungen"},
    {"de", "Wie baue ich eine Tür, die aufgeht?", "seite:k-tuer"},
    {"de", "Eine Figur soll etwas sagen", "seite:k-dialog"},
    {"en", "my level is way too dark", "tour:tut-karte@7|seite:k-hell"},
    {"en", "where do I set the game folder", "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|tour:tut-karte@0|seite:einstellungen"},
    {"en", "how to build a door that opens", "seite:k-tuer"},
    {"en", "a character should say a line", "seite:k-dialog"},
    {"zh", "房间太暗了", "tour:tut-karte@7|seite:k-hell"},
    {"zh", "设置游戏文件夹", "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|tour:tut-karte@0|seite:einstellungen"},
    {"zh", "怎么做一扇会打开的门", "seite:k-tuer"},
    {"zh", "让角色说一句台词", "seite:k-dialog"},
    {"ja", "部屋が暗すぎる", "tour:tut-karte@7|seite:k-hell"},
    {"ja", "ゲームのフォルダを設定する", "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|tour:tut-karte@0|seite:einstellungen"},
    {"ja", "開くドアの作り方", "seite:k-tuer"},
    {"ja", "キャラにセリフを言わせる", "seite:k-dialog"},
};

// Unabhaengige Fragen: nach dem Abstimmen der Wortlisten geschrieben und NICHT
// daran angepasst - die ehrliche Messung (wird gezeigt, ist keine Schranke).
const Frage kUnabhaengig[] = {
    {"de", "Wie stelle ich ein, dass die Figur rennt statt geht?", "seite:k-gangart"},
    {"de", "Ich will dass eine Tür aufgeht wenn der Spieler kommt", "seite:k-tuer|seite:k-trigger|seite:k-tuer-use"},
    {"de", "Kann man das Programm auf Japanisch stellen?", "ort:menue:MenuView>ViewLanguage"},
    {"de", "Meine Karte hat ein Loch nach draußen (Leak)", "seite:karteneditor_bauen|seite:karte_bauen|tour:tut-karte@12|seite:karte_optimierer"},
    {"de", "Wie schalte ich die KI wieder aus?", "seite:helfer"},
    {"de", "Musik in der Szene abspielen", "seite:k-effekt-musik|seite:cmd-play|seite:cmd-sound"},
    {"en", "how do I make the character run instead of walk", "seite:k-gangart"},
    {"en", "open a door when the player gets close", "seite:k-tuer|seite:k-trigger|seite:k-tuer-use"},
    {"en", "switch the program to Japanese", "ort:menue:MenuView>ViewLanguage"},
    {"en", "my map leaks into the void", "seite:karteneditor_bauen|seite:karte_bauen|tour:tut-karte@12|seite:karte_optimierer"},
    {"en", "how do I turn the AI off again", "seite:helfer"},
    {"en", "play music during the scene", "seite:k-effekt-musik|seite:cmd-play|seite:cmd-sound"},
    {"zh", "怎么让角色跑而不是走", "seite:k-gangart"},
    {"zh", "玩家靠近时让门打开", "seite:k-tuer|seite:k-trigger|seite:k-tuer-use"},
    {"zh", "把程序切换成日语", "ort:menue:MenuView>ViewLanguage"},
    {"zh", "我的地图有泄漏", "seite:karteneditor_bauen|seite:karte_bauen|tour:tut-karte@12|seite:karte_optimierer"},
    {"zh", "怎么再关闭 AI", "seite:helfer"},
    {"zh", "在场景中播放音乐", "seite:k-effekt-musik|seite:cmd-play|seite:cmd-sound"},
    {"ja", "キャラを歩きではなく走らせたい", "seite:k-gangart"},
    {"ja", "プレイヤーが近づいたらドアを開ける", "seite:k-tuer|seite:k-trigger|seite:k-tuer-use"},
    {"ja", "プログラムを日本語にする", "ort:menue:MenuView>ViewLanguage"},
    {"ja", "マップがリークしている", "seite:karteneditor_bauen|seite:karte_bauen|tour:tut-karte@12|seite:karte_optimierer"},
    {"ja", "AI をもう一度オフにするには", "seite:helfer"},
    {"ja", "シーンで音楽を流す", "seite:k-effekt-musik|seite:cmd-play|seite:cmd-sound"},
};

bhed::i18n::Language spracheVon(const std::string& s) {
    if (s == "de") { return bhed::i18n::Language::German; }
    if (s == "zh") { return bhed::i18n::Language::ChineseSimplified; }
    if (s == "ja") { return bhed::i18n::Language::Japanese; }
    return bhed::i18n::Language::English;
}

bool passt(const std::string& id, const std::string& erwartet) {
    std::size_t a = 0;
    while (a <= erwartet.size()) {
        std::size_t e = erwartet.find('|', a);
        if (e == std::string::npos) { e = erwartet.size(); }
        const std::string v = erwartet.substr(a, e - a);
        // "tour:tut-karte" passt nur auf die Tour selbst oder "@0", nicht auf jeden
        // Schritt; "tour:x@" (mit @ am Ende) auf jeden Schritt der Tour.
        if (!v.empty() && v.back() == '@') {
            if (id == v.substr(0, v.size() - 1) || id.compare(0, v.size(), v) == 0) { return true; }
        } else if (v.find('@') == std::string::npos && v.rfind("tour:", 0) == 0) {
            if (id == v || id == v + "@0") { return true; }
        } else if (id.compare(0, v.size(), v) == 0 && (id.size() == v.size() || v.find('@') == std::string::npos || id[v.size()] < '0' || id[v.size()] > '9')) {
            return true;
        }
        a = e + 1;
    }
    return false;
}

void trefferProben(const std::string& ordner) {
    std::printf("Trefferquote (Top 3)\n");
    int gesamt = 0;
    int gut = 0;
    int unGesamt = 0;
    int unGut = 0;
    for (const char* sp : {"de", "en", "zh", "ja"}) {
        bhed::tour::Touren t;
        std::vector<bhed::tour::Abschnitt> hb;
        std::string f;
        const bool tOk = bhed::tour::ladeTouren(datei(ordner + "/" + sp + "/touren.json"), t, &f);
        const bool hOk = bhed::tour::ladeSuchindex(datei(ordner + "/" + sp + "/suche.js"), hb, &f);
        expect((std::string(sp) + ": touren.json und suche.js gelesen").c_str(), tOk && hOk);
        const bhed::hilfe::Index ix = bhed::hilfe::baue(t, hb, spracheVon(sp));
        int fehlend = 0;
        for (const std::string& id : bhed::hilfe::zielKennungen()) {
            if (ix.finde(id) == nullptr) {
                std::printf("    fehlt im Index (%s): %s\n", sp, id.c_str());
                ++fehlend;
            }
        }
        expect((std::string(sp) + ": jedes haeufige Ziel steht im Index").c_str(), fehlend == 0);
        int n = 0;
        int g = 0;
        for (const Frage& q : kFragen) {
            if (std::string(q.sprache) != sp) { continue; }
            ++n;
            const std::vector<bhed::hilfe::Treffer> tr = ix.frage(q.text, 3);
            bool ok = false;
            std::string liste;
            for (const bhed::hilfe::Treffer& x : tr) {
                const std::string& id = ix.eintraege()[x.eintrag].id;
                ok = ok || passt(id, q.erwartet);
                liste += " " + id;
            }
            if (ok) {
                ++g;
            } else {
                std::printf("    daneben (%s) \"%s\":%s\n", sp, q.text, liste.c_str());
            }
        }
        std::printf("    %s: %d von %d\n", sp, g, n);
        gesamt += n;
        gut += g;
        int un = 0;
        int ug = 0;
        for (const Frage& q : kUnabhaengig) {
            if (std::string(q.sprache) != sp) { continue; }
            ++un;
            bool ok = false;
            std::string liste;
            for (const bhed::hilfe::Treffer& x : ix.frage(q.text, 3)) {
                const std::string& id = ix.eintraege()[x.eintrag].id;
                ok = ok || passt(id, q.erwartet);
                liste += " " + id;
            }
            if (ok) {
                ++ug;
            } else {
                std::printf("    unabhaengig daneben (%s) \"%s\":%s\n", sp, q.text, liste.c_str());
            }
        }
        std::printf("    %s unabhaengig: %d von %d\n", sp, ug, un);
        unGesamt += un;
        unGut += ug;
    }
    const double quote = gesamt > 0 ? 100.0 * gut / gesamt : 0.0;
    std::printf("    gesamt: %d von %d = %.1f %%\n", gut, gesamt, quote);
    std::printf("    unabhaengig gesamt: %d von %d = %.1f %%\n", unGut, unGesamt, unGesamt > 0 ? 100.0 * unGut / unGesamt : 0.0);
    expect("mindestens 60 Fragen", gesamt >= 60);
    expect("Top-3-Trefferquote >= 85 %", quote >= 85.0);
}

}  // namespace

int main(int argc, char** argv) {
    const std::string handbuch = argc > 1 ? argv[1] : "handbuch";
    woerterProben();
    orteProben();
    trefferProben(handbuch);
    std::printf(fehler == 0 ? "\nalle Helferproben bestanden\n" : "\nFEHLGESCHLAGEN (%d)\n", fehler);
    return fehler == 0 ? 0 : 1;
}
