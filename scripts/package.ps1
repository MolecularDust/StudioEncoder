<#
.SYNOPSIS
  Builds obs-prores-<version>-windows-x64.zip for portable OBS (unzip into the OBS folder; it contains plugins\obs-prores\...).
#>
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$name = 'obs-prores'
$stage = Join-Path $root 'build_x64\package'
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force (Join-Path $stage 'plugins') | Out-Null
cmake --build (Join-Path $root 'build_x64') --config Release | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
cmake --install (Join-Path $root 'build_x64') --config Release --prefix (Join-Path $stage 'plugins') | Out-Null
$version = (Get-Content (Join-Path $root 'buildspec.json') -Raw | ConvertFrom-Json).version
$zip = Join-Path $root "$name-$version-windows-x64.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $stage 'plugins') -DestinationPath $zip
Write-Host "Created $zip"
