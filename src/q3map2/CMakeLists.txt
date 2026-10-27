# src/q3map2/CMakeLists.txt - Stufe 3 Punkt 1: q3map2 aus NetRadiant-Custom
# 20260114 als eigenes Programm bhed_q3map2.exe.
#
# Warum ein eigenes Programm und nicht im Prozess: q3map2 haelt globalen Zustand
# (bspfile, Shader, Lichter ...) und beendet sich bei Fehlern mit exit() - der
# Editor startet es wie NRC sein q3map2.exe und liest die Ausgabe.
#
# Die Quellen liegen unveraendert unter third_party (siehe third_party/q3map2/
# README.md): q3map2 und common des Tags 20260114, die noetigen Teile von libs/
# und include/, dazu zlib 1.3.1, libpng 1.6.53, libjpeg-turbo 3.1.3 (ohne SIMD),
# libxml2 2.15.1 und assimp 6.0.2 - die Fassungen, die NRCs q3map2.exe mitbringt.
# Was fuer MSVC anders sein muss, steht hier und in src/q3map2/kompat und ist in
# NOTIZEN_karte3.md aufgezaehlt.
#
#   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).

enable_language(C)

set(TP ${CMAKE_SOURCE_DIR}/third_party)
set(Q3 ${TP}/q3map2)

# Fremdcode mit seiner eigenen Warnstufe: nur Fehler, keine Warnungen.
set(BHED_FREMD_OPTIONEN /W0 /utf-8)
# Mit clang-cl (BEHAVED_Q3MAP2_LLVM, Stufe 5 Punkt 6) haengt libxml2 fuer
# "Clang" GCC-Schalter wie -pedantic an, die clang-cl nicht kennt und nur
# meldet - die Meldung gehoert zum Fremdcode und wird ebenso still.
if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
  list(APPEND BHED_FREMD_OPTIONEN -Wno-unknown-argument)
endif()

# --- Fliesskomma wie GCC -O3 auf x86-64 (NRCs Bau) -----------------------------
#
# NRCs q3map2.exe ist mit MSYS2 GCC -O3 ohne -march gebaut (Makefile:
# BUILD=release): SSE2, keine FMA-Befehle, keine umgeordneten Rechnungen.
# /fp:precise ist MSVCs Vorgabe und haelt dieselben Regeln (keine Kontraktion zu
# FMA, Reihenfolge der Ausdruecke). /arch bleibt bei der x64-Vorgabe SSE2.
# Das Ergebnis des Vergleichs mit NRCs q3map2.exe steht in NOTIZEN_karte3.md.
set(BHED_Q3_FP /fp:precise)
if(BHED_Q3MAP2_FP)
  set(BHED_Q3_FP ${BHED_Q3MAP2_FP})
endif()

# --- zlib 1.3.1 ------------------------------------------------------------------
add_library(bhed_zlib STATIC
  ${TP}/zlib/adler32.c ${TP}/zlib/compress.c ${TP}/zlib/crc32.c ${TP}/zlib/deflate.c ${TP}/zlib/gzclose.c
  ${TP}/zlib/gzlib.c ${TP}/zlib/gzread.c ${TP}/zlib/gzwrite.c ${TP}/zlib/infback.c ${TP}/zlib/inffast.c
  ${TP}/zlib/inflate.c ${TP}/zlib/inftrees.c ${TP}/zlib/trees.c ${TP}/zlib/uncompr.c ${TP}/zlib/zutil.c)
target_include_directories(bhed_zlib PUBLIC ${TP}/zlib)
target_compile_definitions(bhed_zlib PRIVATE _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_DEPRECATE)
target_compile_options(bhed_zlib PRIVATE ${BHED_FREMD_OPTIONEN})

# Fuer assimp und libxml2 (find_package(ZLIB)): unser zlib statt der Suche.
set(ZLIB_FOUND TRUE CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIR ${TP}/zlib CACHE INTERNAL "")
set(ZLIB_INCLUDE_DIRS ${TP}/zlib CACHE INTERNAL "")
set(ZLIB_LIBRARIES bhed_zlib CACHE INTERNAL "")
set(ZLIB_LIBRARY bhed_zlib CACHE INTERNAL "")
if(NOT TARGET ZLIB::ZLIB)
  add_library(ZLIB::ZLIB ALIAS bhed_zlib)
