// hilfe_fragen_es.inc - die unabhaengige Frageliste auf Spanisch (EINGEFROREN)
//
// Vor dem Abstimmen der Wortlisten (src/hilfe_sprache_es.inc) und vor der
// Uebersetzung von Handbuch und Touren geschrieben und danach nicht mehr
// angepasst (die Fragen nicht und die erwarteten Antworten nicht). Aufbau wie
// hilfe_fragen_h2.inc (FrageH2: Sprache, Modus, Frage, erwartete Kennungen als
// Vorsilben). Die 12 Ziele der H2-Liste je zweimal, frei formuliert (keine
// woertlichen Uebersetzungen), dazu vier Ziele der Hauptliste; drei mit
// Tippfehler, acht mit Modus (1 Vorschau, 2 Modell, 3 Karten-Editor,
// 4 Menue-Editor). Ziel Top-3 >= 85 %.
#define E_LAUFEN "seite:k-wegpunkte|seite:wegpunkte|seite:tut-p3|seite:tut-figur|seite:k-patrouille|tour:k-wegpunkte@|tour:tut-p3@|tour:tut-figur@|tour:k-patrouille@"
#define E_ABSPIELEN "seite:zeitleiste|seite:k-zeitleiste-plus|seite:k-neu-timen|seite:tut-erste-szene|seite:tut-p4|tour:k-zeitleiste-plus@|tour:k-neu-timen@|tour:tut-erste-szene@|tour:tut-p4@"
#define E_MISSION "seite:tut-p1|seite:tut-mission|seite:missionen|seite:tut-projekt|tour:tut-p1@|tour:tut-mission@"
#define E_SKIN "seite:k-skins|tour:k-skins@|tour:k-modell@3|ort:menue:MenuFile>OpenSkin|ort:knopf:ModelSkin"
#define E_SPIELORDNER "ort:menue:MenuEdit>PathsTitle|ort:menue:KeLEdit>KeLPreferences|seite:einstellungen|seite:erste-schritte.html#spielordner|tour:tut-karte@0"
#define E_UPDATE "ort:menue:MenuHelp>UpdMenu|ort:menue:MenuHelp>UpdAutoCheck|seite:einstellungen.html#updates"
#define E_TUER "seite:k-tuer|seite:k-tuer-use|seite:karteneditor.html#entities|seite:entities|tour:k-tuer@"
#define E_TRIGGER "seite:k-trigger|seite:k-tuer-use|tour:k-trigger@"
#define E_MENUE "seite:menue_editor|ort:knopf:ViewMenuEditor@modebar|ort:knopf:Me"
#define E_BEDIENEN "seite:menue_editor.html#bedienen|ort:knopf:MeBedienen"
#define E_KE_EINST "ort:menue:KeLEdit>KeLPreferences"
#define E_TEXTUR "tour:tut-karte@4|tour:tut-karte@6|seite:karteneditor.html#texturen"

const FrageH2 kFragenEs[] = {
    {"es", -1, "¿Cómo consigo que un NPC recorra un camino?", E_LAUFEN},
    {"es", 1, "un personaje que vaya andando de un waypoint a otro", E_LAUFEN},
    {"es", 1, "ver la escena desde el principio", E_ABSPIELEN},
    {"es", -1, "reproducir la cinemática", E_ABSPIELEN},
    {"es", -1, "crear una misión nueva", E_MISSION},
    {"es", -1, "quiero empezar mi propia mison", E_MISSION},
    {"es", 2, "cambiar la skin del modelo", E_SKIN},
    {"es", -1, "elegir otra skin para el personaje", E_SKIN},
    {"es", -1, "¿Dónde indico la carpeta en la que está instalado Jedi Academy?", E_SPIELORDNER},
    {"es", -1, "configurar la ruta del juego", E_SPIELORDNER},
    {"es", -1, "buscar actualisaciones", E_UPDATE},
    {"es", -1, "¿hay una versión nueva del programa?", E_UPDATE},
    {"es", 3, "hacer una puerta que se abra", E_TUER},
    {"es", -1, "puerta automática en mi mapa", E_TUER},
    {"es", 3, "crear un trigger", E_TRIGGER},
    {"es", -1, "que pase algo cuando el jugador entra en una zona", E_TRIGGER},
    {"es", -1, "editar los menús del juego", E_MENUE},
    {"es", -1, "abrir el editor de menús", E_MENUE},
    {"es", 4, "probar los botones del menú", E_BEDIENEN},
    {"es", 4, "modo interactivo para usar el menú", E_BEDIENEN},
    {"es", 3, "preferencias del editor de mapas", E_KE_EINST},
    {"es", -1, "abrir los ajustes del editor de mapas", E_KE_EINST},
    {"es", 3, "poner una texura en un brush", E_TEXTUR},
    {"es", -1, "aplicar una textura a la pared del mapa", E_TEXTUR},
    {"es", -1, "cambiar el idioma del programa", "ort:menue:MenuView>ViewLanguage"},
    {"es", -1, "compilar el mapa", "tour:tut-karte@12|seite:karteneditor_bauen|seite:karte_bauen"},
    {"es", -1, "añadir una luz al mapa", "tour:tut-karte@7"},
    {"es", -1, "cambiar los atajos de teclado", "ort:menue:MenuEdit>KeysTitle|seite:tasten"},
    {nullptr, -1, nullptr, nullptr},
};

#undef E_LAUFEN
#undef E_ABSPIELEN
#undef E_MISSION
#undef E_SKIN
#undef E_SPIELORDNER
#undef E_UPDATE
#undef E_TUER
#undef E_TRIGGER
#undef E_MENUE
#undef E_BEDIENEN
#undef E_KE_EINST
#undef E_TEXTUR
