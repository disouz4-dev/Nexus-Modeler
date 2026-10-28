; Instalador do Nexus Modeler para Windows (Inno Setup 6.3 ou mais novo).
;
; Compilar (na raiz do repositório, depois de compilar o Nexus em Release):
;   iscc /DVersao=0.2.0 packaging\windows\nexus.iss
; Opcional: /DArtefatos=<pasta com VST3 e Standalone> e /O<pasta de saída>.
;
; O que instala:
;   VST3                    {autocf}\VST3\Nexus Modeler.vst3
;                           (Arquivos Comuns de 64 bits; para quem instala só
;                           para si, sem administrador, a pasta VST3 do usuário)
;   Programa independente   {autopf}\Nexus Modeler, com atalho no menu Iniciar
;
; Telas: idioma do instalador, licença GPL v3, componentes, atalhos e a página
; "Idioma do Nexus" com os 15 idiomas da tela do Nexus. A escolha vai para
; %APPDATA%\Nexus Modeler\preferencias.settings, o mesmo arquivo que o Nexus
; lê (formato de ajustes do JUCE, em XML). Na instalação silenciosa, use
; /IDIOMA=pt-BR (ou outro código) para escolher o idioma da tela.
;
; Este arquivo precisa ficar em UTF-8 com BOM (marca de início): os nomes dos
; idiomas usam vários alfabetos.

#ifndef Versao
  #define Versao "0.1.0"
#endif
; Versão só com números para os dados do arquivo ("0.1.0-beta.1" vira 0.1.0).
#ifndef VersaoNum
  #define VersaoNum Versao
#endif
#ifndef Artefatos
  #define Artefatos SourcePath + "..\..\build\plugin\NexusModeler_artefacts\Release"
#endif