endif()

# --- libpng 1.6.53 ---------------------------------------------------------------
configure_file(${TP}/libpng/scripts/pnglibconf.h.prebuilt ${CMAKE_CURRENT_BINARY_DIR}/png/pnglibconf.h COPYONLY)
add_library(bhed_png STATIC
  ${TP}/libpng/png.c ${TP}/libpng/pngerror.c ${TP}/libpng/pngget.c ${TP}/libpng/pngmem.c ${TP}/libpng/pngpread.c
  ${TP}/libpng/pngread.c ${TP}/libpng/pngrio.c ${TP}/libpng/pngrtran.c ${TP}/libpng/pngrutil.c ${TP}/libpng/pngset.c
  ${TP}/libpng/pngtrans.c ${TP}/libpng/pngwio.c ${TP}/libpng/pngwrite.c ${TP}/libpng/pngwtran.c ${TP}/libpng/pngwutil.c)
target_include_directories(bhed_png PUBLIC ${TP}/libpng ${CMAKE_CURRENT_BINARY_DIR}/png)
target_link_libraries(bhed_png PUBLIC bhed_zlib)
target_compile_definitions(bhed_png PRIVATE _CRT_SECURE_NO_DEPRECATE)
target_compile_options(bhed_png PRIVATE ${BHED_FREMD_OPTIONEN})

