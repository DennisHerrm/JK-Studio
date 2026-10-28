// hilfe_daten.inc - Begriffe und haeufige Ziele des Helfers (Stufe H1)
//
// Von Hand gepflegt (anders als hilfe_orte.inc). Eingebunden nur von
// src/hilfe.cpp. Fest im Programm, nicht aus Dateien: was hier steht, kann
// keine Karte und kein Skript veraendern (Sicherheitskonzept H1).
//
// BEGRIFFE: ein Begriff (@name) und seine Woerter in vier Sprachen. Ein Wort
// aus mehreren Teilen ("camera path") zaehlt, wenn alle Teile vorkommen; CJK
// zaehlt, wenn die Zeichenfolge im Text steht. Lateinische Woerter werden wie
// im Index gestutzt (src/hilfe.cpp, stutze), "Kamerafahrt" traegt damit
// @kamera und @fahrt - Woerter ab vier Buchstaben passen auch als Teil eines
// zusammengesetzten Worts.
//
// ZIELE: die haeufigsten Aufgaben je Bereich, mit Woertern, wie Leute fragen
// (dazu Begriffe aus JKHub-Tutorials und NetRadiant). Die Kennung verweist auf
// einen Eintrag des Index (Tour, Tour ab Schritt, Handbuchseite, Ort); Titel
// und Erklaerung kommen von dort, in der Sprache des Index.

struct BegriffRoh {
    const char* name;
    const char* woerter;   // "|" getrennt
};