[Setup]
AppId={{D5358E4A-111B-48B4-8A2E-9AED02B79D1F}
AppName=Nexus Modeler
AppVersion={#Versao}
AppVerName=Nexus Modeler {#Versao}
AppPublisher=Nexus Modeler
AppPublisherURL=https://github.com/disouz4-dev/Nexus-Modeler
AppSupportURL=https://github.com/disouz4-dev/Nexus-Modeler/issues
AppUpdatesURL=https://github.com/disouz4-dev/Nexus-Modeler/releases
DefaultDirName={autopf}\Nexus Modeler
DefaultGroupName=Nexus Modeler
DisableProgramGroupPage=yes
LicenseFile={#SourcePath}..\..\LICENSE
OutputDir={#SourcePath}..\..\build\pacotes
OutputBaseFilename=NexusModeler-{#Versao}-Windows-instalador
Compression=lzma2/max
SolidCompression=yes
WizardStyle=modern
; Ícone do instalador (gerado de assets/icone/nexus_1024.png por gerar_ico.sh).
SetupIconFile={#SourcePath}nexus.ico
; Plugin de 64 bits: instala na pasta de 64 bits dos Arquivos Comuns e Programas.
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
MinVersion=10.0
; Com administrador: para todos os usuários. Sem administrador (escolha na
; primeira tela): só para quem está instalando, nas pastas do próprio usuário.
PrivilegesRequired=admin
PrivilegesRequiredOverridesAllowed=dialog commandline
; Tela de escolha do idioma do instalador, já marcando o idioma do Windows.
ShowLanguageDialog=yes
LanguageDetectionMethod=uilanguage
UninstallDisplayIcon={app}\Nexus Modeler.exe
UninstallDisplayName=Nexus Modeler
VersionInfoVersion={#VersaoNum}
VersionInfoDescription=Instalador do Nexus Modeler
; O arquivo de idioma fica no perfil de quem instala, de propósito.
UsedUserAreasWarning=no

[Languages]
; Traduções oficiais que vêm com o Inno Setup 6.
Name: "english"; MessagesFile: "compiler:Default.isl"
Name: "brazilianportuguese"; MessagesFile: "compiler:Languages\BrazilianPortuguese.isl"
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "french"; MessagesFile: "compiler:Languages\French.isl"
Name: "italian"; MessagesFile: "compiler:Languages\Italian.isl"
Name: "german"; MessagesFile: "compiler:Languages\German.isl"
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"
Name: "russian"; MessagesFile: "compiler:Languages\Russian.isl"
Name: "turkish"; MessagesFile: "compiler:Languages\Turkish.isl"
; Estas só entram se a versão instalada do Inno Setup trouxer a tradução.
; Sem ela, o instalador aparece em inglês, mas a página "Idioma do Nexus"
; continua oferecendo os 15 idiomas.
#if FileExists(CompilerPath + "Languages/Korean.isl")
Name: "korean"; MessagesFile: "compiler:Languages\Korean.isl"
#endif
#if FileExists(CompilerPath + "Languages/ChineseSimplified.isl")
Name: "chinesesimplified"; MessagesFile: "compiler:Languages\ChineseSimplified.isl"
#endif
#if FileExists(CompilerPath + "Languages/Arabic.isl")
Name: "arabic"; MessagesFile: "compiler:Languages\Arabic.isl"
#endif
#if FileExists(CompilerPath + "Languages/Hindi.isl")
Name: "hindi"; MessagesFile: "compiler:Languages\Hindi.isl"
#endif
#if FileExists(CompilerPath + "Languages/Bengali.isl")
Name: "bengali"; MessagesFile: "compiler:Languages\Bengali.isl"
#endif
#if FileExists(CompilerPath + "Languages/Indonesian.isl")
Name: "indonesian"; MessagesFile: "compiler:Languages\Indonesian.isl"
#endif

[CustomMessages]
; Sem prefixo: inglês, usado por todos os idiomas sem tradução própria abaixo.
IdiomaTitulo=Nexus language
IdiomaDescricao=Language of the Nexus Modeler screen
IdiomaTexto=Choose the language of the Nexus screen. You can change it later with the globe on the Nexus screen.
TipoCompleta=Full installation
TipoPersonalizada=Custom installation
ComponenteVST3=VST3 plugin (Reaper, Cubase, Ableton Live, Studio One, FL Studio...)
ComponentePrograma=Standalone app (play without recording software)
brazilianportuguese.IdiomaTitulo=Idioma do Nexus
brazilianportuguese.IdiomaDescricao=Idioma da tela do Nexus Modeler
brazilianportuguese.IdiomaTexto=Escolha o idioma da tela do Nexus. Dá para trocar depois pelo globo na tela do Nexus.
brazilianportuguese.TipoCompleta=Instalação completa
brazilianportuguese.TipoPersonalizada=Instalação personalizada
brazilianportuguese.ComponenteVST3=Plugin VST3 (Reaper, Cubase, Ableton Live, Studio One, FL Studio...)
brazilianportuguese.ComponentePrograma=Programa independente (tocar sem programa de gravação)
spanish.IdiomaTitulo=Idioma de Nexus
spanish.IdiomaDescricao=Idioma de la pantalla de Nexus Modeler
spanish.IdiomaTexto=Elige el idioma de la pantalla de Nexus. Puedes cambiarlo después con el globo en la pantalla de Nexus.
spanish.TipoCompleta=Instalación completa
spanish.TipoPersonalizada=Instalación personalizada
spanish.ComponenteVST3=Plugin VST3 (Reaper, Cubase, Ableton Live, Studio One, FL Studio...)
spanish.ComponentePrograma=Programa independiente (tocar sin software de grabación)
french.IdiomaTitulo=Langue de Nexus
french.IdiomaDescricao=Langue de l'écran de Nexus Modeler
french.IdiomaTexto=Choisissez la langue de l'écran de Nexus. Vous pourrez la changer plus tard avec le globe sur l'écran de Nexus.
french.TipoCompleta=Installation complète
french.TipoPersonalizada=Installation personnalisée
french.ComponenteVST3=Plugin VST3 (Reaper, Cubase, Ableton Live, Studio One, FL Studio...)
french.ComponentePrograma=Application autonome (jouer sans logiciel d'enregistrement)
italian.IdiomaTitulo=Lingua di Nexus
italian.IdiomaDescricao=Lingua dello schermo di Nexus Modeler
italian.IdiomaTexto=Scegli la lingua dello schermo di Nexus. Puoi cambiarla in seguito con il globo sullo schermo di Nexus.
italian.TipoCompleta=Installazione completa
italian.TipoPersonalizada=Installazione personalizzata
italian.ComponenteVST3=Plugin VST3 (Reaper, Cubase, Ableton Live, Studio One, FL Studio...)
italian.ComponentePrograma=Programma autonomo (suonare senza software di registrazione)
german.IdiomaTitulo=Nexus-Sprache
german.IdiomaDescricao=Sprache der Nexus-Modeler-Oberfläche
german.IdiomaTexto=Wähle die Sprache der Nexus-Oberfläche. Du kannst sie später mit dem Globus in der Nexus-Oberfläche ändern.
german.TipoCompleta=Vollständige Installation
german.TipoPersonalizada=Benutzerdefinierte Installation
german.ComponenteVST3=VST3-Plugin (Reaper, Cubase, Ableton Live, Studio One, FL Studio...)
german.ComponentePrograma=Eigenständiges Programm (spielen ohne Aufnahmesoftware)
japanese.IdiomaTitulo=Nexus の言語
japanese.IdiomaDescricao=Nexus Modeler の画面の言語
japanese.IdiomaTexto=Nexus の画面の言語を選んでください。あとで Nexus の画面の地球儀ボタンから変更できます。
japanese.TipoCompleta=フルインストール
japanese.TipoPersonalizada=カスタムインストール
japanese.ComponenteVST3=VST3 プラグイン (Reaper、Cubase、Ableton Live、Studio One、FL Studio など)
japanese.ComponentePrograma=スタンドアロンアプリ (録音ソフトなしで演奏)
russian.IdiomaTitulo=Язык Nexus
russian.IdiomaDescricao=Язык интерфейса Nexus Modeler
russian.IdiomaTexto=Выберите язык интерфейса Nexus. Его можно сменить позже кнопкой с глобусом в окне Nexus.
russian.TipoCompleta=Полная установка
russian.TipoPersonalizada=Выборочная установка
russian.ComponenteVST3=Плагин VST3 (Reaper, Cubase, Ableton Live, Studio One, FL Studio...)
russian.ComponentePrograma=Отдельная программа (игра без программы записи)
turkish.IdiomaTitulo=Nexus dili
turkish.IdiomaDescricao=Nexus Modeler ekranının dili
turkish.IdiomaTexto=Nexus ekranının dilini seçin. Daha sonra Nexus ekranındaki küre simgesiyle değiştirebilirsiniz.
turkish.TipoCompleta=Tam kurulum
turkish.TipoPersonalizada=Özel kurulum
turkish.ComponenteVST3=VST3 eklentisi (Reaper, Cubase, Ableton Live, Studio One, FL Studio...)
turkish.ComponentePrograma=Bağımsız program (kayıt yazılımı olmadan çalın)
#if FileExists(CompilerPath + "Languages/Korean.isl")
korean.IdiomaTitulo=Nexus 언어
korean.IdiomaDescricao=Nexus Modeler 화면 언어
korean.IdiomaTexto=Nexus 화면 언어를 선택하세요. 나중에 Nexus 화면의 지구본 버튼으로 바꿀 수 있습니다.
korean.TipoCompleta=전체 설치
korean.TipoPersonalizada=사용자 지정 설치
korean.ComponenteVST3=VST3 플러그인 (Reaper, Cubase, Ableton Live, Studio One, FL Studio 등)
korean.ComponentePrograma=독립 실행형 프로그램 (녹음 프로그램 없이 연주)
#endif
#if FileExists(CompilerPath + "Languages/ChineseSimplified.isl")
chinesesimplified.IdiomaTitulo=Nexus 语言
chinesesimplified.IdiomaDescricao=Nexus Modeler 界面语言
chinesesimplified.IdiomaTexto=请选择 Nexus 界面的语言。之后可以通过 Nexus 界面上的地球图标更改。
chinesesimplified.TipoCompleta=完整安装
chinesesimplified.TipoPersonalizada=自定义安装
chinesesimplified.ComponenteVST3=VST3 插件 (Reaper、Cubase、Ableton Live、Studio One、FL Studio 等)
chinesesimplified.ComponentePrograma=独立程序 (无需录音软件即可演奏)
#endif

[Types]
Name: "completa"; Description: "{cm:TipoCompleta}"
Name: "personalizada"; Description: "{cm:TipoPersonalizada}"; Flags: iscustom

[Components]
Name: "vst3"; Description: "{cm:ComponenteVST3}"; Types: completa personalizada
Name: "programa"; Description: "{cm:ComponentePrograma}"; Types: completa personalizada

[Tasks]
Name: "atalhomesa"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"; Components: programa; Flags: unchecked

[Files]
Source: "{#Artefatos}\VST3\Nexus Modeler.vst3\*"; DestDir: "{autocf}\VST3\Nexus Modeler.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefatos}\Standalone\Nexus Modeler.exe"; DestDir: "{app}"; Components: programa; Flags: ignoreversion
Source: "{#SourcePath}..\..\LICENSE"; DestDir: "{app}"; DestName: "LICENSE.txt"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\Nexus Modeler"; Filename: "{app}\Nexus Modeler.exe"; Components: programa
Name: "{autodesktop}\Nexus Modeler"; Filename: "{app}\Nexus Modeler.exe"; Components: programa; Tasks: atalhomesa

[Run]
Filename: "{app}\Nexus Modeler.exe"; Description: "{cm:LaunchProgram,Nexus Modeler}"; Components: programa; Flags: nowait postinstall skipifsilent
; Atualização pelo próprio Nexus (instalação silenciosa com /REABRIR): abre o
; Nexus de novo no fim, como a pessoa usava.
Filename: "{app}\Nexus Modeler.exe"; Components: programa; Flags: nowait runasoriginaluser; Check: Reabrir

[UninstallDelete]
; A pasta do plugin some inteira na desinstalação. As preferências do usuário
; (%APPDATA%\Nexus Modeler) ficam, para uma reinstalação lembrar o idioma.
Type: filesandordirs; Name: "{autocf}\VST3\Nexus Modeler.vst3"

[Code]
const
  QtdIdiomas = 15;

var
  PaginaIdioma: TInputOptionWizardPage;

{ O Nexus pediu para ser aberto de novo depois da atualização silenciosa. }
function Reabrir: Boolean;
begin
  Result := WizardSilent and (Pos('/REABRIR', Uppercase(GetCmdTail)) > 0);
end;

{ Código de cada idioma, na mesma ordem e com os mesmos códigos de
  plugin/source/ui/Language.cpp. }
function CodigoIdioma(Indice: Integer): String;
begin
  case Indice of
    0: Result := 'pt-BR';
    1: Result := 'en';
    2: Result := 'es';
    3: Result := 'fr';
    4: Result := 'it';
    5: Result := 'de';
    6: Result := 'ja';
    7: Result := 'zh';
    8: Result := 'ko';
    9: Result := 'hi';
    10: Result := 'ar';
    11: Result := 'bn';
    12: Result := 'ru';
    13: Result := 'id';
    14: Result := 'tr';
  else
    Result := 'en';
  end;
end;

{ Nome de cada idioma escrito no próprio idioma, como no globo do Nexus. }
function NomeIdioma(Indice: Integer): String;
begin
  case Indice of
    0: Result := 'Português (Brasil)';
    1: Result := 'English';
    2: Result := 'Español';
    3: Result := 'Français';
    4: Result := 'Italiano';
    5: Result := 'Deutsch';
    6: Result := '日本語';
    7: Result := '中文（简体）';
    8: Result := '한국어';
    9: Result := 'हिन्दी';
    10: Result := 'العربية';
    11: Result := 'বাংলা';
    12: Result := 'Русский';
    13: Result := 'Bahasa Indonesia';
    14: Result := 'Türkçe';
  else
    Result := 'English';
  end;
end;

function IndiceDoCodigo(Codigo: String): Integer;
var
  I: Integer;
begin
  Result := -1;
  for I := 0 to QtdIdiomas - 1 do
    if CompareText(CodigoIdioma(I), Codigo) = 0 then
    begin
      Result := I;
      Exit;
    end;
end;

{ Idioma do Nexus correspondente ao idioma principal do Windows
  (identificadores de idioma da Microsoft). -1 se o Nexus não tiver. }
function IndiceDoWindows: Integer;
begin
  case GetUILanguage and $3FF of
    $16: Result := 0;   { português }
    $09: Result := 1;   { inglês }
    $0A: Result := 2;   { espanhol }
    $0C: Result := 3;   { francês }
    $10: Result := 4;   { italiano }
    $07: Result := 5;   { alemão }
    $11: Result := 6;   { japonês }
    $04: Result := 7;   { chinês }
    $12: Result := 8;   { coreano }
    $39: Result := 9;   { híndi }
    $01: Result := 10;  { árabe }
    $45: Result := 11;  { bengali }
    $19: Result := 12;  { russo }
    $21: Result := 13;  { indonésio }
    $1F: Result := 14;  { turco }
  else
    Result := -1;
  end;
end;

{ Idioma pré-selecionado: o mesmo do instalador. Quando o instalador está em
  inglês porque o Inno Setup não tem tradução para o idioma do Windows (híndi,
  bengali, árabe, indonésio...), usa o idioma do Windows. }
function IdiomaInicial: Integer;
var
  Instalador: String;
begin
  Instalador := ActiveLanguage;
  if Instalador = 'brazilianportuguese' then Result := 0
  else if Instalador = 'spanish' then Result := 2
  else if Instalador = 'french' then Result := 3
  else if Instalador = 'italian' then Result := 4
  else if Instalador = 'german' then Result := 5
  else if Instalador = 'japanese' then Result := 6
  else if Instalador = 'chinesesimplified' then Result := 7
  else if Instalador = 'korean' then Result := 8
  else if Instalador = 'hindi' then Result := 9
  else if Instalador = 'arabic' then Result := 10
  else if Instalador = 'bengali' then Result := 11
  else if Instalador = 'russian' then Result := 12
  else if Instalador = 'indonesian' then Result := 13
  else if Instalador = 'turkish' then Result := 14
  else
  begin
    Result := IndiceDoWindows;
    { Windows num idioma que o instalador já traduz e a pessoa escolheu inglês:
      respeita o inglês. }
    if (Result < 0) or (Result = 0) or (Result = 2) or (Result = 3) or
       (Result = 4) or (Result = 5) or (Result = 6) or (Result = 12) or
       (Result = 14) then
      Result := 1;
  end;
end;

procedure InitializeWizard;
var
  I, Inicial: Integer;
begin
  PaginaIdioma := CreateInputOptionPage(wpSelectTasks,
    CustomMessage('IdiomaTitulo'), CustomMessage('IdiomaDescricao'),
    CustomMessage('IdiomaTexto'), True, True);
  for I := 0 to QtdIdiomas - 1 do
    PaginaIdioma.Add(NomeIdioma(I));

  { Na linha de comando, /IDIOMA=código manda (útil na instalação silenciosa). }
  Inicial := IndiceDoCodigo(ExpandConstant('{param:IDIOMA|}'));
  if Inicial < 0 then
    Inicial := IdiomaInicial;
  PaginaIdioma.SelectedValueIndex := Inicial;
end;

{ Grava o idioma escolhido no arquivo de preferências do Nexus. Hoje o arquivo
  só guarda o idioma, então é escrito por inteiro. }
procedure GravarIdioma;
var
  Pasta, Codigo: String;
begin
  Codigo := CodigoIdioma(PaginaIdioma.SelectedValueIndex);
  Pasta := ExpandConstant('{userappdata}\Nexus Modeler');
  if not ForceDirectories(Pasta) then
  begin
    Log('Não foi possível criar ' + Pasta);
    Exit;
  end;
  if SaveStringToFile(Pasta + '\preferencias.settings',
    '<?xml version="1.0" encoding="UTF-8"?>' + #13#10 + #13#10 +
    '<PROPERTIES>' + #13#10 +
    '  <VALUE name="idioma" val="' + Codigo + '"/>' + #13#10 +
    '</PROPERTIES>' + #13#10, False) then
    Log('Idioma do Nexus gravado: ' + Codigo)
  else
    Log('Não foi possível gravar o idioma do Nexus em ' + Pasta);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
    GravarIdioma;
end;
