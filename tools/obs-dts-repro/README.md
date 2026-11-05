# Custom Output stale-frame reproduction (OBS issue 14153)

Filed upstream as https://github.com/obsproject/obs-studio/issues/14153 (2026-11-05). `issue-14153.md` is the text that was posted. It includes the code analysis and suggested fixes. The uploaded log is at https://obsproject.com/logs/Lko2Z0NoaupzOk9h.

**The bug:** in Custom Output (FFmpeg) mode, a frame that is still encoding when you press Stop gets written into the next recording first. The next recording's first real packet (dts 0) is then rejected: `process_packet: Error writing packet: Invalid argument`. The recording stops by itself about 0.5 s after it starts and leaves a file holding one frame. On the desktop (Ryzen 9 9950X3D) this happened in 6 of 20 and 5 of 20 quick stop/start cycles. This is OBS code (`plugins/obs-ffmpeg/obs-ffmpeg-output.c`), not FFmpeg. StudioEncoder's encoders use Standard recording, which does not have this bug.

## Test setup (Windows)

The desktop copy lives at `D:\obs-dts-test` and its recordings at `E:\DOWNLOADS\dts-test`. They are kept until the OBS developers reply. To rebuild it elsewhere:

1. Copy a stock portable OBS 33.0.1 into a new folder: `bin`, `data`, `core` (the built-in modules; without it OBS loads no encoders), and an empty `portable_mode.txt`. Leave out third-party plugins. **Do not use your everyday OBS.**
2. Put a 4K clip in the folder as `clip.mov`. A 4K 29.97 ProRes camera clip was used.
3. Config files, under `config\obs-studio\`:
   - `config/basic.ini` → `basic\profiles\DTS_test\basic.ini`. This is a 3840x2160 29.97 P216 Custom Output with prores_ks `profile=3`, mov, pcm_s24le. Change `FFFilePath` / `RecFilePath` to an output folder that exists.
   - `config/DTS_test.json` → `basic\scenes\DTS_test.json`. This is one looping Media Source; change `local_file` if the folder isn't `D:\obs-dts-test`.
   - `config/obs-websocket-config.json` → `plugin_config\obs-websocket\config.json`. This enables port 4466 with no password, for local testing only.
4. Start that OBS once and select profile "DTS test" and scene collection "DTS_test". Then close it.

## Run

```powershell
powershell -ExecutionPolicy Bypass -File tools\obs-dts-repro\dts-cycles.ps1 -ObsDir D:\obs-dts-test -Cycles 20
```

The script starts the test OBS with `--multi`, so your own OBS can stay open. It records for 1.5 to 4 s and waits 0.1 to 0.8 s between recordings, then closes only the OBS whose path is under `-ObsDir`. It prints a summary, an event timeline and the path of the new log.

The websocket timeline can miss events. The OBS log and the files are what count:
- In the log, look for `Error writing packet: Invalid argument` after `Recording Start`.
- Check each file. A failed one holds 1 video packet, and its pts continues the previous file's last pts:

```powershell
ffprobe -v error -select_streams v:0 -show_entries packet=pts -of csv=p=0 "E:\DOWNLOADS\dts-test\<file>.mov"
```
