# BehavEd-Remake rc605

Consolidated changelog of all rc605 work (everything since rc604 8cc7bd1, branches fix605 and fix605r), the final test round and what is still open.
The changelog is written for users in four languages; the development notes with the engine references and red/green tables stay in NOTIZEN_rc605.md and NOTIZEN_rc605r.md.

---

## English

### Scripts
- Scripts started by an entity read that entity's parm keys: `affect ( $get( STRING, "SET_PARM1" )$ )` and `move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` move the entity and tag named there, like the game (the first elevator in vjun3 now rises; the force fields in md_dotf move to their tags); `move` also takes a tag as its destination.
- When a script uses a `target_level_change`, the preview ends there like the game: timeline, camera and figures stop at that moment, and the status list names the next map.
- A `target_level_change` whose map name starts with `+` (OpenJK and Movie Duels, such as academy's `+saberMenu` or `+creditsingame`) only opens that menu and loads no map, like the game: the preview goes on and the status list names the menu; while the player is dead (after `SET_MISSIONFAILED`) no level change happens.
- `SET_MISSIONFAILED` works like the game: the player plays his guilt animation, and every script stops as soon as no camera is running (after a cutscene's `camera ( DISABLE )`); figures already walking finish their walk, and the status list names both times.
- `SET_HEALTH 0` on the player stops all scripts at once, like the game.

### Walking, riding and jumping
- Figures standing on a moving lift or platform ride along with it, also while they walk (vjun3: the troopers on the first lift).
- Doors, lifts and trains stop when a figure is in their way, like the game: a door turns back (a crusher keeps pressing), a train waits, and the figure takes their damage; a figure riding a lift that would be pressed against something stays behind or holds the lift.
- NPCs falling from a ledge, stepping off with a jump too low to leave the ground, or spawned in the air play the game's in-air and landing animations (a hard landing slows them down, like the game); an upper-body script animation that began before a jump no longer comes back after it.

### Damage and death
- Figures that walk or fall into a `trigger_hurt` take its damage like the game and die when their health runs out - they play their death and their `deathscript` runs (md_gd: Anakin falling into the pit).
- trigger_hurt: armour takes its share (not in a fall), figures touch the areas in the game's order, and with `SET_UNDYING` the deathscript runs again with every hit that would kill; a figure moved by `SET_ORIGIN` touches an area only from the next frame.
- An exploding laser trip mine hurts the figures near it like the game's blast damage (less with distance, not through walls) and sets off the mines near it - mines hung close together go off together (t3_byss); a figure whose health runs out dies and its deathscript runs.
- Armour (`SET_ARMOR`, assassin droids, droidekas) takes its share of the damage from trip mines and trigger_hurt, and a rancor takes at most 10 per hit, like the game.

### Trip mines and beams
- Laser trip mine beams (and fx_target_beam shots) stop at other trip mines and at solid breakable models, and the weapon in a figure's hand sets a mine off too - like the game.

### Figures
- NPCs with a gun hold it with their upper body like the game when no script animation plays there (rifle at the ready, pistol in front; under Movie Duels a lowered rifle, carried after `SET_WALKING`); this works under OpenJK, Movie Duels and Jedi Outcast.

### World and effects
- World shaders with `tcGen vector` (the terrain of duel_jedi_battleground, the star wall on duel_tower) are textured like the game; `tcMod entityTranslate` (conveyor belts) is checked to change nothing in the game either.
- Grass and rain splashes (`surfaceSprites`) also grow on brush entities, moving with them and only while they are shown - for example on a wall that appears when it is used; a shader whose first stage carries surfaceSprites no longer shows that stage as a flat layer, like the game.
- Models carried by effects (`useModel`, such as the fog wisps on duel_exegol) are drawn again, and surfaces with `deformVertexes autosprite`/`autosprite2` on models turn towards the camera like the game; the 377 fog models of duel_exegol cost about 0.1 ms per frame.
- Effect images without a shader script are drawn like the game's built-in shader for them: blended by their alpha with the particle's colour and fading, and visible through walls (before: a guessed blend mode).

### Jedi Outcast
- An NPC that cannot get around another NPC pushes it aside - also one that stands without a goal or has already arrived, never the player - and two NPCs blocking each other keep going, like the game.
- A connection blocked by a locked door, something breakable or another NPC counts as failed for all NPCs, who route around it until it is free again; blocked connections are re-checked every 1.5 s like the game (first check 0.75 s after the script starts) and stay open once a check finds them clear.
- A Kyle NPC takes over the player's weapon whenever a script changes it outside the camera, until a script gives Kyle a weapon of his own - like the game.

### Self-test and checks
- The walk overview names the scene end or combat behind figures that never arrive (removed, level change, `SET_ENEMY`) and counts landing and in-air animations without a jump.
- The map audit no longer counts `tcGen vector` and `tcMod entityTranslate` as world shader gaps.
- The guide describes the new behaviour in all four languages.

---

## Deutsch

### Skripte
- Skripte, die eine Entity startet, lesen deren parm-Schlüssel: `affect ( $get( STRING, "SET_PARM1" )$ )` und `move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` bewegen die dort genannte Entity zum genannten Tag, wie im Spiel (der erste Aufzug in vjun3 fährt jetzt hoch; die Kraftfelder in md_dotf fahren zu ihren Tags); `move` nimmt auch ein Tag als Ziel.
- Benutzt ein Skript einen `target_level_change`, endet die Vorschau dort wie im Spiel: Zeitleiste, Kamera und Figuren hören in diesem Moment auf, und die Statusliste nennt die nächste Karte.
- Ein `target_level_change`, dessen Kartenname mit `+` beginnt (OpenJK und Movie Duels, etwa `+saberMenu` in academy oder `+creditsingame`), öffnet wie im Spiel nur dieses Menü und lädt keine Karte: die Vorschau läuft weiter, und die Statusliste nennt das Menü; solange der Spieler tot ist (nach `SET_MISSIONFAILED`), wechselt keine Karte.
- `SET_MISSIONFAILED` wirkt wie im Spiel: der Spieler spielt seine Schuld-Animation, und alle Skripte halten an, sobald keine Kamera läuft (nach dem `camera ( DISABLE )` einer Zwischensequenz); Figuren, die schon gehen, gehen ihren Weg zu Ende, und die Statusliste nennt beide Zeiten.
- `SET_HEALTH 0` auf den Spieler hält wie im Spiel sofort alle Skripte an.

### Gehen, Mitfahren und Springen
- Figuren auf einem fahrenden Aufzug oder einer Plattform fahren mit, auch während sie gehen (vjun3: die Soldaten auf dem ersten Aufzug).
- Türen, Aufzüge und Züge halten an, wenn ihnen eine Figur im Weg ist, wie im Spiel: eine Tür kehrt um (ein Crusher drückt weiter), ein Zug wartet, und die Figur nimmt ihren Schaden; eine Figur auf einem Aufzug, die gegen etwas gedrückt würde, bleibt zurück oder hält den Aufzug auf.
- NPCs, die von einer Kante fallen, mit einem zu flachen Sprung über die Kante gehen oder in der Luft gespawnt werden, spielen die Luft- und Landeanimationen des Spiels (eine harte Landung bremst sie wie im Spiel); eine Skriptanimation auf dem Oberkörper, die vor einem Sprung begann, kommt danach nicht mehr wieder.

### Schaden und Tod
- Figuren, die in einen `trigger_hurt` laufen oder fallen, nehmen dessen Schaden wie im Spiel und sterben, wenn ihre Lebenspunkte aufgebraucht sind - sie spielen ihren Tod, und ihr `deathscript` läuft (md_gd: Anakin stürzt in die Grube).
- trigger_hurt: Rüstung fängt ihren Teil ab (nicht beim Sturz), Figuren berühren die Bereiche in der Reihenfolge des Spiels, und mit `SET_UNDYING` läuft das deathscript bei jedem tödlichen Treffer erneut; eine per `SET_ORIGIN` versetzte Figur berührt einen Bereich erst ab dem nächsten Bild.
- Eine explodierende Laserfalle verletzt die Figuren in ihrer Nähe wie der Explosionsschaden des Spiels (mit dem Abstand weniger, nicht durch Wände) und sprengt die Fallen in ihrer Nähe mit - dicht beieinander hängende Fallen gehen zusammen hoch (t3_byss); eine Figur, deren Lebenspunkte aufgebraucht sind, stirbt, und ihr deathscript läuft.
- Rüstung (`SET_ARMOR`, Attentäterdroiden, Droidekas) fängt ihren Teil des Schadens von Laserfallen und trigger_hurt ab, und ein Rancor nimmt je Treffer höchstens 10, wie im Spiel.

### Laserfallen und Strahlen
- Strahlen von Laserfallen (und Schüsse von fx_target_beam) enden an anderen Laserfallen und an festen zerbrechlichen Modellen, und auch die Waffe in der Hand einer Figur löst eine Falle aus - wie im Spiel.

### Figuren
- NPCs mit einer Schusswaffe halten sie wie im Spiel mit dem Oberkörper, solange dort keine Skriptanimation läuft (Gewehr im Anschlag, Pistole vor sich; unter Movie Duels ein gesenktes Gewehr, nach `SET_WALKING` getragen); das gilt unter OpenJK, Movie Duels und Jedi Outcast.

### Welt und Effekte
- Weltshader mit `tcGen vector` (das Gelände von duel_jedi_battleground, die Sternwand in duel_tower) sind texturiert wie im Spiel; `tcMod entityTranslate` (Förderbänder) ändert auch im Spiel nichts - geprüft.
- Gras und Regenspritzer (`surfaceSprites`) wachsen auch auf Brush-Entities, bewegen sich mit ihnen und nur, solange sie gezeigt werden - etwa auf einer Wand, die beim Benutzen erscheint; ein Shader, dessen erste Stufe surfaceSprites trägt, zeigt diese Stufe wie im Spiel nicht mehr als flache Schicht.
- Modelle, die Effekte mitführen (`useModel`, etwa die Nebelschwaden in duel_exegol), werden wieder gezeichnet, und Flächen mit `deformVertexes autosprite`/`autosprite2` an Modellen drehen sich wie im Spiel zur Kamera; die 377 Nebelmodelle von duel_exegol kosten rund 0,1 ms je Bild.
- Effektbilder ohne Shaderskript zeichnet die Vorschau wie den eingebauten Shader des Spiels: nach ihrem Alpha gemischt, mit Farbe und Ausblenden des Teilchens und durch Wände sichtbar (vorher: eine geschätzte Mischart).

### Jedi Outcast
- Ein NPC, der an einem anderen NPC nicht vorbeikommt, schiebt ihn beiseite - auch einen, der ohne Ziel steht oder schon angekommen ist, nie den Spieler -, und zwei NPCs, die sich gegenseitig blockieren, gehen weiter, wie im Spiel.
- Eine Verbindung, die eine verschlossene Tür, etwas Zerbrechliches oder ein anderer NPC sperrt, gilt für alle NPCs als gescheitert, sie gehen außen herum, bis sie wieder frei ist; gesperrte Verbindungen werden wie im Spiel alle 1,5 s neu geprüft (zuerst 0,75 s nach dem Skriptanfang) und bleiben offen, sobald eine Prüfung sie frei findet.
- Ein Kyle-NPC übernimmt die Waffe des Spielers, wenn ein Skript sie außerhalb der Kamera wechselt, bis ein Skript Kyle eine eigene Waffe gibt - wie im Spiel.

### Selbsttest und Prüfungen
- Die Weg-Übersicht nennt Szenenende oder Kampf hinter nie angekommenen Figuren (entfernt, Kartenwechsel, `SET_ENEMY`) und zählt Lande- und Luftanimationen ohne Sprung.
- Die Kartenprüfung zählt `tcGen vector` und `tcMod entityTranslate` nicht mehr als Weltshader-Lücken.
- Das Handbuch beschreibt das neue Verhalten in allen vier Sprachen.

---

## 中文

### 脚本
- 由实体启动的脚本会读取该实体的 parm 键:`affect ( $get( STRING, "SET_PARM1" )$ )` 和 `move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` 会像游戏中一样把其中指定的实体移到指定的标记(vjun3 的第一部电梯现在会上升;md_dotf 的力场会移到各自的标记);`move` 也接受标记作为目标。
- 当脚本使用 `target_level_change` 时,预览会像游戏一样在此结束:时间轴、摄像机和角色在那一刻停止,状态列表会显示下一张地图。
- 地图名以 `+` 开头的 `target_level_change`(OpenJK 和 Movie Duels,例如 academy 的 `+saberMenu` 或 `+creditsingame`)与游戏一样只打开该菜单而不加载地图:预览继续进行,状态列表会显示该菜单;玩家死亡期间(`SET_MISSIONFAILED` 之后)不会切换关卡。
- `SET_MISSIONFAILED` 与游戏中一样生效:玩家播放愧疚动画,只要没有摄像机在运行(过场的 `camera ( DISABLE )` 之后),所有脚本就会停止;已在行走的角色会走完路程,状态列表会显示这两个时间。
- 对玩家使用 `SET_HEALTH 0` 会像游戏中一样立即停止所有脚本。

### 行走、随乘与跳跃
- 站在移动的电梯或平台上的角色会随之移动,行走时也是如此(vjun3:第一部电梯上的士兵)。
- 门、升降机和列车被角色挡住时会像游戏中一样停下:门会反向(碾压门会继续压),列车会等待,角色会受到它们的伤害;升降机上会被挤到什么东西上的角色会留在原地或挡住升降机。
- 从边缘掉落、以过低的跳跃越过边缘或在空中生成的 NPC 会播放游戏的空中和落地动画(与游戏中一样,重落地会让它们减速);跳跃前开始的上半身脚本动画在跳跃后不再恢复。

### 伤害与死亡
- 走进或掉进 `trigger_hurt` 的角色会像游戏中一样受到伤害,生命值耗尽时死亡——播放死亡动画并运行其 `deathscript`(md_gd:Anakin 掉进深坑)。
- trigger_hurt:护甲会吸收一部分伤害(坠落时除外),角色按游戏中的顺序接触区域,设置 `SET_UNDYING` 时每次致命伤害都会再次运行 deathscript;通过 `SET_ORIGIN` 移动的角色从下一帧起才会接触区域。
- 爆炸的激光绊雷会像游戏的爆炸伤害一样伤害附近的角色(距离越远伤害越小,不会穿墙),并引爆附近的绊雷——挂得很近的绊雷会一起爆炸(t3_byss);生命值耗尽的角色会死亡并运行其 deathscript。
- 护甲(`SET_ARMOR`、刺客机器人、毁灭者机器人)会吸收来自激光绊雷和 trigger_hurt 的一部分伤害,兰科尔兽每次最多受 10 点伤害,与游戏中一样。

### 激光绊雷与光束
- 激光绊雷的光束(以及 fx_target_beam 的射击)会停在其他绊雷和实体可破坏模型处,角色手中的武器也会触发绊雷——与游戏中一样。

### 角色
- 手持枪械的 NPC 在上半身没有脚本动画时,会像游戏中一样用上半身持枪(步枪戒备、手枪在身前;在 Movie Duels 下步枪放低,`SET_WALKING` 后端着);适用于 OpenJK、Movie Duels 和 Jedi Outcast。

### 世界与特效
- 带 `tcGen vector` 的世界着色器(duel_jedi_battleground 的地形、duel_tower 的星形墙)现在与游戏中一样贴图;已核实 `tcMod entityTranslate`(传送带)在游戏中同样不产生变化。
- 草和雨滴溅花(`surfaceSprites`)也会出现在笔刷实体上,随其移动,且只在实体显示时出现——例如被使用后才出现的墙;第一阶段带有 surfaceSprites 的着色器不再把该阶段显示为平面图层,与游戏一致。
- 特效携带的模型(`useModel`,例如 duel_exegol 的雾气)重新被绘制;模型上带 `deformVertexes autosprite`/`autosprite2` 的表面会像游戏中一样朝向镜头;duel_exegol 的 377 个雾气模型每帧约耗时 0.1 毫秒。
- 没有着色器脚本的特效图像按游戏为其内置的着色器绘制:按其 alpha 混合,带有粒子的颜色和淡出,并且可以透过墙壁看到(之前:猜测的混合方式)。

### Jedi Outcast
- 绕不过另一个 NPC 的 NPC 会把对方推到一旁——即使对方没有目标站着不动或已经到达,但绝不推开玩家;两个互相阻挡的 NPC 会继续前进——与游戏中一样。
- 被上锁的门、可破坏物或其他 NPC 挡住的连接对所有 NPC 都算作失败,它们会绕行直到连接重新畅通;被封锁的连接会像游戏中一样每 1.5 秒重新检查一次(首次在脚本开始 0.75 秒后),一旦检查发现其畅通便保持开放。
- 脚本在镜头之外更换玩家武器时,Kyle NPC 会随之换上同样的武器,直到脚本给 Kyle 指定自己的武器为止——与游戏中一样。

### 自检与检查
- 路径总览会指出从未到达的角色背后的场景结束或战斗(被移除、换图、`SET_ENEMY`),并统计没有跳跃的落地和空中动画。
- 地图检查不再把 `tcGen vector` 和 `tcMod entityTranslate` 计为世界着色器缺口。
- 用户手册以四种语言说明了新的行为。

---

## 日本語

### スクリプト
- エンティティが開始したスクリプトは、そのエンティティの parm キーを読みます。`affect ( $get( STRING, "SET_PARM1" )$ )` と `move ( $tag( get( STRING, "SET_PARM1" ), ORIGIN )$, ... )` はゲームと同じく、指定されたエンティティを指定のタグへ動かします(vjun3 の最初のエレベーターが上昇し、md_dotf のフォースフィールドがタグへ動きます)。`move` はタグも目的地として受け付けます。
- スクリプトが `target_level_change` を使うと、プレビューはゲームと同じくそこで終わります。タイムライン、カメラ、キャラクターはその瞬間に止まり、ステータス一覧に次のマップが表示されます。
- マップ名が `+` で始まる `target_level_change`(OpenJK と Movie Duels。たとえば academy の `+saberMenu` や `+creditsingame`)は、ゲームと同じくそのメニューを開くだけでマップを読み込みません。プレビューは続き、ステータス一覧にそのメニューが表示されます。プレイヤーが死んでいる間(`SET_MISSIONFAILED` のあと)はマップが切り替わりません。
- `SET_MISSIONFAILED` はゲームと同じく働きます。プレイヤーは後悔のアニメーションを再生し、カメラが動いていなくなると(カットシーンの `camera ( DISABLE )` のあと)すべてのスクリプトが止まります。すでに歩いているキャラクターは道のりを歩き終え、ステータス一覧に両方の時刻が表示されます。
- プレイヤーへの `SET_HEALTH 0` はゲームと同じくすぐにすべてのスクリプトを止めます。

### 歩行、同乗、ジャンプ
- 動くエレベーターや足場の上に立つキャラクターは、歩いている間も一緒に動きます(vjun3: 最初のエレベーターの兵士)。
- 扉、リフト、列車はキャラクターが邪魔になるとゲームと同じく止まります。扉は逆戻りし(クラッシャーは押し続けます)、列車は待ち、キャラクターはそのダメージを受けます。リフト上で何かに押し付けられそうなキャラクターはその場に残るか、リフトを止めます。
- 段差から落ちる NPC、低すぎるジャンプで段差を越える NPC、空中にスポーンした NPC が、ゲームの空中アニメーションと着地アニメーションを再生します(ゲームと同じく激しい着地では減速します)。ジャンプ前に始まった上半身のスクリプトアニメーションは、ジャンプ後に戻りません。

### ダメージと死
- `trigger_hurt` に歩いて入ったり落ちたりしたキャラクターはゲームと同じくダメージを受け、体力がなくなると死にます。死亡アニメーションを再生し、`deathscript` が実行されます(md_gd: 穴に落ちるアナキン)。
- trigger_hurt:鎧がダメージの一部を受け止め(落下時を除く)、キャラクターはゲームと同じ順序で領域に触れ、`SET_UNDYING` では致命的な攻撃のたびに deathscript が再び実行されます。`SET_ORIGIN` で移動したキャラクターは次のフレームから領域に触れます。
- 爆発したレーザートラップは、ゲームの爆発ダメージと同じく近くのキャラクターを傷つけ(距離が離れるほど弱く、壁越しには届きません)、近くのマインを誘爆させます。近くに並べたマインは一緒に爆発します(t3_byss)。体力がなくなったキャラクターは死に、deathscript が実行されます。
- 鎧(`SET_ARMOR`、アサシンドロイド、ドロイデカ)はレーザートラップと trigger_hurt のダメージの一部を受け止め、ランコアは 1 回に最大 10 しか受けません(ゲームと同じ)。

### レーザートラップとビーム
- レーザートリップマインのビーム(と fx_target_beam の射撃)が、ほかのトリップマインや固体の壊せるモデルで止まり、キャラクターが手に持つ武器でもマインが作動します(ゲームと同じ)。

### キャラクター
- 銃を持つ NPC は、上半身でスクリプトのアニメーションが再生されていない間、ゲームと同じく上半身で銃を構えます(ライフルは構え、ピストルは前に。Movie Duels ではライフルを下げ、`SET_WALKING` のあとは抱えて持ちます)。OpenJK、Movie Duels、Jedi Outcast のいずれでも有効です。

### ワールドとエフェクト
- `tcGen vector` を使うワールドシェーダー(duel_jedi_battleground の地形、duel_tower の星の壁)がゲームと同じようにテクスチャ表示されます。`tcMod entityTranslate`(コンベヤーベルト)はゲームでも何も変えないことを確認しました。
- 草や雨の跳ね(`surfaceSprites`)がブラシエンティティ上にも表示され、エンティティと一緒に動き、表示されている間だけ現れます(たとえば使用されて初めて現れる壁)。最初のステージに surfaceSprites を持つシェーダーは、ゲームと同じくそのステージを平面レイヤーとして表示しません。
- エフェクトが持つモデル(`useModel`、たとえば duel_exegol の霧)が再び描画され、モデル上の `deformVertexes autosprite`/`autosprite2` の面はゲームと同じくカメラの方を向きます。duel_exegol の 377 個の霧モデルの負荷は 1 フレームあたり約 0.1 ms です。
- シェーダースクリプトのないエフェクト画像を、ゲームの組み込みシェーダーと同じく描画します。アルファで合成され、パーティクルの色とフェードを使い、壁越しにも見えます(以前は推測した合成方法)。

### Jedi Outcast
- ほかの NPC をよけられない NPC は相手を脇へ押しやります。目的地なしで立っている NPC や到着済みの NPC も押されますが、プレイヤーは押されません。互いにふさぎ合う 2 体はそのまま進みます(ゲームと同じ)。
- 鍵のかかった扉や壊せる物、ほかの NPC にふさがれた接続はすべての NPC にとって失敗扱いになり、空くまで迂回します。ふさがれた接続はゲームと同じく 1.5 秒ごとに再確認され(最初はスクリプト開始の 0.75 秒後)、通れると分かれば以後は開いたままです。
- スクリプトがカメラの外でプレイヤーの武器を変えると、Kyle の NPC も同じ武器に持ち替えます。スクリプトが Kyle に自分の武器を与えるまで続きます(ゲームと同じ)。

### セルフテストとチェック
- 経路の概要は、到着しないキャラクターの背後にあるシーンの終わりや戦闘(削除、マップ切り替え、`SET_ENEMY`)を示し、ジャンプのない着地・空中アニメーションを数えます。
- マップチェックは `tcGen vector` と `tcMod entityTranslate` をワールドシェーダーの欠落として数えなくなりました。
- ガイドは新しい動作を 4 つの言語で説明しています。

---

## Final test round (rc605 final, branch fix605 at 4f17b2d, plus the fixes ea249bd, 329454f, c7a6d52)

All rc605 work in one round: one test process at a time, self-tests at priority Idle with the 8 GB watchdog (never triggered; highest peak 2.7 GB in the walk overview). Three failures were found and fixed in this round (see the red/green table after the results); every self-test mode, mode 1, the performance runs and the walk overview were then run again on the final state (c7a6d52).

| Test | Result |
|---|---|
| clean rebuild (Release, `--clean-first`, 72 targets) | 0 errors |
| ctest (all) | 67 / 67 passed (33 s), again 67 / 67 after the fixes |
| self-test karte | 227 OK / 0 errors (154 s, peak 1.0 GB) |
| self-test kartenfunde | 53 / 0 (118 s, peak 1.3 GB) |
| self-test fix604b | 32 / 0 |
| self-test laserfalle | 8 / 0 (chain blast, figure damage, blocked door, undying deathscript) |
| self-test geschuetzanim | 3 / 0 |
| self-test zielstrahl | 2 / 0 |
| self-test effektgrund | 1 / 0 |
| self-test figurdeform | 1 / 0 |
| self-test jk2 | 15 / 0 |
| self-test tuerlauf (md_am_sith) | 13 / 0 |
| self-test kamera | 95 / 0 |
| self-test missionsbau | 95 / 0 |
| self-test szenen | 26 / 0 |
| self-test ton | 34 / 0 |
| self-test npcklasse | 12 / 0 |
| self-test gluehen | 8 / 0 |
| self-test gras | 13 / 0 |
| self-test spulen | 13 / 0 (101 s, peak 1.5 GB) |
| self-test figurstufen | 8 / 0 |
| self-test missionsliste | 11 / 0 |
| self-test rest605 (tcgen, brushgras, moversprite, fallen, fallenwaffe, effektohne) | 6 / 0 |
| full run (mode 1) | 2645 / 0 (436 s, peak 534 MB) |
| performance (md_am_sith scenes, BHED_D3D_DEBUG=0, rc604 release and rc605 final run back to back) | scene preparation in total 6.12 → 6.24 s; intro2_sith frame mean 13 → 13 ms (max 177 → 177), flight 7.9 → 8.6 ms (max 21 → 21), second pass 9 → 10 ms (max 19 → 20); 37 figures in both - no notable slowdown. Before the fix below, the second pass took 11 ms against 8 ms (two runs each) |
| map audit (278 maps, Movie Duels, compared with the rc604 final audit) | crashed/hanging, not loaded, GPU errors, D3D messages, log errors 0 → 0; world shader gaps 5 → 0 maps (23 649 → 0 triangles: tcGen vector drawn, tcMod entityTranslate counted as the no-op it is in the game); entities not drawn 1 → 1 (md_toaj_jedi "xwing", model missing in the archives); NPCs without model 2 → 2 maps; figure and effect shader gaps 0 → 0; missing efx/sound/texture/model files 27/29/139/5 maps, unchanged; every other per-map number identical; load time median 696 → 638 ms (523 s for all maps) |
| walk overview (`tuerlauf`, all missions, 1711 s, peak 2.7 GB; compared with the rc605 overview before the last items, ue605end) | 163 missions, 1781 tabs, 983 figures with a path (unchanged); never arrive 174 → 179, all five new ones cut by a real level change (md_jt_intro 16 → 21, md_ithote_sith 0 → 1; hoth2 1 → 0); sway 29 → 22 (md_gd_jedi 2 → 0 through trigger_hurt, vjun3 3 → 0 through riding the lift, md_fh1_jedi 1 → 0 through the level change, hoth2 1 → 0); NPC jumps 191 → 81 (md_fh1_jedi 64 → 0 after the level change, md_gd_jedi 49 → 3 after the death in the pit); marked: removed before 4 → 2, level change 21 → 23, combat 20 → 19; new counters: 1440 landing and 33 in-air animations without a jump; 329 OK / 0 errors |
| guide check (`gen_handbuch.py --pruefen`) | ok |

Fixes in this round:

| Item | Red (before) | Green (after) |
|---|---|---|
| Frame time: the NPC weapon pose (2bda191, found by bisecting 8cc7bd1..4f17b2d on the md_am_sith second pass) compared the leg animation against about 80 names, building a temporary string for each - one heap allocation per name, many times per figure and frame (commit ea249bd) | second pass 11-12 ms against 9 ms on the same build directory, scene preparation +0.5-0.9 s; switching the pose off at run time brought it back | allocation-free name comparison: second pass 9-10 ms; the same poses (waffentest, tuerlauf vjun3 4 OK, tuerlauf md_am_sith output identical) |
| Level change with a `+` map name (f201aa9): G_ChangeMap only opens that menu (`uimenu`, the game pauses) and loads no map; the preview ended there. 310 such uses in the overview (+creditsingame 270, +ingameMissionSelect 32, +ingameGotoTier 4, academy +saberMenu/+saberstyleMenu 4). Also ported: no level change while the player is dead (commit 329454f) | walk overview: academy5 npc_jaden1 cut 300 ms before he arrives, md_afif2_jedi 2, md_ga2_jedi 1, md_ga2_sith 1 new never-arriving figures; ablauftest "kartenwechsel" 3 errors | ablauftest 6 ok (Movie Duels and OpenJK go on and record the menu, Jedi Outcast still ends, no change while dead); tuerlauf academy5 npc_jaden1 arrives at 86 850 ms as before, status "Menu saberMenu opens at 86.55 s", md_fh1_jedi still ends at 11.75 s; guide in four languages |
| Walk overview marker "level change" (tool): it counted every use of a target_level_change, also `+` menus, and only walks evaluated beyond it - since f201aa9 a walk ends exactly at the level change (commit c7a6d52) | md_jt_intro 21 never arriving, 0 marked; md_po_jedi/sith marked by the credits menu | md_jt_intro 21 marked, md_fh1_jedi 1, md_po_jedi/sith 0 |

## Still open (rc605 final)

Walking and combat
- Combat AI after `SET_ENEMY` is not simulated (maintainer's decision): the preview walks on to the old navgoal; with it the weapon pose in WEAPON_READY, weaponBusy (shots, weapon change, crouching), the Movie Duels weapon leg animations (BOTH_WALK_BLASTER ...) with g_torsoMap and the blend when the pose changes.
- The game's 15 s teleport of a stuck NPC depends on the player's view and is not simulated; PM_FallToDeath (falling to death) is not ported.
- Map NPCs with their own AI stand at their spawner; named map figures without CINEMATIC appear in every tab; the longest scenes with moving floors still take a few seconds to prepare.
- A `+` menu pauses the game until the player closes it; what the menu does next (credits leave the game, mission select starts another map, academy's saber menu returns) is the player's choice - the preview goes on and names the menu.

Damage and movers
- G_Damage leftovers: knockback and knockdown (G_ApplyKnockback, G_CheckKnockdown) from blasts and crushers, pain reactions, force protection and rage, the random damage of FL_DINDJARIN (Movie Duels), Galak's shield from NPC_GalakMech_Init, the assassin droid's shield (FL_SHIELDED from its AI); the player is never hurt in the preview.
- Movers: figures that a mover pushes away without being blocked (G_TryPushingEntity succeeds) stay where they are; script movers (move/ROFF) are not blocked and do not make their script wait; func_rotating, func_bobbing and func_pendulum do not push; corpses in a door are not removed; vjun3 elevator_up alone leaves the troopers in lift_waypoint_hack (intro.ibi removes it at map start, the single tab does not play intro).
- SET_MISSIONFAILED: the mission-failed screen itself is not shown (status line only); the guilt death plays only when the player exists in the preview; assumed: more than 40 client frames per second.

Jedi Outcast
- NAV_TestBestNode case 4 takes the contents of the surface hit instead of blocker->contents; NAVNEW_TestNodeConnectionBlocked with a navgoal that is itself a body; a pushed figure without a plan keeps its spawn facing.
- A goal without a tag counted as a point, turning to an entity origin to origin, the player's weapon matched only at spawn; subtitles, sounds and effects not compared one by one; Movie Duels' own Jedi Outcast mode (`com_outcast 1`) stays off.

Turrets, trip mines and beams
- Sentry guns do not look for or aim at enemies and do not fire; the ion cannon's second target (target2) is not used; random numbers come from a fixed seed per entity.
- A figure made visible again keeps its contents in the preview (the game keeps none after SET_INVISIBLE false); fx_target_beam damage (no map in the data sets it) and a target removed while the beam runs.

Effects, grass and figures
- deformVertexes normal and text on figures (no such figure shader in the data); the glow pass of movers uses world axes for autosprite (no case in the data); several deforms in one shader are applied in a fixed order (wave, bulge, move).
- surfaceSprites: no random wind gusts (Q_flrand) and no r_windPoint, no blades on effect particles or figures (no such shader in the data), no blades in mirrors; a blade can float or sink by a unit or two on strongly curved patches; additive splashes in global hardware fog are not fogged; the sway phase starts with the scene.
- The Dlight pass of a tcGen vector stage draws with the file texture coordinates.

Other
- md_toaj_jedi misc_model_breakable "xwing": the model is not in the archives (the game shows nothing either).
- ROFF `loop rof`: the unrolled path is capped at 10 minutes.
- Memory: figure models load when the scene is prepared; GPU buffers of released figure models stay until their address is reused.
