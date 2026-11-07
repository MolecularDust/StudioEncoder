<#
.SYNOPSIS
  Records with a plugin encoder in the portable OBS under _test, stops via UI Automation, and prints a summary.
  Setup is local to _test\obs-<version>; nothing outside the repository is touched.
#>
param(
  [string]$Encoder = 'obs_prores_ffmpeg_aw',   # AW+; others: obs_prores_rust, obs_prores_oxideav
  [int]$Profile = 2,                         # 0 Proxy, 1 LT, 2 422, 3 HQ
  [int]$Seconds = 15,
  [int]$LogLevel = 1,
  [int]$InFlight = 0,                        # 0 = encoder default
  [int]$Overload = 0,                        # 0 = warn, 1 = stop recording
  [int]$Sizing = 0,                          # 0 = grow when late, 1 = size from measured frame cost
  [string]$Clip = "",                         # optional video file to loop as the source instead of a flat colour
  [string]$ColorFormat = 'P216',            # OBS output colour format
  [string]$EncoderJson = '',                # raw recordEncoder.json content; replaces the plugin settings
  [switch]$NoRecord,                        # measure the idle load of the scene only
  [int]$CpuSettle = 8,                       # seconds before the CPU window starts
  [string]$ObsDir = ''
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
$root = Split-Path -Parent $PSScriptRoot
if (-not $ObsDir) { $ObsDir = Join-Path $root '_test\obs-33.0.1' }
$cfg = Join-Path $ObsDir 'config\obs-studio'
$out = Join-Path $root 'test-output'
$prof = Join-Path $cfg 'basic\profiles\ProResTest'
New-Item -ItemType Directory -Force $out, $prof, (Join-Path $cfg 'basic\scenes') | Out-Null
Get-ChildItem $out -Force | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem $cfg -Force -ErrorAction SilentlyContinue | Where-Object { $_.Name -like '.sentinel*' } | Remove-Item -Recurse -Force

$outEsc = $out.Replace([string][char]92, ([string][char]92 + [string][char]92))
@"
[General]
Name=ProResTest

[Video]
BaseCX=3840
BaseCY=2160
OutputCX=3840
OutputCY=2160
FPSType=0
FPSCommon=29.97
ColorFormat=$ColorFormat
ColorSpace=709
ColorRange=Partial

[Output]
Mode=Advanced

[AdvOut]
RecType=Standard
RecFilePath=$outEsc
RecFormat2=mov
RecEncoder=$Encoder
RecTracks=1
"@ | Set-Content (Join-Path $prof 'basic.ini') -Encoding utf8
$inflightJson = if ($InFlight -gt 0) { ",`"in_flight`":$InFlight" } else { "" }
$encJson = if ($EncoderJson) { $EncoderJson } else { "{`"profile`":$Profile,`"threads`":0,`"log_level`":$LogLevel,`"vendor_apl0`":true,`"overload_action`":$Overload,`"inflight_sizing`":$Sizing$inflightJson}" }
$encJson | Set-Content (Join-Path $prof 'recordEncoder.json') -Encoding utf8
if ($Clip) {
  # loop a real recording; hardware decoding off so the CPU load resembles a capture source plus decode
  $clipJson = ($Clip -replace '\\', '/')
  $source = "{`"id`":`"ffmpeg_source`",`"name`":`"Source`",`"versioned_id`":`"ffmpeg_source`",`"settings`":{`"local_file`":`"$clipJson`",`"is_local_file`":true,`"looping`":true,`"hw_decode`":false,`"restart_on_activate`":true}}"
} else {
  $source = '{"id":"color_source_v3","name":"Source","settings":{"color":4285426688,"width":3840,"height":2160},"versioned_id":"color_source_v3"}'
}
@"
{"name":"ProResTest","current_scene":"Scene","current_program_scene":"Scene","scene_order":[{"name":"Scene"}],
"sources":[
 $source,
 {"id":"scene","name":"Scene","versioned_id":"scene","settings":{"id_counter":1,"items":[{"name":"Source","source_uuid":"","id":1,"visible":true,"locked":false,"pos":{"x":0.0,"y":0.0},"scale":{"x":1.0,"y":1.0},"rot":0.0,"align":5}]}}
],"groups":[],"quick_transitions":[],"transitions":[],"saved_projectors":[],"modules":{},"version":2}
"@ | Set-Content (Join-Path $cfg 'basic\scenes\ProResTest.json') -Encoding utf8
@"
[General]
FirstRun=true
ConfirmOnExit=false
EnableAutoUpdates=false
[Basic]
Profile=ProResTest
ProfileDir=ProResTest
SceneCollection=ProResTest
SceneCollectionFile=ProResTest
"@ | Set-Content (Join-Path $cfg 'user.ini') -Encoding utf8

$bin = Join-Path $ObsDir 'bin\64bit'
$obsArgs = @('--disable-shutdown-check'); if (-not $NoRecord) { $obsArgs += '--startrecording' }
$p = Start-Process -FilePath (Join-Path $bin 'obs64.exe') -WorkingDirectory $bin -ArgumentList $obsArgs -PassThru
Start-Sleep -Seconds $CpuSettle
$p.Refresh(); $cpu0 = $p.TotalProcessorTime; $t0 = Get-Date
Start-Sleep -Seconds ([Math]::Max(1, $Seconds - $CpuSettle - 1))
$p.Refresh(); $cpu1 = $p.TotalProcessorTime; $t1 = Get-Date
$cores = [Environment]::ProcessorCount
$cpuPct = ($cpu1 - $cpu0).TotalSeconds / (($t1 - $t0).TotalSeconds * $cores) * 100
"OBS CPU: {0:N1}% of {1} threads over {2:N0} s" -f $cpuPct, $cores, ($t1 - $t0).TotalSeconds
Start-Sleep -Seconds 1
if ($NoRecord) {
  $null = $p.CloseMainWindow(); $null = $p.WaitForExit(30000)
  if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force }
  Get-ChildItem $cfg -Force | Where-Object { $_.Name -like ".sentinel*" } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue
  return
}
$p.Refresh()
$win = [System.Windows.Automation.AutomationElement]::FromHandle($p.MainWindowHandle)
$btnCond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Button)
$stop = $null
foreach ($b in $win.FindAll([System.Windows.Automation.TreeScope]::Descendants, $btnCond)) { if ($b.Current.Name -match 'Stop Recording') { $stop = $b } }
if ($stop) { $stop.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke() } else { Write-Warning 'Stop Recording button not found' }

# wait for the recording to finalize, then close OBS cleanly
$deadline = (Get-Date).AddSeconds(90)
$log = Get-ChildItem (Join-Path $cfg 'logs') | Sort-Object LastWriteTime -Descending | Select-Object -First 1
while ((Get-Date) -lt $deadline) {
  Start-Sleep -Seconds 2
  if (Select-String -Path $log.FullName -Pattern 'Recording Stop' -Quiet) { break }
}
Start-Sleep -Seconds 2
$null = $p.CloseMainWindow()
$null = $p.WaitForExit(30000)
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; Write-Warning 'OBS did not exit; killed' }
Get-ChildItem $cfg -Force | Where-Object { $_.Name -like '.sentinel*' } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

"--- log summary"
Select-String -Path $log.FullName -Pattern 'obs-studioencoder|Total frames|skipped frames|error|Error|dts' | ForEach-Object { $_.Line } | Where-Object { $_ -notmatch 'frame \d+: |took ' }
"--- output"
Get-ChildItem $out | Select-Object Name, Length | Format-Table | Out-String
