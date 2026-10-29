include(FetchContent)

# JUCE 9 (plano Starter gratuito). Baixado na primeira configuração.
FetchContent_Declare(JUCE
  GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
  GIT_TAG 9.0.3
  GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Correção no JUCE: o JACK do PipeWire pode informar bloco 0 quando a placa
# está parada no instante em que o áudio religa. O JUCE divide o som em
# pedaços desse tamanho e, com 0, o laço não avança: o áudio fica preso a 100%
# e a tela congela (visto no Linux em 27/10/2026). Com 0, não divide.
set(_nexus_juce_adm "${juce_SOURCE_DIR}/modules/juce_audio_devices/audio_io/juce_AudioDeviceManager.cpp")
file(READ "${_nexus_juce_adm}" _nexus_text)
string(REPLACE "const auto blockLength = jmin (maximumSize, numSamples - position);"
               "const auto blockLength = maximumSize > 0 ? jmin (maximumSize, numSamples - position) : numSamples - position;"
               _nexus_fixed "${_nexus_text}")
if(NOT _nexus_fixed STREQUAL _nexus_text)
  file(WRITE "${_nexus_juce_adm}" "${_nexus_fixed}")
elseif(NOT _nexus_text MATCHES "maximumSize > 0 \\?")
  message(FATAL_ERROR "A correção do bloco 0 no JUCE não se aplicou (o JUCE mudou?)")
endif()

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
  ICON_BIG "${PROJECT_SOURCE_DIR}/assets/icone/nexus_1024.png"
  BUNDLE_ID "dev.disouz4.nexusmodeler"
  PLUGIN_MANUFACTURER_CODE Nxsm
  PLUGIN_CODE Nxm1
  FORMATS ${NEXUS_FORMATS}
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT TRUE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE
  LV2URI "https://github.com/disouz4-dev/Nexus-Modeler"
  VST3_CATEGORIES Fx Distortion
  AU_MAIN_TYPE kAudioUnitType_MusicEffect
  MICROPHONE_PERMISSION_ENABLED TRUE
  MICROPHONE_PERMISSION_TEXT "O Nexus Modeler usa a entrada de áudio para a sua guitarra.")

set(NEXUS_UI_SOURCES
  source/PluginEditor.cpp
  source/ui/AudioPanel.cpp
  source/ui/Language.cpp
  source/ui/Controls.cpp
  source/ui/Gear.cpp
  source/ui/CapturePanel.cpp
  source/ui/GearBrowser.cpp
  source/ui/PresetBar.cpp
  source/ui/UpdateBadge.cpp
  source/ui/WhatsNew.cpp
  source/ui/PackMapper.cpp
  source/ui/TestBar.cpp
  source/ui/StudioView.cpp
  source/ui/EqStage.cpp
  source/ui/Tuner.cpp
  source/ui/IrMic.cpp
  source/update/Updater.cpp
  source/report/Report.cpp
  source/tone3000/Tone3000.cpp)

# Versão que o programa conhece (o atualizador compara com o último lançamento).
# O fluxo de pacotes passa -DNEXUS_VERSAO com a versão da etiqueta.
if(NOT NEXUS_VERSAO)
  set(NEXUS_VERSAO "${PROJECT_VERSION}")
endif()
set_source_files_properties(source/update/Updater.cpp PROPERTIES COMPILE_DEFINITIONS "NEXUS_VERSAO=\"${NEXUS_VERSAO}\"")
set(NEXUS_PROCESSOR_SOURCES
  source/PluginProcessor.cpp
  source/studio/Library.cpp
  source/studio/Player.cpp
  source/fx/PitchDrop.cpp
  source/TestSource.cpp
  source/MidiMap.cpp)
target_sources(NexusModeler PRIVATE
  ${NEXUS_PROCESSOR_SOURCES}
  ${NEXUS_UI_SOURCES})

