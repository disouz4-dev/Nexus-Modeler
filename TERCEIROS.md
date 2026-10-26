# Componentes de terceiros

| Componente | Uso no Nexus Modeler | Licença |
|---|---|---|
| [NeuralAmpModelerCore](https://github.com/sdatkinson/NeuralAmpModelerCore) (Steven Atkinson) | Motor que roda as capturas NAM | MIT |
| [AudioDSPTools](https://github.com/sdatkinson/AudioDSPTools) (Steven Atkinson) | Conversor de taxa de amostragem | MIT; o conversor vem do iPlug2, licença zlib |
| [Eigen](https://eigen.tuxfamily.org) | Contas de matriz dentro do motor do NAM | MPL 2.0 |
| [nlohmann/json](https://github.com/nlohmann/json) | Leitura de arquivos `.nam` e `pack.json` | MIT |
| [JUCE](https://juce.com) | Biblioteca do plugin (VST3, AU, LV2, independente) | AGPLv3 ou licença JUCE (plano Starter gratuito) |
| Modelo `A2.nam` de exemplo do NeuralAmpModelerCore | Base dos pacotes de teste gerados na compilação | MIT |

O texto completo de cada licença acompanha o código de cada componente
(baixado como submódulo ou pelo CMake).

A fórmula do equalizador segue D. T. Yeh e J. O. Smith, "Discretization of
the '59 Fender Bassman Tone Stack", DAFx 2006. As prateleiras e os
equalizadores de faixa seguem o "Audio EQ Cookbook" de R. Bristow-Johnson.
