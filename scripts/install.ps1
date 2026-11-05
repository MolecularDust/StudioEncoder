<#
.SYNOPSIS
  Installs StudioEncoder (obs-studioencoder) into the OBS 33 plugin layout and removes legacy-layout copies.

  Installed OBS : C:\ProgramData\obs-studio\plugins\obs-studioencoder\obs-studioencoder.dll  (+ data\)
  Portable OBS  : pass -Portable <OBS folder>  ->  <OBS folder>\plugins\obs-studioencoder\...

  Run from the repository root after: cmake --build build_x64 --config Release
#>
param(
  [string]$Portable = "",
  [string]$Config = "Release"
)

$ErrorActionPreference = 'Stop'
$name = 'obs-studioencoder'
$root = Split-Path -Parent $PSScriptRoot
$stage = Join-Path $root 'build_x64\stage'

if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
cmake --install (Join-Path $root 'build_x64') --config $Config --prefix $stage | Out-Null
$src = Join-Path $stage $name
if (-not (Test-Path (Join-Path $src "$name.dll"))) { throw "Build output not found in $src. Build first." }

if ($Portable) {
  $dest = Join-Path $Portable "plugins\$name"
  $legacy = @(
    (Join-Path $Portable "obs-plugins\64bit\$name.dll"),
    (Join-Path $Portable "obs-plugins\64bit\$name.pdb"),
    (Join-Path $Portable "data\obs-plugins\$name")
  )
} else {
  $dest = "C:\ProgramData\obs-studio\plugins\$name"
  $legacy = @(
    "C:\Program Files\obs-studio\obs-plugins\64bit\$name.dll",
    "C:\Program Files\obs-studio\obs-plugins\64bit\$name.pdb",
    "C:\Program Files\obs-studio\data\obs-plugins\$name",
    (Join-Path $dest 'bin')
  )
}

# earlier names of this plugin (obs-prores, then obs-studioencoders): remove those copies so OBS never loads two
$oldNames = @('obs-prores', 'obs-studioencoders')
$legacy += $oldNames | ForEach-Object { if ($Portable) { Join-Path $Portable "plugins\$_" } else { "C:\ProgramData\obs-studio\plugins\$_" } }

# remove legacy copies so OBS never sees two
foreach ($p in $legacy) {
  if (Test-Path $p) { Write-Host "Removing legacy copy: $p"; Remove-Item $p -Recurse -Force }
}

if (Test-Path $dest) { Remove-Item $dest -Recurse -Force }
New-Item -ItemType Directory -Force $dest | Out-Null
Copy-Item (Join-Path $src '*') $dest -Recurse -Force
Write-Host "Installed to $dest"
Get-ChildItem $dest -Recurse | Where-Object { -not $_.PSIsContainer } | ForEach-Object { "  " + $_.FullName.Substring($dest.Length) }
