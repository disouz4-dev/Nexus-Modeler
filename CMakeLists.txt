cmake_minimum_required(VERSION 3.24)

project(NexusModeler VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
# Símbolos internos escondidos: o NAM guarda uma lista global de tipos de
# captura, e um programa de gravação com dois plugins de NAM abertos (o Nexus e
# o oficial, por exemplo) misturaria as listas e travaria.
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_VISIBILITY_INLINES_HIDDEN ON)

# Os textos da tela estão em português (UTF-8); o Visual Studio precisa saber.
add_compile_options($<$<CXX_COMPILER_ID:MSVC>:/utf-8>)

# Windows: as bibliotecas do Visual Studio vão dentro do programa e do plugin.
# Sem isso, um Windows sem o "Visual C++ Redistributable" instalado não abre
# o Nexus ("MSVCP140.dll não foi encontrado", visto no teste de 27/10/2026).
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Tipo de compilação" FORCE)
endif()

if(APPLE)
  set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "macOS mínimo")
endif()

option(NEXUS_BUILD_PLUGIN "Compilar o plugin (JUCE)" ON)
option(NEXUS_BUILD_TOOLS "Compilar a ferramenta de medida" ON)

add_subdirectory(engine)

if(NEXUS_BUILD_TOOLS)
  enable_testing()
  add_subdirectory(tools)
endif()

if(NEXUS_BUILD_PLUGIN)
  add_subdirectory(plugin)
endif()

# Pacote .deb (Linux). Desligado por padrão: só o fluxo de pacotes liga.
# O .exe (Windows) e o .dmg (Mac) são feitos pelos scripts em packaging/.
option(NEXUS_PACOTES "Preparar o pacote .deb com o CPack (Linux)" OFF)
if(NEXUS_PACOTES AND CMAKE_SYSTEM_NAME STREQUAL "Linux")
  include(packaging/linux/pacotes.cmake)
endif()