const BegriffRoh kBegriffe[] = {
    {"kamera", "kamera|camera|cam|shot|einstellung der kamera|镜头|相机|摄像机|カメラ"},
    {"fahrt", "kamerafahrt|kamerapfad|kamerabewegung|camera path|camera move|dolly|fahrt|运镜|镜头移动|カメラワーク"},
    {"szene", "zwischensequenz|cutscene|cinematic|filmszene|kinoszene|szene|scene|过场|过场动画|カットシーン|ムービー|シーン"},
    {"vorschau", "vorschau|preview|abspielen|wiedergabe|ansehen|watch|playback|预览|播放|プレビュー|再生"},
    {"skript", "skript|script|icarus|behaved|behavedit|脚本|スクリプト"},
    {"befehl", "befehl|command|zeile|line|anweisung|命令|指令|コマンド"},
    {"uebersetzen", "uebersetzen|kompilieren|compile|ibi|编译|コンパイル"},
    {"npc", "npc|figur|charakter|character|actor|person|gegner|enemy|角色|人物|敌人|キャラクター|キャラ|敵"},
    {"laufen", "laufen|gehen|walk|run|bewegen|move|navgoal|走路|行走|移动|走|歩く|走る|移動|歩か"},
    {"wegpunkt", "wegpunkt|waypoint|navgoal|waypoints|路径点|路点|ウェイポイント"},
    {"karte", "karte|map|level|bsp|地图|关卡|マップ|レベル"},
    {"karteneditor", "karteneditor|map editor|radiant|netradiant|gtkradiant|地图编辑器|マップエディター|マップエディタ"},
    {"pinsel", "brush|pinsel|block|wand|wall|raum|room|quader|画刷|方块|房间|墙|ブラシ|部屋|壁"},
    {"textur", "textur|texture|shader|material|tapete|纹理|贴图|材质|テクスチャ|シェーダー"},
    {"licht", "licht|light|beleuchtung|lighting|lampe|lamp|dunkel|dark|finster|zu hell|heller|brighter|灯光|光照|灯|太暗|ライト|照明|明るく|暗い|暗す"},
    {"spawn", "info_player_start|spawn|spielerstart|startpunkt|player start|startposition|出生点|起点|玩家起点|スポーン|開始位置"},
    {"bauen", "bauen|build|compile map|q3map2|kompilieren|vis|lightmap|构建|编译地图|ビルド"},
    {"optimierer", "optimierer|optimizer|optimieren|optimize|limits|grenzen|zu gross|too big|优化|最適化|制限"},
    {"pk3", "pk3|packen|pack|archiv|archive|zip|veroeffentlichen|release|ins spiel|into the game|打包|发布|パック|配布"},
    {"spielen", "im editor spielen|play in editor|testen|test|ausprobieren|walk through|durchlaufen|试玩|测试|テストプレイ|歩き回"},
    {"speichern", "speichern|save|sichern|保存|セーブ"},
    {"oeffnen", "oeffnen|open|laden|load|打开|载入|加载|開く|読み込"},
    {"neu", "neu|new|erstellen|create|anlegen|machen|make|新建|创建|制作|新規|作成|作る"},
    {"rueckgaengig", "rueckgaengig|undo|zuruecknehmen|wiederherstellen|redo|撤销|重做|元に戻|やり直"},
    {"suchen", "suchen|finden|find|search|ersetzen|replace|查找|搜索|替换|検索|置換|探"},
    {"einstellungen", "einstellungen|settings|preferences|optionen|options|prefs|konfigurieren|设置|选项|設定|オプション"},
    {"sprache", "sprache|language|deutsch|german|englisch|english|chinesisch|chinese|japanisch|japanese|语言|中文|言語|日本語"},
    {"thema", "thema|theme|design|dark mode|dunkles design|farbschema|colors|aussehen|主题|深色|颜色|テーマ|ダーク"},
    {"update", "update|aktualisieren|neue version|new version|upgrade|aktualisierung|更新|升级|アップデート"},
    {"modell", "modell|model|glm|ghoul2|mesh|md3|模型|モデル"},
    {"skin", "skin|haut|kleidung|outfit|皮肤|スキン"},
    {"animation", "animation|animieren|gla|pose|anim|动画|アニメーション|アニメ"},
    {"mission", "mission|missionen|missions|einsatz|任务|ミッション"},
    {"tutorial", "tutorial|anleitung|lernen|learn|schritt fuer schritt|step by step|tour|einfuehrung|教程|教学|入门|チュートリアル|入門"},
    {"handbuch", "handbuch|manual|guide|dokumentation|documentation|doku|手册|说明|マニュアル|ガイド"},
    {"tasten", "tastenkuerzel|shortcut|shortcuts|tasten|keys|hotkey|keyboard|tastatur|快捷键|键盘|ショートカット|キー"},
    {"entity", "entity|entitaet|entities|objekt|object|实体|エンティティ"},
    {"tuer", "tuer|door|func_door|tor|门|ドア|扉"},
    {"trigger", "trigger|ausloeser|触发器|触发|トリガー"},
    {"ton", "ton|sound|audio|lautstaerke|volume|musik|music|geraeusch|声音|音量|音乐|音楽|サウンド|音"},
    {"dialog", "dialog|sprechen|sagen|talk|say|says|voice|stimme|sprachausgabe|untertitel|subtitle|对话|台词|说话|台詞|会話|セリフ|話す"},
    {"saber", "lichtschwert|saber|lightsaber|光剑|ライトセーバー"},
    {"kampf", "kampf|fight|combat|angreifen|attack|kaempfen|战斗|攻击|戦闘|攻撃"},
    {"signal", "signal|warten|wait|waitsignal|信号|等待|待つ|待機"},
    {"variable", "variable|variablen|var|变量|変数"},
    {"zeitleiste", "zeitleiste|timeline|tracks|spuren|timing|zeitlich|时间轴|时间线|タイムライン"},
    {"caulk", "caulk|autocaulk|unsichtbare flaechen|填缝|コーク"},
    {"hohl", "hohl|hollow|aushoehlen|csg|挖空|空心|中空|くり抜"},
    {"ordner", "pfad|pfade|ordner|folder|directory|gamedata|spielordner|game folder|文件夹|路径|目录|フォルダ|パス"},
    {"backup", "backup|sicherung|sicherungskopie|autosave|备份|バックアップ"},
    {"fehler", "fehler|error|warnung|warning|problem|meldung|meldungen|kaputt|broken|错误|警告|问题|エラー|問題"},
    {"leck", "leck|leak|leaked|undicht|泄漏|リーク"},
    {"ki", "ki|ai|claude|assistent|assistant|chatbot|mcp|llm|人工智能|助手|アシスタント"},
    {"graph", "graph|flussdiagramm|flow|diagramm|knoten|图表|流程|グラフ|フロー"},
    {"lesezeichen", "lesezeichen|bookmark|书签|ブックマーク"},
    {"skalierung", "dpi|skalierung|scale|schriftgroesse|font size|groesser|bigger|缩放|字体|拡大|文字サイズ"},
    {"mehrspieler", "multiplayer|mehrspieler|mp|多人|マルチプレイ"},
    {"effekt", "effekt|effect|efx|partikel|particle|feuer|fire|rauch|smoke|特效|粒子|エフェクト"},
    {"spiel", "spiel|game|jedi academy|jka|jedi outcast|jk2|游戏|ゲーム"},
    {"verlauf", "verlauf|history|aenderungen|changes|changelog|neuigkeiten|whats new|更新日志|变更|変更履歴"},
    {"modus", "modus|mode|modusleiste|mode bar|bereich|模式|モード"},
};

