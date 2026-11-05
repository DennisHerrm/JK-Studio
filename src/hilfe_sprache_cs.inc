// hilfe_sprache_cs.inc - der Helfer auf Tschechisch (Sprache cs, tools/sprachen.py)
//
// Von Hand gepflegt, eingebunden nur von src/hilfe.cpp (kSprachDaten). Alles
// klein und gefaltet geschrieben (ohne Hacek und Akzente: "cestina" fuer
// "čeština", "svetlo" fuer "světlo") - so zerlegt src/hilfe.cpp jeden Text.
//
// STOPP: Fuellwoerter von Fragen ("jak mam ...", "kde je ..."), Praepositionen
// und Pronomen, zusaetzlich zu den deutschen und englischen (kStoppAllgemein).
// ENDUNGEN: vor den allgemeinen gestutzt, laengste zuerst, der Stamm behaelt
// mindestens vier Buchstaben (stutze): kamera, kameru, kamery, kamerou ->
// kamer; otevrit, otevreni -> otevr; kompilovat, kompilace -> kompil;
// textura, texturu -> textur wie texture. Kurze Woerter (mapa, mapu) bleiben -
// die Begriffe fassen ihre Formen zusammen.
// BEGRIFFE: tschechische Woerter (mit den haeufigen Beugungsformen) zu den
// Begriffen aus hilfe_daten.inc (gleicher Name).
// ZIELE: haeufige Aufgaben, wie Leute auf Tschechisch fragen (Kennung und
// Bereich wie in kZiele).

const char* const kStoppCs[] = {
    "jak", "jaky", "jaka", "jake", "co", "kde", "kdy", "proc", "ktery", "ktera", "ktere", "chci", "chtel",
    "chtela", "bych", "by", "se", "si", "sem", "tam", "to", "ten", "ta", "tu", "tento", "tato", "toto",
    "tenhle", "tahle", "tohle", "tady", "je", "jsou", "byl", "byla", "bylo", "mam", "mas", "muzu", "mohu",
    "muze", "musim", "potrebuji", "potrebuju", "lze", "a", "i", "o", "u", "v", "ve", "na", "do", "z", "ze", "s",
    "k", "ke", "po", "pro", "pri", "od", "za", "pred", "pod", "nad", "mezi", "bez", "ja", "ty", "on", "ona",
    "my", "vy", "oni", "me", "mi", "mne", "moje", "muj", "moji", "svuj", "svou", "sve", "jeho", "jeji",
    "jejich", "ho", "mu", "ji", "jim", "nebo", "ale", "uz", "jeste", "take", "tak", "jen", "prosim", "nejak",
    "neco", "nic", "pak", "potom", "aby", "kdyz", "jestli", "zda", "ano", "ne", nullptr};

const char* const kEndungenCs[] = {
    "ovanou", "ovani", "ovana", "ovane", "ovany", "ovych", "ovat", "ovou", "oval", "ovem", "ovym", "ace", "aci",
    "ani", "eni", "ich", "ych", "ami", "emi", "ach", "ech", "ove", "ova", "ovy", "uje", "ete", "ite", "jte",
    "ime", "it", "at", "et", "te", "ou", "em", "um", "ym", "mi", "a", "e", "i", "o", "u", "y", nullptr};

