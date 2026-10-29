// hilfe_sprache_es.inc - der Helfer auf Spanisch (Sprache es, tools/sprachen.py)
//
// Von Hand gepflegt, eingebunden nur von src/hilfe.cpp (kSprachDaten). Alles
// klein und gefaltet geschrieben (ohne Akzente: "camara" fuer "cámara",
// "animacion" fuer "animación") - so zerlegt src/hilfe.cpp jeden Text.
//
// STOPP: Fuellwoerter von Fragen ("como hago una ..."), zusaetzlich zu den
// deutschen und englischen (kStoppAllgemein).
// ENDUNGEN: vor den allgemeinen gestutzt, laengste zuerst, der Stamm behaelt
// mindestens vier Buchstaben (stutze): animacion, animaciones, animar,
// animado -> anim; textura, texturas -> textur.
// BEGRIFFE: spanische Woerter zu den Begriffen aus hilfe_daten.inc (gleicher
// Name; ein neuer Name ist ein neuer Begriff).
// ZIELE: haeufige Aufgaben, wie Leute auf Spanisch fragen (Kennung wie in
// kZiele).

const char* const kStoppEs[] = {
    "como", "que", "hago", "puedo", "quiero", "necesito", "deseo", "se", "un", "una", "unos", "unas", "el", "la",
    "los", "las", "lo", "de", "del", "al", "a", "en", "con", "para", "por", "mi", "mis", "tu", "tus", "su", "sus",
    "y", "o", "u", "e", "es", "son", "esta", "este", "esto", "estos", "estas", "ese", "esa", "eso", "donde", "cuando",
    "cual", "cuales", "quien", "me", "te", "le", "les", "nos", "hay", "muy", "mas", "pero", "si", "no", "ya", "algo",
    "favor", "puede", "pueden", "debo", "tengo", "hacer", "hace", "hacen", nullptr};

const char* const kEndungenEs[] = {
    "aciones", "amiento", "imiento", "adores", "ciones", "acion", "mente", "ador", "ando", "iendo", "ados", "adas", "idos",
    "idas", "cion", "ado", "ada", "ido", "ida", "ar", "er", "ir", "as", "os", "es", "a", "o", "e", "s", nullptr};

const BegriffRoh kBegriffeEs[] = {
    {"kamera", "camara|plano|toma|encuadre"},
    {"fahrt", "movimiento de camara|trayecto de camara|travelling|recorrido de camara|mover la camara"},
    {"szene", "cinematica|escena|secuencia|cutscene|video"},
    {"vorschau", "vista previa|previsualizar|reproducir|ver|mirar|reproduccion|desde el principio"},
    {"skript", "script|guion|icarus"},
    {"befehl", "comando|orden|linea|instruccion"},
    {"uebersetzen", "compilar|compilacion|ibi"},
    {"npc", "npc|personaje|figura|actor|enemigo|pnj"},
    {"laufen", "caminar|andar|correr|ir|moverse|mover|recorrer|andando|caminando"},
    {"wegpunkt", "waypoint|waypoints|punto de ruta|puntos de ruta|navgoal|destino"},
    {"patrouille", "patrulla|patrullar|ronda|guardia|vigilar|hacer la ronda"},
    {"karte", "mapa|nivel|bsp"},
    {"karteneditor", "editor de mapas|radiant|netradiant|mapeado|mapear"},
    {"pinsel", "brush|bloque|pared|muro|sala|habitacion|cuarto|caja"},
    {"textur", "textura|texturas|texturizar|shader|material"},
    {"licht", "luz|luces|iluminacion|iluminar|lampara|oscuro|oscuridad"},
    {"spawn", "info_player_start|inicio del jugador|punto de inicio|punto de aparicion|aparecer|aparicion"},
    {"bauen", "compilar el mapa|q3map2|vis|lightmap|hornear|horneado"},
    {"optimierer", "optimizador|optimizar|limites|demasiados"},
    {"spielen", "jugar en el editor|probar|prueba|recorrer el mapa|probar en el juego"},
    {"speichern", "guardar|guardado|salvar"},
    {"oeffnen", "abrir|cargar"},
    {"neu", "nuevo|nueva|crear|hacer|empezar|anadir|poner"},
    {"rueckgaengig", "deshacer|rehacer|revertir|volver atras"},
    {"suchen", "buscar|encontrar|reemplazar|busqueda"},
    {"einstellungen", "ajustes|preferencias|opciones|configuracion|configurar"},
    {"sprache", "idioma|lengua|espanol|ingles|aleman|chino|japones|checo"},
    {"thema", "tema|modo oscuro|colores|combinacion de colores|apariencia"},
    {"update", "actualizacion|actualizaciones|actualizar|version nueva|nueva version"},
    {"modell", "modelo|glm|ghoul2|malla|md3"},
    {"skin", "skin|apariencia|ropa|aspecto"},
    {"animation", "animacion|animaciones|animar|pose|anim"},
    {"mission", "mision|misiones"},
    {"tutorial", "tutorial|tutoriales|aprender|paso a paso|guia rapida"},
    {"handbuch", "guia|manual|documentacion|ayuda"},
    {"tasten", "atajo|atajos|atajos de teclado|teclas|tecla|teclado"},
    {"entity", "entidad|entidades|objeto"},
    {"tuer", "puerta|puertas|func_door|porton"},
    {"trigger", "trigger|disparador|activador|zona de activacion"},
    {"ton", "sonido|audio|volumen|musica|ruido"},
    {"dialog", "dialogo|hablar|decir|voz|frase|conversacion"},
    {"saber", "sable laser|sable|lightsaber"},
    {"kampf", "combate|luchar|lucha|pelea|atacar|ataque"},
    {"signal", "senal|esperar|espera|waitsignal"},
    {"variable", "variable|variables"},
    {"zeitleiste", "linea de tiempo|pistas|sincronizar|tiempos|cronologia"},
    {"caulk", "caulk|autocaulk|caras invisibles"},
    {"hohl", "vaciar|hueco|hueca|csg|ahuecar"},
    {"ordner", "ruta|rutas|carpeta|carpetas|directorio|gamedata|carpeta del juego|instalado"},
    {"backup", "copia de seguridad|copias de seguridad|autoguardado|respaldo"},
    {"fehler", "error|errores|aviso|avisos|problema|mensaje|mensajes|roto"},
    {"leck", "fuga|leak|leaked|agujero|no sellado"},
    {"ki", "ia|inteligencia artificial|claude|asistente|chatbot|mcp"},
    {"graph", "grafo|diagrama|nodos|flujo"},
    {"lesezeichen", "marcador|marcadores"},
    {"skalierung", "escala|tamano de letra|letra mas grande|mas grande|interfaz grande"},
    {"mehrspieler", "multijugador|mp"},
    {"effekt", "efecto|efectos|particulas|fuego|humo|chispas"},
    {"spiel", "juego|jedi academy|jka|jedi outcast|jk2"},
    {"verlauf", "historial|cambios|registro de cambios|novedades"},
    {"modus", "modo|barra de modos"},
    // nur Spanisch: der Bedienen-Modus des Menue-Editors heisst "Interactuar"
    {"bedienen", "interactuar|interactivo|probar el menu|usar el menu|probar los botones"},
    {nullptr, nullptr},
};