# --- libjpeg-turbo 3.1.3 (libjpeg-8.dll von NRC: JPEG8-Schnittstelle, ohne SIMD) ----
#
# Ohne SIMD: die genauen Ganzzahl-Verfahren (islow-IDCT, Farbumrechnung,
# "fancy upsampling") liefern bei libjpeg-turbo mit und ohne SIMD dieselben Bytes
# (die Pruefsummen seiner Tests sind fuer beide gleich); NASM braucht es so nicht.
set(LJT ${TP}/libjpeg-turbo)
set(JPEG_LIB_VERSION 80)
set(VERSION 3.1.3)
set(LIBJPEG_TURBO_VERSION_NUMBER 3001003)
set(C_ARITH_CODING_SUPPORTED 1)
set(D_ARITH_CODING_SUPPORTED 1)
set(BUILD "20251212")
set(HIDDEN "")
set(INLINE "__forceinline")
set(THREAD_LOCAL "__declspec(thread)")
set(CMAKE_PROJECT_NAME_ALT ${CMAKE_PROJECT_NAME})
set(SIZE_T 8)
set(HAVE_INTRIN_H 1)
configure_file(${LJT}/src/jconfig.h.in ${CMAKE_CURRENT_BINARY_DIR}/jpeg/jconfig.h)
set(CMAKE_PROJECT_NAME_SICHER ${CMAKE_PROJECT_NAME})
set(CMAKE_PROJECT_NAME libjpeg-turbo)
configure_file(${LJT}/src/jconfigint.h.in ${CMAKE_CURRENT_BINARY_DIR}/jpeg/jconfigint.h)
set(CMAKE_PROJECT_NAME ${CMAKE_PROJECT_NAME_SICHER})
configure_file(${LJT}/src/jversion.h.in ${CMAKE_CURRENT_BINARY_DIR}/jpeg/jversion.h)
set(LJT_W ${LJT}/src/wrapper)
add_library(bhed_jpeg STATIC
  ${LJT}/src/jcapimin.c
  ${LJT_W}/jcapistd-8.c ${LJT_W}/jcapistd-12.c ${LJT_W}/jcapistd-16.c
  ${LJT_W}/jccoefct-8.c ${LJT_W}/jccoefct-12.c
  ${LJT_W}/jccolor-8.c ${LJT_W}/jccolor-12.c ${LJT_W}/jccolor-16.c
  ${LJT_W}/jcdctmgr-8.c ${LJT_W}/jcdctmgr-12.c
  ${LJT_W}/jcdiffct-8.c ${LJT_W}/jcdiffct-12.c ${LJT_W}/jcdiffct-16.c
  ${LJT}/src/jchuff.c ${LJT}/src/jcicc.c ${LJT}/src/jcinit.c ${LJT}/src/jclhuff.c
  ${LJT_W}/jclossls-8.c ${LJT_W}/jclossls-12.c ${LJT_W}/jclossls-16.c
  ${LJT_W}/jcmainct-8.c ${LJT_W}/jcmainct-12.c ${LJT_W}/jcmainct-16.c
  ${LJT}/src/jcmarker.c ${LJT}/src/jcmaster.c ${LJT}/src/jcomapi.c ${LJT}/src/jcparam.c ${LJT}/src/jcphuff.c
  ${LJT_W}/jcprepct-8.c ${LJT_W}/jcprepct-12.c ${LJT_W}/jcprepct-16.c
  ${LJT_W}/jcsample-8.c ${LJT_W}/jcsample-12.c ${LJT_W}/jcsample-16.c
  ${LJT}/src/jctrans.c ${LJT}/src/jdapimin.c
  ${LJT_W}/jdapistd-8.c ${LJT_W}/jdapistd-12.c ${LJT_W}/jdapistd-16.c
  ${LJT}/src/jdatadst.c ${LJT}/src/jdatasrc.c
  ${LJT_W}/jdcoefct-8.c ${LJT_W}/jdcoefct-12.c
  ${LJT_W}/jdcolor-8.c ${LJT_W}/jdcolor-12.c ${LJT_W}/jdcolor-16.c
  ${LJT_W}/jddctmgr-8.c ${LJT_W}/jddctmgr-12.c
  ${LJT_W}/jddiffct-8.c ${LJT_W}/jddiffct-12.c ${LJT_W}/jddiffct-16.c
  ${LJT}/src/jdhuff.c ${LJT}/src/jdicc.c ${LJT}/src/jdinput.c ${LJT}/src/jdlhuff.c
  ${LJT_W}/jdlossls-8.c ${LJT_W}/jdlossls-12.c ${LJT_W}/jdlossls-16.c
  ${LJT_W}/jdmainct-8.c ${LJT_W}/jdmainct-12.c ${LJT_W}/jdmainct-16.c
  ${LJT}/src/jdmarker.c ${LJT}/src/jdmaster.c
  ${LJT_W}/jdmerge-8.c ${LJT_W}/jdmerge-12.c
  ${LJT}/src/jdphuff.c
  ${LJT_W}/jdpostct-8.c ${LJT_W}/jdpostct-12.c ${LJT_W}/jdpostct-16.c
  ${LJT_W}/jdsample-8.c ${LJT_W}/jdsample-12.c ${LJT_W}/jdsample-16.c
  ${LJT}/src/jdtrans.c ${LJT}/src/jerror.c ${LJT}/src/jfdctflt.c
  ${LJT_W}/jfdctfst-8.c ${LJT_W}/jfdctfst-12.c
  ${LJT_W}/jfdctint-8.c ${LJT_W}/jfdctint-12.c
  ${LJT_W}/jidctflt-8.c ${LJT_W}/jidctflt-12.c
  ${LJT_W}/jidctfst-8.c ${LJT_W}/jidctfst-12.c
  ${LJT_W}/jidctint-8.c ${LJT_W}/jidctint-12.c
  ${LJT_W}/jidctred-8.c ${LJT_W}/jidctred-12.c
  ${LJT}/src/jmemmgr.c ${LJT}/src/jmemnobs.c ${LJT}/src/jpeg_nbits.c
  ${LJT_W}/jquant1-8.c ${LJT_W}/jquant1-12.c
  ${LJT_W}/jquant2-8.c ${LJT_W}/jquant2-12.c
  ${LJT_W}/jutils-8.c ${LJT_W}/jutils-12.c ${LJT_W}/jutils-16.c
  ${LJT}/src/jaricom.c ${LJT}/src/jcarith.c ${LJT}/src/jdarith.c)
