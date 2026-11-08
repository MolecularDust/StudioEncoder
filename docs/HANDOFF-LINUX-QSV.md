# Handoff: port the Intel QuickSync 4:2:2 encoder (0.16.0) to Linux

For the Claude Code session on Linux. Read this first, then `docs/HANDOFF.md`, especially the sections "Intel QuickSync HEVC 4:2:2 intra encoder (0.16.0)", "Build and test on Linux", "Linux results" and "Working rules agreed with Mike".

## Situation

- **Same laptop as the Windows work**: Intel Core Ultra X7 358H with Arc B390 graphics (Panther Lake, Xe3), no NVIDIA, no AMD. Capture: AverMedia Live Gamer Ultra 2.1 (USB 07ca:2553) with a camera on it, plus a second "USB3. 0 capture" adapter (534d:2109, 1080p).
- Mike boots this laptop from his **Ubuntu** SSD first. Ubuntu has been behind on drivers and codecs before (see "Linux results" in HANDOFF.md). If something fails there because of the platform, the fallback is **CachyOS** (Arch-based, newer kernel, Mesa, FFmpeg and Intel media stack). Note which distro each result comes from.
- Repository: `https://github.com/MolecularDust/StudioEncoder` (private). Start from `main` at commit `6a44875` ("0.16.0: HEVC 10-bit 4:2:2 intra encoder for Intel QuickSync") or later: `git pull`.

## What 0.16.0 added (on Windows)