struct ZielRoh {
    const char* id;
    const char* bereich;
    const char* woerter;
};

const ZielRoh kZiele[] = {
    // --- Karte (Karten-Editor) -------------------------------------------------
    {"tour:tut-karte", "karte",
     "erste karte bauen eigene karte erstellen level machen first map make a level build my own map create a map "
     "制作第一张地图 做地图 自己的地图 初めてのマップ マップを作る 自分のマップ"},
    {"tour:tut-karte@2", "karte", "raum bauen brush ziehen wand bauen draw a brush make a room 画刷 画一个房间 ブラシを描く 部屋を作る"},
    {"tour:tut-karte@3", "karte", "hoeher machen brush groesser ziehen make it taller resize brush 拉高 高くする"},
    {"tour:tut-karte@4", "karte", "textur anwenden textur zuweisen apply texture put a texture on 应用纹理 贴纹理 テクスチャを貼る テクスチャを適用"},
    {"tour:tut-karte@5", "karte", "raum aushoehlen hohl machen hollow a brush 挖空 中空にする くり抜く"},
    {"tour:tut-karte@7", "karte", "licht setzen licht hinzufuegen lampe add a light lighting 添加灯光 放灯 ライトを置く 照明を追加"},
    {"tour:tut-karte@8", "karte", "spielerstart setzen spawn punkt player start where the player starts 玩家出生点 出生点 スポーン地点 開始位置"},
    {"tour:tut-karte@10", "karte", "aussenseiten caulk autocaulk unsichtbare flaechen 填缝 コーク"},
    {"tour:tut-karte@11", "karte", "optimierer benutzen karte pruefen grenzen use the optimizer check limits 使用优化器 检查地图 最適化 制限を確認"},
    {"tour:tut-karte@12", "karte", "karte bauen kompilieren bsp erzeugen compile the map build q3map2 编译地图 构建地图 マップをビルド コンパイル"},
    {"tour:tut-karte@13", "karte", "im editor spielen durchlaufen herumlaufen durch die karte laufen play in editor walk around 在编辑器中试玩 走一走 エディターで遊ぶ 歩き回る"},
    {"tour:tut-karte@14", "karte", "karte als pk3 packen ins spiel bringen veroeffentlichen pack map as pk3 release 打包成pk3 放进游戏 pk3にパック ゲームに入れる"},
    {"ort:knopf:ViewMapEditor@modebar", "karte", "karteneditor oeffnen open the map editor radiant 打开地图编辑器 マップエディターを開く"},
    // --- Skripte -----------------------------------------------------------------
    {"tour:k-neues-skript", "skript", "erstes skript schreiben neues skript anfangen write my first script new script 写第一个脚本 新建脚本 最初のスクリプト 新しいスクリプト"},
    {"tour:k-befehl-einfuegen", "skript", "befehl einfuegen zeile hinzufuegen insert a command add a line 插入命令 添加一行 コマンドを挿入 行を追加"},
    {"tour:k-werte-bearbeiten", "skript", "werte aendern parameter bearbeiten edit values change parameters 修改数值 编辑参数 値を変更 パラメーター"},
    {"tour:k-zeilen-ordnen", "skript", "zeilen verschieben reihenfolge aendern move lines reorder 移动行 调整顺序 行を移動 並べ替え"},
    {"tour:k-uebersetzen", "skript", "skript uebersetzen kompilieren ibi erzeugen compile script make ibi 编译脚本 生成ibi スクリプトをコンパイル"},
    {"seite:k-suchen-ersetzen.html", "skript", "suchen ersetzen find and replace 查找替换 検索と置換"},
    {"seite:k-rueckgaengig.html", "skript", "rueckgaengig machen fehler zuruecknehmen undo a mistake 撤销 元に戻す"},
    {"seite:k-pruefen.html", "skript", "skript pruefen fehler finden check script for errors 检查脚本错误 スクリプトのエラーを確認"},
    // --- Vorschau, Zwischensequenzen -----------------------------------------------
    {"seite:k-kamera-ansicht.html", "vorschau",
     "kamerafahrt erstellen kamera setzen kamera einstellung camera shot camera path make a camera move "
     "镜头运动 设置镜头 创建镜头 カメラワーク カメラを置く カメラの移動"},
    {"seite:tut-projekt.html", "vorschau",
     "zwischensequenz erstellen cutscene machen erste szene eigene mission make a cutscene cinematic 制作过场动画 第一个过场 カットシーンを作る"},
    {"seite:tut-p4.html", "vorschau", "kamera fuer die zwischensequenz kamerafahrt camera for the cutscene 过场动画的镜头 カットシーンのカメラ"},
    {"seite:tut-p3.html", "vorschau", "figuren laufen lassen let them walk characters walk 让角色走 キャラを歩かせる"},
    {"seite:tut-p8.html", "vorschau", "mission speichern und im spiel spielen save and play in the game 保存并在游戏中运行 保存してゲームで遊ぶ"},
    {"seite:k-gangart.html", "vorschau", "npc laufen lassen figur gehen lassen make an npc walk run 让角色走路 NPC行走 NPCを歩かせる キャラを歩かせる"},
    {"seite:k-wegpunkte.html", "vorschau", "wegpunkte setzen navgoal waypoints path for npc 路径点 ウェイポイント"},
    {"seite:k-animation-abspielen.html", "vorschau", "animation abspielen figur animieren play an animation 播放动画 アニメーションを再生"},
    {"seite:k-dialog.html", "vorschau", "dialog sprechen lassen stimme untertitel make a character talk dialogue 让角色说话 对话 会話 セリフ"},
    {"seite:zeitleiste.html", "vorschau", "zeitleiste timing tracks timeline 时间轴 タイムライン"},
    {"ort:knopf:ViewMap@modebar", "vorschau", "vorschau karte ansehen preview map 3d 预览地图 マップのプレビュー"},
    // --- Missionen -------------------------------------------------------------------
    {"seite:tut-p1.html", "mission", "eigene mission machen mission erstellen neue mission make a mission create a mission 制作任务 创建任务 ミッションを作る"},
    {"ort:menue:MenuFile>LoadMission", "mission", "mission laden mission oeffnen load a mission open mission 载入任务 ミッションを読み込む"},
    {"seite:k-pk3.html", "mission", "mission als pk3 packen pack mission pk3 打包任务 ミッションをパック"},
    // --- Modelle -----------------------------------------------------------------------
    {"ort:menue:MenuFile>OpenGlm", "modell", "modell oeffnen glm laden open a model load glm 打开模型 モデルを開く"},
    {"ort:knopf:ViewModel@modebar", "modell", "modell ansehen modellansicht view a model 查看模型 モデルを見る"},
    {"seite:k-skins.html", "modell", "skin wechseln skin aendern change skin 更换皮肤 スキンを変える"},
    {"seite:k-modell.html", "modell", "modell in der karte zeigen model in the map 模型 モデル"},
    // --- Einstellungen, Updates, Hilfe ------------------------------------------------------
    {"ort:menue:MenuEdit>AppPrefs", "einstellungen", "einstellungen oeffnen optionen settings preferences 打开设置 设定 設定を開く"},
    {"ort:menue:MenuView>ViewLanguage", "einstellungen", "sprache aendern sprache umstellen change the language 更改语言 切换语言 言語を変更"},
    {"ort:menue:MenuView>ViewTheme", "einstellungen", "dunkles design thema farben dark mode theme 深色模式 主题 ダークモード テーマ"},
    {"ort:menue:MenuView>ViewDpiScale", "einstellungen", "schrift groesser skalierung text too small bigger font 字体太小 放大 文字を大きく"},
    {"ort:menue:MenuEdit>KeysTitle", "einstellungen", "tastenkuerzel aendern tasten belegen change shortcuts keyboard 修改快捷键 ショートカットを変更"},
    {"ort:menue:MenuEdit>PathsTitle", "einstellungen", "spielordner pfade einstellen game folder paths where is the game 游戏路径 游戏文件夹 ゲームのフォルダ"},
    {"ort:menue:MenuHelp>UpdMenu", "updates", "nach updates suchen neue version check for updates new version 检查更新 新版本 アップデートを確認 新しいバージョン"},
    {"ort:menue:MenuHelp>TourMenu", "hilfe", "alle tutorials liste interaktive anleitung all tutorials 所有教程 チュートリアル一覧"},
    {"ort:menue:MenuFile>FileBackupFolder", "programm", "backup ordner sicherungen finden backup folder 备份文件夹 バックアップフォルダ"},
};
