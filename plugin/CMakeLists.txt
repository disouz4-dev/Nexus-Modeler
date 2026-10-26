include(FetchContent)

# JUCE 9 (plano Starter gratuito). Baixado na primeira configuração.
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 9.0.3
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

set(NEXUS_FORMATS VST3 Standalone)
if(APPLE)
  list(APPEND NEXUS_FORMATS AU)
endif()
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  list(APPEND NEXUS_FORMATS LV2)
endif()

juce_add_plugin(NexusModeler
  PRODUCT_NAME "Nexus Modeler"
  COMPANY_NAME "Nexus Modeler"
  BUNDLE_ID "dev.disouz4.nexusmodeler"
  PLUGIN_MANUFACTURER_CODE Nxsm
  PLUGIN_CODE Nxm1
  FORMATS ${NEXUS_FORMATS}
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  LV2URI "https://github.com/disouz4-dev/Nexus-Modeler"
  VST3_CATEGORIES Fx Distortion
  AU_MAIN_TYPE kAudioUnitType_Effect
  MICROPHONE_PERMISSION_ENABLED TRUE
  MICROPHONE_PERMISSION_TEXT "O Nexus Modeler usa a entrada de áudio para a sua guitarra.")

target_sources(NexusModeler PRIVATE
  source/PluginProcessor.cpp
  source/PluginEditor.cpp)

set(NEXUS_RESAMPLER_DIR
  "${PROJECT_SOURCE_DIR}/external/NeuralAmpModelerCore/Dependencies/AudioDSPTools/dsp/ResamplingContainer")
target_include_directories(NexusModeler SYSTEM PRIVATE "${NEXUS_RESAMPLER_DIR}")

target_compile_definitions(NexusModeler PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_CURL=0
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

target_link_libraries(NexusModeler
  PRIVATE
    nexus_engine
    juce::juce_audio_utils
    juce::juce_dsp
  PUBLIC
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags)

# Teste que abre o plugin compilado como um programa de gravação.
if(NEXUS_BUILD_TOOLS)
  juce_add_console_app(nexus_host_test PRODUCT_NAME "nexus_host_test")
  target_sources(nexus_host_test PRIVATE host_test/HostTest.cpp)
  target_compile_definitions(nexus_host_test PRIVATE
    JUCE_PLUGINHOST_VST3=1
    JUCE_PLUGINHOST_AU=1
    JUCE_PLUGINHOST_LV2=0
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0)
  target_link_libraries(nexus_host_test PRIVATE
    nexus_engine
    juce::juce_audio_utils
    juce::juce_recommended_config_flags)
  add_dependencies(nexus_host_test NexusModeler_VST3)
  juce_add_console_app(nexus_snapshot PRODUCT_NAME "nexus_snapshot")
  target_sources(nexus_snapshot PRIVATE host_test/Snapshot.cpp source/PluginProcessor.cpp source/PluginEditor.cpp)
  target_include_directories(nexus_snapshot SYSTEM PRIVATE "${NEXUS_RESAMPLER_DIR}")
  target_compile_definitions(nexus_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  target_link_libraries(nexus_snapshot PRIVATE
    nexus_engine
    juce::juce_audio_utils
    juce::juce_recommended_config_flags)

  add_test(NAME plugin_aberto_no_programa
           COMMAND nexus_host_test "$<GENEX_EVAL:$<TARGET_PROPERTY:NexusModeler_VST3,JUCE_PLUGIN_ARTEFACT_FILE>>" "${CMAKE_BINARY_DIR}/test_packs/ganho")
endif()