`SE: HEVC 10-bit 4:2:2 Intra (Intel QuickSync)`, id `obs_prores_qsv_hevc422_intra`, in `src/encoder-qsv.c`:
- OBS 33's own QuickSync encoder refuses Color Format P216, and OBS's bundled FFmpeg on Windows has no QuickSync encoders, so the plugin calls Intel's **oneVPL** API directly (like OBS's obs-qsv11).
- P216 from OBS, repacked to Y210 on the worker thread (SSE2), HEVC Rext 4:2:2 10-bit, every frame IDR (GopPicSize 1, IdrInterval 1, per-frame FrameType I|IDR|REF), ICQ presets 24/21/18/12 (Small/Standard/High/Maximum, default High), `hvc1` tag, colour tags via `mfxExtVideoSignalInfo`, VPS/SPS/PPS via `GetVideoParam` for the MOV header.
- **Three frames are queued in the hardware** (AsyncDepth 3, a ring of sync points and bitstreams; packets come back 2 frames later with their timestamps through `mfxBitstream.TimeStamp`). Waiting for each frame before sending the next took 37 ms per 4K frame; queued it takes about 3 ms. Keep this design.
- Start-up probe opens the encoder at 640x360 and closes it; the encoder is hidden if that fails.
- `CMakeLists.txt` builds it only when oneVPL is found: first `find_package(VPL CONFIG)` (target `VPL::dispatcher`, which libvpl's own CMake package provides on Linux), otherwise `vpl/mfx.h` + `vpl` library found by hand (that is the Windows obs-deps case). Without oneVPL the plugin builds without this encoder and **says so only in one CMake status line**, so check the configure output.
- `scripts/test-record.ps1` (Windows only) gained live-card options; there is no Linux equivalent.

Windows results to compare against (Arc B390, 4K 30): 2.5-3 ms per frame on the worker, about 6 ms on OBS's thread, OBS CPU about 4-5% of 16 threads, 0 repeated frames, ICQ 18 gave 44-170 Mbit/s depending on the scene.

## What is probably needed on Linux

1. **Packages** (neither `scripts/linux-setup-ubuntu.sh` nor `scripts/linux-setup-arch.sh` installs these yet; add them once confirmed):
   - oneVPL dispatcher with headers and CMake files: Ubuntu `libvpl-dev`; Arch `libvpl`.
   - Intel's oneVPL GPU runtime (the part that drives the hardware): Ubuntu `libmfx-gen1.2` or `libmfx-gen1` depending on the release; Arch `vpl-gpu-rt`.
   - VA-API driver and tools: Ubuntu `intel-media-va-driver-non-free` and `vainfo`; Arch `intel-media-driver` and `libva-utils`.
   - Check the exact package names with `apt search` / `pacman -Ss`; they have changed between releases.
2. **Check the hardware path before touching OBS**:
   - `lspci -k`: which kernel driver runs the Arc B390 (`xe` is expected for Xe3; Panther Lake needs a recent kernel). If no driver binds, that is a distro limitation: go to CachyOS.
   - `vainfo`: look for `VAProfileHEVCMain422_10` with an encode entrypoint (`VAEntrypointEncSliceLP`). If it is missing, the driver stack cannot do 4:2:2 10-bit encode on this distro.
   - FFmpeg test like the one on Windows (if the distro FFmpeg has `hevc_qsv`): `ffmpeg -f lavfi -i testsrc2=size=3840x2160:rate=30,format=y210 -frames:v 300 -c:v hevc_qsv -profile:v rext -g 1 -global_quality 18 out.mp4`, then `ffprobe` for `Rext`, `yuv422p10le`. On Windows this ran at about 65 fps. `hevc_vaapi` with `-profile:v rext` is a second check of the same hardware.
3. **Build**: `bash scripts/linux-build.sh`; the configure output must say `StudioEncoder: oneVPL found (VPL::dispatcher), building the QuickSync HEVC 4:2:2 encoder`. The code has GCC paths for SSE2 (`emmintrin.h`, `__SSE2__`) and `usleep`; expect a few first-build fixes on GCC 15, as before.
4. **Likely code change: VA display.** On Windows the runtime creates its own device. On Linux, oneVPL hardware sessions usually need a VA display handed over with `MFXVideoCORE_SetHandle(session, MFX_HANDLE_VA_DISPLAY, va_display)` after `MFXCreateSession`, opened from a render node with `vaGetDisplayDRM` + `vaInitialize` (link `libva` and `libva-drm`). Not verified: try without it first and add it if `MFXVideoENCODE_Init` (or the probe) fails. If added, close it in `qsv_close` after `MFXClose`, keep the render node choice like the AMD VAAPI code in `src/encoder-ffmpeg.c` (`/dev/dri/renderD128...`, override with `STUDIOENCODER_VAAPI_DEVICE`), and keep it `#ifndef _WIN32` so the Windows build is unchanged.
5. **Fallback if oneVPL does not work on a distro**: `src/encoder-ffmpeg.c` already drives `hevc_vaapi` on Linux for the AMD encoder (P010, CQP). A VAAPI 4:2:2 path (Y210 frames, `AV_PROFILE_HEVC_REXT`) under the same encoder id would be the alternative. Ask Mike before building it.

## Testing on Linux

- Use the **native OBS package**, not Flatpak. Install with `bash scripts/linux-install.sh` (to `~/.config/obs-studio/plugins/obs-studioencoder/`). Check the OBS log for `obs_prores_qsv_hevc422_intra: hardware check passed` and `loaded version 0.16.0`.
- OBS settings: Settings > Advanced: Color Format **P216**, Color Space **Rec. 709**, Range **Limited**; Video 3840x2160, 30 fps; Output: Advanced, Recording Type Standard, MOV, encoder "SE: HEVC 10-bit 4:2:2 Intra (Intel QuickSync)".
- Source: stock OBS on Linux cannot take P010 from V4L2 (see "Capture cards and 10-bit on Linux" in HANDOFF.md). For testing the encoder, the AverMedia in **YUYV** (8-bit 4:2:2) is enough; the 10-bit P010 route needs the patched source in `~/obs-v4l2-10bit` (Ubuntu drive) and a kernel whose UVC driver lists P010 (CachyOS's 7.3 did, stock Ubuntu's 7.0 did not).
- Measure as on Windows: the plugin's repeated-frame counter and end summary, frame hashes (`ffmpeg -i file -map 0:v -f framemd5 out.txt`, count identical consecutive frames), full decode (`ffmpeg -v error -i file -f null -`), `ffprobe` for Rext / `yuv422p10le` / `hvc1` / bt709 / every frame a keyframe, and one frame looked at. Stop recordings through obs-websocket or the UI, not SIGTERM (unfinalised MOV), and delete `~/.config/obs-studio/.sentinel*` after a forced stop.

## Lessons from the Windows session (so they are not repeated)

- **Only one USB capture device at a time.** With the AverMedia at 4K P010 (about 750 MB/s) and the second USB adapter open together, Windows reported "not enough USB controller resources" and the recording was black. Hide or remove the second adapter's source.
- **P010 source with colour space "Default" gave strongly oversaturated, red skin** in OBS on Windows; Rec. 709 / Limited on the source fixed it. Check colours on Linux too.
- The plugin's counter can say 0 repeats while frames are identical: that means the **source** delivered the same picture (card warm-up in the first second, or no signal). A black or frozen file with "no repeats" is a source problem, not the encoder.
- Report outcomes plainly, including what was not tested.

## Working rules (from HANDOFF.md)

- Don't commit or push without Mike's go-ahead.
- Keep outward-facing actions behind explicit confirmation.
- Judge recordings by the repeated-frame counter or frame hashes, not by frame counts or timestamps.
- Keep the Windows build working; it uses the same files.
- Git identity: the Windows laptop had none set; commits there were made as `MolecularDust <entropologist@gmail.com>` with `git -c user.name=... -c user.email=...`. Check `git config user.email` on Linux before committing.

## Done looks like

On Ubuntu (or CachyOS if Ubuntu cannot): the plugin builds with the QuickSync encoder, the hardware check passes, a 1-minute 4K 30 recording from the AverMedia through P216 has no repeated frames after the first second, decodes cleanly and looks right; the setup scripts install the oneVPL packages; HANDOFF.md has a "Linux results: Intel QuickSync 4:2:2" section with the distro, kernel, driver and package versions, and what failed where.
