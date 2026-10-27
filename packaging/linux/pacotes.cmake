# Pacote .deb do Nexus Modeler (Debian, Ubuntu e derivados), gerado pelo CPack.
#
# Só entra na configuração com -DNEXUS_PACOTES=ON (veja o CMakeLists.txt de
# topo); a compilação normal não muda. Depois de compilar:
#
#   cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DNEXUS_PACOTES=ON
#   cmake --build build --config Release
#   cpack --config build/CPackConfig.cmake -G DEB -B build/pacotes
#
# Onde cada parte vai parar:
#   /usr/lib/vst3/Nexus Modeler.vst3          plugin VST3
#   /usr/lib/lv2/Nexus Modeler.lv2            plugin LV2
#   /usr/bin/nexus-modeler                    programa independente
#   /usr/share/applications/...desktop        atalho no menu de aplicativos
#   /usr/share/icons/hicolor/1024x1024/apps/  ícone (também em /usr/share/pixmaps)
#
# Os plugins são copiados da pasta que a compilação do JUCE gera. A variável
# NEXUS_ARTEFATOS permite apontar para outra pasta (útil para testar o pacote
# sem compilar tudo de novo).

set(NEXUS_ARTEFATOS "${CMAKE_BINARY_DIR}/plugin/NexusModeler_artefacts/Release"
    CACHE PATH "Pasta com os plugins compilados (VST3, LV2, Standalone)")

# Nome do pacote em minúsculas e sem espaço, como pede o Debian.
set(NEXUS_PACOTE_NOME "nexus-modeler")

# Tudo do Nexus vai no componente "nexus" e o pacote leva só ele: regras de
# instalação de terceiros (bibliotecas baixadas pelo FetchContent, por
# exemplo) ficam de fora.
set(CPACK_INSTALL_CMAKE_PROJECTS "${CMAKE_BINARY_DIR};${PROJECT_NAME};nexus;/")

install(DIRECTORY "${NEXUS_ARTEFATOS}/VST3/Nexus Modeler.vst3"
        DESTINATION lib/vst3
        USE_SOURCE_PERMISSIONS
        COMPONENT nexus)
install(DIRECTORY "${NEXUS_ARTEFATOS}/LV2/Nexus Modeler.lv2"
        DESTINATION lib/lv2
        USE_SOURCE_PERMISSIONS
        COMPONENT nexus)
install(PROGRAMS "${NEXUS_ARTEFATOS}/Standalone/Nexus Modeler"
        DESTINATION bin
        RENAME nexus-modeler
        COMPONENT nexus)
install(FILES "${CMAKE_CURRENT_LIST_DIR}/nexus-modeler.desktop"
        DESTINATION share/applications
        RENAME dev.disouz4.nexusmodeler.desktop
        COMPONENT nexus)
# Ícone do Nexus (assets/icone/nexus_1024.png). Vai para o tema padrão de
# ícones e também para /usr/share/pixmaps, que os menus consultam quando o
# tema não tem a pasta de 1024 pixels.
install(FILES "${PROJECT_SOURCE_DIR}/assets/icone/nexus_1024.png"
        DESTINATION share/icons/hicolor/1024x1024/apps
        RENAME nexus-modeler.png
        COMPONENT nexus)
install(FILES "${PROJECT_SOURCE_DIR}/assets/icone/nexus_1024.png"
        DESTINATION share/pixmaps
        RENAME nexus-modeler.png
        COMPONENT nexus)
# Aviso de direitos no formato do Debian (aponta para a GPL v3 do sistema).
install(FILES "${CMAKE_CURRENT_LIST_DIR}/copyright"
        DESTINATION share/doc/${NEXUS_PACOTE_NOME}
        COMPONENT nexus)

# Versão: a do project() ou a de NEXUS_VERSAO (o fluxo de pacotes passa a
# versão da etiqueta, por exemplo 0.2.0 de v0.2.0).
if(NOT NEXUS_VERSAO)
  set(NEXUS_VERSAO "${PROJECT_VERSION}")
endif()

set(CPACK_PACKAGE_NAME "${NEXUS_PACOTE_NOME}")
set(CPACK_PACKAGE_VERSION "${NEXUS_VERSAO}")
set(CPACK_PACKAGE_VENDOR "Nexus Modeler")
set(CPACK_PACKAGE_CONTACT "Nexus Modeler <https://github.com/disouz4-dev/Nexus-Modeler/issues>")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Plugin de guitarra: a qualidade da captura com a facilidade da modelagem")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/disouz4-dev/Nexus-Modeler")
set(CPACK_RESOURCE_FILE_LICENSE "${PROJECT_SOURCE_DIR}/LICENSE")
set(CPACK_PACKAGING_INSTALL_PREFIX "/usr")
# Os binários vão como a compilação gera: o CPack só tira símbolos de alvos
# instalados com install(TARGETS), e quase todo o tamanho são as imagens
# embutidas, que tirar símbolos não reduz.
set(CPACK_STRIP_FILES OFF)

set(CPACK_DEBIAN_PACKAGE_NAME "${NEXUS_PACOTE_NOME}")
set(CPACK_DEBIAN_PACKAGE_SECTION "sound")
set(CPACK_DEBIAN_PACKAGE_PRIORITY "optional")
set(CPACK_DEBIAN_PACKAGE_ARCHITECTURE "amd64" CACHE STRING "Arquitetura do pacote .deb")
set(CPACK_DEBIAN_PACKAGE_DESCRIPTION
  "Plugin de guitarra gratuito e de código aberto.\n\
VST3, LV2 e programa independente. O Nexus Modeler une a captura (o som\n\
real de um equipamento) com a facilidade de timbragem da modelagem: o\n\
botão troca a captura.")
set(CPACK_DEBIAN_FILE_NAME "NexusModeler-${NEXUS_VERSAO}-Linux-${CPACK_DEBIAN_PACKAGE_ARCHITECTURE}.deb")

# Dependências. A lista fixa cobre também o que o JUCE abre só na hora do uso
# (curl para a TONE3000), que a varredura automática não enxerga. Os pares com
# "|" atendem tanto o Ubuntu 24.04 (nomes com t64) quanto versões anteriores.
set(CPACK_DEBIAN_PACKAGE_DEPENDS
  "libasound2t64 | libasound2, libfreetype6, libfontconfig1, libcurl4t64 | libcurl4 | libcurl3-gnutls, libgl1, libx11-6, libxext6, libxrandr2, libxinerama1, libxcursor1")
# JACK é opcional: o programa independente também toca pelo ALSA. Com o
# PipeWire (Ubuntu, Mint, Fedora), o pipewire-jack dá cada placa pelo nome e a
# latência baixa; o Nexus usa ele sozinho quando está instalado.
set(CPACK_DEBIAN_PACKAGE_RECOMMENDS "pipewire-jack, libjack-jackd2-0 | libjack0")
# Varredura automática das bibliotecas usadas pelos binários (precisa do
# dpkg-shlibdeps; desligue com -DNEXUS_DEB_SHLIBDEPS=OFF fora do Debian).
option(NEXUS_DEB_SHLIBDEPS "Calcular dependências com o dpkg-shlibdeps" ON)
set(CPACK_DEBIAN_PACKAGE_SHLIBDEPS ${NEXUS_DEB_SHLIBDEPS})

set(CPACK_GENERATOR "DEB")
include(CPack)