target_include_directories(bhed_jpeg PUBLIC ${LJT}/src ${CMAKE_CURRENT_BINARY_DIR}/jpeg)
target_compile_definitions(bhed_jpeg PRIVATE _CRT_SECURE_NO_DEPRECATE)
target_compile_options(bhed_jpeg PRIVATE ${BHED_FREMD_OPTIONEN})

# --- libxml2 2.15.1 (nur fuer den XML-Strom an den Editor) ------------------------
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
foreach(o CATALOG DEBUG HTML HTTP ICONV ICU LEGACY MODULES PROGRAMS PYTHON READLINE TESTS TLS ZLIB LZMA DOCS SCHEMAS
          SCHEMATRON RELAXNG XINCLUDE XPTR C14N ISO8859X)
  set(LIBXML2_WITH_${o} OFF CACHE BOOL "" FORCE)
endforeach()
set(LIBXML2_WITH_THREADS ON CACHE BOOL "" FORCE)
set(LIBXML2_WITH_OUTPUT ON CACHE BOOL "" FORCE)
add_subdirectory(${TP}/libxml2 ${CMAKE_CURRENT_BINARY_DIR}/libxml2 EXCLUDE_FROM_ALL)
target_compile_options(LibXml2 PRIVATE ${BHED_FREMD_OPTIONEN})

# --- assimp 6.0.2 (Modelle fuer misc_model) ----------------------------------------
set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DRACO OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_IGNORE_GIT_HASH ON CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_USE_CCACHE OFF CACHE BOOL "" FORCE)
add_subdirectory(${TP}/assimp ${CMAKE_CURRENT_BINARY_DIR}/assimp EXCLUDE_FROM_ALL)
# llvm-rc (clang-cl-Bau) lehnt das (c) in assimp.rc ab ("Non-ASCII 8-bit
# codepoint", trotz #pragma code_page(1252)). Die Versionsressource landet
# ohnehin nicht in bhed_q3map2.exe (eine .res in einer statischen Bibliothek
# bindet niemand ein) - mit llvm-rc wird sie gar nicht erst uebersetzt.
if(CMAKE_RC_COMPILER MATCHES "llvm-rc")
  set_source_files_properties(${TP}/assimp/code/res/assimp.rc TARGET_DIRECTORY assimp PROPERTIES HEADER_FILE_ONLY ON)
endif()

# --- q3map2 ---------------------------------------------------------------------------
set(Q3C ${Q3}/tools/quake3/common)
set(Q3M ${Q3}/tools/quake3/q3map2)

# Angepasste Kopien: die Quelle von NRC bleibt unveraendert, uebersetzt wird eine
# Kopie im Bauordner mit den Ersetzungen unten (CMake bricht ab, wenn eine Stelle
# sich aendert, und laeuft neu, wenn die Quelle sich aendert).
#
# Nr. 5: MSVC leitet die Vorlagenargumente des Alias-Templates Span
# (libs/generic/arrayrange.h: template<...> using Span = std::span<...>) aus einem
# Behaelter nicht ab (CTAD fuer Alias-Templates ueber den eingeschraenkten
# Ableitungsleitfaden span(R&&) von std::span, C2641); GCC kann es, und Span(
# Zeiger, Anzahl ) klappt auch bei MSVC. An den zwei Stellen mit einem Behaelter
# steht das Argument ausdruecklich.
# Nr. 7: std::sort und std::ranges::sort mit der Reihenfolge von libstdc++
# (kompat/gcc_sort.h) - gleichwertige Elemente stehen danach wie bei NRCs
# q3map2.exe.
function(bhed_q3_kopie ordner datei ergebnis)
  file(READ ${ordner}/${datei} text)
  set(paare ${ARGN})
  list(LENGTH paare n)
  if(n GREATER 0)
    math(EXPR letzte "${n} - 1")
    foreach(i RANGE 0 ${letzte} 2)
      math(EXPR j "${i} + 1")
      list(GET paare ${i} alt)
      list(GET paare ${j} neu)
      string(FIND "${text}" "${alt}" stelle)
      if(stelle EQUAL -1)
        message(FATAL_ERROR "q3map2 ${datei}: Stelle fuer eine Anpassung nicht gefunden: ${alt}")
      endif()
      string(REPLACE "${alt}" "${neu}" text "${text}")
    endforeach()
  endif()
  # Nr. 7 in jeder Kopie
  string(REPLACE "std::ranges::sort(" "bhed_gcc::ranges_sort(" text "${text}")
  string(REPLACE "std::sort(" "bhed_gcc::sort(" text "${text}")
  file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/angepasst/${datei} "${text}")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${ordner}/${datei})
  set(${ergebnis} ${CMAKE_CURRENT_BINARY_DIR}/angepasst/${datei} PARENT_SCOPE)
