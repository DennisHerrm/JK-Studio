// hilfe_fragen_h2.inc - die unabhaengige Frageliste der Stufe H2 (EINGEFROREN)
//
// Geschrieben am Anfang von H2, BEVOR Beugungen, Tippfehler-Toleranz,
// Untermenue-Inhalte und das Modus-Gewicht eingebaut wurden, und danach
// nicht mehr angepasst (die Fragen nicht und die erwarteten Antworten
// nicht). 12 Ziele x 4 Sprachen = 48 Fragen, je Sprache frei formuliert
// (keine woertlichen Uebersetzungen), mit je einem Tippfehler und fuenf
// Fragen, die erst mit dem aktuellen Modus eindeutig werden (modus >= 0:
// 1 Vorschau, 2 Modell, 3 Karten-Editor, 4 Menue-Editor; -1 = ohne Kontext).
// Erwartet: Vorsilben der Kennungen wie in kFragen (passt()); "tour:x@" =
// jeder Schritt der Tour x. Ziel Top-3 >= 85 %. (Vor der ersten Abstimmung
// korrigiert: Anker als "seite:x.html#anker" geschrieben wie die Kennungen -
// "seite:x#anker" passte nie; Grundlinie damit 38/48 = 79,2 %.)
#define G_LAUFEN "seite:k-wegpunkte|seite:wegpunkte|seite:tut-p3|seite:tut-figur|seite:k-patrouille|tour:k-wegpunkte@|tour:tut-p3@|tour:tut-figur@|tour:k-patrouille@"
#define G_ABSPIELEN "seite:zeitleiste|seite:k-zeitleiste-plus|seite:k-neu-timen|seite:tut-erste-szene|seite:tut-p4|tour:k-zeitleiste-plus@|tour:k-neu-timen@|tour:tut-erste-szene@|tour:tut-p4@"
#define G_MISSION "seite:tut-p1|seite:tut-mission|seite:missionen|seite:tut-projekt|tour:tut-p1@|tour:tut-mission@"
#define G_SKIN "seite:k-skins|tour:k-skins@|tour:k-modell@3|ort:menue:MenuFile>OpenSkin|ort:knopf:ModelSkin"
#define G_SPIELORDNER "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|seite:einstellungen|seite:erste-schritte.html#spielordner|tour:tut-karte@0"
#define G_UPDATE "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck|seite:einstellungen.html#updates"
// rc609 KEMD: dazu die Tour "Tuer mit Trigger im Karten-Editor" (tut-karte-tuer) - sie zeigt genau das; vorher lag
// k-tuer mit k-tod gleichauf (21.710), und jeder neue Absatz im Handbuch kippte die Reihenfolge.
#define G_TUER "seite:k-tuer|seite:k-tuer-use|seite:karteneditor.html#entities|seite:entities|tour:k-tuer@|tour:tut-karte-tuer"
#define G_TRIGGER "seite:k-trigger|seite:k-tuer-use|tour:k-trigger@"
#define G_MENUE "seite:menue_editor|ort:knopf:ViewMenuEditor@modebar|ort:knopf:Me"
#define G_BEDIENEN "seite:menue_editor.html#bedienen|ort:knopf:MeBedienen"
#define G_KE_EINST "ort:menue:KeLEdit>KeLPreferences"
#define G_TEXTUR "tour:tut-karte@4|tour:tut-karte@6|seite:karteneditor.html#texturen"

struct FrageH2 {
    const char* sprache;
    int modus;
    const char* text;
    const char* erwartet;
};

