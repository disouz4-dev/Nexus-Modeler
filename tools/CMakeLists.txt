find_package(Python3 REQUIRED COMPONENTS Interpreter)

add_executable(nexus_bench nexus_bench.cpp)
target_link_libraries(nexus_bench PRIVATE nexus_engine)
if(MSVC)
  target_compile_options(nexus_bench PRIVATE /utf-8)
endif()

# Pacotes de teste gerados a partir do A2 de exemplo do motor do NAM.
set(NEXUS_TEST_PACKS "${CMAKE_BINARY_DIR}/test_packs")
add_custom_command(
  OUTPUT "${NEXUS_TEST_PACKS}/ganho/pack.json"
  COMMAND Python3::Interpreter "${CMAKE_CURRENT_SOURCE_DIR}/make_test_packs.py"
          "${PROJECT_SOURCE_DIR}/external/NeuralAmpModelerCore/example_models/A2.nam" "${NEXUS_TEST_PACKS}"
  DEPENDS make_test_packs.py
  COMMENT "Gerando pacotes de captura de teste")
add_custom_target(test_packs ALL DEPENDS "${NEXUS_TEST_PACKS}/ganho/pack.json")

add_test(NAME motor_de_troca
         COMMAND nexus_bench "${NEXUS_TEST_PACKS}" "${CMAKE_BINARY_DIR}/medidas"
                 "${PROJECT_SOURCE_DIR}/external/NeuralAmpModelerCore/example_audio/input.wav")
