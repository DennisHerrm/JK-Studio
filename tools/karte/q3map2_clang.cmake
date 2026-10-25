# q3map2_clang.cmake - Stufe 5 Punkt 6: bhed_q3map2.exe mit clang-cl bauen
# (CMakeLists.txt, BEHAVED_Q3MAP2_LLVM). Ein eigener Ninja-Bauordner mit
# clang-cl/lld-link; beim ersten Mal konfigurieren, danach baut Ninja nur, was
# sich geaendert hat (und konfiguriert selbst neu, wenn sich CMakeLists.txt
# aendern). Kein ExternalProject: dessen Stempel-Regeln erzeugen mit dem
# Visual-Studio-Generator die Warnung MSB8064.
#
#   cmake -DQUELLE=<repo> -DBAU=<ordner> -DLLVM=<llvm-bin> -DNINJA=<ninja.exe> -DZIEL=<exe> -P q3map2_clang.cmake
#
#   Copyright (C) 2026 BehavEd-Remake contributors. GPL v2 (LICENSE).
foreach(v QUELLE BAU LLVM NINJA ZIEL)
  if(NOT DEFINED ${v})
    message(FATAL_ERROR "q3map2_clang.cmake: ${v} fehlt")
  endif()
endforeach()

if(NOT EXISTS "${BAU}/build.ninja")
  set(mt "")
  if(EXISTS "${LLVM}/llvm-mt.exe")
    set(mt "-DCMAKE_MT=${LLVM}/llvm-mt.exe")
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -S "${QUELLE}" -B "${BAU}" -G Ninja
            -DCMAKE_BUILD_TYPE=Release
            "-DCMAKE_MAKE_PROGRAM=${NINJA}"
            "-DCMAKE_C_COMPILER=${LLVM}/clang-cl.exe"
            "-DCMAKE_CXX_COMPILER=${LLVM}/clang-cl.exe"
            "-DCMAKE_LINKER=${LLVM}/lld-link.exe"
            "-DCMAKE_AR=${LLVM}/llvm-lib.exe"
            "-DCMAKE_RC_COMPILER=${LLVM}/llvm-rc.exe"
            ${mt}
            -DBEHAVED_TESTS=OFF -DBEHAVED_D3D11=OFF -DBEHAVED_Q3MAP2_LLVM= -DIMGUI_DIR=
    RESULT_VARIABLE rc)
  if(NOT rc EQUAL 0)
    file(REMOVE "${BAU}/build.ninja")
    message(FATAL_ERROR "q3map2 (clang-cl): Konfigurieren fehlgeschlagen (${rc})")
  endif()
endif()

execute_process(COMMAND "${CMAKE_COMMAND}" --build "${BAU}" --target bhed_q3map2 RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "q3map2 (clang-cl): Bauen fehlgeschlagen (${rc})")
endif()
execute_process(COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${BAU}/bin/bhed_q3map2.exe" "${ZIEL}" RESULT_VARIABLE rc)
if(NOT rc EQUAL 0)
  message(FATAL_ERROR "q3map2 (clang-cl): Kopieren nach ${ZIEL} fehlgeschlagen")
endif()