const FrageH2 kFragenH2[] = {
    // --- Deutsch ---
    {"de", -1, "Wie bringe ich einen NPC dazu, einen Weg abzulaufen?", G_LAUFEN},
    {"de", 1, "Szene von Anfang an ansehen", G_ABSPIELEN},
    {"de", -1, "neue Mission anlegen", G_MISSION},
    {"de", 2, "anderen Skin für das Modell auswählen", G_SKIN},
    {"de", -1, "Wo gebe ich an, wo Jedi Academy installiert ist?", G_SPIELORDNER},
    {"de", -1, "Gibt es eine neuere Version des Programms?", G_UPDATE},
    {"de", 3, "Tür einbauen, die sich öffnet", G_TUER},
    {"de", -1, "Szene starten, sobald der Spieler einen Bereich betritt", G_TRIGGER},
    {"de", -1, "Ein Menü aus dem Spiel bearbeiten", G_MENUE},
    {"de", 4, "Menü ausprobieren wie im Spiel, Knöpfe drücken", G_BEDIENEN},
    {"de", 3, "Einstellungen", G_KE_EINST},
    {"de", -1, "Textur auf eine Wand legn", G_TEXTUR},
    // --- English ---
    {"en", -1, "make an NPC patrol between points", G_LAUFEN},
    {"en", 1, "play back the cutscene", G_ABSPIELEN},
    {"en", -1, "start a brand new mission on a map", G_MISSION},
    {"en", 2, "swap the skin on this character", G_SKIN},
    {"en", -1, "tell the program where my game is installed", G_SPIELORDNER},
    {"en", -1, "is there an update available", G_UPDATE},
    {"en", 3, "add a door that slides open", G_TUER},
    {"en", -1, "run a script when the player walks into an area", G_TRIGGER},
    {"en", -1, "edit the main menu of the game", G_MENUE},
    {"en", 4, "click through the menu like in game", G_BEDIENEN},
    {"en", 3, "preferences", G_KE_EINST},
    {"en", -1, "put a texure on the floor", G_TEXTUR},
    // --- 中文 ---
    {"zh", -1, "让NPC沿着路线巡逻", G_LAUFEN},
    {"zh", 1, "播放过场动画看看效果", G_ABSPIELEN},
    {"zh", -1, "新建一个任务", G_MISSION},
    {"zh", 2, "给模型换皮肤", G_SKIN},
    {"zh", -1, "告诉程序游戏装在哪里", G_SPIELORDNER},
    {"zh", -1, "检查有没有新版本", G_UPDATE},
    {"zh", 3, "在地图里加一扇门", G_TUER},
    {"zh", -1, "玩家走进某个区域时开始剧情", G_TRIGGER},
    {"zh", -1, "修改游戏的主菜单", G_MENUE},
    {"zh", 4, "像在游戏里一样点击菜单按钮", G_BEDIENEN},
    {"zh", 3, "设置在哪里", G_KE_EINST},
    {"zh", -1, "给地板贴上纹理", G_TEXTUR},
    // --- 日本語 ---
    {"ja", -1, "NPCに決まった道を歩かせる", G_LAUFEN},
    {"ja", 1, "カットシーンを再生して確認したい", G_ABSPIELEN},
    {"ja", -1, "新しいミッションを作る", G_MISSION},
    {"ja", 2, "モデルのスキンを変える", G_SKIN},
    {"ja", -1, "ゲームのインストール先を指定する", G_SPIELORDNER},
    {"ja", -1, "アップデートがあるか確認", G_UPDATE},
    {"ja", 3, "マップにドアを置く", G_TUER},
    {"ja", -1, "プレイヤーがエリアに入ったらシーンを始める", G_TRIGGER},
    {"ja", -1, "ゲームのメインメニューを編集したい", G_MENUE},
    {"ja", 4, "ゲームと同じようにメニューを操作してみる", G_BEDIENEN},
    {"ja", 3, "設定はどこ", G_KE_EINST},
    {"ja", -1, "床にテクスチャを貼る", G_TEXTUR},
};

#undef G_LAUFEN
#undef G_ABSPIELEN
#undef G_MISSION
#undef G_SKIN
#undef G_SPIELORDNER
#undef G_UPDATE
#undef G_TUER
#undef G_TRIGGER
#undef G_MENUE
#undef G_BEDIENEN
#undef G_KE_EINST
#undef G_TEXTUR
