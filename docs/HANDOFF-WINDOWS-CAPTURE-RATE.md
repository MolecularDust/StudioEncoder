# Handoff: repeat the AverMedia capture-rate test on Windows

For the Claude Code session on the **Windows** install of this laptop. Read this, then `docs/HANDOFF.md` ("Working rules agreed with Mike", the AverMedia/P010 notes) and `scripts/test-record.ps1` (`-CaptureDevice`, `-CaptureFormat`, `-Fps`).

## Why

On Linux (Ubuntu 26.04, kernel 7.0.0-27, 2026-11-09) the AverMedia Live Gamer Ultra 2.1 (07ca:2553) with the camera (Sony ZV-E1, HDMI, Mike says it outputs 29.97) stopped delivering the requested frame rate. The encoder is not involved: the QuickSync 4:2:2 encoder reported no repeated frames and the recorded file is valid, but **8% of its frames are exact repeats** (144 of 1890 in a 63 s 4K recording), because the card hands OBS only about 27.5 new frames per second.

Windows is the known-good reference (4K 30/29.97 with 0 repeated frames, see HANDOFF.md). This test decides whether the problem is **the hardware/signal** (it shows on Windows too) or **the Linux side** (it does not).

## What was measured on Linux (so the Windows numbers are comparable)

Measured with `ffmpeg -f v4l2 ... -t 10 -i /dev/video4 -fps_mode passthrough` and counting frames (the kernel's own frame timestamps gave the same 27.5 fps; the audio sample clock agreed with the system clock, so the measurement is sound):

| Mode (Linux) | Requested | Delivered |
|---|---|---|
| 4K YUYV | 29.97 | 27.5 fps (275 frames / 10 s) |
| 4K NV12 | 29.97 | 27.5 fps |
| 1440p / 1080p YUYV | 29.97 | 27.5 fps |
| 4K / 1440p / 1080p MJPEG | 29.97 | 55.1-55.4 fps |
| 1080p YUYV | 15 / 24 / 25 / 30 | 27.3-27.5 fps |
| 1080p YUYV | 50 / 60 | 46.0 / 55.2 fps |

Delivered is always about **92% (11/12) of the nominal rate**, whatever the format, resolution or requested rate. Frame arrival gaps alternate about 33 ms and 41 ms (average 36.4 ms). Not the cause: USB hub (card moved to a direct 10 Gbps laptop port: same result), pixel format, resolution, system clock, camera power cycle, reboot.

What changed: the first recording of the day (30 fps, 4K YUYV, OBS) was fine (4 repeats in 1208 frames, all at warm-up/start). Afterwards the kernel logged `uvcvideo 2-4.1:1.1: Failed to resubmit video URB (-1)` and every test since has been at 92%. Mike did not touch the camera. The Ubuntu session on 2026-11-03/04 also got full rate from this card (AW+ live tests, 4K 30 YUY2, no repeated frames).

## Test setup (keep identical, write down any deviation)

- Same card, same camera, same HDMI cable. Camera output **4K 29.97** (note the camera's HDMI/NTSC/frame-rate menu settings: Mike says they were not changed).
- Card on a **direct 10 Gbps USB port** of the laptop, no hub or dock. Only this one capture device connected/opened (HANDOFF.md: two USB capture devices at once break at 4K P010).
- Close OBS before the ffmpeg tests (it holds the card).
- Note: Windows build of OBS in `_test\obs-<version>` per the working rules; do not touch Mike's own OBS installs.

## Test 1: raw delivered rate, no OBS, no encoder

Use ffmpeg with DirectShow. First list the device's formats:

```powershell
ffmpeg -hide_banner -f dshow -list_options true -i video="Live Gamer Ultra 2.1"
```

(the device name may differ: `ffmpeg -list_devices true -f dshow -i dummy`). Then for each row below capture 10 s and count frames. Use wall-clock timestamps and passthrough so ffmpeg neither duplicates nor drops:

```powershell
ffmpeg -hide_banner -v error -f dshow -video_size 3840x2160 -framerate 29.97 -pixel_format yuyv422 `
  -use_wallclock_as_timestamps 1 -i video="Live Gamer Ultra 2.1" -t 10 -an -fps_mode passthrough `
  -f framemd5 cap-4k-yuy2.txt
```

Count lines that do not start with `#` (frames), and count how many have a hash equal to the previous frame's (duplicates). Rows to run (4K and 1080p cover the resolution question; P010 only exists in the dshow list on Windows, use `-pixel_format p010le` if listed):

| Size | Pixel format | Requested fps |
|---|---|---|
| 3840x2160 | YUY2 (yuyv422) | 29.97 |
| 3840x2160 | NV12 | 29.97 |
| 3840x2160 | P010 (p010le) | 29.97 |
| 1920x1080 | YUY2 | 29.97 |
| 1920x1080 | YUY2 | 25 and 50 and 60 (if offered) |
| 3840x2160 | MJPEG | 29.97 |

**Pass criteria (healthy card and signal):** at least 295 frames in 10 s (>= 29.5 fps) and duplicates below 0.5% (a few at warm-up are fine; ignore the first 3 frames). The Linux failure signature is 275 frames (27.5 fps) with the 33/41 ms alternating arrival pattern. If a row fails, record the exact frame count so it can be compared with Linux's 275.

Also record, for the 4K YUY2 row, the per-frame arrival times, to see the 33/41 ms pattern: `-vf showinfo` and read `pts_time`, or a Python/PowerShell one-liner over the framemd5 timestamps.

## Test 2: a real OBS recording on Windows, same criteria as Linux

Use the repo's script with the live card, 60 s, P216 canvas, the QuickSync 4:2:2 encoder (the encoder under test on Linux; Mike's Windows results for it are in HANDOFF.md):

```powershell
powershell -File scripts\test-record.ps1 -Encoder obs_prores_qsv_hevc422_intra -Seconds 60 `
  -CaptureDevice "<video_device_id from a scene file>" -CaptureFormat 301 -Fps 29.97 -ColorFormat P216
```

Repeat with `-CaptureFormat 204` (P010) if the card offers it, as in the Windows runs already in HANDOFF.md. The Linux run used YUY2.

Check the recording exactly as on Linux:

```powershell
ffprobe -v error -select_streams v:0 -show_entries stream=codec_name,profile,pix_fmt,width,height,codec_tag_string,r_frame_rate,nb_frames -of default=nw=1 test-output\<file>
ffmpeg -v error -i <file> -f null -                                  # must print nothing
ffmpeg -v error -i <file> -map 0:v -f framemd5 md5.txt               # then count identical consecutive hashes
```

Judge by the plugin's end summary (`no frames were repeated` or the count) **and** by the frame hashes. Linux result for comparison: plugin said 0 repeats, hashes found 144 of 1890. If both agree on Windows (0 / ~0), the Windows capture path is healthy.

## Decision

| Windows result | Meaning | Next step |
|---|---|---|
| Delivers >= 29.5 fps, duplicates < 0.5% in YUY2/NV12/P010 | Card, camera and cable are fine. The Linux `uvcvideo` path loses frames (the URB errors point at the USB stream) | Back on Linux: try CachyOS (kernel 7.3.1 lists P010 and was clean for the AMD tests), or a newer kernel; capture `dmesg`/`journalctl -k` for `uvcvideo` errors during a test; try `usbcore.autosuspend=-1` |
| Also about 27.5 fps (275 frames / 10 s) in every mode | The card or the camera's HDMI signal is at fault, not Linux | Try another HDMI cable/port, the camera's HDMI output settings (resolution, frame rate, "clean HDMI"), a different HDMI source on the card, and the AverMedia firmware/tool if one exists for Windows |
| Rate depends on the mode (e.g. only 4K YUY2 is short) | Bandwidth or a mode-specific card limit | Note which modes; compare the USB link speed Windows reports |

## What to report back

A short table of frame counts per row (as above, with the Linux values alongside), duplicates per recording, the plugin's repeat summary, the camera's HDMI output settings, USB port used, and Windows/driver versions. Say plainly what was not tested. Add the results to a "Windows results: AverMedia capture rate" section in HANDOFF.md.

## Working rules (from HANDOFF.md)

- Don't commit or push without Mike's go-ahead; keep outward-facing actions behind explicit confirmation.
- Judge recordings by the repeated-frame counter or frame hashes, not by frame counts or timestamps. A black/frozen file with "no repeats" is a source problem.
- Don't run test scripts against Mike's own OBS installs; use the `_test` copy.
- Keep the Linux build working; it uses the same files.
