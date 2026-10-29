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
  src/ToneStack.cpp
  src/UniversalEq.cpp)
target_include_directories(nexus_engine PUBLIC include)
# O NAM registra cada tipo de captura num objeto estático; sem "carga
# inteira" o ligador descarta esses registros e nenhuma captura abre.
target_link_libraries(nexus_engine PUBLIC "$<LINK_LIBRARY:WHOLE_ARCHIVE,nam_core>")
if(MSVC)
  target_compile_options(nexus_engine PRIVATE /W4 /utf-8 "$<$<CONFIG:Release>:/O2>")
else()
  target_compile_options(nexus_engine PRIVATE -Wall -Wextra "$<$<CONFIG:Release>:-O3>")
endif()

# Estúdio: separação de faixas pelo demucs.cpp (licença MIT), baixado pelo
# CMake numa versão fixa. Só o código do modelo (src/); o Eigen é o mesmo do
# motor do NAM.
include(FetchContent)
FetchContent_Declare(demucs_cpp
  GIT_REPOSITORY https://github.com/sevagh/demucs.cpp.git
  GIT_TAG f1206e9adeea103aef4a636b9e62297cf1f8e34e
  GIT_SUBMODULES "")
FetchContent_GetProperties(demucs_cpp)
if(NOT demucs_cpp_POPULATED)
  FetchContent_Populate(demucs_cpp)
endif()
file(GLOB DEMUCS_SOURCES "${demucs_cpp_SOURCE_DIR}/src/*.cpp")
add_library(demucs_core STATIC ${DEMUCS_SOURCES})
target_include_directories(demucs_core SYSTEM PUBLIC
  "${demucs_cpp_SOURCE_DIR}/src"
  "${NAM_DIR}/Dependencies/eigen")
target_compile_features(demucs_core PUBLIC cxx_std_17)
target_compile_definitions(demucs_core PUBLIC $<$<CONFIG:Release>:NDEBUG>)
if(MSVC)
  target_compile_definitions(demucs_core PUBLIC NOMINMAX _USE_MATH_DEFINES)
  target_compile_options(demucs_core PRIVATE /bigobj /W0 "$<$<CONFIG:Release>:/O2>")
else()
  target_compile_options(demucs_core PRIVATE -w "$<$<CONFIG:Release>:-O3>")
endif()
option(NEXUS_DEMUCS_BLAS "Contas grandes do separador pelo Accelerate (Mac)" ON)
if(APPLE AND NEXUS_DEMUCS_BLAS)
  target_compile_definitions(demucs_core PUBLIC EIGEN_USE_BLAS)
  target_link_libraries(demucs_core PUBLIC "-framework Accelerate")
endif()

add_library(nexus_studio STATIC src/Separator.cpp)
target_include_directories(nexus_studio PUBLIC include)
target_link_libraries(nexus_studio PUBLIC demucs_core)
if(MSVC)
  target_compile_options(nexus_studio PRIVATE /W4 /utf-8 "$<$<CONFIG:Release>:/O2>")
else()
  target_compile_options(nexus_studio PRIVATE -Wall -Wextra "$<$<CONFIG:Release>:-O3>")
endif()

# Teste dos efeitos nativos novos (wah, rotativa, delay analógico, oitavador e
# compressor): silêncio, valores finitos, desligar sem estalo e a resposta de cada um.
if(NEXUS_BUILD_TOOLS)
  enable_testing()
  add_executable(nexus_fx_test test/NativeFxTest.cpp src/Effects.cpp src/UniversalEq.cpp)
  target_include_directories(nexus_fx_test PRIVATE include)
  add_test(NAME efeitos_nativos COMMAND nexus_fx_test)
endif()
