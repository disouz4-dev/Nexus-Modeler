# Motor do NAM (código de terceiros, licença MIT), compilado como biblioteca.
set(NAM_DIR "${PROJECT_SOURCE_DIR}/external/NeuralAmpModelerCore")
if(NOT EXISTS "${NAM_DIR}/NAM/dsp.h")
  message(FATAL_ERROR "Falta o motor do NAM. Rode: git submodule update --init --recursive")
endif()

file(GLOB NAM_SOURCES "${NAM_DIR}/NAM/*.cpp" "${NAM_DIR}/NAM/*/*.cpp")
add_library(nam_core STATIC ${NAM_SOURCES})
target_include_directories(nam_core SYSTEM PUBLIC
  "${NAM_DIR}"
  "${NAM_DIR}/NAM"
  "${NAM_DIR}/Dependencies/eigen"
  "${NAM_DIR}/Dependencies/nlohmann")
target_compile_definitions(nam_core PUBLIC NAM_ENABLE_A2_FAST)
if(MSVC)
  target_compile_definitions(nam_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN _USE_MATH_DEFINES)
  target_compile_options(nam_core PRIVATE /bigobj /W0 "$<$<CONFIG:Release>:/O2>")
else()
  target_compile_options(nam_core PRIVATE -w "$<$<CONFIG:Release>:-O3>")
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux" AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU"
   AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 9)
  target_link_libraries(nam_core PUBLIC stdc++fs)
endif()

# Motor do Nexus: troca de capturas e equalizadores das famílias.
add_library(nexus_engine STATIC
  src/AmpVoice.cpp
  src/Effects.cpp
  src/MorphEngine.cpp
  src/Pack.cpp
  src/TestSignal.cpp
  src/ToneStack.cpp)
target_include_directories(nexus_engine PUBLIC include)
# O NAM registra cada tipo de captura num objeto estático; sem "carga
# inteira" o ligador descarta esses registros e nenhuma captura abre.
target_link_libraries(nexus_engine PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,nam_core>")
if(MSVC)
  target_compile_options(nexus_engine PRIVATE /W4 /utf-8 "$<$<CONFIG:Release>:/O2>")
else()
  target_compile_options(nexus_engine PRIVATE -Wall -Wextra "$<$<CONFIG:Release>:-O3>")
endif()