# Mac: o JUCE gera o ícone só com os tamanhos grandes (512 e 1024) e o Dock e
# o Finder, que usam os pequenos, mostram o ícone em branco. Troca pelo
# assets/icone/nexus.icns completo (16 a 1024).
if(APPLE)
  add_custom_command(TARGET NexusModeler_Standalone POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy "${PROJECT_SOURCE_DIR}/assets/icone/nexus.icns"
            "$<TARGET_BUNDLE_CONTENT_DIR:NexusModeler_Standalone>/Resources/AppIcon.icns"
    COMMAND touch "$<TARGET_BUNDLE_DIR:NexusModeler_Standalone>"
    VERBATIM)
endif()

# Programa independente: janela e painel de áudio próprios do Nexus.
target_sources(NexusModeler_Standalone PRIVATE source/StandaloneApp.cpp)
target_compile_definitions(NexusModeler_Standalone PRIVATE JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)

# Interfaces de áudio de baixa latência no programa independente.
#   Windows: ASIO (kit da Steinberg, licença GPL v3 desde 10/2025), WASAPI e DirectSound.
#   Mac: Core Audio.   Linux: JACK e ALSA.
if(WIN32)
  FetchContent_Declare(asiosdk
    URL https://download.steinberg.net/sdk_downloads/ASIO-SDK_2.3.4_2025-10-15.zip
    URL_HASH SHA256=d5ebf0c20dd2c5f43771fd0c1418f4b361bf52434ee670097cfa6b3a335e2eca)
  FetchContent_MakeAvailable(asiosdk)
  target_include_directories(NexusModeler SYSTEM PRIVATE "${asiosdk_SOURCE_DIR}/common")
  # WinRT MIDI: pedaleiras por Bluetooth (BLE MIDI) no Windows 10 1809 ou mais novo.
  target_compile_definitions(NexusModeler PUBLIC JUCE_ASIO=1 JUCE_WASAPI=1 JUCE_DIRECTSOUND=1 JUCE_USE_WINRT_MIDI=1)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  # Internet (TONE3000) no Linux pelo curl, carregado só quando usado.
target_compile_definitions(NexusModeler PUBLIC JUCE_JACK=1 JUCE_ALSA=1 JUCE_USE_CURL=1 JUCE_LOAD_CURL_SYMBOLS_LAZILY=1)
else()
  target_compile_definitions(NexusModeler PUBLIC JUCE_USE_CURL=0)
endif()

# Imagens dos equipamentos (renderizadas pelo tools/gear_studio) e fontes,
# embutidas no plugin. As fontes ganham nomes simples na cópia.
set(NEXUS_ART_DIR "${CMAKE_CURRENT_BINARY_DIR}/art")
configure_file("${PROJECT_SOURCE_DIR}/assets/fonts/inter-Inter[opsz,wght].ttf" "${NEXUS_ART_DIR}/inter.ttf" COPYONLY)
configure_file("${PROJECT_SOURCE_DIR}/assets/fonts/oswald-Oswald[wght].ttf" "${NEXUS_ART_DIR}/oswald.ttf" COPYONLY)
file(GLOB NEXUS_GEAR_FILES CONFIGURE_DEPENDS "${PROJECT_SOURCE_DIR}/assets/gear/*.png" "${PROJECT_SOURCE_DIR}/assets/gear/*.jpg" "${PROJECT_SOURCE_DIR}/assets/gear/*.json"
     "${PROJECT_SOURCE_DIR}/assets/idiomas/*.txt" "${PROJECT_SOURCE_DIR}/assets/tone3000/*.json"
     "${PROJECT_SOURCE_DIR}/assets/marca/*.svg"
     "${PROJECT_SOURCE_DIR}/assets/teste/*.flac")
# As notas de versão vão junto: depois de atualizar, a tela mostra as novidades.
list(APPEND NEXUS_GEAR_FILES "${PROJECT_SOURCE_DIR}/NOTAS-DE-VERSAO.md")
juce_add_binary_data(NexusArt
  HEADER_NAME BinaryData.h
  NAMESPACE BinaryData
  SOURCES "${NEXUS_ART_DIR}/inter.ttf" "${NEXUS_ART_DIR}/oswald.ttf" ${NEXUS_GEAR_FILES})
