"""REL-TEMPO (15.11.): Installieren und aktualisieren - der Starter (JK-Studio-Starter.exe) als ein Download, Updates nur mit
dem, was sich geaendert hat (Deltas), das Zip als Weg ohne Starter.

shank 14.11.: "lassen wir die aktuelle exe einfach weg in Zukunft und bieten eine zentrale an und der Rest wird automatisch
geupdated". Neue Seite "installation" (Gruppe start, nach erste-schritte); dazu nimmt erste-schritte (inhalt_start.py) die
Schritte unter "Installieren" und den Hinweis von hier (statt der eigenstaendigen .exe, die es als Datei des Releases nicht mehr gibt).
Jeder Text einmal in allen sechs Sprachen (en, de, zh, ja, es, cs) wie in inhalt_cwimport.py. Die Knopftexte des Starters
stehen wortgleich wie in tools/gen_starter_texte.py.
"""

X = {
    "titel": ("Install and update", "Installieren und aktualisieren", "安装与更新", "インストールと更新", "Instalar y actualizar",
              "Instalace a aktualizace"),
    "kurz": (
        "One download: JK-Studio-Starter.exe fetches JK Studio, checks it, keeps it up to date and starts it. Updates load only what has "
        "changed since your version.",
        "Ein Download: JK-Studio-Starter.exe holt JK Studio, prüft es, hält es aktuell und startet es. Updates laden nur, was sich seit "
        "deiner Version geändert hat.",
        "只需一次下载:JK-Studio-Starter.exe 会获取 JK Studio、校验它、保持最新并启动它。更新只下载自你的版本以来有变化的部分。",
        "ダウンロードはひとつだけ: JK-Studio-Starter.exe が JK Studio を取得し、確認し、最新に保って起動します。更新はお使いのバージョンから"
        "変わった部分だけを読み込みます。",
        "Una sola descarga: JK-Studio-Starter.exe obtiene JK Studio, lo comprueba, lo mantiene actualizado y lo inicia. Las actualizaciones "
        "solo descargan lo que ha cambiado desde tu versión.",
        "Jedno stažení: JK-Studio-Starter.exe získá JK Studio, zkontroluje ho, udržuje ho aktuální a spustí ho. Aktualizace stahují jen to, "
        "co se od vaší verze změnilo."),
    "h_starter": ("The starter", "Der Starter", "启动器", "スターター", "El iniciador", "Spouštěč"),
    "starter": (
        "Download **JK-Studio-Starter.exe** from the releases page (under **Assets**; the address "
        "`github.com/DennisHerrm/JK-Studio/releases/latest/download/JK-Studio-Starter.exe` always gives the newest one). Put it into a folder "
        "of your own, for example Documents, and start it. On the first start it downloads the complete program into a folder **JK-Studio** "
        "next to it, checks the download and starts JK Studio. Each later start first looks for a newer version, loads it if there is one "
        "and then starts the program.",
        "Lade **JK-Studio-Starter.exe** von der Release-Seite (unter **Assets**; die Adresse "
        "`github.com/DennisHerrm/JK-Studio/releases/latest/download/JK-Studio-Starter.exe` liefert immer den neuesten). Leg ihn in einen "
        "eigenen Ordner, zum Beispiel Dokumente, und starte ihn. Beim ersten Start lädt er das ganze Programm in einen Ordner **JK-Studio** "
        "neben sich, prüft den Download und startet JK Studio. Jeder weitere Start sieht zuerst nach einer neueren Version, lädt sie, wenn es "
        "eine gibt, und startet dann das Programm.",
        "从发布页面下载 **JK-Studio-Starter.exe**(在 **Assets** 下;地址 "
        "`github.com/DennisHerrm/JK-Studio/releases/latest/download/JK-Studio-Starter.exe` 始终提供最新版)。把它放到自己的文件夹中,"
        "例如“文档”,然后启动它。首次启动时,它会把完整程序下载到旁边的 **JK-Studio** 文件夹,校验下载并启动 JK Studio。之后每次启动都会先"
        "查找新版本,有则下载,然后启动程序。",
        "リリースページから **JK-Studio-Starter.exe** をダウンロードします(**Assets** の下。アドレス "
        "`github.com/DennisHerrm/JK-Studio/releases/latest/download/JK-Studio-Starter.exe` は常に最新版を指します)。ドキュメントなど"
        "自分のフォルダーに置いて起動します。初回はプログラム全体を隣の **JK-Studio** フォルダーにダウンロードし、確認してから JK Studio を"
        "起動します。2 回目以降は、まず新しいバージョンを確認し、あれば読み込んでからプログラムを起動します。",
        "Descarga **JK-Studio-Starter.exe** de la página de versiones (en **Assets**; la dirección "
        "`github.com/DennisHerrm/JK-Studio/releases/latest/download/JK-Studio-Starter.exe` siempre da la más reciente). Ponlo en una "
        "carpeta propia, por ejemplo Documentos, e inícialo. En el primer inicio descarga el programa completo en una carpeta **JK-Studio** "
        "a su lado, comprueba la descarga e inicia JK Studio. Cada inicio posterior busca primero una versión más nueva, la descarga si la "
        "hay y luego inicia el programa.",
        "Stáhněte **JK-Studio-Starter.exe** ze stránky vydání (pod **Assets**; adresa "
        "`github.com/DennisHerrm/JK-Studio/releases/latest/download/JK-Studio-Starter.exe` vždy vede na nejnovější). Dejte ho do vlastní "
        "složky, například Dokumenty, a spusťte ho. Při prvním spuštění stáhne celý program do složky **JK-Studio** vedle sebe, zkontroluje "
        "stažení a spustí JK Studio. Každé další spuštění nejdřív hledá novější verzi, stáhne ji, pokud nějaká je, a pak program spustí."),
    "bild_laden": (
        "The starter on the first start: it downloads the complete program (the bar and the megabytes show how far it is).",
        "Der Starter beim ersten Start: er lädt das ganze Programm (Balken und Megabyte zeigen, wie weit er ist).",
        "首次启动时的启动器:正在下载完整程序(进度条和兆字节数显示进度)。",
        "初回起動時のスターター: プログラム全体をダウンロードしています(バーとメガバイト数で進み具合が分かります)。",
        "El iniciador en el primer inicio: descarga el programa completo (la barra y los megabytes muestran cuánto lleva).",
        "Spouštěč při prvním spuštění: stahuje celý program (pruh a megabajty ukazují, jak je daleko)."),
    "ordner": (
        "**Folder:** JK Studio lives next to the starter in `JK-Studio`, as if you had unpacked the zip there. If the starter lies inside a "
        "JK Studio folder (next to `jkstudio.exe`), it updates that folder. You can always start `JK-Studio\\jkstudio.exe` directly.",
        "**Ordner:** JK Studio liegt neben dem Starter in `JK-Studio`, als hättest du das Zip dort ausgepackt. Liegt der Starter in einem "
        "JK-Studio-Ordner (neben `jkstudio.exe`), aktualisiert er diesen Ordner. `JK-Studio\\jkstudio.exe` kannst du jederzeit direkt starten.",
        "**文件夹:**JK Studio 位于启动器旁边的 `JK-Studio` 中,就像你在那里解压了 zip。如果启动器放在 JK Studio 文件夹中(`jkstudio.exe` "
        "旁边),它就更新该文件夹。你随时可以直接启动 `JK-Studio\\jkstudio.exe`。",
        "**フォルダー:** JK Studio はスターターの隣の `JK-Studio` に入ります。そこで zip を展開したのと同じです。スターターが JK Studio の"
        "フォルダー内(`jkstudio.exe` の隣)にあれば、そのフォルダーを更新します。`JK-Studio\\jkstudio.exe` はいつでも直接起動できます。",
        "**Carpeta:** JK Studio queda junto al iniciador en `JK-Studio`, como si hubieras descomprimido allí el zip. Si el iniciador está "
        "dentro de una carpeta de JK Studio (junto a `jkstudio.exe`), actualiza esa carpeta. Siempre puedes iniciar `JK-Studio\\jkstudio.exe` "
        "directamente.",
        "**Složka:** JK Studio leží vedle spouštěče ve složce `JK-Studio`, jako byste tam rozbalili zip. Leží-li spouštěč ve složce JK "
        "Studia (vedle `jkstudio.exe`), aktualizuje tuto složku. `JK-Studio\\jkstudio.exe` můžete kdykoli spustit přímo."),
    "rechte": (
        "**Rights:** the starter never asks for administrator rights. If it cannot write next to itself (for example in Program Files), it "
        "says so - move it into a folder of your own.",
        "**Rechte:** der Starter fragt nie nach Administratorrechten. Kann er neben sich nicht schreiben (etwa in Programme), sagt er das - "
        "leg ihn dann in einen eigenen Ordner.",
        "**权限:**启动器从不请求管理员权限。如果它无法在自身旁边写入(例如在 Program Files 中),会给出提示——请把它移到自己的文件夹。",
        "**権限:** スターターは管理者権限を求めません。隣に書き込めない場合(Program Files など)はそう表示します。自分のフォルダーに移してください。",
        "**Permisos:** el iniciador nunca pide derechos de administrador. Si no puede escribir junto a sí mismo (por ejemplo en Archivos de "
        "programa), lo dice; muévelo entonces a una carpeta propia.",
        "**Oprávnění:** spouštěč nikdy nežádá o práva správce. Nemůže-li zapisovat vedle sebe (například v Program Files), řekne to - pak ho "
        "přesuňte do vlastní složky."),
    "abbrechen": (
        "**Cancel:** **Cancel** (on the first start) or **Start without updating** (when a version is installed) stops the download; "
        "nothing is changed.",
        "**Abbrechen:** **Abbrechen** (beim ersten Start) oder **Ohne Update starten** (wenn eine Version installiert ist) hält das Laden an; "
        "nichts wird geändert.",
        "**取消:****取消**(首次启动时)或 **不更新直接启动**(已安装某个版本时)会停止下载;不做任何更改。",
        "**キャンセル:** **キャンセル**(初回起動時)または **アップデートせずに起動**(インストール済みのとき)でダウンロードを止めます。"
        "何も変更しません。",
        "**Cancelar:** **Cancelar** (en el primer inicio) o **Iniciar sin actualizar** (si hay una versión instalada) detiene la descarga; "
        "no se cambia nada.",
        "**Zrušení:** **Zrušit** (při prvním spuštění) nebo **Spustit bez aktualizace** (když je nějaká verze nainstalovaná) zastaví "
        "stahování; nic se nezmění."),
    "offline": (
        "**Offline:** without a connection the starter starts the installed version. If nothing is installed yet, it says so and offers "
        "**Try again**.",
        "**Ohne Verbindung:** startet der Starter die installierte Version. Ist noch nichts installiert, sagt er das und bietet "
        "**Noch einmal** an.",
        "**离线:**没有网络时,启动器会启动已安装的版本。如果尚未安装,它会给出提示并提供 **重试**。",
        "**オフライン:** 接続がないときは、インストール済みのバージョンを起動します。まだ何もインストールされていなければ、そう表示して "
        "**再試行** を提示します。",
        "**Sin conexión:** el iniciador inicia la versión instalada. Si todavía no hay nada instalado, lo dice y ofrece **Reintentar**.",
        "**Bez připojení:** spouštěč spustí nainstalovanou verzi. Není-li ještě nic nainstalováno, řekne to a nabídne **Zkusit znovu**."),
    "geprueft": (
        "**Checked:** every download is checked against `SHA256SUMS.txt` of the release (and its signature) before anything changes; an "
        "update goes in as a whole or not at all.",
        "**Geprüft:** jeder Download wird gegen `SHA256SUMS.txt` des Releases (und deren Signatur) geprüft, bevor sich etwas ändert; ein "
        "Update kommt ganz oder gar nicht.",
        "**已校验:**每次下载在做任何更改之前都会根据该发布的 `SHA256SUMS.txt`(及其签名)进行校验;更新要么全部安装,要么完全不安装。",
        "**確認済み:** どのダウンロードも、何かを変える前にリリースの `SHA256SUMS.txt`(とその署名)で確認されます。更新は全部入るか、"
        "まったく入らないかのどちらかです。",
        "**Comprobado:** cada descarga se comprueba con `SHA256SUMS.txt` de la versión (y su firma) antes de cambiar nada; una "
        "actualización se aplica entera o no se aplica.",
        "**Zkontrolováno:** každé stažení se před jakoukoli změnou ověří proti `SHA256SUMS.txt` vydání (a jeho podpisu); aktualizace se "
        "provede celá, nebo vůbec."),
    "h_update": ("Updates: only what has changed", "Updates: nur, was sich geändert hat", "更新:只下载有变化的部分",
                 "更新: 変わった部分だけ", "Actualizaciones: solo lo que ha cambiado", "Aktualizace: jen to, co se změnilo"),
    "update": (
        "An update no longer downloads the whole program. For each of the last versions a release carries a delta with only the content "
        "that version does not have yet - large parts of the guide and the data stay the same from one version to the next. The starter "
        "and the update bar of JK Studio (see [[einstellungen#updates]]) take the delta that fits the installed version; if there is none, "
        "or if a file in the folder was changed by hand, they load the complete package instead. Every file is checked against the signed "
        "list of the new version (`…-manifest.txt`).",
        "Ein Update lädt nicht mehr das ganze Programm. Für jede der letzten Versionen trägt ein Release ein Delta mit nur den Inhalten, "
        "die diese Version noch nicht hat - große Teile des Handbuchs und der Daten bleiben von einer Version zur nächsten gleich. Der "
        "Starter und die Update-Leiste von JK Studio (siehe [[einstellungen#updates]]) nehmen das Delta, das zur installierten Version "
        "passt; gibt es keins oder wurde eine Datei im Ordner von Hand geändert, laden sie stattdessen das ganze Paket. Jede Datei wird "
        "gegen die unterschriebene Liste der neuen Version (`…-manifest.txt`) geprüft.",
        "更新不再下载整个程序。对最近的每个版本,发布中都附带一个增量包(delta),只包含该版本还没有的内容——手册和数据的大部分在版本之间"
        "保持不变。启动器和 JK Studio 的更新栏(参见 [[einstellungen#updates]])会选用与已安装版本匹配的增量包;如果没有,或文件夹中的"
        "某个文件被手动修改过,就改为下载完整包。每个文件都会根据新版本的已签名列表(`…-manifest.txt`)进行校验。",
        "更新でプログラム全体をダウンロードすることはなくなりました。リリースには直近の各バージョン向けに、そのバージョンにまだない内容だけを"
        "含む差分(デルタ)が付きます。ガイドやデータの大部分はバージョン間で変わりません。スターターと JK Studio のアップデートバー"
        "([[einstellungen#updates]] を参照)は、インストール済みのバージョンに合う差分を使います。合うものがないか、フォルダー内のファイルが"
        "手で変更されていれば、代わりに完全なパッケージを読み込みます。どのファイルも新しいバージョンの署名付き一覧(`…-manifest.txt`)で確認されます。",
        "Una actualización ya no descarga el programa completo. Para cada una de las últimas versiones, una publicación lleva un delta con "
        "solo el contenido que esa versión aún no tiene: gran parte de la guía y de los datos no cambia de una versión a la siguiente. El "
        "iniciador y la barra de actualización de JK Studio (ver [[einstellungen#updates]]) usan el delta que corresponde a la versión "
        "instalada; si no hay ninguno, o si un archivo de la carpeta se cambió a mano, descargan en su lugar el paquete completo. Cada "
        "archivo se comprueba con la lista firmada de la nueva versión (`…-manifest.txt`).",
        "Aktualizace už nestahuje celý program. Pro každou z posledních verzí nese vydání delta jen s obsahem, který tato verze ještě nemá - "
        "velká část příručky a dat zůstává mezi verzemi stejná. Spouštěč i pruh aktualizace JK Studia (viz [[einstellungen#updates]]) "
        "použijí delta, které odpovídá nainstalované verzi; pokud žádné není nebo byl soubor ve složce ručně změněn, stáhnou místo toho celý "
        "balík. Každý soubor se ověří proti podepsanému seznamu nové verze (`…-manifest.txt`)."),
    "bild_delta": (
        "An update with the starter: only what has changed since the installed version.",
        "Ein Update mit dem Starter: nur, was sich seit der installierten Version geändert hat.",
        "使用启动器更新:只下载自已安装版本以来有变化的部分。",
        "スターターによる更新: インストール済みのバージョンから変わった部分だけ。",
        "Una actualización con el iniciador: solo lo que ha cambiado desde la versión instalada.",
        "Aktualizace spouštěčem: jen to, co se od nainstalované verze změnilo."),
    "beispiel": (
        "From rc611 to rc612, for example, 4,183 of 5,980 files stayed the same; the delta has 131 MB instead of the 449 MB of the complete "
        "package. Versions older than the last few, and JK Studio up to rc612, take the complete package as before.",
        "Von rc611 auf rc612 zum Beispiel blieben 4183 von 5980 Dateien gleich; das Delta hat 131 MB statt der 449 MB des ganzen Pakets. "
        "Ältere Versionen als die letzten und JK Studio bis rc612 nehmen wie bisher das ganze Paket.",
        "例如从 rc611 到 rc612,5980 个文件中有 4183 个保持不变;增量包为 131 MB,而完整包为 449 MB。比最近几个更旧的版本,以及 rc612 及以前的 "
        "JK Studio,仍像以前一样下载完整包。",
        "たとえば rc611 から rc612 では 5980 個中 4183 個のファイルが同じままで、差分は 131 MB で、完全なパッケージの 449 MB より小さくなります。直近より"
        "古いバージョンと rc612 までの JK Studio は、これまでどおり完全なパッケージを使います。",
        "Por ejemplo, de rc611 a rc612 se mantuvieron iguales 4183 de 5980 archivos; el delta ocupa 131 MB en lugar de los 449 MB "
        "del paquete completo. Las versiones más antiguas que las últimas, y JK Studio hasta rc612, usan como antes el paquete completo.",
        "Například z rc611 na rc612 zůstalo 4183 z 5980 souborů stejných; delta má 131 MB místo 449 MB celého balíku. Verze starší "
        "než ty poslední a JK Studio do rc612 berou jako dosud celý balík."),
    "h_zip": ("Without the starter", "Ohne den Starter", "不使用启动器", "スターターを使わない場合", "Sin el iniciador", "Bez spouštěče"),
    "zip": (
        "The complete program is also on the releases page as `JK-Studio-…-windows-x64.zip`: unpack it, open the `JK-Studio` folder and start "
        "`jkstudio.exe` (keep it next to its `data` folder). It updates itself in the same way.",
        "Das ganze Programm steht auch als `JK-Studio-…-windows-x64.zip` auf der Release-Seite: auspacken, den Ordner `JK-Studio` öffnen und "
        "`jkstudio.exe` starten (neben ihrem Ordner `data` lassen). Es aktualisiert sich genauso.",
        "完整程序也以 `JK-Studio-…-windows-x64.zip` 的形式放在发布页面上:解压,打开 `JK-Studio` 文件夹并启动 `jkstudio.exe`(让它留在 "
        "`data` 文件夹旁边)。它会以同样的方式自行更新。",
        "プログラム全体はリリースページに `JK-Studio-…-windows-x64.zip` としてもあります。展開して `JK-Studio` フォルダーを開き、"
        "`jkstudio.exe` を起動します(`data` フォルダーの隣に置いたままにします)。同じように自動で更新されます。",
        "El programa completo también está en la página de versiones como `JK-Studio-…-windows-x64.zip`: descomprímelo, abre la carpeta "
        "`JK-Studio` e inicia `jkstudio.exe` (déjalo junto a su carpeta `data`). Se actualiza de la misma manera.",
        "Celý program je na stránce vydání také jako `JK-Studio-…-windows-x64.zip`: rozbalte ho, otevřete složku `JK-Studio` a spusťte "
        "`jkstudio.exe` (nechte ho vedle jeho složky `data`). Aktualizuje se stejně."),
    "smartscreen": (
        "Windows SmartScreen may warn the first time, because the starter is not code-signed: **More info → Run anyway**. `SHA256SUMS.txt` "
        "lists every file of the release; a download can be checked with `certutil -hashfile JK-Studio-Starter.exe SHA256`.",
        "Windows SmartScreen warnt beim ersten Mal womöglich, weil der Starter nicht codesigniert ist: **Weitere Informationen → Trotzdem "
        "ausführen**. `SHA256SUMS.txt` nennt jede Datei des Releases; einen Download prüft `certutil -hashfile JK-Studio-Starter.exe SHA256`.",
        "由于启动器没有代码签名,Windows SmartScreen 首次可能会发出警告:**更多信息 → 仍要运行**。`SHA256SUMS.txt` 列出了该发布的每个文件;"
        "可以用 `certutil -hashfile JK-Studio-Starter.exe SHA256` 校验下载。",
        "スターターはコード署名されていないため、初回に Windows SmartScreen が警告することがあります: **詳細情報 → 実行**。"
        "`SHA256SUMS.txt` にはリリースのすべてのファイルが載っています。ダウンロードは `certutil -hashfile JK-Studio-Starter.exe SHA256` で"
        "確認できます。",
        "Windows SmartScreen puede advertir la primera vez, porque el iniciador no tiene firma de código: **Más información → Ejecutar de "
        "todas formas**. `SHA256SUMS.txt` lista cada archivo de la versión; una descarga se comprueba con "
        "`certutil -hashfile JK-Studio-Starter.exe SHA256`.",
        "Windows SmartScreen může poprvé varovat, protože spouštěč není podepsaný: **Další informace → Přesto spustit**. `SHA256SUMS.txt` "
        "uvádí každý soubor vydání; stažení ověříte příkazem `certutil -hashfile JK-Studio-Starter.exe SHA256`."),
    # --- erste-schritte, Abschnitt "Installieren" ---
    "schritt1": (
        "Download **JK-Studio-Starter.exe** from the releases page (under **Assets**), put it into a folder of your own and start it: it "
        "downloads JK Studio into `JK-Studio` next to it and starts it. Details, and the zip without the starter, on [[installation]].",
        "Lade **JK-Studio-Starter.exe** von der Release-Seite (unter **Assets**), leg ihn in einen eigenen Ordner und starte ihn: er lädt "
        "JK Studio nach `JK-Studio` neben sich und startet es. Einzelheiten und das Zip ohne Starter auf [[installation]].",
        "从发布页面(**Assets** 下)下载 **JK-Studio-Starter.exe**,放到自己的文件夹中并启动:它会把 JK Studio 下载到旁边的 `JK-Studio` "
        "并启动。详细说明以及不使用启动器的 zip 见 [[installation]]。",
        "リリースページ(**Assets** の下)から **JK-Studio-Starter.exe** をダウンロードし、自分のフォルダーに置いて起動します。JK Studio を"
        "隣の `JK-Studio` にダウンロードして起動します。詳細とスターターを使わない zip については [[installation]] を参照。",
        "Descarga **JK-Studio-Starter.exe** de la página de versiones (en **Assets**), ponlo en una carpeta propia e inícialo: descarga "
        "JK Studio en `JK-Studio` a su lado y lo inicia. Detalles, y el zip sin el iniciador, en [[installation]].",
        "Stáhněte **JK-Studio-Starter.exe** ze stránky vydání (pod **Assets**), dejte ho do vlastní složky a spusťte ho: stáhne JK Studio "
        "do `JK-Studio` vedle sebe a spustí ho. Podrobnosti a zip bez spouštěče na [[installation]]."),
    "schritt2": (
        "Later, start JK Studio with the starter (it updates first) or with `JK-Studio\\jkstudio.exe`. At startup the program also looks for "
        "a newer version on GitHub and offers it; nothing is downloaded without asking you (the switch is in **{{MenuHelp}}**).",
        "Später startest du JK Studio mit dem Starter (er aktualisiert vorher) oder mit `JK-Studio\\jkstudio.exe`. Beim Start sieht auch das "
        "Programm auf GitHub nach einer neueren Fassung und bietet sie an; ohne Nachfrage wird nichts geladen (der Schalter steht unter "
        "**{{MenuHelp}}**).",
        "以后可以用启动器(它会先更新)或 `JK-Studio\\jkstudio.exe` 启动 JK Studio。程序启动时也会在 GitHub 上检查新版本并提示你;未经同意"
        "不会下载任何内容(开关位于 **{{MenuHelp}}**)。",
        "以後はスターター(先に更新します)か `JK-Studio\\jkstudio.exe` で JK Studio を起動します。起動時にはプログラム自身も GitHub で"
        "新しいバージョンを確認して案内します。確認なしにダウンロードすることはありません(切り替えは **{{MenuHelp}}** にあります)。",
        "Después, inicia JK Studio con el iniciador (actualiza antes) o con `JK-Studio\\jkstudio.exe`. Al iniciar, el programa también busca "
        "en GitHub una versión más nueva y la ofrece; no se descarga nada sin preguntarte (el interruptor está en **{{MenuHelp}}**).",
        "Později spouštějte JK Studio spouštěčem (nejdřív aktualizuje) nebo přes `JK-Studio\\jkstudio.exe`. Při spuštění i program hledá na "
        "GitHubu novější verzi a nabídne ji; bez dotazu se nic nestahuje (přepínač je v **{{MenuHelp}}**)."),
    "hinweis": (
        "**Already have JK Studio or BehavEd-Remake?** It updates itself as before; the guide is in the program folder. Windows SmartScreen "
        "may warn the first time because the starter is not code-signed: **More info → Run anyway**.",
        "**JK Studio oder BehavEd-Remake schon da?** Es aktualisiert sich wie bisher selbst; das Handbuch liegt im Programmordner. Windows "
        "SmartScreen warnt beim ersten Mal womöglich, weil der Starter nicht codesigniert ist: **Weitere Informationen → Trotzdem ausführen**.",
        "**已经有 JK Studio 或 BehavEd-Remake?**它会像以前一样自行更新;手册位于程序文件夹中。由于启动器没有代码签名,Windows SmartScreen "
        "首次可能会发出警告:**更多信息 → 仍要运行**。",
        "**JK Studio や BehavEd-Remake をすでにお持ちですか?** これまでどおり自動で更新されます。ガイドはプログラムフォルダーにあります。"
        "スターターはコード署名されていないため、初回に Windows SmartScreen が警告することがあります: **詳細情報 → 実行**。",
        "**¿Ya tienes JK Studio o BehavEd-Remake?** Se actualiza solo como antes; la guía está en la carpeta del programa. Windows "
        "SmartScreen puede advertir la primera vez porque el iniciador no tiene firma de código: **Más información → Ejecutar de todas formas**.",
        "**Už máte JK Studio nebo BehavEd-Remake?** Aktualizuje se samo jako dosud; příručka je ve složce programu. Windows SmartScreen může "
        "poprvé varovat, protože spouštěč není podepsaný: **Další informace → Přesto spustit**."),
}