const BegriffRoh kBegriffeCs[] = {
    {"kamera", "kamera|kameru|kamery|kamerou|zaber|zaberu|zabery"},
    {"fahrt", "jizda kamery|pohyb kamery|draha kamery|let kamery|prulet kamery|jizda|prulet"},
    {"szene", "animacni sekvence|animacni sekvenci|cutscena|cutscenu|cutsceny|scena|scenu|sceny|filmova scena|filmecek"},
    {"vorschau", "nahled|nahledu|prehrat|prehravani|prehraj|podivat|sledovat|prohlednout"},
    {"skript", "skript|skriptu|skripty|skriptovani|skriptovat"},
    {"befehl", "prikaz|prikazu|prikazy|radek|radku|instrukce"},
    {"uebersetzen", "kompilovat|zkompilovat|kompilace|kompilaci|prelozit|preklad"},
    {"npc", "postava|postavu|postavy|postave|postavou|nepritel|nepritele|nepratele|herec|charakter"},
    {"laufen", "chodit|chuze|chuzi|jit|jde|bezet|beh|behat|pohyb|pohybovat|presunout|dojit"},
    {"wegpunkt", "navigacni bod|navigacni body|navigacnich bodu|waypoint|waypointy|navgoal"},
    {"patrouille", "hlidka|hlidku|hlidkovat|hlidkuje|pochuzka|pochuzku|straz|strazny|strazce|obchuzka"},
    {"karte", "mapa|mapu|mapy|mape|uroven|level"},
    {"karteneditor", "editor map|editoru map|editor mapy|radiant|netradiant"},
    {"pinsel", "brush|brushe|brushu|blok|zed|zdi|stena|stenu|mistnost|mistnosti|kvadr"},
    {"textur", "textura|texturu|textury|material|shader"},
    {"licht", "svetlo|svetla|svetel|osvetleni|lampa|lampu|tma|tmave|tmavy|prilis tmave|svetlejsi|jas"},
    {"spawn", "start hrace|startovni bod|vychozi bod|kde hrac zacina|spawn|info_player_start"},
    {"bauen", "sestavit|sestaveni|kompilovat mapu|zkompilovat mapu|postavit mapu|bsp"},
    {"optimierer", "optimalizator|optimalizovat|optimalizace|limity|limit|prilis velka|moc velka"},
    {"pk3", "pk3|zabalit|zabaleni|balicek|archiv|zip|vydat|zverejnit|do hry"},
    {"spielen", "hrat v editoru|vyzkouset|otestovat|test|projit|prochazet|prohlidka mapy"},
    {"speichern", "ulozit|ulozeni|ukladat|ulozte"},
    {"oeffnen", "otevrit|otevreni|nacist|nahrat|otevru"},
    {"neu", "novy|nova|nove|novou|vytvorit|udelat|zalozit"},
    {"rueckgaengig", "zpet|vratit|vraceni|znovu|krok zpet"},
    {"suchen", "hledat|najit|vyhledat|nahradit|nahrazeni"},
    {"einstellungen", "nastaveni|predvolby|predvoleb|moznosti|konfigurace"},
    {"sprache", "jazyk|jazyka|cestina|cestiny|cestinu|cesky|anglictina|anglicky|nemcina|spanelstina"},
    {"thema", "motiv|vzhled|tmavy rezim|tmavy motiv|barvy|barevne schema"},
    {"update", "aktualizace|aktualizovat|aktualizaci|nova verze|novejsi verze|upgrade"},
    {"modell", "model|modelu|modely|glm|md3|sit"},
    {"skin", "skin|skinu|skiny|kuze|obleceni|oblek"},
    {"animation", "animace|animaci|animovat|poza|pozu"},
    {"mission", "mise|misi|misi"},
    {"tutorial", "tutorial|tutorialu|navod|naucit|krok za krokem|uvod|zacatecnik"},
    {"handbuch", "prirucka|prirucku|prirucce|manual|dokumentace|napoveda"},
    {"tasten", "klavesova zkratka|zkratka|zkratky|klavesy|klavesnice|klavesa"},
    {"entity", "entita|entitu|entity|objekt"},
    {"tuer", "dvere|dveri|func_door|brana|vrata"},
    {"trigger", "trigger|triggeru|spoust|spoustec"},
    {"ton", "zvuk|zvuky|zvuku|hlasitost|hudba|hudbu|audio"},
    {"dialog", "dialog|dialogu|mluvit|rict|rekne|hlas|replika|repliku|titulky|mluvi"},
    {"saber", "svetelny mec|svetelneho mece|mec|mece|saber"},
    {"kampf", "boj|bojovat|utok|zautocit|souboj"},
    {"signal", "signal|cekat|pockat|waitsignal"},
    {"variable", "promenna|promennou|promenne"},
    {"zeitleiste", "casova osa|casove ose|casovou osu|stopy|nacasovani|nacasovat|casovani|strihy"},
    {"caulk", "caulk|autocaulk|neviditelne steny"},
    {"hohl", "vydlabat|duty|dutou|csg|vyhloubit"},
    {"ordner", "cesta|cesty|slozka|slozku|slozce|slozka hry|gamedata|nainstalovana|instalace"},
    {"backup", "zaloha|zalohu|zalohy|automaticke ukladani"},
    {"fehler", "chyba|chyby|chybu|varovani|problem|hlaseni|nefunguje|rozbite"},
    {"leck", "unik|propousti|leak|neutesnena|dira"},
    {"ki", "ai|umela inteligence|claude|asistent|chatbot|mcp"},
    {"graph", "graf|grafu|diagram|uzel|uzly|tok"},
    {"lesezeichen", "zalozka|zalozku|zalozky"},
    {"skalierung", "meritko|zvetsit|pismo|velikost pisma|vetsi|male pismo"},
    {"mehrspieler", "vice hracu|multiplayer|mp"},
    {"effekt", "efekt|efekty|castice|ohen|kour"},
    {"spiel", "hra|hru|hry|hre|jedi academy|jka|jedi outcast"},
    {"verlauf", "historie|zmeny|seznam zmen|co je noveho|novinky"},
    {"modus", "rezim|rezimy|lista rezimu|oblast"},
    {nullptr, nullptr},
};