set_target_properties(NexusArt PROPERTIES POSITION_INDEPENDENT_CODE ON)

set(NEXUS_RESAMPLER_DIR
  "${PROJECT_SOURCE_DIR}/external/NeuralAmpModelerCore/Dependencies/AudioDSPTools/dsp/ResamplingContainer")
target_include_directories(NexusModeler SYSTEM PRIVATE "${NEXUS_RESAMPLER_DIR}")

# Chave pública do Nexus na TONE3000 (conta do projeto, tone3000.com/settings).
# É pública por natureza: vai dentro do programa. A chave secreta nunca entra
# no projeto.
set(NEXUS_T3K_KEY "t3k_pub_cdG-fr_wYGEPeHbUW8LnKQ5Yl5qoSQL-" CACHE STRING "Chave pública da TONE3000")
target_compile_definitions(NexusModeler PUBLIC
  JUCE_WEB_BROWSER=0
  JUCE_USE_MP3AUDIOFORMAT=1
  JUCE_USE_WINDOWS_MEDIA_FORMAT=1
  NEXUS_T3K_KEY="${NEXUS_T3K_KEY}"
  JUCE_VST3_CAN_REPLACE_VST2=0
  JUCE_DISPLAY_SPLASH_SCREEN=0)

# Estúdio: mudar velocidade e tom das faixas (Signalsmith Stretch, licença MIT).
FetchContent_Declare(signalsmith_stretch
  GIT_REPOSITORY https://github.com/Signalsmith-Audio/signalsmith-stretch.git
  GIT_TAG 88061fd4eda56a2353c5d65c74aa3f4ccf76f9da)
FetchContent_MakeAvailable(signalsmith_stretch)

