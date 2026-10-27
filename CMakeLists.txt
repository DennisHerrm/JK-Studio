cmake_minimum_required(VERSION 3.20)
project(behaved VERSION 0.6.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "" FORCE)
endif()
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# --- Statische C++-Laufzeit (rc601 release) ------------------------------
#
# shank: "bei jedem Release sollte es auch eine .exe geben, die gebaut wird,
# dass die Leute sie runterladen und nutzen koennen". Bis rc600 band MSVC die
# Laufzeit dynamisch (/MD): behaved.exe brauchte MSVCP140.dll,
# MSVCP140_ATOMIC_WAIT.dll, VCRUNTIME140.dll, VCRUNTIME140_1.dll und elf
# api-ms-win-crt-*.dll - keine davon lag im Zip. tools/check_exe_imports.py
# meldete am veroeffentlichten rc600 "Importpruefung: 16 Beanstandung(en)".
# Ohne VC++-Redistributable, das mindestens so neu ist wie die Bauwerkzeuge
# (so verlangt es Microsoft), startet so ein Programm gar nicht.
#
# /MT bindet die Laufzeit ins Programm (etwa 1 MB mehr). Die DLLs NEBEN die
# .exe zu legen waere keine Loesung: der Updater ersetzt alles ausser der
# laufenden .exe mit MoveFileExW, und eine geladene DLL laesst sich nicht
# ersetzen (gui/update_win32.cpp). Seit rc601 prueft der ctest exe_importe
# die Importtabelle bei jedem Testlauf.
#
# Eigene Bauordner (Fuzzing mit AddressSanitizer) koennen weiterhin
# -DCMAKE_MSVC_RUNTIME_LIBRARY=MultiThreadedDLL angeben.
if(MSVC AND NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
  set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

add_library(bhed_core STATIC
  src/script.cpp
  src/update.cpp
  # rc601 tour: JSON lesen und schreiben (der Leser stammt aus update.cpp).
  src/json.cpp
  # rc601 tour: Tutorials im Programm - Daten, Zielangaben, Pruefungen,
  # Ablauf, Fortschritt, Suche im Handbuch (die Oberflaeche: gui/tour.cpp).
  src/tour.cpp
  src/bhc.cpp
  src/validate.cpp
  src/tree.cpp
  src/edit.cpp
  src/ibi.cpp
  src/i18n.cpp
  src/changelog.cpp
  src/mpfelder.cpp
  src/theme.cpp
  src/settings.cpp
  src/keys.cpp
  src/diag.cpp
  src/bsp.cpp
  src/inflate.cpp
  src/pk3.cpp
  src/pk3write.cpp
  src/entdefs.cpp
  src/missionbau.cpp
  src/wegpunkte.cpp
  src/moveranim.cpp
  src/roffanim.cpp
  src/missionsgraph.cpp
  src/sound.cpp
  src/timeline.cpp
  src/bspgeo.cpp
  src/gpucam.cpp
  src/gpuconst.cpp
  src/gpudraw.cpp
  src/gpushader.cpp
  src/gpuskin.cpp
  src/gpustate.cpp
  src/mapview.cpp
  # rc601 saber: Lichtschwerter beruehren Waende und einander (Funken, Klang,
  # gekuerzte Klinge) wie WP_SaberDamageTrace/CG_AddSaberBladeGo. Siehe
  # bhed/saberkontakt.h.
  src/saberkontakt.cpp
  # rc601 integ: der zentrale Schalter OpenJK / Movie Duels. Siehe
  # bhed/regelwerk.h.
  src/regelwerk.cpp
  # rc601 saber (Review): Brandspuren an Waenden - CG_CreateSaberMarks,
  # R_MarkFragments, cg_marks.cpp. Siehe bhed/saberspuren.h.
  src/saberspuren.cpp
  src/image.cpp
  src/interplay.cpp
  src/image_jpeg.cpp
  src/image_png.cpp
  src/shaderscript.cpp
  src/glm.cpp
  src/gla.cpp
  src/camtrack.cpp
  src/scene.cpp
  # rc602: SP_NPC_* -> NPC_type (beide Regelwerke), siehe bhed/npcklasse.h.
  src/npcklasse.cpp
  # rc602: Abgleich aller Karten mit dem Spiel (Selbsttest kartenpruefung).
  src/kartenpruefung.cpp
  # rc602: Gegenstaende der Karte (items.dat, G_CallSpawn, FinishSpawningItem).
  src/gegenstaende.cpp
  # rc602: Shaderstufen der Figuren (ComputeColors/ComputeTexCoords).
  src/figurstufen.cpp
  # rc604: surfaceSprites (Gras, Buesche, Regenspritzer) wie
  # tr_surfacesprites.cpp. Siehe bhed/grasflaechen.h.
  src/grasflaechen.cpp
  # Waffe und Lichtschwerter einer Figur ueber die Zeit (SET_WEAPON,
  # SET_SABER*, Handmodelle). Siehe den Kopf der Datei.
  src/ausruestung.cpp
  src/mover.cpp
  # EINE Uhr fuer Kamerabahn, Zeitleiste und Szene. Warum, steht in
  # bhed/clock.h.
  src/clock.cpp
  # Der ICARUS-Nachbau: alle Skripte einer Zwischensequenz Bild fuer Bild
  # ausfuehren (run, affect, do/dowait, use, Spawner). Siehe bhed/ablauf.h.
  src/ablauf.cpp
  # rc601 bewegung: Schwerkraft wie Pmove (fallen, Stufen, Boden). Siehe
  # bhed/scene.h, schwerkraftEinrechnen.
  src/schwerkraft.cpp
  # rc601 bewegung: Laufziele ohne Weg im Wegnetz (STEER::SafeToGoTo /
  # NAV::GoTo). Siehe bhed/scene.h, navZieleOhneWeg.
  src/navweg.cpp
  # rc601 bewegung: Laufen und Wege wie im Spiel (STEER, NAV::GoTo, A*,
  # Pmove je Denkbild). Siehe bhed/navlauf.h.
  src/navlauf.cpp
  # rc601 bewegung: Spuren gegen die Brushes der Karte (CM_BoxTrace) fuer
  # NAV/STEER. Siehe bhed/kollision.h.
  src/kollision.cpp
  # ROFF-Bahnen (*.rof) fuer play ( "PLAY_ROFF", ... ): lesen wie
  # G_LoadRoff, abspielen wie G_Roff. Siehe bhed/roff.h.
  src/roff.cpp
  # Zahlen aus Textdateien lesen, ohne Ausnahmen. EINE Stelle, die weiss,
  # dass std::from_chars keine fuehrenden Leerzeichen ueberspringt.
  src/num.cpp
  # .md3-Modelle: Konsolen, Kisten, Rohre, Raumschiffe. 1549 Entities in den
  # Karten der Mod zeigen darauf.
  src/md3.cpp
  # Die Wellenformen der Shader: deformVertexes wave, rgbGen wave.
  src/wave.cpp
  # Effekte. Der Leser und die Simulation stammen aus efxed und sind dort
  # gegen FxScheduler.cpp und FxPrimitives.cpp gebaut; hier kommt nur das
  # Zeichnen dazu (src/efxdraw.cpp).
  src/efx_gp2.cpp
  src/efx_effect.cpp
  src/efx_read.cpp
  src/efx_sim.cpp
  src/efx_curve.cpp
  src/efxdraw.cpp
  src/arbeiter.cpp
  src/mission.cpp)
target_include_directories(bhed_core PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
# third_party als SYSTEM: dann schweigt der Uebersetzer ueber fremden Code.
#
# minimp3.h erzeugt bei /W4 ein Dutzend C4244 ("moeglicher Datenverlust").
# Die sind nicht falsch, aber wir koennen sie nicht beheben - es ist fremder
# Code, und ein Eingriff ginge beim naechsten Abgleich verloren. Warnungen,
# die niemand beheben kann, verdecken nur die eigenen.
target_include_directories(bhed_core SYSTEM PUBLIC
                           ${CMAKE_CURRENT_SOURCE_DIR}/third_party)

if(MSVC)
  target_compile_options(bhed_core PUBLIC /W4 /permissive- /utf-8 /Zc:__cplusplus)
  target_compile_definitions(bhed_core PUBLIC _CRT_SECURE_NO_WARNINGS NOMINMAX)

  # Ausweg fuer einen INTERNEN COMPILERFEHLER.
  #
  # MSVC 14.50 (Visual Studio 18) stuerzt bei manchen Rechnern in der
  # Codeerzeugung ab - "INTERNER COMPILERFEHLER in CL.exe", Rueckgabe
  # -1073741819, das ist eine Zugriffsverletzung IM UEBERSETZER. Nicht unser
  # Code ist falsch; der Uebersetzer kommt mit ihm nicht zurecht.
  #
  # Zwei Stellschrauben, die in solchen Faellen fast immer helfen:
  #
  #   /Ob1   weniger Einbetten von Funktionen. Die Codeerzeugung arbeitet
  #          dann mit kleineren Einheiten, und genau dort stuerzt sie ab.
  #   /MP-   jede Datei einzeln statt in Buendeln. Das Buendel war hier
  #          image_png + shaderscript + glm + gla + camtrack + scene +
  #          mission; einzeln uebersetzt faellt oft auf, WELCHE Datei es
  #          ist.
  #
  # Kostet etwas Geschwindigkeit im fertigen Programm - aber ein Programm,
  # das sich nicht bauen laesst, ist unendlich langsam.
  option(BEHAVED_MSVC_SAFE_CODEGEN
         "Vorsichtige Codeerzeugung - Ausweg bei internem Compilerfehler" OFF)
  if(BEHAVED_MSVC_SAFE_CODEGEN)
    target_compile_options(bhed_core PRIVATE /Ob1)
    message(STATUS "Vorsichtige Codeerzeugung: /Ob1")
  endif()
else()
  # -Wshadow dazu.
  #
  # MSVC meldet Verdeckungen seit jeher als C4456 und hatte bei /W4 sechs
  # Stellen gefunden, die hier stumm durchliefen - GCC macht das nur mit
  # -Wshadow, und das war nicht eingeschaltet. Ein Bau, der weniger findet
  # als der andere, ist der schlechtere Bau.
  #
  # Eine verdeckte Variable ist selten sofort ein Fehler, aber sie ist
  # immer eine Falle: wer spaeter die aeussere meint und die innere
  # erwischt, sucht lange.
  target_compile_options(bhed_core PUBLIC -Wall -Wextra -Wshadow)
endif()

# --- Eingebaute Daten (rc601 release) ------------------------------------
#
# Eine behaved.exe ALLEIN lief bis rc600 nicht: ohne data\base daneben kam
# "behaved.bhc not found in ..." und das Programm endete mit 2
# (gui/main_win32.cpp). Fuer die eigenstaendige .exe im Release stecken die
# Dateien von data/base jetzt im Programm (bhed/eingebaut.h); fehlt der Ordner
# neben der .exe, packt behaved sie in den Einstellungsordner aus.
target_sources(bhed_core PRIVATE src/eingebaut.cpp)
include(${CMAKE_CURRENT_SOURCE_DIR}/tools/eingebaute_daten.cmake)
file(GLOB BEHAVED_EINGEBAUT CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/base/*")
# rc604: der Befehlssatz fuer Jedi Outcast (data/jk2, tools/gen_jk2daten.py)
# kommt mit - im Programm unter "jk2/<name>", ausgepackt neben data/base.
file(GLOB BEHAVED_EINGEBAUT_JK2 CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/jk2/*")
list(APPEND BEHAVED_EINGEBAUT ${BEHAVED_EINGEBAUT_JK2})
# Karten-Editor (Stufe 1b): die Entitaetsklassen data/karte/<spiel>/*.def -
# im Programm unter "karte/<spiel>/<name>", ausgepackt nach data/karte.
file(GLOB BEHAVED_EINGEBAUT_KARTE CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/data/karte/*/*")
list(APPEND BEHAVED_EINGEBAUT ${BEHAVED_EINGEBAUT_KARTE})
behaved_eingebaute_daten(bhed_core ${CMAKE_BINARY_DIR}/eingebaut/eingebaute_daten.inc ${BEHAVED_EINGEBAUT})

# Werkzeug
add_executable(bhed tools/bhed_cli.cpp)
target_link_libraries(bhed PRIVATE bhed_core)

# Befehlsmodell neben die .exe legen, damit bhed ohne --data laeuft
add_custom_command(TARGET bhed POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/data/base
          $<TARGET_FILE_DIR:bhed>/data/base
  COMMAND ${CMAKE_COMMAND} -E copy_directory
          ${CMAKE_CURRENT_SOURCE_DIR}/data/jk2
          $<TARGET_FILE_DIR:bhed>/data/jk2)

# --- Stufe 3: q3map2 aus NetRadiant-Custom 20260114 als bhed_q3map2.exe -----------
# (src/q3map2/CMakeLists.txt; die Quellen liegen unveraendert unter third_party).
option(BEHAVED_Q3MAP2 "q3map2 (Karten bauen) mitbauen" ${MSVC})
# Stufe 5 Punkt 6: q3map2 mit clang-cl statt MSVC - dieselben Ergebnisse
# (q3map2_vergleich, der Byte-Vergleich wie Stufe 3), -light 14-21 % und -vis
# bis 25 % schneller (NOTIZEN_karte5.md). BEHAVED_Q3MAP2_LLVM = Ordner mit
# clang-cl.exe, lld-link.exe, llvm-lib.exe und llvm-rc.exe (LLVM 23.1.4, mit
# dieser Fassung ist verglichen); leer = MSVC wie bisher. q3map2 wird dann als
# eigenes Projekt mit Ninja und clang-cl gebaut (tools/karte/q3map2_clang.cmake,
# Ordner q3map2_clang im Bauordner) und neben behaved.exe gelegt. clang-cl und
# lld-link finden Visual Studio und das Windows SDK selbst.
set(BEHAVED_Q3MAP2_LLVM "" CACHE PATH "LLVM-bin-Ordner mit clang-cl: q3map2 damit bauen (schneller, gleiches Ergebnis)")
set(BHED_Q3MAP2_ZIEL "")
if(MSVC AND BEHAVED_Q3MAP2)
  set(BHED_Q3_CLANG OFF)
  if(BEHAVED_Q3MAP2_LLVM AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
    find_program(BHED_NINJA ninja
                 HINTS "${CMAKE_GENERATOR_INSTANCE}/Common7/IDE/CommonExtensions/Microsoft/CMake/Ninja"
                       "${BEHAVED_Q3MAP2_LLVM}")
    set(BHED_Q3_FEHLT "")
    foreach(w clang-cl lld-link llvm-lib llvm-rc)
      if(NOT EXISTS "${BEHAVED_Q3MAP2_LLVM}/${w}.exe")
        string(APPEND BHED_Q3_FEHLT " ${w}.exe")
      endif()
    endforeach()
    if(NOT BHED_NINJA)
      string(APPEND BHED_Q3_FEHLT " ninja.exe")
    endif()
    if(BHED_Q3_FEHLT)
      message(WARNING "BEHAVED_Q3MAP2_LLVM=${BEHAVED_Q3MAP2_LLVM}: es fehlt${BHED_Q3_FEHLT} - q3map2 wird mit MSVC gebaut")
    else()
      set(BHED_Q3_CLANG ON)
    endif()
  endif()
  if(BHED_Q3_CLANG)
    set(BHED_Q3_CLANG_BAU ${CMAKE_BINARY_DIR}/q3map2_clang)
    # tools/karte/q3map2_clang.cmake: beim ersten Mal konfigurieren, dann
    # Ninja bauen lassen und die .exe neben behaved.exe kopieren - bei jedem
    # Bau (Ninja ist schnell, wenn nichts zu tun ist).
    add_custom_target(bhed_q3map2_clang ALL
      COMMAND ${CMAKE_COMMAND} -DQUELLE=${CMAKE_CURRENT_SOURCE_DIR} -DBAU=${BHED_Q3_CLANG_BAU} -DLLVM=${BEHAVED_Q3MAP2_LLVM}
              -DNINJA=${BHED_NINJA} -DZIEL=${CMAKE_BINARY_DIR}/bin/$<CONFIG>/bhed_q3map2.exe
              -P ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_clang.cmake
      COMMENT "bhed_q3map2 (clang-cl)"
      USES_TERMINAL VERBATIM)
    add_executable(bhed_q3map2 IMPORTED GLOBAL)
    foreach(k Release Debug RelWithDebInfo MinSizeRel)
      string(TOUPPER ${k} K)
      set_target_properties(bhed_q3map2 PROPERTIES IMPORTED_LOCATION_${K} ${CMAKE_BINARY_DIR}/bin/${k}/bhed_q3map2.exe)
    endforeach()
    set(BHED_Q3MAP2_ZIEL bhed_q3map2_clang)
    message(STATUS "q3map2: clang-cl aus ${BEHAVED_Q3MAP2_LLVM}")
  else()
    add_subdirectory(src/q3map2)
    set(BHED_Q3MAP2_ZIEL bhed_q3map2)
  endif()
endif()

# --- Oberflaeche -------------------------------------------------------
#
# Dear ImGui 1.92 oder neuer wird erwartet. Nicht mitgeliefert:
#   git clone --depth 1 -b v1.92.9b-docking https://github.com/ocornut/imgui.git
set(IMGUI_DIR "" CACHE PATH "Pfad zu Dear ImGui (>= 1.92)")

if(WIN32 AND IMGUI_DIR AND EXISTS "${IMGUI_DIR}/imgui.cpp")
  # Direct3D 11 nur, wenn verzoegertes Laden moeglich ist.
  #
  # Fest gebunden stuenden d3d11.dll und d3dcompiler_47.dll in der
  # Importtabelle - Windows laedt das Programm dann gar nicht, wenn eine
  # davon fehlt, und der Rueckfall auf OpenGL kaeme nie zum Zug. MSVC kann
  # /DELAYLOAD, MinGW nicht; dort wird nur OpenGL gebaut.
  option(BEHAVED_D3D11 "Direct3D 11 mitbauen (braucht /DELAYLOAD)" ${MSVC})

  # Dear ImGui als eigenes Ziel, OHNE unsere Warnungspolitik.
  #
  # Mit -Werror scheitert der Bau an ImGuis eigenen Quellen: GCC meldet dort
  # falsch-positive -Warray-bounds in DockNodeTreeUpdatePosSize und
  # IsMouseDragging. Fremdcode gehoert nicht unter unsere Regeln - wir
  # koennen ihn nicht aendern, und ein abgeschalteter -Werror im ganzen
  # Projekt waere der falsche Preis dafuer.
  add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_win32.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
    $<$<BOOL:${BEHAVED_D3D11}>:${IMGUI_DIR}/backends/imgui_impl_dx11.cpp>)
  target_include_directories(imgui PUBLIC ${IMGUI_DIR} ${IMGUI_DIR}/backends
                             ${CMAKE_CURRENT_SOURCE_DIR}/gui)
  target_compile_definitions(imgui PUBLIC IMGUI_USER_CONFIG="imgui_config.h")
  if(MSVC)
    target_compile_options(imgui PRIVATE /W0)
  else()
    target_compile_options(imgui PRIVATE -w)
  endif()

  add_executable(behaved WIN32
    gui/app.cpp
    gui/app_view3d.cpp
    gui/app_mission.cpp
    gui/uiicons.cpp
    gui/app_ansichten.cpp
    gui/app_kamera.cpp
    gui/app_wegpunkte.cpp
    gui/app_modell.cpp
    gui/app_animation.cpp
    gui/app_wetter.cpp
    gui/app_gras.cpp
    gui/app_ton.cpp
    gui/app_saber.cpp
    # rc601 bewegung: Schwerkraft der Figuren gegen die Karte
    gui/app_bewegung.cpp
    # rc601 integ: der zentrale Schalter OpenJK / Movie Duels
    gui/app_regelwerk.cpp
    # Stufe 1b: der Arbeitsbereich Karten-Editor (NetRadiant-Custom)
    gui/app_karte.cpp
    gui/app_karte_befehle.cpp
    # Stufe 3: Karten bauen (Build-Menue, Customize, RunBSP, Ueberwachung)
    gui/app_karte_bau.cpp
    gui/karte_bau_win32.cpp
    gui/app_karte_leck.cpp
    gui/app_karte_portale.cpp
    gui/app_karte_bsp.cpp
    gui/app_karte_rueck.cpp
    gui/app_karte_zusammen.cpp
    gui/app_karte_optimierer.cpp
    gui/app_karte_echtlicht.cpp
    gui/app_karte_plugins.cpp
    gui/app_karte_brushexport.cpp
    gui/app_karte_kleineplugins.cpp
    gui/app_karte_terrain.cpp
    gui/app_karte_meshtex.cpp
    gui/karte_qtdialog.cpp
    gui/app_karte_menue.cpp
    gui/app_karte_dialoge.cpp
    gui/app_karte_csg.cpp
    gui/app_karte_formen.cpp
    gui/app_karte_transform.cpp
    gui/app_karte_patch.cpp
    gui/app_karte_si.cpp
    gui/app_karte_tags.cpp
    gui/app_karte_datei.cpp
    gui/app_karte_filter.cpp
    gui/app_karte_ebenen.cpp
    gui/app_karte_modelle.cpp
    gui/app_karte_modellbrowser.cpp
    gui/karte_vektoricons.cpp
    gui/app_karte_xy.cpp
    gui/app_karte_cam.cpp
    gui/app_karte_auswahl.cpp
    gui/app_karte_texturen.cpp
    gui/app_karte_entitaeten.cpp
    gui/app_karte_konsole.cpp
    gui/app_karte_qtmenue.cpp
    gui/karte_gpu_win32.cpp
    gui/tastatur_win32.cpp
    gui/backend_win32.cpp
    gui/gpumap_win32.cpp
    gui/main_win32.cpp
    gui/platform_win32.cpp
    # Stufe 3, Punkt 0: Textbreiten wie Qt 5 (DirectWrite, Entwurfsmasse)
    gui/qttext_win32.cpp
    gui/audio_win32.cpp
    gui/icons_gen.cpp
    gui/selbsttest.cpp
    # rc601 tour: das Element-Register (bis rc600 in selbsttest.cpp) - auch
    # fuer die Tutorials im Programm, gefiltert.
    gui/elemente.cpp
    # rc601 tour: Tutorials im Programm - Fenster, Hervorhebung, Auswertung.
    gui/tour.cpp
    gui/update_win32.cpp
    gui/vektoricons.cpp
    gui/wache_win32.cpp
    # Symbol, Versionsangaben und Manifest. Ohne das zeigt der Explorer das
    # Standardsymbol, und Rechtsklick -> Details bleibt leer.
    res/behaved.rc)
  # IM_ASSERT wird ueber gui/imgui_config.h umgeleitet (IMGUI_USER_CONFIG am
  # Ziel imgui), damit eine ImGui-Zusicherung im Protokoll landet und nicht
  # nur im Meldungskasten der Laufzeitbibliothek.
  # MSVC erzeugt selbst ein Manifest und legt es als Ressource 1 ab. Unseres
  # steht schon in res/behaved.rc - dann bricht der Ressourcenwandler ab:
  #     CVT1100: Doppelte Ressource. type:MANIFEST, name:1
  #     LNK1123: Fehler bei der Konvertierung in COFF
  # MinGW macht das nicht, deshalb faellt es erst beim Bau mit Visual Studio
  # auf. /MANIFEST:NO schaltet das erzeugte ab; unseres bleibt und ist das
  # bessere - es nennt die unterstuetzten Windows-Fassungen und die Codepage.
  if(MSVC)
    # target_link_options, nicht set_target_properties(LINK_FLAGS): die
    # Eigenschaft ist eine einzige Zeichenkette, und wer sie spaeter noch
    # einmal setzt, ueberschreibt alles davor. Die /DELAYLOAD-Angaben weiter
    # unten laufen ebenfalls ueber target_link_options und wuerden dabei
    # lautlos verschwinden.
    target_link_options(behaved PRIVATE /MANIFEST:NO)
    # Eine PDB auch im Release (rc596): ohne sie ist ein Absturzabbild
    # (behaved-absturz.dmp, gui/wache.h) kaum zu lesen. Am Code aendert das
    # nichts; /OPT:REF und /OPT:ICF stehen ausdruecklich da, weil /DEBUG sie
    # sonst abschaltet. Die PDB wird nicht ausgeliefert, nur aufbewahrt.
    target_compile_options(behaved PRIVATE $<$<CONFIG:Release>:/Zi>)
    target_compile_options(bhed_core PRIVATE $<$<CONFIG:Release>:/Zi>)
    target_compile_options(imgui PRIVATE $<$<CONFIG:Release>:/Zi>)
    target_link_options(behaved PRIVATE $<$<CONFIG:Release>:/DEBUG /OPT:REF /OPT:ICF>)
    # Der Selbsttest ist eine einzige grosse Datei voller Lambdas - mehr als
    # die 65536 Abschnitte einer normalen Objektdatei (C1128).
    set_source_files_properties(gui/selbsttest.cpp PROPERTIES COMPILE_OPTIONS /bigobj)
  endif()

  target_link_libraries(behaved PRIVATE bhed_core imgui opengl32 gdi32 imm32 dwmapi comdlg32 ole32 shell32 uuid version winmm winhttp ws2_32)
  # Stufe 3: das Build-Menue startet bhed_q3map2.exe neben behaved.exe - mit
  # behaved immer mitbauen (lauf_wp.sh baut nur das Ziel behaved).
  if(BHED_Q3MAP2_ZIEL)
    add_dependencies(behaved ${BHED_Q3MAP2_ZIEL})
  endif()

  if(BEHAVED_D3D11)
    target_compile_definitions(behaved PRIVATE BHED_WITH_D3D11)
    target_link_libraries(behaved PRIVATE d3d11 d3dcompiler)
    if(MSVC)
      target_link_options(behaved PRIVATE
        # dxgi.dll steht bewusst NICHT dabei: wir rufen nichts daraus direkt
        # auf, und MSVC meldet dann LNK4199 "wird ignoriert; keine Importe
        # von dxgi.dll gefunden".
        /DELAYLOAD:d3d11.dll /DELAYLOAD:d3dcompiler_47.dll)
      target_link_libraries(behaved PRIVATE delayimp)
    endif()
  endif()

  if(MSVC)
    target_link_options(behaved PRIVATE /ENTRY:wWinMainCRTStartup)
  else()
    target_link_options(behaved PRIVATE -municode)
  endif()

  add_custom_command(TARGET behaved POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/base
            $<TARGET_FILE_DIR:behaved>/data/base
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/jk2
            $<TARGET_FILE_DIR:behaved>/data/jk2
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${CMAKE_CURRENT_SOURCE_DIR}/data/karte
            $<TARGET_FILE_DIR:behaved>/data/karte)
elseif(WIN32)
  message(STATUS "Oberflaeche wird nicht gebaut: -DIMGUI_DIR=<pfad> setzen")
endif()

# --- Tracy (rc596) ----------------------------------------------------
#
# Zuschaltbarer Profiler: -DBEHAVED_TRACY=ON holt Tracy (BSD-Lizenz) beim
# Konfigurieren und schaltet die Messpunkte aus bhed/messpunkt.h ein. Ohne
# die Option bleibt alles beim Alten.
option(BEHAVED_TRACY "Messpunkte fuer den Profiler Tracy einbauen" OFF)
if(BEHAVED_TRACY)
  include(FetchContent)
  FetchContent_Declare(tracy
    GIT_REPOSITORY https://github.com/wolfpld/tracy.git
    GIT_TAG v0.11.1
    GIT_SHALLOW TRUE)
  FetchContent_MakeAvailable(tracy)
  target_link_libraries(bhed_core PUBLIC TracyClient)
  target_compile_definitions(bhed_core PUBLIC BHED_TRACY TRACY_ENABLE)
endif()

# --- Fuzzing (rc596) ---------------------------------------------------
#
# Ein libFuzzer-Ziel fuer alle Dateileser (tools/fuzzziel.cpp). Nur in einem
# eigenen Build-Ordner, zusammen mit dem AddressSanitizer:
#   cmake -B build-fuzz -DBEHAVED_FUZZ=ON -DBEHAVED_TESTS=OFF
#         -DCMAKE_CXX_FLAGS="/DWIN32 /D_WINDOWS /EHsc /fsanitize=address /Zi"
# Die Leser in bhed_core bekommen die Abdeckungszaehler, damit der Fuzzer
# sieht, welche Wege eine Eingabe neu oeffnet.
option(BEHAVED_FUZZ "libFuzzer-Ziel fuer die Dateileser bauen (MSVC)" OFF)
if(BEHAVED_FUZZ AND MSVC)
  target_compile_options(bhed_core PRIVATE
    /fsanitize-coverage=inline-8bit-counters /fsanitize-coverage=edge
    /fsanitize-coverage=trace-cmp /fsanitize-coverage=trace-div)
  add_executable(fuzzziel tools/fuzzziel.cpp)
  target_link_libraries(fuzzziel PRIVATE bhed_core)
  target_compile_options(fuzzziel PRIVATE /fsanitize=fuzzer)
  target_link_options(fuzzziel PRIVATE /INCREMENTAL:NO)
endif()

# --- Pruefstaende ------------------------------------------------------
option(BEHAVED_TESTS "Pruefstaende bauen" ON)
if(BEHAVED_TESTS)
  enable_testing()
  set(BEHAVED_DATA "${CMAKE_CURRENT_SOURCE_DIR}/data/base")
  set(BEHAVED_BHC  "${BEHAVED_DATA}/behaved.bhc")
  set(BEHAVED_SUPP "${BEHAVED_DATA}/supplement.bhc")
  # Ordner mit .icarus-Skripten. Ohne ihn laufen nur die Tests, die keinen
  # brauchen. Setzen mit:  cmake -DBEHAVED_SCRIPTS=C:/pfad/zu/JAscripts
  set(BEHAVED_SCRIPTS "" CACHE PATH "Ordner mit .icarus-Skripten (JAscripts)")
  set(BEHAVED_IBI "" CACHE FILEPATH "Eine echte .ibi-Datei zum Vergleich")
  set(BEHAVED_IBIDIR "" CACHE PATH "Ordner mit echten .ibi von IBIZE.EXE")
  if(BEHAVED_IBIDIR)
    add_test(NAME engine_liest_auf
             COMMAND enginecheck ${BEHAVED_IBIDIR})
    add_test(NAME ibi_gegen_ibize
             COMMAND ibiround ${BEHAVED_IBIDIR} ${BEHAVED_BHC} ${BEHAVED_DATA} ${BEHAVED_SUPP})
  endif()

  # blockmove, einzelkind, kennungen und tabellen liefen bisher NUR ueber
  # tools/verify.sh. Wer mit CMake baut, hat sie nie gesehen - genau die
  # Sorte Luecke, die tools/verify.sh bei den Quelldateien hatte (rc531).
  foreach(t checks keytest diagtest bsptest bspgeotest mapviewtest imagetest glmtest glatest camtracktest scenetest missiontest pk3test soundtest timelinetest enginecheck screenshot roundtrip checkall edittest edittour treedump fuzz editfuzz ibitest ibiround shaderabgleich)
    add_executable(${t} tests/${t}.cpp)
    target_link_libraries(${t} PRIVATE bhed_core)
  endforeach()

  foreach(t blockmove einzelkind kennungen abweisung sprache makrozeilen rundlauf_kennung tabellen)
    add_executable(${t} tests/${t}.cpp)
    target_link_libraries(${t} PRIVATE bhed_core)
  endforeach()
  # Die Proben des GPU-Wegs und des Gluehens. Sie lagen in tests/, wurden
  # aber nicht gebaut - Aenderungen an Konstantenlagen, Shadertext und
  # Zeichenliste liefen damit ungeprueft durch (rc568, gDeformSpread).
  foreach(t gpuconst gpudraw gpushader gpustate gpucam gpuskin efxtest ersetzen mover arbeiter saberkontakt saberspuren npcklasse kartenpruefung figurstufen grasflaechen)
    add_executable(${t}_probe tests/${t}.cpp)
    target_link_libraries(${t}_probe PRIVATE bhed_core)
    add_test(NAME ${t} COMMAND ${t}_probe)
  endforeach()
  # Uebersetzt jeder erzeugte Shader wirklich? Braucht d3dcompiler (MSVC).
  if(MSVC)
    add_executable(hlslprobe tests/hlslprobe.cpp)
    target_link_libraries(hlslprobe PRIVATE bhed_core d3dcompiler)
    add_test(NAME hlsl COMMAND hlslprobe)
  endif()
  # Alle Modelle und Karten der Spielordner laden und pruefen (lange Probe).
  # Ohne Ordner meldet sie "uebersprungen". Setzen mit:
  #   cmake -DBEHAVED_GAMEDIRS="C:/.../base;C:/.../MD"
  add_executable(meshmasse tests/meshmasse.cpp)
  target_link_libraries(meshmasse PRIVATE bhed_core)
  set(BEHAVED_GAMEDIRS "" CACHE STRING "Spielordner fuer meshmasse, mit ; getrennt")
  if(BEHAVED_GAMEDIRS)
    add_test(NAME netze COMMAND meshmasse ${BEHAVED_GAMEDIRS})
  endif()
  # Der ICARUS-Nachbau (bhed/ablauf.h): Takt, do/dowait, affect, INSERT,
  # Signale, use auf Skriptstarter und Spawner.
  # Waffe und Lichtschwerter (src/ausruestung.cpp): SET_WEAPON, SET_SABER*,
  # Klingen, Farben, Handmodelle. Mit drei Argumenten (.bhc, Kopfdateien,
  # Ordner mit ausgepackten .pk3) zaehlt sie zusaetzlich die echten Skripte.
  add_executable(waffentest tests/waffentest.cpp)
  target_link_libraries(waffentest PRIVATE bhed_core)
  add_test(NAME waffen COMMAND waffentest)
  # Auto-Updater ohne Netz: JSON von GitHub, Fassungen, Zielpfade im .zip.
  add_executable(updatetest tests/updatetest.cpp)
  target_link_libraries(updatetest PRIVATE bhed_core)
  add_test(NAME update COMMAND updatetest)
  # rc601 tour: die Tutorials im Programm ohne Oberflaeche - Kennung <-> Text,
  # JSON, Tourdaten, Zielangaben, Schrittfolge, Fortschritt, Suche. Das
  # Argument ist der Handbuchordner: sind handbuch/<sprache>/touren.json
  # erzeugt (tools/handbuch/gen_handbuch.py), werden sie mitgeprueft.
  add_executable(tourtest tests/tourtest.cpp)
  target_link_libraries(tourtest PRIVATE bhed_core)
  add_test(NAME tour COMMAND tourtest ${CMAKE_CURRENT_SOURCE_DIR}/handbuch)
  # Gegenproben zur Code-Pruefung vom 03.10.2026 (Skriptkern, Kartenleser,
  # Einstellungen, Modell- und Archivleser).
  add_executable(pruefung tests/pruefung.cpp)
  target_link_libraries(pruefung PRIVATE bhed_core)
  if(WIN32)
    target_link_libraries(pruefung PRIVATE psapi)
  endif()
  add_test(NAME pruefung COMMAND pruefung ${BEHAVED_BHC} ${BEHAVED_DATA} ${BEHAVED_SUPP})
  # Das Wegnetz der NPCs: lesen wie die Engine, erzeugen, pruefen (wegpunkte.h).
  add_executable(wegpunkttest tests/wegpunkttest.cpp)
  target_link_libraries(wegpunkttest PRIVATE bhed_core)
  add_test(NAME wegpunkte COMMAND wegpunkttest)
  add_executable(moveranimtest tests/moveranimtest.cpp)
  target_link_libraries(moveranimtest PRIVATE bhed_core)
  add_test(NAME moveranim COMMAND moveranimtest)

  # Eine Mission anlegen und packen (missionbau.h, entdefs.h, writePk3).
  add_executable(missionbautest tests/missionbautest.cpp)
  target_link_libraries(missionbautest PRIVATE bhed_core)
  add_test(NAME missionbau COMMAND missionbautest ${BEHAVED_BHC} ${BEHAVED_DATA} ${BEHAVED_SUPP}
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/md_twj_jedi.ent)
  add_executable(ablauftest tests/ablauftest.cpp)
  target_link_libraries(ablauftest PRIVATE bhed_core)
  add_test(NAME ablauf COMMAND ablauftest)
  # rc601 bewegung: NPCs laufen, drehen sich und fallen wie im Spiel
  # (scene.cpp, ablauf.cpp) - reine Proben, dazu die md_twj-Daten.
  add_executable(bewegungtest tests/bewegungtest.cpp)
  target_link_libraries(bewegungtest PRIVATE bhed_core)
  add_test(NAME bewegung COMMAND bewegungtest
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/intro_jedi.txt
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/md_twj_jedi.ent)
  # rc604 fix604 (JK2 Stufe 3): Laufen wie in Jedi Outcast (NPC_MoveToGoal,
  # NPC_Accelerate, maps/<karte>.nav) - Teil 2 nur mit JK2-Installation.
  add_executable(jk2navtest tests/jk2navtest.cpp)
  target_link_libraries(jk2navtest PRIVATE bhed_core)
  add_test(NAME jk2nav COMMAND jk2navtest)
  # ROFF-Bahnen (bhed/roff.h): kleine Dateien im Speicher, beide Fassungen,
  # Zeitplan, Mover- und Figurenweg, Notizen, kaputte Dateien.
  add_executable(rofftest tests/rofftest.cpp)
  target_link_libraries(rofftest PRIVATE bhed_core)
  add_test(NAME roff COMMAND rofftest)
  # ROFF-Bahnen BAUEN (rc599): schreiben, aus Schluesseln backen, zerlegen,
  # die Markierung im Skript.
  add_executable(roffbautest tests/roffbautest.cpp)
  target_link_libraries(roffbautest PRIVATE bhed_core)
  add_test(NAME roffbau COMMAND roffbautest)
  # Jede .rof der Spielordner lesen und abspielen. Ohne BEHAVED_GAMEDIRS
  # meldet sie "uebersprungen" (wie meshmasse).
  add_executable(roffmasse tests/roffmasse.cpp)
  target_link_libraries(roffmasse PRIVATE bhed_core)
  add_test(NAME roffs COMMAND roffmasse ${BEHAVED_GAMEDIRS})
  add_test(NAME blockbewegung COMMAND blockmove)
  add_test(NAME einzelkind COMMAND einzelkind)
  add_test(NAME kennungen COMMAND kennungen)
  add_test(NAME abweisung COMMAND abweisung)
  add_test(NAME makrozeilen COMMAND makrozeilen)
  add_test(NAME sprache COMMAND sprache)
  add_executable(helferwerte tests/helferwerte.cpp)
  add_test(NAME helferwerte COMMAND helferwerte)
  add_test(NAME rundlauf_kennung COMMAND rundlauf_kennung
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/intro_jedi.txt)
  add_test(NAME tabellen COMMAND tabellen ${BEHAVED_BHC} ${BEHAVED_DATA})

  add_test(NAME tasten COMMAND keytest)
  add_test(NAME protokoll COMMAND diagtest)
  set(BEHAVED_BSP "" CACHE FILEPATH "Eine .bsp zum Pruefen des Entity-Lesers")
  add_test(NAME karte COMMAND bsptest ${BEHAVED_BSP})
  add_test(NAME kartengeometrie COMMAND bspgeotest ${BEHAVED_BSP})
  add_test(NAME kartenansicht COMMAND mapviewtest)
  add_test(NAME bilder COMMAND imagetest)
  set(BEHAVED_GLM "" CACHE FILEPATH "Ein .glm zum Pruefen des Modell-Lesers")
  set(BEHAVED_SKIN "" CACHE FILEPATH "Die zugehoerige .skin")
  set(BEHAVED_GLA "" CACHE FILEPATH "Eine .gla zum Pruefen des Skelett-Lesers")
  add_test(NAME modell COMMAND glmtest ${BEHAVED_GLM} ${BEHAVED_SKIN})
  add_test(NAME skelett COMMAND glatest ${BEHAVED_GLA})
  add_test(NAME mission COMMAND missiontest)
  add_test(NAME szene COMMAND scenetest
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/intro_jedi.txt
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/md_twj_jedi.ent)
  add_test(NAME kamerabahn COMMAND camtracktest
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/intro_jedi.txt
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/cin2_jedi.txt)
  set(BEHAVED_PK3 "" CACHE FILEPATH "Ein .pk3 zum Pruefen des Archivlesers")
  add_test(NAME archiv COMMAND pk3test ${BEHAVED_PK3})
  add_test(NAME klang COMMAND soundtest)
  add_test(NAME zeitleiste COMMAND timelinetest ${BEHAVED_BHC} ${BEHAVED_DATA}
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/intro_jedi.txt
           ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/cin2_jedi.txt)
  add_test(NAME gegenproben COMMAND checks ${BEHAVED_BHC} ${BEHAVED_DATA} ${BEHAVED_SUPP})
  add_test(NAME bildschirmfoto COMMAND screenshot ${BEHAVED_BHC} ${BEHAVED_DATA} ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/intro_jedi.txt
                   ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/gonkability.txt
                   ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/cin2_jedi.txt
                   ${CMAKE_CURRENT_SOURCE_DIR}/data/fixtures/cin3_jedi.txt)
  if(BEHAVED_SCRIPTS)
    # Benannte Ausnahme: in seccam_planted.icarus fehlt in Zeile 55 das
    # Leerzeichen vor der schliessenden Klammer. Das ist Ravens Handarbeit -
    # genau ein Fall unter 30158 Argumentlisten, gemessen. Unser Schreiber
    # ist gleichmaessiger als die Vorlage.
    add_test(NAME rundlauf COMMAND roundtrip ${BEHAVED_SCRIPTS} seccam_planted.icarus)
    add_test(NAME befehlsmodell COMMAND checkall ${BEHAVED_SCRIPTS} ${BEHAVED_BHC} ${BEHAVED_DATA} ${BEHAVED_SUPP})
    add_test(NAME bearbeitungsrundgang COMMAND edittour ${BEHAVED_SCRIPTS} ${BEHAVED_BHC} ${BEHAVED_DATA} ${BEHAVED_SUPP})
    add_test(NAME bearbeiten COMMAND edittest ${BEHAVED_SCRIPTS} ${BEHAVED_BHC} ${BEHAVED_DATA} ${BEHAVED_SUPP})
    add_test(NAME fuzzer   COMMAND fuzz ${BEHAVED_SCRIPTS} ${BEHAVED_BHC} ${BEHAVED_DATA} 5000)
    add_test(NAME fuzzer_bearbeiten COMMAND editfuzz ${BEHAVED_SCRIPTS} ${BEHAVED_BHC} ${BEHAVED_DATA} 300)
    # Die .ibi-Probe laeuft IMMER.
    #
    # Hier stand `if(BEHAVED_IBI)`. Wer die Variable nicht setzte, bekam die
    # Probe schweigend gar nicht - und im Prueflauf sieht das genauso aus
    # wie bestanden. Von rc347 bis rc359 hat sie deshalb nichts geprueft.
    #
    # `-` heisst: die eingebaute start_cin2.IBI nehmen (30 Byte, siehe
    # tests/ibitest.cpp). Ein gesetztes BEHAVED_IBI geht weiterhin vor.
    if(BEHAVED_IBI)
      set(BEHAVED_IBI_ARG ${BEHAVED_IBI})
    else()
      set(BEHAVED_IBI_ARG "-")
    endif()
    add_test(NAME ibi COMMAND ibitest ${BEHAVED_IBI_ARG} ${BEHAVED_SCRIPTS} ${BEHAVED_BHC} ${BEHAVED_DATA} ${BEHAVED_SUPP})
  endif()
endif()

# --- Pruefungen fuer das Release (rc601 release) -------------------------
#
# Was beim Herunterladen zaehlt, lief bisher nirgends als Test: die
# Importpruefung meldete nur in build.bat einen Hinweis, das Zip entstand von
# Hand (siehe .github/workflows/release.yml und tools/release_paket.py).
if(BEHAVED_TESTS)
  # data/base im Programm (bhed/eingebaut.h): Byte fuer Byte gleich, und
  # loadCommandDb zaehlt aus der ausgepackten Kopie dasselbe wie aus data/base.
  add_executable(eingebautedatentest tests/eingebautedatentest.cpp)
  target_link_libraries(eingebautedatentest PRIVATE bhed_core)
  add_test(NAME eingebaute_daten COMMAND eingebautedatentest ${BEHAVED_DATA})

  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    # Was behaved.exe FEST braucht. Nur Windows-DLLs duerfen dort stehen -
    # am rc600 standen die VC-Laufzeit (MSVCP140, VCRUNTIME140 ...) und elf
    # api-ms-win-crt-*.dll darin, die kein Zip mitbrachte.
    if(TARGET behaved)
      add_test(NAME exe_importe
               COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_exe_imports.py
                       $<TARGET_FILE:behaved>)
    endif()
    # Das Werkzeug fuer Zip, Pruefung und Notizen prueft sich selbst: es muss
    # die Fehler eines kaputten Zips finden (zwei oberste Ordner, "..",
    # Rueckstrich, Packverfahren 9, VC-Laufzeit in der .exe, falscher Name).
    add_test(NAME release_paket
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/release_paket.py selbsttest)
    # rc601: jede Handbuchseite in allen vier Sprachen (en, de, zh, ja) - kein
    # Text faellt auf Englisch zurueck, kein Bild mit Oberflaeche fehlt in
    # einer Sprache (Bilder nur, wenn handbuch/bilder vorhanden ist).
    add_test(NAME handbuch_sprachen
             COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/handbuch/gen_handbuch.py --pruefen)
    # Stufe 3 Punkt 1/2: bhed_q3map2.exe bindet nur Windows-DLLs fest (statische
    # Laufzeit wie behaved.exe), und jede Stufe (-bsp, -vis, -light) schreibt,
    # was NRCs q3map2.exe unter denselben Bedingungen schreibt
    # (tools/karte/q3map2_vergleich.py, Erwartungen tests/karte/q3map2).
    if(TARGET bhed_q3map2)
      add_test(NAME q3map2_importe
               COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/check_exe_imports.py
                       $<TARGET_FILE:bhed_q3map2>)
      # Stufe 6 (NRC#130): Pfade mit Umlaut, Leerzeichen, chinesischen und japanischen Zeichen
      # (das q3map2 neben behaved.exe: MSVC oder clang-cl)
      add_test(NAME q3map2_pfade
               COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_pfade.py
                       $<TARGET_FILE_DIR:behaved>/bhed_q3map2.exe)
      # Stufe 6: bekannte q3map2-Fehler (NRC#270, #43, #72, ...) an bhed_q3map2 nachgestellt
      add_test(NAME q3map2_funde
               COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_funde.py
                       $<TARGET_FILE_DIR:behaved>/bhed_q3map2.exe)
      set_tests_properties(q3map2_funde PROPERTIES TIMEOUT 3600)
      # Stufe 6: die Regressionsfaelle aus NRC-PR #316 (regression_tests/auto-pigeon des Forks
      # auto-pigeon/q3map2, nicht im Repo) - mit -DBEHAVED_Q3MAP2_PR316=<Ordner mit cases.json>
      set(BEHAVED_Q3MAP2_PR316 "" CACHE PATH "Ordner regression_tests/auto-pigeon aus NRC-PR #316 (optional)")
      if(BEHAVED_Q3MAP2_PR316)
        add_test(NAME q3map2_pr316
                 COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_pr316.py
                         $<TARGET_FILE_DIR:behaved>/bhed_q3map2.exe ${BEHAVED_Q3MAP2_PR316}
                         --erwartet ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_pr316_erwartet.json)
        set_tests_properties(q3map2_pr316 PROPERTIES TIMEOUT 3600)
      endif()
      # Stufe 6 (GL#173): -bsp/-vis/-light je 3 Laeufe gleich und gleich -threads 1
      add_test(NAME q3map2_determinismus
               COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_determinismus.py ctest
                       $<TARGET_FILE_DIR:behaved>/bhed_q3map2.exe ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2)
      # Stufe 8a Punkt 5: Teil-Kompilierungen Byte fuer Byte gleich der vollen (VIS wiederverwenden, ...)
      add_test(NAME q3map2_teilbau
               COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_teilbau.py
                       $<TARGET_FILE_DIR:behaved>/bhed_q3map2.exe ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2
                       ${CMAKE_CURRENT_BINARY_DIR}/q3map2_teilbau)
      set_tests_properties(q3map2_teilbau PROPERTIES TIMEOUT 1800)
      # Stufe 8a Punkt 5c: Zur fertigen Karte hinzufuegen (-mergebsp -world -fixnames, Pruefungen, Kollision)
      add_test(NAME q3map2_zusammen
               COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_zusammen.py
                       $<TARGET_FILE_DIR:behaved>/bhed_q3map2.exe $<TARGET_FILE:kartezusammen> ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2
                       ${CMAKE_CURRENT_BINARY_DIR}/q3map2_zusammen)
      add_test(NAME q3map2_vergleich
               COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_vergleich.py pruefen
                       $<TARGET_FILE:bhed_q3map2> ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2)
      # Stufe 5 Punkt 6: mit BEHAVED_Q3MAP2_LLVM liegt die clang-cl-q3map2 neben behaved.exe
      if(BHED_Q3MAP2_ZIEL STREQUAL "bhed_q3map2_clang")
        set(BHED_Q3_SOLL clang)
      else()
        set(BHED_Q3_SOLL msvc)
      endif()
      add_test(NAME q3map2_uebersetzer
               COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/q3map2_vergleich.py uebersetzer
                       $<TARGET_FILE:bhed_q3map2> ${BHED_Q3_SOLL})
    endif()
  endif()
endif()

# --- Karten-Editor (Stufe 1a, ab karte1) -------------------------------------
#
# Der neue Tab baut eigene Karten fuer Jedi Academy und Jedi Outcast, Vorlage
# NetRadiant-Custom (Entscheidung shank, 21.10.2026). Der Kern steht ohne
# Oberflaeche in src/karte/ und include/bhed/karte/ (Namensraum bhed::karte),
# 1:1 portiert aus NRC - damit ist das Programm GPL v2 (LICENSE).
target_sources(bhed_core PRIVATE
  src/karte/wicklung.cpp
  src/karte/texdef.cpp
  src/karte/rueckgaengig.cpp
  src/karte/pinsel.cpp
  src/karte/patch.cpp
  src/karte/eklasse.cpp
  src/karte/eklasse_lesen.cpp
  src/karte/karte.cpp
  src/karte/kartenformat.cpp
  src/karte/texturen.cpp
  # Stufe 1b: Kernteile der Oberflaeche (zuletzt geoeffnete Karten, ...)
  src/karte/zuletzt.cpp
  # Stufe 1b: Befehlstabelle, Menues, Werkzeugleisten, Tastenkuerzel wie NRC/Qt
  src/karte/befehle.cpp
  src/karte/raster.cpp
  # Stufe 1b: Platzverteilung wie Qt 5.15 (QSplitter, QStatusBar)
  src/karte/teiler.cpp
  # Stufe 1b: 2D-Ansichten (XYWnd: Raster, Zoom, Kanten, Entities)
  src/karte/xyansicht.cpp
  # Stufe 1b: 3D-Kamera (CamWnd: Bewegung, Freelook, Fokus, Szene)
  src/karte/kamera.cpp
  src/karte/kameraszene.cpp
  # Stufe 1b: Auswahl und Bearbeiten (SelectionSystem, Manipulatoren, select.cpp)
  src/karte/auswahltest.cpp
  src/karte/auswahlsystem.cpp
  src/karte/manipulator.cpp
  src/karte/bearbeitung.cpp
  src/karte/auswahlbefehle.cpp
  src/karte/quickhull.cpp
  # Stufe 2: Komponenten (Ecken, Kanten, Flaechen, Patch-Punkte; brush.h, patch.h)
  src/karte/komponenten.cpp
  # Stufe 2: CSG und Clipper (csg.cpp, clippertool.cpp)
  src/karte/csg.cpp
  src/karte/grundformen.cpp
  src/karte/patchwerkzeuge.cpp
  src/karte/patchbefehle.cpp
  src/karte/texturbefehle.cpp
  src/karte/texturtags.cpp
  src/karte/region.cpp
  src/karte/kartendatei.cpp
  src/karte/filter.cpp
  src/karte/ebenen.cpp
  src/karte/modell.cpp
  src/karte/modellbrowser.cpp
  src/karte/spielbeschreibung.cpp
  # Stufe 1b: Texturbrowser (texwindow.cpp, aktive Shader)
  src/karte/texturbrowser.cpp
  # Stufe 1b: Texturen mit M3 kopieren und auftragen (TexManipulator, surfacedialog.cpp)
  src/karte/texturkopie.cpp
  # Stufe 1b: Entitaeten erzeugen, umwandeln, verbinden, Schluessel (entity.cpp, entityinspector.cpp)
  src/karte/entitaetbefehle.cpp
  # Stufe 3: Build-Menue (build.cpp) und der XML-Leser dafuer und fuer den
  # Rueckkanal von q3map2 (watchbsp.cpp)
  src/karte/xmlstrom.cpp
  src/karte/bau.cpp
  src/karte/baumonitor.cpp
  src/karte/baufortschritt.cpp
  src/karte/editordaten.cpp
  src/karte/modellrueck.cpp
  src/karte/lichtschaetzung.cpp
  src/karte/zusammenfuehren.cpp
  # Stufe 3 Punkt 5: der Portal Viewer (contrib/prtview) ohne Oberflaeche
  src/karte/portale.cpp
  # Stufe 4 Punkte 1/2: fertige Karten oeffnen, Originallicht (-onlyents)
  src/karte/bspoeffnen.cpp
  # Stufe 4 Punkt 3: das Plugin bobToolz (contrib/bobtoolz) ohne Oberflaeche
  src/karte/bobtoolz_d.cpp
  src/karte/bobtoolz.cpp
  src/karte/bobtoolz_vis.cpp
  # Stufe 4 Punkt 3: brushexport2 (contrib/brushexport/export.cpp)
  src/karte/brushexport.cpp
  # Stufe 4 Punkt 3: ShaderPlug, SunPlug, UFO:AI
  src/karte/kleineplugins.cpp
  # Stufe 4 Punkt 3: MeshTex (contrib/meshtex/MeshEntity.cpp)
  src/karte/meshtex.cpp
  # Stufe 8b Punkt 1: Terrain Generator (NRC master, contrib/terrain_generator)
  src/karte/terraingen.cpp
  # Stufe 5 Punkt 1: die Grenzen von JKA / MP / Movie Duels / JK2 als Daten
  src/karte/grenzen.cpp
  # Stufe 5 Punkt 2: der Optimierer zaehlt (cm_patch.cpp fuer die Patch-Kollision)
  src/karte/patchkollision.cpp
  src/karte/optimierer.cpp
  # Stufe 5 Punkt 3: Pruefungen auf Bauprobleme
  src/karte/optimierer_pruefen.cpp
  # Stufe 5 Punkt 4: Beheben auf Knopfdruck (mit Rueckgaengig)
  src/karte/optimierer_beheben.cpp
  # Stufe 7 Punkt 5: Echtzeit-Licht (die Lichter der Karte wie q3map2 -light)
  src/karte/echtlicht.cpp)
if(BEHAVED_TESTS)
  # Punkt 1: LICENSE ist GPL v2, README nennt Lizenz und Herkunft, jede
  # portierte Datei traegt ihren Herkunftskopf.
  add_executable(kartelizenz tests/kartelizenz.cpp)
  add_test(NAME karte_lizenz COMMAND kartelizenz ${CMAKE_CURRENT_SOURCE_DIR})
  # Punkt 2a: Ebenen, Wicklungen, Aufraeumen, Kanten/Ecken, Detail, Undo.
  add_executable(kartegeometrie tests/kartegeometrie.cpp)
  target_link_libraries(kartegeometrie PRIVATE bhed_core)
  add_test(NAME karte_geometrie COMMAND kartegeometrie)
  # Punkt 2b: Texturprojektion AP/BP/Valve, Umrechnung, Texture Lock, Fit.
  add_executable(kartetexdef tests/kartetexdef.cpp)
  target_link_libraries(kartetexdef PRIVATE bhed_core)
  add_test(NAME karte_texdef COMMAND kartetexdef)
  # Punkt 2c: Patches - adaptive und feste Tesselierung, setDims, Undo.
  add_executable(kartepatch tests/kartepatch.cpp)
  target_link_libraries(kartepatch PRIVATE bhed_core)
  add_test(NAME karte_patch COMMAND kartepatch)
  # Punkt 2d: Entitaeten (Schluessel wie EntityKeyValues), Klassen, Baum,
  # worldspawn, Auswahl, Rueckgaengig ueber die ganze Karte.
  add_executable(karteentitaet tests/karteentitaet.cpp)
  target_link_libraries(karteentitaet PRIVATE bhed_core)
  add_test(NAME karte_entitaet COMMAND karteentitaet)
  # Punkt 2e: .map lesen/schreiben wie NRC - bytegleicher Rundlauf fuer
  # NRC-geschriebene Karten (tests/karte), Fehlerverhalten, Zahlenformat.
  # Weitere NRC-Karten und fremde Karten (Ravens JA-Beispiele) optional:
  #   cmake -DBEHAVED_KARTEN_NRC=<Ordner> -DBEHAVED_KARTEN_FREMD=<Ordner>
  set(BEHAVED_KARTEN_NRC "" CACHE PATH "Ordner mit von NetRadiant-Custom geschriebenen .map")
  set(BEHAVED_KARTEN_FREMD "" CACHE PATH "Ordner mit .map in anderem Format (z. B. JA-Beispielkarten)")
  add_executable(karterundlauf tests/karterundlauf.cpp)
  target_link_libraries(karterundlauf PRIVATE bhed_core)
  add_test(NAME karte_rundlauf COMMAND karterundlauf ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte
           "${BEHAVED_KARTEN_NRC}" "${BEHAVED_KARTEN_FREMD}")
  # Punkt 2f: Entitaetsklassen aus .def/.ent (data/karte, aus OpenJK erzeugt).
  # Mit -DBEHAVED_NRC_GAMEPACKS=<NRC>/gamepacks zusaetzlich der Vergleich.
  set(BEHAVED_NRC_GAMEPACKS "" CACHE PATH "gamepacks-Ordner von NetRadiant-Custom (nur Vergleich)")
  add_executable(karteklassen tests/karteklassen.cpp)
  target_link_libraries(karteklassen PRIVATE bhed_core)
  add_test(NAME karte_klassen COMMAND karteklassen ${CMAKE_CURRENT_SOURCE_DIR}/data/karte "${BEHAVED_NRC_GAMEPACKS}")
  # Punkt 2g: Dateisystem und Textur-Inventar. Ohne Spielordner nur der
  # selbst gebaute Spielordner; mit -DBEHAVED_KARTE_JA=<JA base>,
  # -DBEHAVED_KARTE_JK2=<JK2 base> und -DBEHAVED_KARTE_MD="<base>;<MD>"
  # zusaetzlich die Zahlen der echten Daten (nur lesen).
  set(BEHAVED_KARTE_JA "" CACHE PATH "Jedi Academy base (nur lesen)")
  set(BEHAVED_KARTE_JK2 "" CACHE PATH "Jedi Outcast base (nur lesen)")
  set(BEHAVED_KARTE_MD "" CACHE STRING "Movie Duels base;MD (nur lesen)")
  add_executable(kartetexturen tests/kartetexturen.cpp)
  target_link_libraries(kartetexturen PRIVATE bhed_core)
  add_test(NAME karte_texturen COMMAND kartetexturen "${BEHAVED_KARTE_JA}" "${BEHAVED_KARTE_JK2}" ${BEHAVED_KARTE_MD})
  # Punkt 3: Geometrie und st gegen das echte q3map2 (BSPs in tests/karte,
  # erzeugt ohne -meta, siehe tools/gen_karte_vergleich.py).
  add_executable(kartevergleich tests/kartevergleich.cpp)
  target_link_libraries(kartevergleich PRIVATE bhed_core)
  add_test(NAME karte_vergleich COMMAND kartevergleich ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte)
  # Punkt 3: Massenlast - alle mit q3map2 -convert dekompilierten Original-BSPs
  # (<wurzel>/<spiel>/bp und /q3). Ohne -DBEHAVED_KARTE_DEKOMP=<wurzel>
  # uebersprungen; die Karten gehoeren nicht ins Repo.
  set(BEHAVED_KARTE_DEKOMP "" CACHE PATH "Ordner mit dekompilierten Karten (<spiel>/bp, <spiel>/q3)")
  add_executable(kartemassenlast tests/kartemassenlast.cpp)
  target_link_libraries(kartemassenlast PRIVATE bhed_core)
  if(WIN32)
    target_link_libraries(kartemassenlast PRIVATE psapi)
  endif()
  add_test(NAME karte_massenlast COMMAND kartemassenlast "${BEHAVED_KARTE_DEKOMP}")
  # Stufe 1b: was die Oberflaeche aus dem Kern braucht und ohne Fenster
  # pruefbar ist (Punkt 2: zuletzt geoeffnete Karten, "Karte geaendert").
  add_executable(karteeditor tests/karteeditor.cpp)
  target_link_libraries(karteeditor PRIVATE bhed_core)
  add_test(NAME karte_editor COMMAND karteeditor)
  # Feldgroessen wie NRC: Qts QSplitter/QStatusBar-Rechnung gegen die am
  # Foto von NRC gemessenen Bildpunkte.
  add_executable(karteteiler tests/karteteiler.cpp)
  target_link_libraries(karteteiler PRIVATE bhed_core)
  add_test(NAME karte_teiler COMMAND karteteiler)
  # Punkt 5: 2D-Ansichten rechnen wie NRC (Raster, Zoom, Kanten, Entities).
  add_executable(kartexyansicht tests/kartexyansicht.cpp)
  target_link_libraries(kartexyansicht PRIVATE bhed_core)
  add_test(NAME karte_xyansicht COMMAND kartexyansicht)
  # Punkt 6: die 3D-Kamera (Projektion, Freelook, Tasten, Fokus, Etagen, Szene)
  add_executable(kartekamera tests/kartekamera.cpp)
  target_link_libraries(kartekamera PRIVATE bhed_core)
  add_test(NAME karte_kamera COMMAND kartekamera)
  # Punkt 7: Auswaehlen (Treffer, Tunnel, Rechteck, Flaechen, Malen)
  # Stufe 7 Punkt 5: Echtzeit-Licht - Lichter wie q3map2, Formeln von LightContributionToSample
  add_executable(karteechtlicht tests/karteechtlicht.cpp)
  target_link_libraries(karteechtlicht PRIVATE bhed_core)
  add_test(NAME karte_echtlicht COMMAND karteechtlicht)
  add_executable(karteauswahl tests/karteauswahl.cpp)
  target_link_libraries(karteauswahl PRIVATE bhed_core)
  add_test(NAME karte_auswahl COMMAND karteauswahl)
  # Punkt 7: Manipulatoren (Verschieben, Drehen, Skalieren, Transform, Ziehen, neue Pinsel)
  add_executable(kartebearbeitung tests/kartebearbeitung.cpp)
  target_link_libraries(kartebearbeitung PRIVATE bhed_core)
  add_test(NAME karte_bearbeitung COMMAND kartebearbeitung)
  # Punkt 8: Texturbrowser (Lagen, Schalter, Verzeichnisse, Fokus) und Shader-Befehle
  add_executable(kartetexturbrowser tests/kartetexturbrowser.cpp)
  target_link_libraries(kartetexturbrowser PRIVATE bhed_core)
  add_test(NAME karte_texturbrowser COMMAND kartetexturbrowser)
  # Punkt 9: Entitaeten (Inspector, Erzeugen, Klasse wechseln, Verbinden, Farbe, Auswahl)
  add_executable(karteentitaeten tests/karteentitaeten.cpp)
  target_link_libraries(karteentitaeten PRIVATE bhed_core)
  add_test(NAME karte_entitaeten COMMAND karteentitaeten)
  # Stufe 2, Punkt 1: Komponenten (Ecken, Kanten, Flaechen, Patch-Punkte) waehlen,
  # ziehen, einfuegen, loeschen; Texture Vertex Lock; Build-Werkzeug
  add_executable(kartekomponenten tests/kartekomponenten.cpp)
  target_link_libraries(kartekomponenten PRIVATE bhed_core)
  add_test(NAME karte_komponenten COMMAND kartekomponenten)
  # Stufe 2, Punkte 2 und 3: Clipper und CSG
  add_executable(kartecsg tests/kartecsg.cpp)
  target_link_libraries(kartecsg PRIVATE bhed_core)
  add_test(NAME karte_csg COMMAND kartecsg)
  add_executable(kartegrundformen tests/kartegrundformen.cpp)
  target_link_libraries(kartegrundformen PRIVATE bhed_core)
  add_test(NAME karte_grundformen COMMAND kartegrundformen)
  add_executable(kartetransform tests/kartetransform.cpp)
  target_link_libraries(kartetransform PRIVATE bhed_core)
  add_test(NAME karte_transform COMMAND kartetransform)
  add_executable(kartepatchwerkzeuge tests/kartepatchwerkzeuge.cpp)
  target_link_libraries(kartepatchwerkzeuge PRIVATE bhed_core)
  add_test(NAME karte_patchwerkzeuge COMMAND kartepatchwerkzeuge)
  add_executable(kartetexturbefehle tests/kartetexturbefehle.cpp)
  target_link_libraries(kartetexturbefehle PRIVATE bhed_core)
  add_test(NAME karte_texturbefehle COMMAND kartetexturbefehle)
  add_executable(kartetexturtags tests/kartetexturtags.cpp)
  target_link_libraries(kartetexturtags PRIVATE bhed_core)
  add_test(NAME karte_texturtags COMMAND kartetexturtags)
  add_executable(karteregion tests/karteregion.cpp)
  target_link_libraries(karteregion PRIVATE bhed_core)
  add_test(NAME karte_region COMMAND karteregion)
  add_executable(kartefilter tests/kartefilter.cpp)
  target_link_libraries(kartefilter PRIVATE bhed_core)
  add_test(NAME karte_filter COMMAND kartefilter)
  add_executable(karteebenen tests/karteebenen.cpp)
  target_link_libraries(karteebenen PRIVATE bhed_core)
  add_test(NAME karte_ebenen COMMAND karteebenen)
  add_executable(kartemodell tests/kartemodell.cpp)
  target_link_libraries(kartemodell PRIVATE bhed_core)
  add_test(NAME karte_modell COMMAND kartemodell)
  add_executable(kartemodellbrowser tests/kartemodellbrowser.cpp)
  target_link_libraries(kartemodellbrowser PRIVATE bhed_core)
  add_test(NAME karte_modellbrowser COMMAND kartemodellbrowser)
  add_executable(kartespiel tests/kartespiel.cpp)
  target_link_libraries(kartespiel PRIVATE bhed_core)
  add_test(NAME karte_spiel COMMAND kartespiel)
  # Stufe 3, Punkt 0: Textbreiten wie Qt 5 bei NRC (DirectWrite in Entwurfsmassen
  # mit Unterschneidung statt GDI) - Erwartungen aus den Schriftdateien gerechnet.
  if(WIN32)
    add_executable(karteqttext tests/karteqttext.cpp gui/qttext_win32.cpp)
    target_include_directories(karteqttext PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/gui)
    add_test(NAME karte_qttext COMMAND karteqttext)
  endif()
  # Punkte 3 und 4: die Befehlstabelle gegen die Checkliste (jeder Befehl mit
  # NRCs Taste, jeder Menuepunkt und Knopf), Uebersetzungen = NRC-Englisch,
  # Tastenkuerzel wie Qt (Text, possibleKeys, deutsche Tastatur).
  # Stufe 3 Punkt 3: Build-Menue lesen, schreiben, auswerten (build.cpp) und
  # der XML-Leser.
  add_executable(kartebau tests/kartebau.cpp)
  target_link_libraries(kartebau PRIVATE bhed_core)
  add_test(NAME karte_bau COMMAND kartebau ${CMAKE_CURRENT_SOURCE_DIR}/data/karte ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2)
  # Stufe 8a Punkt 3: misc_model aus der Geometrie (Lage-Rechnung, Kandidaten, Dump; mit Argumenten: Messung)
  add_executable(kartemodellrueck tests/kartemodellrueck.cpp)
  target_link_libraries(kartemodellrueck PRIVATE bhed_core)
  add_test(NAME karte_modellrueck COMMAND kartemodellrueck)
  # Stufe 8a Punkt 5c: Zur fertigen Karte hinzufuegen (Pruefungen, Kollision; gerufen von tools/karte/q3map2_zusammen.py)
  add_executable(kartezusammen tests/kartezusammen.cpp)
  target_link_libraries(kartezusammen PRIVATE bhed_core)
  # Stufe 8a Punkt 4: Lichter schaetzen (raum_licht.bsp; mit Argumenten: Messung)
  add_executable(kartelichtschaetzung tests/kartelichtschaetzung.cpp)
  target_link_libraries(kartelichtschaetzung PRIVATE bhed_core)
  add_test(NAME karte_lichtschaetzung COMMAND kartelichtschaetzung ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2)
  # Stufe 3 Punkt 5: Portaldatei lesen, prtview.ini (contrib/prtview)
  add_executable(karteportale tests/karteportale.cpp)
  target_link_libraries(karteportale PRIVATE bhed_core)
  add_test(NAME karte_portale COMMAND karteportale ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2)
  # Stufe 3 Punkt 6: AutoCaulk Selected (map_autocaulk_selected) ohne Oberflaeche
  add_executable(karteautocaulk tests/karteautocaulk.cpp)
  target_link_libraries(karteautocaulk PRIVATE bhed_core)
  add_test(NAME karte_autocaulk COMMAND karteautocaulk)
  # Stufe 4 Punkte 1/2: .bsp oeffnen (q3map2 -convert wie tryDecompile) und
  # das Originallicht behalten (-onlyents: jeder Lump ausser den Entities
  # Byte-gleich); mit -DBEHAVED_KARTE_DEKOMP und den Spielordnern auch je zwei
  # Originalkarten aus Jedi Academy, Jedi Outcast und Movie Duels.
  # Stufe 6 Punkt 5: kaputte .map/.bsp/.prt/.lin - kein Absturz, Festpunkt
  add_executable(kartefuzz tests/kartefuzz.cpp)
  target_link_libraries(kartefuzz PRIVATE bhed_core)
  add_test(NAME karte_fuzz COMMAND kartefuzz ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte 2000)
  add_executable(kartebspoeffnen tests/kartebspoeffnen.cpp)
  target_link_libraries(kartebspoeffnen PRIVATE bhed_core)
  if(TARGET bhed_q3map2)
    add_test(NAME karte_bspoeffnen COMMAND kartebspoeffnen $<TARGET_FILE:bhed_q3map2> ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2
             "${BEHAVED_KARTE_DEKOMP}" "${BEHAVED_KARTE_JA}" "${BEHAVED_KARTE_JK2}" "${BEHAVED_KARTE_MD}")
  else()
    add_test(NAME karte_bspoeffnen COMMAND kartebspoeffnen - ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2)
  endif()
  # Stufe 4 Punkt 3: bobToolz - die Wirkung jedes Befehls (funchandlers-GTK.cpp)
  add_executable(kartebobtoolz tests/kartebobtoolz.cpp)
  target_link_libraries(kartebobtoolz PRIVATE bhed_core)
  add_test(NAME karte_bobtoolz COMMAND kartebobtoolz ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2)
  # Stufe 8b Punkt 1: Terrain Generator gegen NRCs eigenen Code (tests/karte/terrain_nrc,
  # NRC master unveraendert, Radiant-Schnittstellen ueber shim/nrc_shim.h)
  add_executable(karteterrain tests/karteterrain.cpp tests/karte/terrain_nrc/noise.cpp tests/karte/terrain_nrc/terrain_engine.cpp
                 tests/karte/terrain_nrc/brush_builder.cpp)
  target_include_directories(karteterrain PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/terrain_nrc/shim
                             ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/terrain_nrc)
  target_link_libraries(karteterrain PRIVATE bhed_core)
  add_test(NAME karte_terrain COMMAND karteterrain)
  # Stufe 4 Punkt 3: brushexport2 - .obj / .mtl wie export.cpp
  add_executable(kartebrushexport tests/kartebrushexport.cpp)
  target_link_libraries(kartebrushexport PRIVATE bhed_core)
  add_test(NAME karte_brushexport COMMAND kartebrushexport)
  # Stufe 4 Punkt 3: ShaderPlug, SunPlug, UFO:AI
  add_executable(kartekleineplugins tests/kartekleineplugins.cpp)
  target_link_libraries(kartekleineplugins PRIVATE bhed_core)
  add_test(NAME karte_kleineplugins COMMAND kartekleineplugins)
  # Stufe 4 Punkt 3: MeshTex
  add_executable(kartemeshtex tests/kartemeshtex.cpp)
  target_link_libraries(kartemeshtex PRIVATE bhed_core)
  add_test(NAME karte_meshtex COMMAND kartemeshtex)
  # Stufe 5 Punkt 1: die Grenzen-Tabelle gegen die Quelltexte der Engines
  # (Wert und Zeile jedes #define/constexpr, JK2_MODE-Zweige, Pruefstellen).
  # Ohne -DBEHAVED_OPENJK_QUELLEN=<oj> / -DBEHAVED_MD_QUELLEN=<mdgit> werden
  # deren Stellen uebersprungen; q3map2 liegt unter third_party.
  set(BEHAVED_OPENJK_QUELLEN "" CACHE PATH "OpenJK-Quelltext (Ordner mit code/, codemp/, codeJK2/), nur lesen")
  set(BEHAVED_MD_QUELLEN "" CACHE PATH "Movie-Duels-Quelltext (Ordner mit code/), nur lesen")
  add_executable(kartegrenzen tests/kartegrenzen.cpp)
  target_link_libraries(kartegrenzen PRIVATE bhed_core)
  add_test(NAME karte_grenzen COMMAND kartegrenzen "${BEHAVED_OPENJK_QUELLEN}" "${BEHAVED_MD_QUELLEN}"
           ${CMAKE_CURRENT_SOURCE_DIR}/third_party/q3map2 ${CMAKE_CURRENT_SOURCE_DIR}/tools/handbuch/grenzen.json)
  # Stufe 5: der Optimierer - Testkarten an jeder Grenze, erzeugte RBSP, und
  # mit -DBEHAVED_KARTE_JA/JK2/MD der Abgleich gegen alle Originalkarten (nur lesen).
  add_executable(karteoptimierer tests/karteoptimierer.cpp)
  target_link_libraries(karteoptimierer PRIVATE bhed_core)
  add_test(NAME karte_optimierer COMMAND karteoptimierer "${BEHAVED_KARTE_JA}" "${BEHAVED_KARTE_JK2}" "${BEHAVED_KARTE_MD}"
           ${CMAKE_CURRENT_SOURCE_DIR}/tests/karte/q3map2)
  set_tests_properties(karte_optimierer PROPERTIES TIMEOUT 1800)
  add_executable(kartebefehle tests/kartebefehle.cpp)
  target_link_libraries(kartebefehle PRIVATE bhed_core)
  add_test(NAME karte_befehle COMMAND kartebefehle ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/nrc_inventar.md)
  # Stufe 1b, Punkt 1: die Checkliste aller NRC-Menues, Knoepfe, Befehle,
  # Dialoge, Einstellungen und Plugins (tools/karte/nrc_inventar.md) - jeder
  # Eintrag hat einen Stand, ausgegraute nennen ihre Stufe.
  find_package(Python3 COMPONENTS Interpreter)
  if(Python3_Interpreter_FOUND)
    add_test(NAME karte_inventar
             COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/nrc_inventar.py pruefen
                     ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/nrc_inventar.md)
    # Stufe 6, Punkt 1: jeder Eintrag der Checkliste und jeder eigene Eintrag hat
    # eine Wirkungspruefung im Selbsttest (Protokoll des letzten vollen Laufs:
    # tools/karte/abdeckung_lauf.txt); die Tabelle tools/karte/abdeckung.md passt dazu.
    add_test(NAME karte_abdeckung
             COMMAND ${Python3_EXECUTABLE} -I ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/nrc_inventar.py abdeckung
                     ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/nrc_inventar.md ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/abdeckung_lauf.txt
                     --vergleich ${CMAKE_CURRENT_SOURCE_DIR}/tools/karte/abdeckung.md --pruefen)
  endif()
endif()
