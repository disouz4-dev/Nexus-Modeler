# Componentes de terceiros

| Componente | Uso no Nexus Modeler | Licença |
|---|---|---|
| [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore) (Steven Atkinson) | Motor que roda as capturas NAM | MIT |
| [AudioDSPTools](https://github.com/sdatkinson/AudioDSPTools) (Steven Atkinson) | Conversor de taxa de amostragem | MIT; o conversor vem do iPlug2, licença zlib |
| [Eigen](https://eigen.tuxfamily.org) | Contas de matriz dentro do motor do NAM | MPL 2.0 |
| [nlohmann/json](https://github.com/nlohmann/json) | Leitura de arquivos `.nam` e `pack.json` | MIT |
| [JUCE](https://juce.com) | Biblioteca do plugin (VST3, AU, LV2, independente) | AGPLv3 ou licença JUCE (plano Starter gratuito) |
| Modelo `A2.nam` de exemplo do NeuralAmpModelerCore | Base dos pacotes de teste gerados na compilação | MIT |
| [Inter](https://github.com/rsms/inter) (Rasmus Andersson) | Letras da tela | SIL Open Font License 1.1 |
| [Oswald](https://github.com/googlefonts/OswaldFont) (Vernon Adams) | Nome na tela e letras impressas nos painéis 3D | SIL Open Font License 1.1 |
| [Yellowtail](https://fonts.google.com/specimen/Yellowtail) (Astigmatic) | Logo em letra cursiva dos equipamentos 3D | Apache 2.0 |
| [ASIO SDK](https://www.steinberg.net/developers/) (Steinberg) | Interfaces de áudio de baixa latência no Windows (baixado na compilação) | GPL v3 (licença dupla desde 10/2025) |
| [tone3000-plugin](https://github.com/tone-3000/tone3000-plugin) (TONE3000) | Base do login na TONE3000 (código do PKCE e do servidor local de retorno), adaptada em `plugin/source/tone3000/` | MIT |
| [TONE3000](https://www.tone3000.com) | Fonte das capturas: o Nexus guarda só o número dos tons recomendados; as capturas são baixadas pela conta de quem usa e continuam sendo dos criadores, com a licença de cada tom | Termos da TONE3000 |
| [Guitar-TECHS](https://guitar-techs.github.io/) (Hegel Pedroza Villalobos, Termeh Taheri, Wallace Abreu, Ryan Corey e Iran R. Roman; [Zenodo](https://zenodo.org/records/14963133)) | Gravações "Limpo", "Blues" e "Rock" do Testar o som (`assets/teste/limpo.flac`, `blues.flac` e `rock.flac`: trechos 09, 10 e 02 da guitarra direta, cortados em compassos inteiros e nivelados) | CC BY 4.0 (atribuição) |
| ["Guitar-Riff-Heavy01-DI"](https://freesound.org/s/531801/) (FrankyFire, Freesound) | Gravação "Pesado" do Testar o som (`assets/teste/pesado.flac`, 20 s do início) | CC BY-NC 4.0 (atribuição, uso não comercial) |
| [Blender](https://www.blender.org) | Ferramenta usada para renderizar os equipamentos (não vai dentro do plugin) | GPL; as imagens geradas são nossas |

O texto completo de cada licença acompanha o código de cada componente
(baixado como submódulo ou pelo CMake). As licenças das fontes estão em
`assets/fonts/`.

Os equipamentos 3D são criados do zero pelo Nexus Gear Studio
(`tools/gear_studio`), com materiais gerados por fórmula. Nenhuma imagem do
ToneX, do AmpliTube ou de fabricantes entra no Nexus: esses produtos servem só
como régua de qualidade.

A fórmula do equalizador segue D. T. Yeh e J. O. Smith, "Discretization of
the '59 Fender Bassman Tone Stack", DAFx 2006. As prateleiras e os
equalizadores de faixa seguem o "Audio EQ Cookbook" de R. Bristow-Johnson.