endfunction()
bhed_q3_kopie(${Q3M} surface.cpp Q3_SURFACE
              "ColorMod( ds.shaderInfo->colorMod, Span( ds.verts ) );" "ColorMod( ds.shaderInfo->colorMod, Span<bspDrawVert_t>( ds.verts ) );")
bhed_q3_kopie(${Q3M} model.cpp Q3_MODEL
              "SnapPlaneImproved( fplane, Span( std::as_const( fw ) ) )"
              "SnapPlaneImproved( fplane, Span<const winding_accu_t::value_type>( std::as_const( fw ) ) )")
bhed_q3_kopie(${Q3M} convert_map.cpp Q3_CONVERT_MAP)
# Nr. 8: entities waechst beim Laden; MSVCs std::vector wuerde die Entities dabei
# kopieren statt verschieben (std::list ist bei MSVC nicht noexcept verschiebbar)
# und brush_t::original ins Leere zeigen lassen (kompat/gcc_vektor.h).
bhed_q3_kopie(${Q3M} map.cpp Q3_MAP
              "entity_t& mapEnt = entities.emplace_back();" "entity_t& mapEnt = bhed_gcc::emplace_back( entities );")
bhed_q3_kopie(${Q3M} lightmaps_ydnar.cpp Q3_LIGHTMAPS)
bhed_q3_kopie(${Q3M} surface_meta.cpp Q3_SURFACE_META)
bhed_q3_kopie(${Q3M} tjunction.cpp Q3_TJUNCTION)
bhed_q3_kopie(${Q3M} vis.cpp Q3_VIS)
bhed_q3_kopie(${Q3C} vfs.cpp Q3_VFS)

