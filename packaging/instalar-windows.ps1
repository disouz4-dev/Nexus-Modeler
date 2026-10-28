<#
  Instalação do Nexus Modeler pelo terminal no Windows (PowerShell).

  Uso rápido (baixa o último lançamento do GitHub e instala só para você,
  sem administrador):
    irm https://raw.githubusercontent.com/disouz4-dev/Nexus-Modeler/main/packaging/instalar.ps1 | iex

  Com opções:
    & ([scriptblock]::Create((irm https://raw.githubusercontent.com/disouz4-dev/Nexus-Modeler/main/packaging/instalar.ps1))) -Idioma pt-BR

  Opções:
    -Idioma <código>   idioma da tela do Nexus, sem perguntar (pt-BR, en, es,
                       fr, it, de, ja, zh, ko, hi, ar, bn, ru, id, tr).
                       Também aceita a variável de ambiente NEXUS_IDIOMA.
    -Versao <etiqueta> instala um lançamento específico (por exemplo v0.2.0)
    -Arquivo <caminho> instala de um .zip já baixado em vez de baixar
    -TodoSistema       VST3 na pasta comum de todos os usuários
                       (C:\Program Files\Common Files\VST3); precisa de administrador
    -Desinstalar       remove o que este script instalou

  Onde instala (padrão, só para o usuário):
    VST3       %LOCALAPPDATA%\Programs\Common\VST3\Nexus Modeler.vst3
               (a pasta VST3 do usuário, pela regra da Steinberg)
    Programa   %LOCALAPPDATA%\Programs\Nexus Modeler, com atalho no menu Iniciar
    Idioma     %APPDATA%\Nexus Modeler\preferencias.settings
#>
[CmdletBinding()]
param(
  [string]$Idioma = $env:NEXUS_IDIOMA,
  [string]$Versao = "",
  [string]$Arquivo = "",
  [switch]$TodoSistema,
  [switch]$Desinstalar
)

# Este arquivo fica em UTF-8 sem marca de início (BOM), porque com ela o
# "irm | iex" falha. O PowerShell 5 do Windows, ao abrir o arquivo do disco,
# leria os acentos errado; então, nesse caso, o script se lê de novo em UTF-8.
if ($PSVersionTable.PSVersion.Major -lt 6 -and $PSCommandPath -and -not $env:NEXUS_INSTALADOR_UTF8) {
  $env:NEXUS_INSTALADOR_UTF8 = "1"
  try {
    & ([scriptblock]::Create([IO.File]::ReadAllText($PSCommandPath, [Text.Encoding]::UTF8))) @PSBoundParameters
  } finally {
    Remove-Item Env:\NEXUS_INSTALADOR_UTF8 -ErrorAction SilentlyContinue
  }
  return
}

$ErrorActionPreference = "Stop"
$ProgressPreference = "SilentlyContinue"   # o download fica muito mais rápido sem a barra
[Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12

$Repositorio = "disouz4-dev/Nexus-Modeler"
$Codigos = @("pt-BR", "en", "es", "fr", "it", "de", "ja", "zh", "ko", "hi", "ar", "bn", "ru", "id", "tr")
# Nomes de cada idioma escritos no próprio idioma, como no globo do Nexus.
# Guardados em base64 (texto UTF-8 codificado só com letras simples) para o
# arquivo ler igual em qualquer versão do PowerShell.
$Nomes = [Text.Encoding]::UTF8.GetString([Convert]::FromBase64String(
  "UG9ydHVndcOqcyAoQnJhc2lsKXxFbmdsaXNofEVzcGHDsW9sfEZyYW7Dp2Fpc3xJdGFsaWFub3xEZXV0c2NofOaXpeacrOiqnnzkuK3mlofvvIjnroDkvZPvvIl87ZWc6rWt7Ja0fOCkueCkv+CkqOCljeCkpuClgHzYp9mE2LnYsdio2YrYqXzgpqzgpr7gpoLgprLgpr580KDRg9GB0YHQutC40Ll8QmFoYXNhIEluZG9uZXNpYXxUw7xya8OnZQ==")).Split("|")

if ($TodoSistema) {
  $Vst3Dir = Join-Path ${env:CommonProgramFiles} "VST3"
} else {
  $Vst3Dir = Join-Path $env:LOCALAPPDATA "Programs\Common\VST3"
}
$ProgramaDir = Join-Path $env:LOCALAPPDATA "Programs\Nexus Modeler"
$Atalho = Join-Path ([Environment]::GetFolderPath("Programs")) "Nexus Modeler.lnk"
$Preferencias = Join-Path $env:APPDATA "Nexus Modeler\preferencias.settings"

# Sem "exit": quando o script roda por "irm | iex", exit fecharia a janela do
# PowerShell da pessoa.
function Parar([string]$Mensagem) {
  throw "Erro: $Mensagem"
}

# --- Desinstalação ------------------------------------------------------------
if ($Desinstalar) {
  foreach ($Item in @((Join-Path $Vst3Dir "Nexus Modeler.vst3"), $ProgramaDir, $Atalho)) {
    if (Test-Path $Item) { Remove-Item -Recurse -Force $Item }
  }
  Write-Host "Nexus Modeler removido. As preferências ficaram em: $(Split-Path $Preferencias)"
  return
}

if ($TodoSistema) {
  $Admin = ([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole(
    [Security.Principal.WindowsBuiltInRole]::Administrator)
  if (-not $Admin) { Parar "-TodoSistema precisa do PowerShell aberto como administrador." }
}

# --- Idioma ------------------------------------------------------------------
function IdiomaDoWindows {
  $Nome = (Get-UICulture).Name.ToLower()   # "pt-br", "ja-jp", "zh-cn"...
  if ($Nome.StartsWith("pt")) { return "pt-BR" }
  foreach ($Codigo in $Codigos) {
    if ($Nome -eq $Codigo -or $Nome.StartsWith("$Codigo-")) { return $Codigo }
  }
  return "en"
}

if ($Idioma) {
  $Achado = $Codigos | Where-Object { $_ -ieq $Idioma } | Select-Object -First 1
  if (-not $Achado) { Parar "idioma desconhecido: $Idioma (use um destes: $($Codigos -join ' '))" }
  $Idioma = $Achado
} else {
  $Padrao = IdiomaDoWindows
  try { $Interativo = [Environment]::UserInteractive -and -not [Console]::IsInputRedirected } catch { $Interativo = $false }
  if ($Interativo) {
    Write-Host "Idioma da tela do Nexus / Nexus screen language:"
    for ($i = 0; $i -lt $Codigos.Count; $i++) {
      $Marca = if ($Codigos[$i] -eq $Padrao) { "*" } else { " " }
      Write-Host (" {0} {1,2}) {2} ({3})" -f $Marca, ($i + 1), $Nomes[$i], $Codigos[$i])
    }
    $Resposta = Read-Host "Número [Enter = $Padrao]"
    $Numero = 0
    if ([string]::IsNullOrWhiteSpace($Resposta)) {
      $Idioma = $Padrao
    } elseif ([int]::TryParse($Resposta, [ref]$Numero) -and $Numero -ge 1 -and $Numero -le $Codigos.Count) {
      $Idioma = $Codigos[$Numero - 1]
    } elseif ($Codigos -icontains $Resposta) {
      $Idioma = $Codigos | Where-Object { $_ -ieq $Resposta } | Select-Object -First 1
    } else {
      Write-Host "Resposta não reconhecida; usando $Padrao."
      $Idioma = $Padrao
    }
  } else {
    $Idioma = $Padrao
  }
}

# --- Download ----------------------------------------------------------------
$Temp = Join-Path ([IO.Path]::GetTempPath()) ("nexus-" + [Guid]::NewGuid().ToString("N"))
New-Item -ItemType Directory -Force -Path $Temp | Out-Null
try {
  if (-not $Arquivo) {
    if ($Versao) {
      $Api = "https://api.github.com/repos/$Repositorio/releases/tags/$Versao"
    } else {
      $Api = "https://api.github.com/repos/$Repositorio/releases/latest"
    }
    Write-Host "Procurando o lançamento em github.com/$Repositorio ..."
    try {
      $Lancamento = Invoke-RestMethod -Uri $Api -Headers @{ "User-Agent" = "instalador-nexus" }
    } catch {
      if ($_.Exception.Response -and [int]$_.Exception.Response.StatusCode -eq 404) {
        Parar "ainda não há versão publicada para baixar. Acompanhe em https://github.com/$Repositorio/releases"
      }
      Parar "não consegui falar com o GitHub ($($_.Exception.Message)). Confira a internet e tente de novo."
    }
    $Pacote = $Lancamento.assets | Where-Object { $_.name -like "*-Windows.zip" } | Select-Object -First 1
    if (-not $Pacote) { Parar "não achei o arquivo *-Windows.zip no lançamento. Veja https://github.com/$Repositorio/releases" }
    Write-Host "Baixando $($Pacote.name) ..."
    $Arquivo = Join-Path $Temp $Pacote.name
    Invoke-WebRequest -Uri $Pacote.browser_download_url -OutFile $Arquivo -UseBasicParsing
  }
  if (-not (Test-Path $Arquivo)) { Parar "arquivo não encontrado: $Arquivo" }
  try { Unblock-File -Path $Arquivo } catch { }

  $Extraido = Join-Path $Temp "pacote"
  Expand-Archive -Path $Arquivo -DestinationPath $Extraido -Force
  $Origem = Join-Path $Extraido "Nexus Modeler"
  if (-not (Test-Path $Origem)) { Parar "o arquivo não tem a pasta ""Nexus Modeler"" esperada." }

  # --- Instalação --------------------------------------------------------------
  New-Item -ItemType Directory -Force -Path $Vst3Dir, $ProgramaDir | Out-Null
  $Vst3Destino = Join-Path $Vst3Dir "Nexus Modeler.vst3"
  if (Test-Path $Vst3Destino) { Remove-Item -Recurse -Force $Vst3Destino }
  Copy-Item -Recurse -Path (Join-Path $Origem "Nexus Modeler.vst3") -Destination $Vst3Dir
  Copy-Item -Force -Path (Join-Path $Origem "Nexus Modeler.exe") -Destination $ProgramaDir
  Copy-Item -Force -Path (Join-Path $Origem "LICENSE.txt") -Destination $ProgramaDir -ErrorAction SilentlyContinue

  $Shell = New-Object -ComObject WScript.Shell
  $Link = $Shell.CreateShortcut($Atalho)
  $Link.TargetPath = Join-Path $ProgramaDir "Nexus Modeler.exe"
  $Link.WorkingDirectory = $ProgramaDir
  $Link.Save()

  # --- Preferência de idioma -------------------------------------------------
  # Arquivo de ajustes do JUCE (XML), em UTF-8 sem marca de início. Se já
  # existir, troca só o idioma.
  New-Item -ItemType Directory -Force -Path (Split-Path $Preferencias) | Out-Null
  $Valor = "<VALUE name=""idioma"" val=""$Idioma""/>"
  if (Test-Path $Preferencias) {
    $Texto = [IO.File]::ReadAllText($Preferencias)
    if ($Texto -match '<VALUE name="idioma" val="[^"]*"/>') {
      $Texto = $Texto -replace '<VALUE name="idioma" val="[^"]*"/>', $Valor
    } elseif ($Texto -match '</PROPERTIES>') {
      $Texto = $Texto -replace '</PROPERTIES>', "  $Valor`r`n</PROPERTIES>"
    } else {
      $Texto = $null
    }
  }
  if (-not $Texto) {
    $Texto = "<?xml version=""1.0"" encoding=""UTF-8""?>`r`n`r`n<PROPERTIES>`r`n  $Valor`r`n</PROPERTIES>`r`n"
  }
  [IO.File]::WriteAllText($Preferencias, $Texto, (New-Object Text.UTF8Encoding $false))

  Write-Host "Instalado:"
  Write-Host "  VST3      $Vst3Destino"
  Write-Host "  Programa  $ProgramaDir (e no menu Iniciar)"
  Write-Host "  Idioma    $Idioma (troque depois pelo globo na tela do Nexus)"
} finally {
  Remove-Item -Recurse -Force $Temp -ErrorAction SilentlyContinue
}
