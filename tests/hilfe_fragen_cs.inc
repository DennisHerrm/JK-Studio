// hilfe_fragen_cs.inc - die Frageliste fuer Tschechisch (Stufe Sprachen, EINGEFROREN)
//
// Geschrieben, BEVOR der Helfer tschechische Begriffe kannte, und danach
// nicht mehr angepasst (weder die Fragen noch die erwarteten Antworten).
// 26 Fragen in 17 Zielen, frei formuliert wie ein Anwender fragt (keine
// woertlichen Uebersetzungen der H2-Liste), mit zwei Tippfehlern und acht
// Fragen, die erst mit dem aktuellen Modus eindeutig werden (modus >= 0:
// 1 Vorschau, 2 Modell, 3 Karten-Editor, 4 Menue-Editor; -1 = ohne Kontext).
// Erwartet: Vorsilben der Kennungen wie in kFragen (passt()); "tour:x@" =
// jeder Schritt der Tour x. Gemessen wird Top-3.
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

struct FrageCs {
    int modus;
    const char* text;
    const char* erwartet;
};

const FrageCs kFragenCs[] = {
    {-1, "Jak donutím NPC chodit po trase mezi body?", CS_LAUFEN},
    {-1, "postava má běžet místo chůze", CS_LAUFEN},
    {1, "přehrát animační sekvenci od začátku", CS_ABSPIELEN},
    {1, "načasovat střihy podle dialogu", CS_ABSPIELEN},
    {-1, "založit novou misi na mapě", CS_MISSION},
    {2, "vyměnit skin postavy", CS_SKIN},
    {2, "jiné oblečení pro tenhle model", CS_SKIN},
    {-1, "kde nastavím, kam je nainstalovaná hra", CS_SPIELORDNER},
    {-1, "program nenašel složku s hrou", CS_SPIELORDNER},
    {-1, "je k dispozici nová verze programu?", CS_UPDATE},
    {3, "přidat dveře, které se otevírají", CS_TUER},
    {-1, "spustit scénu, když hráč vejde do oblasti", CS_TRIGGER},
    {-1, "upravit hlavní menu hry", CS_MENUE},
    {-1, "změnit tlačítko v menu hry", CS_MENUE},
    {4, "vyzkoušet menu jako ve hře, mačkat tlačítka", CS_BEDIENEN},
    {3, "nastavení", CS_KE_EINST},
    {-1, "dát texturu na podlahu", CS_TEXTUR},
    {3, "nanést textru na zeď", CS_TEXTUR},
    {-1, "umístit kameru a udělat záběr", CS_KAMERA},
    {1, "kamera má sledovat postavu", CS_KAMERA},
    {3, "přidat do místnosti světlo", CS_LICHT},
    {-1, "přepnout program do češtiny", CS_SPRACHE},
    {-1, "zkompilovat skript na ibi", CS_UEBERSETZEN},
    {-1, "skript se nedá zkompilovt, kde je chyba", CS_UEBERSETZEN},
    {3, "zkompilovat mapu do bsp", CS_BAUEN},
    {-1, "vrátit poslední změnu zpět", CS_RUECK},
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