target_link_libraries(NexusModeler
  PRIVATE
    nexus_engine
    nexus_studio
    signalsmith-stretch
    NexusArt
    juce::juce_audio_utils
    juce::juce_dsp
    juce::juce_cryptography
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
  target_sources(nexus_snapshot PRIVATE host_test/Snapshot.cpp ${NEXUS_PROCESSOR_SOURCES} ${NEXUS_UI_SOURCES})
  target_include_directories(nexus_snapshot SYSTEM PRIVATE "${NEXUS_RESAMPLER_DIR}")
  target_compile_definitions(nexus_snapshot PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 NEXUS_T3K_KEY="${NEXUS_T3K_KEY}")
  target_link_libraries(nexus_snapshot PRIVATE
    nexus_engine
    nexus_studio
    signalsmith-stretch
    NexusArt
    juce::juce_dsp
    juce::juce_audio_utils
    juce::juce_cryptography
    juce::juce_recommended_config_flags)

  juce_add_console_app(nexus_cab_test PRODUCT_NAME "nexus_cab_test")
  target_sources(nexus_cab_test PRIVATE host_test/CabTest.cpp ${NEXUS_PROCESSOR_SOURCES} ${NEXUS_UI_SOURCES})
  target_include_directories(nexus_cab_test SYSTEM PRIVATE "${NEXUS_RESAMPLER_DIR}")
  target_compile_definitions(nexus_cab_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  target_link_libraries(nexus_cab_test PRIVATE
    nexus_engine
    nexus_studio
    signalsmith-stretch
    NexusArt
    juce::juce_dsp
    juce::juce_audio_utils
    juce::juce_cryptography
    juce::juce_recommended_config_flags)
  add_dependencies(nexus_cab_test test_packs)
  add_test(NAME caixa_e_full_rig
           COMMAND nexus_cab_test "${CMAKE_BINARY_DIR}/test_packs" "${CMAKE_BINARY_DIR}/medidas")

  # Teste com interface de áudio real (manual: a compilação automática não tem placa de som).
  juce_add_console_app(nexus_device_test PRODUCT_NAME "nexus_device_test")
  target_sources(nexus_device_test PRIVATE host_test/DeviceTest.cpp ${NEXUS_PROCESSOR_SOURCES} ${NEXUS_UI_SOURCES})
  target_include_directories(nexus_device_test SYSTEM PRIVATE "${NEXUS_RESAMPLER_DIR}")
  target_compile_definitions(nexus_device_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1)
  target_link_libraries(nexus_device_test PRIVATE
    nexus_engine
    nexus_studio
    signalsmith-stretch
    NexusArt
    juce::juce_dsp
    juce::juce_audio_utils
    juce::juce_cryptography
    juce::juce_recommended_config_flags)

  # Teste da TONE3000 na conta de verdade (manual: internet e login no navegador).
  juce_add_console_app(nexus_t3k_test PRODUCT_NAME "nexus_t3k_test")
  target_sources(nexus_t3k_test PRIVATE host_test/Tone3000Test.cpp source/tone3000/Tone3000.cpp)
  target_compile_definitions(nexus_t3k_test PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 NEXUS_T3K_KEY="${NEXUS_T3K_KEY}")
  target_link_libraries(nexus_t3k_test PRIVATE
    nexus_engine
    NexusArt
    juce::juce_events
    juce::juce_data_structures
    juce::juce_cryptography
    juce::juce_recommended_config_flags)

  # Ferramenta manual: passa uma gravação de guitarra pela cadeia com material da TONE3000.
  juce_add_console_app(nexus_render PRODUCT_NAME "nexus_render")
  target_sources(nexus_render PRIVATE host_test/RenderChain.cpp ${NEXUS_PROCESSOR_SOURCES} ${NEXUS_UI_SOURCES})
  target_include_directories(nexus_render SYSTEM PRIVATE "${NEXUS_RESAMPLER_DIR}")
  target_compile_definitions(nexus_render PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 NEXUS_T3K_KEY="${NEXUS_T3K_KEY}")
  target_link_libraries(nexus_render PRIVATE
    nexus_engine
    nexus_studio
    signalsmith-stretch
    NexusArt
    juce::juce_dsp
    juce::juce_audio_utils
    juce::juce_cryptography
    juce::juce_recommended_config_flags)

  # Peso do áudio com pacotes reais (manual): nexus_perfil <pedal> <cabeçote> <ir.wav>
  juce_add_console_app(nexus_perfil PRODUCT_NAME "nexus_perfil")
  target_sources(nexus_perfil PRIVATE host_test/Perfil.cpp ${NEXUS_PROCESSOR_SOURCES} ${NEXUS_UI_SOURCES})
  target_include_directories(nexus_perfil SYSTEM PRIVATE "${NEXUS_RESAMPLER_DIR}")
  target_compile_definitions(nexus_perfil PRIVATE JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_MODAL_LOOPS_PERMITTED=1 NEXUS_T3K_KEY="${NEXUS_T3K_KEY}")
  target_link_libraries(nexus_perfil PRIVATE
    nexus_engine
    nexus_studio
    signalsmith-stretch
    NexusArt
    juce::juce_dsp
    juce::juce_audio_utils
    juce::juce_cryptography
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags)

  add_test(NAME plugin_aberto_no_programa
           COMMAND nexus_host_test "$<GENEX_EVAL:$<TARGET_PROPERTY:NexusModeler_VST3,JUCE_PLUGIN_ARTEFACT_FILE>>" "${CMAKE_BINARY_DIR}/test_packs/ganho")
endif()

# Testes que abrem a tela: captura padrão desligada (não baixam nada da TONE3000).
foreach(teste caixa_e_full_rig plugin_aberto_no_programa)
  if(TEST ${teste})
    set_tests_properties(${teste} PROPERTIES ENVIRONMENT "NEXUS_SEM_PADRAO=1")
  endif()
endforeach()
