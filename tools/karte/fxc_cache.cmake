# fxc_cache.cmake - REL-TEMPO: die vorab uebersetzten GPU-Shader von bhed_q3map2 (src/q3map2/CMakeLists.txt, bhed_fxc) ueber
# einen Cache. Der Bau von bhed_q3map2 verbrachte gemessen 3099 s Rechenzeit in fxc (eine Variante allein 324 s), mit ccache
# nur noch 225 s in clang-cl - fxc ist keine Sache fuer ccache. Gleiche Eingaben -> dieselbe Ausgabe: der Schluessel ist
# SHA-256 ueber fxc.exe (Inhalt), die Schalter und die .hlsl (sie hat keine #include). Ein Treffer kopiert den Header, wie
# ihn fxc bei genau diesen Eingaben geschrieben hat; sonst laeuft fxc und das Ergebnis kommt in den Cache.
#
#   cmake -DFXC=<fxc.exe> -DQUELLE=<.hlsl> -DAUS=<.h> -DSCHALTER_TEXT=<schalter|...> -P fxc_cache.cmake
#
# Der Cache liegt unter $CCACHE_DIR/bhed_fxc (der Runner sichert ihn mit dem ccache-Ordner); ohne CCACHE_DIR laeuft fxc wie
# bisher. Jeder Treffer frischt die Zeit des Eintrags auf (ci.yml raeumt nach dem Bau weg, was nicht gebraucht wurde).
#
#   Copyright (C) 2026 JK Studio contributors. GPL v2 (LICENSE).
foreach(v FXC QUELLE AUS SCHALTER_TEXT)
  if(NOT DEFINED ${v})
    message(FATAL_ERROR "fxc_cache.cmake: ${v} fehlt")
  endif()
endforeach()
# die Schalter kommen mit "|" getrennt (ein ";" teilte CMake schon im Aufruf)
string(REPLACE "|" ";" SCHALTER "${SCHALTER_TEXT}")
get_filename_component(aus_ordner "${AUS}" DIRECTORY)
file(MAKE_DIRECTORY "${aus_ordner}")
set(cache "")
if(DEFINED ENV{CCACHE_DIR} AND NOT "$ENV{CCACHE_DIR}" STREQUAL "")
  file(TO_CMAKE_PATH "$ENV{CCACHE_DIR}/bhed_fxc" cache)
endif()
if(cache)
  file(SHA256 "${FXC}" h_fxc)
  file(SHA256 "${QUELLE}" h_quelle)
  string(SHA256 schluessel "fxc_cache 1|${h_fxc}|${SCHALTER}|${h_quelle}")
  set(eintrag "${cache}/${schluessel}.h")
  if(EXISTS "${eintrag}")
    file(COPY_FILE "${eintrag}" "${AUS}")
    file(TOUCH_NOCREATE "${eintrag}")
    message(STATUS "fxc aus dem Cache: ${AUS}")
    return()
  endif()
endif()
execute_process(COMMAND "${FXC}" ${SCHALTER} /Fh "${AUS}" "${QUELLE}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  file(REMOVE "${AUS}")
  message(FATAL_ERROR "fxc: ${QUELLE} -> ${AUS} fehlgeschlagen (${rc})")
endif()
if(cache)
  file(MAKE_DIRECTORY "${cache}")
  # erst unter einem eigenen Namen, dann umbenennen: zwei Baeue nebeneinander sehen nie eine halbe Datei
  string(RANDOM LENGTH 12 zufall)
  file(COPY_FILE "${AUS}" "${eintrag}.${zufall}")
  file(RENAME "${eintrag}.${zufall}" "${eintrag}")
endif()
