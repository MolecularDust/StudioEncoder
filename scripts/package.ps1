<#
.SYNOPSIS
  Builds obs-studioencoder-<version>-windows-<arch>.zip for portable OBS (unzip into the OBS folder; it contains plugins\obs-studioencoder\...).
  -Arch x64 (default) uses build_x64, -Arch arm64 uses build_arm64 (configure first: cmake --preset windows-arm64).
#>
param([ValidateSet('x64', 'arm64')][string]$Arch = 'x64')
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $PSScriptRoot
$name = 'obs-studioencoder'
$build = Join-Path $root "build_$Arch"
$stage = Join-Path $build 'package'
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item -ItemType Directory -Force (Join-Path $stage 'plugins') | Out-Null
cmake --build $build --config Release | Out-Null
if ($LASTEXITCODE -ne 0) { throw 'Release build failed' }
cmake --install $build --config Release --prefix (Join-Path $stage 'plugins') | Out-Null
$version = (Get-Content (Join-Path $root 'buildspec.json') -Raw | ConvertFrom-Json).version
$zip = Join-Path $root "$name-$version-windows-$Arch.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path (Join-Path $stage 'plugins') -DestinationPath $zip
Write-Host "Created $zip"