const ZielRoh kZieleCs[] = {
    {"tour:tut-karte", "karte", "prvni mapa postavit vlastni mapu vytvorit mapu udelat uroven"},
    {"tour:tut-karte@2", "karte", "postavit mistnost nakreslit brush postavit zed"},
    {"tour:tut-karte@3", "karte", "zvysit brush zvetsit vysku"},
    {"tour:tut-karte@4", "karte", "pouzit texturu dat texturu nanest texturu"},
    {"tour:tut-karte@5", "karte", "vydlabat mistnost udelat dutou"},
    {"tour:tut-karte@7", "karte", "pridat svetlo umistit svetlo lampu osvetleni"},
    {"tour:tut-karte@8", "karte", "start hrace umistit spawn kde hrac zacina"},
    {"tour:tut-karte@10", "karte", "caulk zvenku autocaulk neviditelne steny"},
    {"tour:tut-karte@11", "karte", "pouzit optimalizator zkontrolovat mapu limity"},
    {"tour:tut-karte@12", "karte", "sestavit mapu zkompilovat mapu vytvorit bsp q3map2"},
    {"tour:tut-karte@13", "karte", "hrat v editoru projit mapu chodit po mape"},
    {"tour:tut-karte@14", "karte", "zabalit mapu jako pk3 dostat do hry vydat"},
    {"ort:knopf:ViewMapEditor@modebar", "karte", "otevrit editor map radiant"},
    {"tour:k-neues-skript", "skript", "napsat prvni skript novy skript zacit skript"},
    {"tour:k-befehl-einfuegen", "skript", "vlozit prikaz pridat radek"},
    {"tour:k-werte-bearbeiten", "skript", "zmenit hodnoty upravit parametry"},
    {"tour:k-zeilen-ordnen", "skript", "presunout radky zmenit poradi"},
    {"tour:k-uebersetzen", "skript", "zkompilovat skript vytvorit ibi"},
    {"seite:k-suchen-ersetzen.html", "skript", "najit a nahradit hledat nahradit"},
    {"seite:k-rueckgaengig.html", "skript", "vratit chybu zpet krok zpet"},
    {"seite:k-pruefen.html", "skript", "zkontrolovat skript najit chyby"},
    {"tour:k-kamera-ansicht", "vorschau", "vytvorit jizdu kamery umistit kameru zaber kamery"},
    {"seite:tut-projekt.html", "vorschau", "vytvorit animacni sekvenci cutscenu prvni scena vlastni mise"},
    {"seite:tut-p4.html", "vorschau", "kamera pro animacni sekvenci jizda kamery"},
    {"seite:tut-p3.html", "vorschau", "nechat postavy chodit nechat je jit"},
    {"seite:tut-p8.html", "vorschau", "ulozit misi a hrat ve hre"},
    {"seite:k-gangart.html", "vorschau", "nechat npc chodit nechat postavu jit bezet"},
    {"seite:k-wegpunkte.html", "vorschau", "navigacni body umistit waypointy cesta pro npc"},
    {"seite:k-animation-abspielen.html", "vorschau", "prehrat animaci animovat postavu"},
    {"seite:k-dialog.html", "vorschau", "nechat postavu mluvit dialog hlas titulky"},
    {"seite:zeitleiste.html", "vorschau", "casova osa nacasovani stopy"},
    {"ort:knopf:ViewMap@modebar", "vorschau", "nahled mapy podivat se na mapu 3d"},
    {"seite:tut-p1.html", "mission", "vlastni mise vytvorit misi nova mise"},
    {"ort:menue:MenuFile>LoadMission", "mission", "nacist misi otevrit misi"},
    {"seite:k-pk3.html", "mission", "zabalit misi jako pk3"},
    {"ort:menue:MenuFile>OpenGlm", "modell", "otevrit model nacist glm"},
    {"ort:knopf:ViewModel@modebar", "modell", "prohlednout model zobrazeni modelu"},
    {"tour:k-modell", "modell", "prohlednout model postavy podivat se na postavu"},
    {"seite:k-skins.html", "modell", "zmenit skin vymenit skin"},
    {"seite:k-modell.html", "modell", "model v mape"},
    {"ort:menue:MenuEdit>AppPrefs", "einstellungen", "otevrit nastaveni predvolby moznosti"},
    {"ort:menue:MenuView>ViewLanguage", "einstellungen", "zmenit jazyk prepnout jazyk cestina"},
    {"ort:menue:MenuView>ViewTheme", "einstellungen", "tmavy rezim motiv barvy"},
    {"ort:menue:MenuView>ViewDpiScale", "einstellungen", "zvetsit pismo meritko text je moc maly"},
    {"ort:menue:MenuEdit>KeysTitle", "einstellungen", "zmenit klavesove zkratky klavesy"},
    {"ort:menue:MenuEdit>PathsTitle", "einstellungen", "slozka hry nastavit cesty kde je hra"},
    {"ort:menue:MenuHelp>UpdMenu", "updates", "vyhledat aktualizace nova verze"},
    {"ort:menue:MenuHelp>TourMenu", "hilfe", "vsechny tutorialy seznam navodu"},
    {"ort:menue:MenuFile>FileBackupFolder", "programm", "slozka zaloh najit zalohy"},
    {"ort:menue:KeLEdit>KeLPreferences", "karte", "nastaveni editoru map predvolby editoru nastaveni editor map predvolby netradiant"},
    {"tour:k-geometrie", "karte", "model na brushe udelat z modelu brushe importovat geometrii jako brushe importovat obj z blenderu"},   // rc610 TUT-GEO
    {nullptr, nullptr, nullptr},
};
