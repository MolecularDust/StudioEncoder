<#
.SYNOPSIS
  Starts and stops recording repeatedly in one OBS session (portable OBS under _test) to check for the
  "non monotonically increasing dts" failure seen with Custom Output. Run test-record.ps1 first so the
  profile exists; this script reuses it.
#>
param(
  [int]$Cycles = 6,
  [int]$RecordSeconds = 4,
  [double]$PauseSeconds = 1.0,
  [string]$ObsDir = ''
)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName UIAutomationClient, UIAutomationTypes
$root = Split-Path -Parent $PSScriptRoot
if (-not $ObsDir) { $ObsDir = Join-Path $root '_test\obs-33.0.1' }
$cfg = Join-Path $ObsDir 'config\obs-studio'
$out = Join-Path $root 'test-output'
Get-ChildItem $out -Force | Remove-Item -Force -ErrorAction SilentlyContinue
Get-ChildItem $cfg -Force | Where-Object { $_.Name -like '.sentinel*' } | Remove-Item -Recurse -Force

function Find-Button($win, $pattern) {
  $cond = New-Object System.Windows.Automation.PropertyCondition([System.Windows.Automation.AutomationElement]::ControlTypeProperty, [System.Windows.Automation.ControlType]::Button)
  foreach ($b in $win.FindAll([System.Windows.Automation.TreeScope]::Descendants, $cond)) { if ($b.Current.Name -match $pattern) { return $b } }
  return $null
}
function Click($win, $pattern) {
  $deadline = (Get-Date).AddSeconds(30)
  while ((Get-Date) -lt $deadline) {
    $b = Find-Button $win $pattern
    if ($b) { $t = Get-Date; $b.GetCurrentPattern([System.Windows.Automation.InvokePattern]::Pattern).Invoke(); return $t }
    Start-Sleep -Milliseconds 200
  }
  throw "button '$pattern' not found"
}

$bin = Join-Path $ObsDir 'bin\64bit'
$p = Start-Process -FilePath (Join-Path $bin 'obs64.exe') -WorkingDirectory $bin -ArgumentList '--disable-shutdown-check' -PassThru
Start-Sleep -Seconds 8
$p.Refresh()
$win = [System.Windows.Automation.AutomationElement]::FromHandle($p.MainWindowHandle)

for ($i = 1; $i -le $Cycles; $i++) {
  $null = Click $win '^Start Recording$'
  Start-Sleep -Seconds $RecordSeconds
  $stopAt = Click $win '^Stop Recording$'
  "stop clicked at " + $stopAt.ToString('HH:mm:ss.fff')
  Start-Sleep -Milliseconds ([int]($PauseSeconds * 1000))
}
Start-Sleep -Seconds 4
$null = $p.CloseMainWindow()
$null = $p.WaitForExit(30000)
if (-not $p.HasExited) { Stop-Process -Id $p.Id -Force; Write-Warning 'OBS did not exit; killed' }
Get-ChildItem $cfg -Force | Where-Object { $_.Name -like '.sentinel*' } | Remove-Item -Recurse -Force -ErrorAction SilentlyContinue

$log = Get-ChildItem (Join-Path $cfg 'logs') | Sort-Object LastWriteTime -Descending | Select-Object -First 1
"--- log"
Select-String -Path $log.FullName -Pattern 'Recording Start|Recording Stop|Total frames output|dts|Error|error|frames still in flight' | ForEach-Object { $_.Line }
"--- files"
foreach ($f in Get-ChildItem $out -Filter *.mov | Sort-Object Name) {
  $n = (ffprobe -v error -select_streams v:0 -count_packets -show_entries stream=nb_read_packets -of csv=p=0 $f.FullName)
  "{0}  {1:N1} MB  {2} frames" -f $f.Name, ($f.Length / 1MB), $n
}