const ZielRoh kZieleEs[] = {
    {"tour:tut-karte", "karte", "primer mapa crear un mapa hacer un nivel mi propio mapa"},
    {"tour:tut-karte@4", "karte", "poner una textura en un brush aplicar textura a la pared texturizar"},
    {"tour:tut-karte@5", "karte", "vaciar la sala hacer una habitacion hueca"},
    {"tour:tut-karte@7", "karte", "anadir una luz poner una luz iluminar el mapa"},
    {"tour:tut-karte@8", "karte", "inicio del jugador donde aparece el jugador punto de inicio"},
    {"tour:tut-karte@12", "karte", "compilar el mapa compilar mapa"},
    {"tour:tut-karte@14", "karte", "empaquetar el mapa como pk3 llevar el mapa al juego"},
    {"tour:k-neues-skript", "skript", "escribir un script nuevo primer script"},
    {"tour:k-uebersetzen", "skript", "compilar el script"},
    {"tour:k-kamera-ansicht", "vorschau", "poner la camara donde miro plano de camara"},
    {"seite:k-wegpunkte.html", "vorschau", "personaje que camine de un waypoint a otro recorrer un camino ir de un punto a otro"},
    {"seite:k-gangart.html", "vorschau", "hacer que un npc camine o corra"},
    {"seite:zeitleiste.html", "vorschau", "ver la escena desde el principio reproducir la cinematica linea de tiempo"},
    {"seite:tut-p1.html", "mission", "crear una mision nueva empezar mi propia mision"},
    {"ort:menue:MenuFile>LoadMission", "mission", "cargar una mision abrir una mision"},
    {"seite:k-skins.html", "modell", "cambiar la skin del modelo otra skin"},
    {"ort:menue:MenuEdit>AppPrefs", "einstellungen", "abrir los ajustes preferencias del programa"},
    {"ort:menue:MenuView>ViewLanguage", "einstellungen", "cambiar el idioma del programa idioma"},
    {"ort:menue:MenuEdit>KeysTitle", "einstellungen", "cambiar los atajos de teclado teclas"},
    {"ort:menue:MenuEdit>PathsTitle", "einstellungen", "carpeta del juego ruta del juego donde esta instalado jedi academy"},
    {"ort:menue:MenuHelp>UpdMenu", "updates", "buscar actualizaciones version nueva del programa"},
    {"ort:menue:KeLEdit>KeLPreferences", "karte", "preferencias del editor de mapas ajustes del editor de mapas"},
    {"seite:k-tuer.html", "vorschau", "puerta que se abre puerta automatica abrir una puerta"},
    {"seite:k-trigger.html", "vorschau", "crear un trigger que pase algo cuando el jugador entra en una zona"},
    {"seite:menue_editor.html", "menue", "editar los menus del juego editor de menus"},
    {"seite:menue_editor.html#bedienen", "menue", "probar los botones del menu modo interactivo"},
    {nullptr, nullptr, nullptr},
};
