// hilfe_fragen_cs.inc - die unabhaengige Frageliste auf Tschechisch (EINGEFROREN)
//
// Geschrieben, BEVOR der Helfer tschechische Begriffe kannte (Commit
// 246de054, vor dem Merge der Grundlage), und danach nicht mehr angepasst -
// weder die Fragen noch die erwarteten Antworten; beim Merge nur in den
// Aufbau der Grundlage gebracht (FrageH2: Sprache, Modus, Frage, erwartete
// Kennungen als Vorsilben, {nullptr, ...} am Ende). 26 Fragen in 18 Zielen,
// frei formuliert wie ein Anwender fragt (keine woertlichen Uebersetzungen der
// H2-Liste), mit zwei Tippfehlern und elf Fragen, die erst mit dem aktuellen
// Modus eindeutig werden (1 Vorschau, 2 Modell, 3 Karten-Editor,
// 4 Menue-Editor; -1 = ohne Kontext). "tour:x@" = jeder Schritt der Tour x.
// Ziel Top-3 >= 85 %.
#define CS_LAUFEN "seite:k-wegpunkte|seite:wegpunkte|seite:tut-p3|seite:k-patrouille|seite:k-gangart|seite:figuren|tour:k-wegpunkte@|tour:k-gangart@|tour:tut-p3@|tour:k-patrouille@"
#define CS_ABSPIELEN "seite:zeitleiste|seite:k-zeitleiste-plus|seite:k-neu-timen|seite:tut-erste-szene|seite:tut-p4|seite:k-ansichten|tour:k-zeitleiste-plus@|tour:k-neu-timen@|tour:tut-erste-szene@|tour:tut-p4@"
#define CS_MISSION "seite:tut-p1|seite:tut-mission|seite:missionen|seite:tut-projekt|tour:tut-p1@|tour:tut-mission@|ort:menue:MenuFile>MissionNew|ort:knopf:MissionNew"
#define CS_SKIN "seite:k-skins|seite:modell|tour:k-skins@|tour:k-modell@3|ort:menue:MenuFile>OpenSkin|ort:knopf:ModelSkin"
#define CS_SPIELORDNER "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|seite:einstellungen|seite:erste-schritte.html#spielordner|seite:k-spielordner|tour:k-spielordner@|tour:tut-karte@0"
#define CS_UPDATE "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck|seite:einstellungen.html#updates|seite:k-update|tour:k-update@|ort:menue:KeLHelp>KeLCheckForNetRadiantUpdate"
#define CS_TUER "seite:k-tuer|seite:k-tuer-use|seite:karteneditor.html#entities|seite:entities|seite:tut-karte-tuer|tour:k-tuer@|tour:tut-karte-tuer@"
#define CS_TRIGGER "seite:k-trigger|seite:k-tuer-use|seite:tut-karte-tuer|tour:k-trigger@|tour:tut-karte-tuer@"
#define CS_MENUE "seite:menue_editor|seite:tut-menue|ort:knopf:ViewMenuEditor@modebar|ort:knopf:Me|tour:tut-menue@"
#define CS_BEDIENEN "seite:menue_editor.html#bedienen|ort:knopf:MeBedienen|tour:tut-menue@4"
#define CS_KE_EINST "ort:menue:KeLEdit>KeLPreferences"
#define CS_TEXTUR "tour:tut-karte@4|tour:tut-karte@6|seite:karteneditor.html#texturen"
#define CS_KAMERA "seite:kamera|seite:k-kamera-ansicht|seite:k-pilot|seite:k-blickpunkte|seite:k-ansichten|seite:k-kamera-folgen|seite:tut-p4|tour:k-kamera-ansicht@|tour:k-pilot@|tour:tut-p4@"
#define CS_LICHT "tour:tut-karte@7|seite:tut-karte|seite:karte_tempo|ort:menue:KeLBuild>KeLichtMenue"
#define CS_SPRACHE "ort:menue:MenuView>ViewLanguage|ort:menue:KeLMisc>KeLLanguage|ort:menue:KeLView>ViewLanguage|seite:einstellungen|seite:erste-schritte"
#define CS_UEBERSETZEN "seite:k-uebersetzen|seite:speichern|tour:k-uebersetzen@|ort:knopf:Compile"
#define CS_BAUEN "tour:tut-karte@12|seite:karte_bauen|ort:menue:KeLBuild"
#define CS_RUECK "seite:k-rueckgaengig|tour:k-rueckgaengig@|ort:menue:MenuEdit>EditUndo|ort:menue:MenuEdit>UndoListMenu|ort:knopf:ActUndo|ort:menue:KeLEdit>KeLUndo"

const FrageH2 kFragenCs[] = {
    {"cs", -1, "Jak donutím NPC chodit po trase mezi body?", CS_LAUFEN},
    {"cs", -1, "postava má běžet místo chůze", CS_LAUFEN},
    {"cs", 1, "přehrát animační sekvenci od začátku", CS_ABSPIELEN},
    {"cs", 1, "načasovat střihy podle dialogu", CS_ABSPIELEN},
    {"cs", -1, "založit novou misi na mapě", CS_MISSION},
    {"cs", 2, "vyměnit skin postavy", CS_SKIN},
    {"cs", 2, "jiné oblečení pro tenhle model", CS_SKIN},
    {"cs", -1, "kde nastavím, kam je nainstalovaná hra", CS_SPIELORDNER},
    {"cs", -1, "program nenašel složku s hrou", CS_SPIELORDNER},
    {"cs", -1, "je k dispozici nová verze programu?", CS_UPDATE},
    {"cs", 3, "přidat dveře, které se otevírají", CS_TUER},
    {"cs", -1, "spustit scénu, když hráč vejde do oblasti", CS_TRIGGER},
    {"cs", -1, "upravit hlavní menu hry", CS_MENUE},
    {"cs", -1, "změnit tlačítko v menu hry", CS_MENUE},
    {"cs", 4, "vyzkoušet menu jako ve hře, mačkat tlačítka", CS_BEDIENEN},
    {"cs", 3, "nastavení", CS_KE_EINST},
    {"cs", -1, "dát texturu na podlahu", CS_TEXTUR},
    {"cs", 3, "nanést textru na zeď", CS_TEXTUR},
    {"cs", -1, "umístit kameru a udělat záběr", CS_KAMERA},
    {"cs", 1, "kamera má sledovat postavu", CS_KAMERA},
    {"cs", 3, "přidat do místnosti světlo", CS_LICHT},
    {"cs", -1, "přepnout program do češtiny", CS_SPRACHE},
    {"cs", -1, "zkompilovat skript na ibi", CS_UEBERSETZEN},
    {"cs", -1, "skript se nedá zkompilovt, kde je chyba", CS_UEBERSETZEN},
    {"cs", 3, "zkompilovat mapu do bsp", CS_BAUEN},
    {"cs", -1, "vrátit poslední změnu zpět", CS_RUECK},
    {nullptr, -1, nullptr, nullptr},
};

#undef CS_LAUFEN
#undef CS_ABSPIELEN
#undef CS_MISSION
#undef CS_SKIN
#undef CS_SPIELORDNER
#undef CS_UPDATE
#undef CS_TUER
#undef CS_TRIGGER
#undef CS_MENUE
#undef CS_BEDIENEN
#undef CS_KE_EINST
#undef CS_TEXTUR
#undef CS_KAMERA
#undef CS_LICHT
#undef CS_SPRACHE
#undef CS_UEBERSETZEN
#undef CS_BAUEN
#undef CS_RUECK