add_executable(bhed_q3map2
  ${Q3C}/cmdlib.cpp ${Q3C}/qimagelib.cpp ${Q3C}/inout.cpp ${Q3C}/jpeg.cpp ${Q3C}/md4.cpp ${Q3C}/mutex.cpp
  ${Q3C}/polylib.cpp ${Q3C}/scriplib.cpp ${Q3C}/threads.cpp ${Q3C}/unzip.cpp ${Q3_VFS} ${Q3C}/miniz.cpp
  ${Q3M}/autopk3.cpp ${Q3M}/brush.cpp ${Q3M}/bspfile_abstract.cpp ${Q3M}/bspfile_ibsp.cpp ${Q3M}/bspfile_rbsp.cpp
  ${Q3M}/bsp.cpp ${Q3M}/convert_ase.cpp ${Q3M}/convert_bsp.cpp ${Q3M}/convert_json.cpp ${Q3_CONVERT_MAP}
  ${Q3M}/convert_obj.cpp ${Q3M}/decals.cpp ${Q3M}/exportents.cpp ${Q3M}/facebsp.cpp ${Q3M}/fog.cpp ${Q3M}/games.cpp
  ${Q3M}/help.cpp ${Q3M}/image.cpp ${Q3M}/leakfile.cpp ${Q3M}/light_bounce.cpp ${Q3_LIGHTMAPS}
  ${Q3M}/light.cpp ${Q3M}/light_trace.cpp ${Q3M}/light_ydnar.cpp ${Q3M}/main.cpp ${Q3_MAP} ${Q3M}/minimap.cpp
  ${Q3M}/mesh.cpp ${Q3_MODEL} ${Q3M}/patch.cpp ${Q3M}/path_init.cpp ${Q3M}/portals.cpp ${Q3M}/prtfile.cpp
  ${Q3M}/shaders.cpp ${Q3M}/surface_extra.cpp ${Q3M}/surface_foliage.cpp ${Q3M}/surface_fur.cpp
  ${Q3_SURFACE_META} ${Q3_SURFACE} ${Q3_TJUNCTION} ${Q3M}/tree.cpp ${Q3M}/visflow.cpp ${Q3_VIS}
  ${Q3M}/writebsp.cpp
  # libs aus dem Tag (Makefile: libcrnlib, libddslib, libetclib, libfilematch, libl_net, libwebplib)
  ${Q3}/libs/crnlib/crnlib.cpp ${Q3}/libs/ddslib/ddslib.c ${Q3}/libs/filematch.c ${Q3}/libs/l_net/l_net.c
  ${Q3}/libs/l_net/l_net_wins.c ${Q3}/libs/webplib/webplib.cpp
  # eigene Fassung statt libs/etclib.c (Apache-2.0)
  kompat/etclib.c
  # Stufe 8a: -light -bhedlightcache (Teil-Kompilierung, Nachbearbeitung ohne neu zu tracen)
  bhed_lichtcache.cpp
  # Stufe 8b: -light -bhedgpu (Schattentests auf der GPU, Direct3D 11 zur Laufzeit geladen)
  bhed_gpuschatten.cpp
  ${Q3M}/q3map2.rc
  # Stufe 6 (NRC#130): UTF-8 als Codepage - Pfade ausserhalb von ASCII
  q3map2.manifest)
# kompat zuerst: glib.h und etclib.h von hier
target_include_directories(bhed_q3map2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/kompat ${Q3M} ${Q3C} ${Q3}/libs ${Q3}/include ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(bhed_q3map2 PRIVATE
  WIN32 _WIN32 NDEBUG _inline=inline
  # Nr. 3: windows.h ohne die Makros min/max (inout.cpp ruft std::min/std::max)
  NOMINMAX
  # Dieselben Kennungen wie NRCs q3map2.exe (Tag 20260114 = Commit 68ecbed): sie
  # stehen in jeder .bsp (_q3map2_version) und im Kopf der Ausgabe.
  Q3MAP_VERSION="2.5.17n-git-68ecbed" RADIANT_VERSION="1.6.0n-git-68ecbed"
  RADIANT_MAJOR_VERSION="6" RADIANT_MINOR_VERSION="0"
  _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE _WINSOCK_DEPRECATED_NO_WARNINGS)
set_target_properties(bhed_q3map2 PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)
target_compile_options(bhed_q3map2 PRIVATE ${BHED_FREMD_OPTIONEN} ${BHED_Q3_FP} /permissive- /Zc:__cplusplus
  /FI${CMAKE_CURRENT_SOURCE_DIR}/kompat/msvc_vorab.h)
target_link_libraries(bhed_q3map2 PRIVATE bhed_png bhed_jpeg bhed_zlib LibXml2 assimp ws2_32 user32)
# Nr. 6: libs/crnlib/crnlib.cpp:33 definiert "inline int LittleLong( int )",
# common/cmdlib.cpp:230 dieselbe Funktion ohne inline. Bei GCC ist die inline-
# Fassung ein schwaches Symbol und verschwindet hinter der von cmdlib; MSVC meldet
# LNK2005. In crnlib.cpp bekommt die Funktion darum einen eigenen Namen (sie tut
# dasselbe: Little-Endian ist hier die Byte-Reihenfolge der Maschine).
set_source_files_properties(${Q3}/libs/crnlib/crnlib.cpp PROPERTIES COMPILE_DEFINITIONS "LittleLong=crnlib_LittleLong")
# Stapel 4 MB wie NRC (Makefile: -Wl,--stack,4194304)
target_link_options(bhed_q3map2 PRIVATE /STACK:4194304)
