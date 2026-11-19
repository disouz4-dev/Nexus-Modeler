# Nexus Modeler: instalacao pelo PowerShell (ponto de entrada).
#   irm https://disouz4-dev.github.io/Nexus-Modeler/instalar.ps1 | iex
#
# Este arquivo fica so com letras sem acento: o "irm" do Windows PowerShell le
# o texto com a codificacao errada e trocaria os acentos. Ele baixa o
# instalador de verdade (instalar-windows.ps1, em UTF-8) como bytes e roda
# com a codificacao certa.
$ErrorActionPreference = "Stop"
$NexusInstalador = "https://disouz4-dev.github.io/Nexus-Modeler/instalar-windows.ps1"
$NexusBytes = (New-Object Net.WebClient).DownloadData($NexusInstalador)
$NexusTexto = [Text.Encoding]::UTF8.GetString($NexusBytes).TrimStart([char]0xFEFF)
& ([ScriptBlock]::Create($NexusTexto)) @args
