# -*- coding: utf-8 -*-
"""Oberflaechentexte des Menue-Editors, Stufe M2, fuer tools/gen_i18n.py."""

# (Kennung, English, Deutsch, 中文, 日本語)
MENUE2 = [
    # Punkt 2: Videos
    ("MeVideoImport", "Import video...", "Video importieren...", "导入视频...", "動画をインポート..."),
    ("MeVideoTitel", "Video to RoQ", "Video nach RoQ", "视频转换为 RoQ", "動画を RoQ に変換"),
    ("MeVideoGuete", "Quality", "Qualität", "质量", "画質"),
    ("MeVideoFps", "Frames per second", "Bilder pro Sekunde", "每秒帧数", "フレームレート"),
    ("MeVideoGroesse", "Longest side (pixels)", "Längste Seite (Pixel)", "最长边(像素)", "長辺(ピクセル)"),
    ("MeVideoStart", "Encode", "Kodieren", "编码", "エンコード"),
    ("MeVideoLaeuft", "Encoding frame %d of %d...", "Kodiere Bild %d von %d...", "正在编码第 %d 帧(共 %d 帧)...",
     "%d / %d フレームをエンコード中..."),
    ("MeVideoFertig", "Video stored in the package as %s.", "Video im Paket als %s abgelegt.", "视频已作为 %s 存入包中。",
     "動画をパッケージに %s として保存しました。"),
    ("MeVideoFehler", "The video could not be read: %s", "Das Video ließ sich nicht lesen: %s", "无法读取视频:%s",
     "動画を読み込めませんでした: %s"),
    # Punkt 1: Spieltakt fuer Animationen
    ("MeSpielFps", "Game frame rate", "Bildrate des Spiels", "游戏帧率", "ゲームのフレームレート"),
    # Punkt 3: Menuesatz
    ("MeMenuesatz", "Menu set", "Menüsatz", "菜单集", "メニューセット"),
    ("MeMenuesatzOeffnen", "Open whole menu set (menus.txt)", "Ganzen Menüsatz öffnen (menus.txt)", "打开整个菜单集(menus.txt)",
     "メニューセット全体を開く(menus.txt)"),
    # Punkt 4: Bauteil-Bibliothek
    ("MeBibliothek", "Library", "Bibliothek", "组件库", "ライブラリ"),
    ("MeBibliothekHinweis", "Drag onto the canvas or double-click to insert.", "Zum Einsetzen auf die Fläche ziehen oder doppelklicken.",
     "拖到画布上或双击以插入。", "キャンバスへドラッグするかダブルクリックで挿入します。"),
    ("MeVorlageSpeichern", "Save selection as template...", "Auswahl als Vorlage speichern...", "将所选内容另存为模板...",
     "選択をテンプレートとして保存..."),
    ("MeVorlageName", "Template name", "Name der Vorlage", "模板名称", "テンプレート名"),
    ("MeBtKnopf", "Button", "Knopf", "按钮", "ボタン"),
    ("MeBtSchieber", "Slider with cvar", "Schieber mit Cvar", "带 cvar 的滑块", "cvar 付きスライダー"),
    ("MeBtJaNein", "Yes/No", "Ja/Nein", "是/否", "はい/いいえ"),
    ("MeBtReiter", "Tab bar", "Reiterleiste", "标签栏", "タブバー"),
    ("MeBtDetail", "Detail area (3 levels)", "Detailbereich (3 Ebenen)", "详情区(3 级)", "詳細エリア(3 階層)"),
    ("MeBtListe", "List", "Liste", "列表", "リスト"),
    # Punkt 5: Klaenge
    ("MeKlaenge", "Sounds", "Klänge", "声音", "サウンド"),
    # Punkt 6: Mehrsprachigkeit
    ("MeSprache", "Language", "Sprache", "语言", "言語"),
    ("MeTexte", "Texts", "Texte", "文本", "テキスト"),
    ("MeTextNeu", "New @MENUS text", "Neuer @MENUS-Text", "新建 @MENUS 文本", "新しい @MENUS テキスト"),
    ("MeTextSchluessel", "Key", "Schlüssel", "键", "キー"),
    ("MeTextFehlt", "(missing)", "(fehlt)", "(缺失)", "(なし)"),
    ("MeTexteImPaket", "%d texts in the package (%s)", "%d Texte im Paket (%s)", "包中有 %d 条文本(%s)",
     "パッケージ内のテキスト %d 件(%s)"),
    # Punkt 7: Exportpruefungen
    ("MePruefen", "Check for the game", "Für das Spiel prüfen", "按游戏检查", "ゲーム向けにチェック"),
    ("MePruefungOk", "No problems found.", "Keine Probleme gefunden.", "未发现问题。", "問題は見つかりませんでした。"),
    ("MePruefungAnzahl", "%d problems found.", "%d Probleme gefunden.", "发现 %d 个问题。", "%d 件の問題が見つかりました。"),
    ("MeExportAenderungen", "Export changes only...", "Nur Änderungen exportieren...", "仅导出更改...", "変更分のみエクスポート..."),
    ("MeExportNichts", "Nothing changed - no package written.", "Nichts geändert - kein Paket geschrieben.", "没有更改——未写入包。",
     "変更がないため、パッケージは書き出しませんでした。"),
    # Punkt 8: Im Spiel testen
    ("MeImSpielTesten", "Test in game", "Im Spiel testen", "在游戏中测试", "ゲームでテスト"),
    ("MeTestBereit", "Test folder %s ready - starting: %s", "Testordner %s bereit - Start: %s", "测试文件夹 %s 已就绪——启动:%s",
     "テストフォルダー %s を準備しました - 起動: %s"),
    ("MeTestKeinSpiel", "No game program found next to the game folders.", "Kein Spielprogramm neben den Spielordnern gefunden.",
     "在游戏文件夹旁未找到游戏程序。", "ゲームフォルダーの隣にゲームプログラムが見つかりません。"),
    # Punkt 9: Schriften
    ("MeSchriftBauen", "Font from TTF...", "Schrift aus TTF...", "从 TTF 生成字体...", "TTF からフォントを作成..."),
    ("MeSchriftName", "Font name", "Name der Schrift", "字体名称", "フォント名"),
    ("MeSchriftGroesse", "Size (points)", "Größe (Punkte)", "大小(磅)", "サイズ(ポイント)"),
    ("MeSchriftFertig", "Font stored in the package as fonts/%s.fontdat.", "Schrift im Paket als fonts/%s.fontdat abgelegt.",
     "字体已作为 fonts/%s.fontdat 存入包中。", "フォントをパッケージに fonts/%s.fontdat として保存しました。"),
    ("MeSchriftFehler", "The TTF file could not be read.", "Die TTF-Datei ließ sich nicht lesen.", "无法读取 TTF 文件。",
     "TTF ファイルを読み込めませんでした。"),
]