ES = {t[0]: t[4] for t in X.values()}
CS = {t[0]: t[5] for t in X.values()}


def text(schluessel):
    """T(en, de, zh, ja) - es und cs kommen ueber ES/CS (inhalt_es.py, inhalt_cs.py); auch fuer erste-schritte (inhalt_start.py)."""
    from inhalt_basis import T
    t = X[schluessel]
    return T(t[0], t[1], t[2], t[3])


_t = text


def seite():
    """Die Seite installation: Starter, Updates mit Deltas, das Zip ohne Starter."""
    return {
        "id": "installation",
        "gruppe": "start",
        "titel": _t("titel"),
        "kurz": _t("kurz"),
        "bloecke": [
            ("h2", "starter", _t("h_starter")),
            ("p", _t("starter")),
            ("img", "h_starter_laden", _t("bild_laden"), "breit"),
            ("list", [_t("ordner"), _t("rechte"), _t("abbrechen"), _t("offline"), _t("geprueft")]),
            ("h2", "update", _t("h_update")),
            ("p", _t("update")),
            ("img", "h_starter_delta", _t("bild_delta"), "breit"),
            ("note", _t("beispiel")),
            ("h2", "zip", _t("h_zip")),
            ("p", _t("zip")),
            ("note", _t("smartscreen")),
        ],
    }
