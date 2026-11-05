# Reproduces obsproject/obs-studio#14153 (Custom Output leaks a frame into the next recording).
# Starts a portable test OBS, drives StartRecord/StopRecord over obs-websocket, then closes it.
# Never point -ObsDir at your everyday OBS. See README.md in this folder.
param([string]$ObsDir = 'D:\obs-dts-test', [int]$Port = 4466, [int]$Cycles = 20, [int]$MinRecMs = 1500, [int]$MaxRecMs = 4000, [int]$MinGapMs = 100, [int]$MaxGapMs = 800)
$ErrorActionPreference = 'Stop'
$obsDir = $ObsDir
$exe = "$obsDir\bin\64bit\obs64.exe"
$logDir = "$obsDir\config\obs-studio\logs"

function Test-OBS { Get-Process obs64 -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq $exe } }
if (Test-OBS) { throw "the test OBS is already running" }
$before = @(Get-ChildItem $logDir -Filter *.txt -ErrorAction SilentlyContinue | ForEach-Object Name)
Start-Process $exe -ArgumentList '--multi', '--disable-shutdown-check', '--disable-updater' -WorkingDirectory "$obsDir\bin\64bit" | Out-Null

# --- obs-websocket v5 client ---
$ws = $null
$end = (Get-Date).AddSeconds(90)
while (-not $ws -and (Get-Date) -lt $end) {
	try {
		$c = New-Object System.Net.WebSockets.ClientWebSocket
		$c.ConnectAsync([Uri]"ws://127.0.0.1:$Port", [Threading.CancellationToken]::None).GetAwaiter().GetResult()
		$ws = $c
	} catch { Start-Sleep -Milliseconds 1000 }
}
if (-not $ws) { throw "could not connect to the test OBS websocket on port $Port" }

$buf = New-Object byte[] 1048576
$pending = $null
function Receive-Msg([int]$timeoutMs) {
	if (-not $script:pending) { $script:pending = $ws.ReceiveAsync([ArraySegment[byte]]::new($buf), [Threading.CancellationToken]::None) }
	if (-not $script:pending.Wait($timeoutMs)) { return $null }
	$r = $script:pending.Result; $script:pending = $null
	$ms = New-Object System.IO.MemoryStream
	$ms.Write($buf, 0, $r.Count)
	while (-not $r.EndOfMessage) { $r = $ws.ReceiveAsync([ArraySegment[byte]]::new($buf), [Threading.CancellationToken]::None).GetAwaiter().GetResult(); $ms.Write($buf, 0, $r.Count) }
	return ([Text.Encoding]::UTF8.GetString($ms.ToArray()) | ConvertFrom-Json)
}
function Send-Msg($obj) {
	$b = [Text.Encoding]::UTF8.GetBytes(($obj | ConvertTo-Json -Depth 6 -Compress))
	$ws.SendAsync([ArraySegment[byte]]::new($b), [System.Net.WebSockets.WebSocketMessageType]::Text, $true, [Threading.CancellationToken]::None).GetAwaiter().GetResult()
}
$events = New-Object System.Collections.ArrayList
$t0 = Get-Date
function Note($text) { $line = "{0,7:N2}s  {1}" -f ((Get-Date) - $t0).TotalSeconds, $text; [void]$events.Add($line); $line }
function Handle($m) {
	if ($m.op -eq 5 -and $m.d.eventType -eq 'RecordStateChanged') { Note ("event  " + $m.d.eventData.outputState + "  " + $m.d.eventData.outputPath) | Out-Null; return $m.d.eventData.outputState }
	return $null
}
$hello = Receive-Msg 10000
Send-Msg @{ op = 1; d = @{ rpcVersion = 1; eventSubscriptions = 64 } }
$id = Receive-Msg 10000
if ($id.op -ne 2) { throw "identify failed" }

$reqn = 0
function Request($type) {
	$script:reqn++; $rid = "r$script:reqn"
	Send-Msg @{ op = 6; d = @{ requestType = $type; requestId = $rid } }
	$end = (Get-Date).AddSeconds(15)
	while ((Get-Date) -lt $end) {
		$m = Receive-Msg 500
		if (-not $m) { continue }
		if ($m.op -eq 7 -and $m.d.requestId -eq $rid) { return $m.d.requestStatus }
		$null = Handle $m
	}
	return $null
}
function Wait-State($want, [int]$sec) {
	$end = (Get-Date).AddSeconds($sec)
	while ((Get-Date) -lt $end) {
		$m = Receive-Msg 500
		if (-not $m) { continue }
		$s = Handle $m
		if ($s -and ($want -contains $s)) { return $s }
	}
	return $null
}

Start-Sleep -Seconds 8 # let the scene and media source settle
$rand = New-Object System.Random
$summary = @()
for ($i = 1; $i -le $Cycles; $i++) {
	$st = Request 'StartRecord'
	if (-not $st.result) { Note "cycle $i StartRecord refused: $($st.code) $($st.comment)" | Out-Null; Start-Sleep -Seconds 2; continue }
	$s = Wait-State @('OBS_WEBSOCKET_OUTPUT_STARTED', 'OBS_WEBSOCKET_OUTPUT_STOPPED') 20
	if ($s -ne 'OBS_WEBSOCKET_OUTPUT_STARTED') { Note "cycle $i did not start (state: $s)" | Out-Null; $summary += "cycle ${i}: FAILED TO START"; Start-Sleep -Seconds 2; continue }
	# record for a random time, watching for an unexpected stop
	$recMs = $rand.Next($MinRecMs, $MaxRecMs)
	$unexpected = Wait-State @('OBS_WEBSOCKET_OUTPUT_STOPPED') ([int][Math]::Ceiling($recMs / 1000.0))
	if ($unexpected) { Note "cycle $i STOPPED BY ITSELF while recording" | Out-Null; $summary += "cycle ${i}: STOPPED BY ITSELF"; Start-Sleep -Milliseconds 500; continue }
	$sp = Request 'StopRecord'
	$s = Wait-State @('OBS_WEBSOCKET_OUTPUT_STOPPED') 30
	$summary += "cycle ${i}: ok (recorded ~$recMs ms)"
	Start-Sleep -Milliseconds $rand.Next($MinGapMs, $MaxGapMs)
}
try { $ws.CloseAsync([System.Net.WebSockets.WebSocketCloseStatus]::NormalClosure, 'done', [Threading.CancellationToken]::None).Wait(3000) | Out-Null } catch {}
$p = Test-OBS
if ($p) { [void]$p.CloseMainWindow(); if (-not $p.WaitForExit(40000)) { "test OBS did not exit within 40 s" } }
Start-Sleep -Seconds 2
$log = Get-ChildItem $logDir -Filter *.txt | Where-Object { $before -notcontains $_.Name } | Sort-Object LastWriteTime -Descending | Select-Object -First 1
"=== summary"; $summary
"=== event timeline"; $events
"LOG=" + $log.FullName
