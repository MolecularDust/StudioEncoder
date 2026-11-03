# Handoff: OBS ProRes plugin

For a new Claude Code session on another machine. Read this first, then `README.md`, `docs/phase0-design.md` and `docs/phase1-report.md`.

## What this is

A Windows OBS 33 plugin (StudioEncoders, folder and DLL `obs-studioencoders`, called `obs-prores` before 0.9.0) that adds ProRes-compatible video encoders for Standard recording, 4K 10-bit 4:2:2, from OBS's P216 output format:

1. **ProRes (FFmpeg prores_aw, fast)**: FFmpeg's `prores_aw` (the same code as FFmpeg's default `prores`). Recommended.
2. **ProRes (FFmpeg prores_ks)**: FFmpeg's `prores_ks`. Heavy CPU use.
3. **ProRes-compatible (Rust / FilmCraft)**: Rust encoder from the FilmCraft project (MIT OR Apache-2.0), pinned to commit `68919ed87aa6a097a4d21414ec7c438883f5fe22`. Heavy CPU use.

Unofficial; not Apple ProRes. Keep that wording in any user-facing text.

Current version: **0.10.2**.

## Layout

- `src/`: C glue. `encoder-common.c/.h` has the shared pipeline, P216 conversion and overload check. `encoder-ffmpeg.c` has both FFmpeg encoders. Also `encoder-rust.c`, `prores-rs.h` and `plugin-main.c`.
- `rust/`: Rust C-ABI staticlib (`src/lib.rs`) wrapping the FilmCraft crate. Built by CMake through cargo.
- `harness/`: Rust benchmark CLI used in phase 1 (`prores-bench`).
- `scripts/install.ps1`: installs into a portable OBS (`-Portable <folder>`), or into the system OBS at `C:\ProgramData\obs-studio\plugins`. Removes legacy-layout copies.
- `scripts/package.ps1`: builds `obs-studioencoders-<version>-windows-x64.zip` for portable OBS.
- `scripts/test-record.ps1`, `scripts/test-cycles.ps1`: automated recordings in a portable OBS under `_test\obs-33.0.1`, which is not in git. Download the official 33.0.1 portable zip there first. These scripts overwrite that copy's test profile.
- `buildspec.json`, `CMakePresets.json`, `CMakeLists.txt`, `cmake/`: build setup from obs-plugintemplate, pinned to OBS 33.0.0 sources and obs-deps 2026-10-23.
- Not in git: `_ref/` (reference clones of obs-studio, obs-plugintemplate, filmcraft and StreamFX-Public), `.deps/`, `build_x64/`, `_test/`, `test-output/`, zips.

## Build (Windows)

Needs:
- Visual Studio 2026 Build Tools (C++ workload, Windows SDK 10.0.26100)
- CMake 3.28 or newer
- Rust stable `x86_64-pc-windows-msvc`

Then:

```
cmake --preset windows-x64
cmake --build build_x64 --config Release
powershell -ExecutionPolicy Bypass -File scripts\package.ps1
```

The first configure downloads OBS 33.0.0 sources and obs-deps into `.deps\` and builds libobs, which takes a few minutes. Cargo fetches FilmCraft at the pinned commit.

## Decisions made

- FFmpeg: option (a), build against the FFmpeg that obs-deps ships (avcodec 62, FFmpeg 8.x). Keep the backend seam so a bundled LGPL FFmpeg could replace it.
- Target OBS 33.0.0 / 33.0.1. No promise of OBS 32 compatibility.
- Input format P216. OBS 33 has no P210 or I210 output. Conversion is (v + 32) >> 6 per sample with SSE2.
- The muxer gets the FourCC through the encoder setting `codec_type`, as a big-endian integer. The encoder must return true from `get_extra_data`, or obs-ffmpeg-mux silently drops every packet.
- Frame-parallel pipeline: several frames encoded at once, each on its own worker and encoder context, returned in order.
- Automatic frames in flight is the default. It starts at half the logical CPUs and grows while the oldest frame is late, up to one worker per logical CPU. 0.4.1 also stopped growing once a worker "didn't help", which left prores_ks short on the 32-thread desktop; that rule was removed in 0.4.2.
- Stopping costs no frames. OBS keeps feeding the encoder until a packet past the stop point arrives.
- Overload detection (0.5.0): the plugin counts repeated frames itself, per recording. When encoding holds up OBS's video thread, OBS hands the encoder the same cached frame buffer again, so a repeat is "the same frame buffer twice in a row". It warns every overloaded second (more than 2% repeats), writes a summary at the end, and can optionally stop the recording after 3 overloaded seconds in a row. Earlier versions read OBS's shared `video_output_get_skipped_frames` counter. OBS resets that counter whenever an output connects, which made the summary wrong for later recordings in a session (0.4.1 and 0.4.2).
- Automatic sizing toggle (0.5.0): "Grow when frames are late" (default), or "Size from measured frame cost" (experimental). The second starts at 4 workers and jumps to ceil(fps x seconds per frame x 1.25) + 1, re-evaluated at most once a second, and only ever grows.
- NVENC H.264 10-bit 4:2:2 all-intra encoder (0.6.0), RTX 50 series only: FFmpeg `h264_nvenc` from OBS's obs-deps, P216 straight in (no conversion), profile high422, `-g 1 -bf 0`, CBR, `tune ll` (`tune hq` is rejected with a GOP of 1 at p4-p7), preset p5 or p1, `delay 0`, SPS/PPS in extradata (global header). Default 200 Mbit/s, chosen from the desktop FFmpeg test (`REPORT-INTRA.md`: 100M 58.8 dB, 200M 62.2 dB, 300M 65.3 dB; ProRes LT 59.7 dB). Max 2 frames in flight (NVENC session limit). The ProRes profile list, Threads setting and FourCC are hidden/zero for this backend. Pre-RTX-50 support (HEVC/AV1 4:2:0) is deferred. Tested in OBS on the desktop RTX 5080 with the AverMedia card (REPORT-NVENC-PLUGIN.md, 2026-11-03): 100/200/300 (p1) 1 min, 200 for 10 min and 5 start/stop cycles, all High 4:2:2 Intra yuv422p10le, every frame I, bitrate on target, 0 repeated frames (log and hashes), about 20 ms per frame, OBS about 1% CPU, NVENC about 27%. On the laptop RTX 4070 it fails cleanly with the RTX 50 message.
- Plugin Manager version (0.7.1): the OBS 33 Plugin Manager reads the version from `data/manifest.json`; `cmake/manifest.json.in` is filled from buildspec.json at configure time and installed by CMakeLists.txt. Verified: Tools > Plugin Manager > Installed shows "ProRes-compatible encoders 0.7.1".
- Settings window (0.8.0): Tools > "StudioEncoders" (Qt dialog, `src/settings-dialog.cpp`) lists all five encoders; Recommended (prores_aw, NVENC H.264) on, Experimental (prores_ks, FilmCraft, OxideAV) off by default. Choices live in `plugin_config\obs-prores\settings.json` (written by the dialog, not meant to be hand-edited). A disabled encoder is registered with `OBS_ENCODER_CAP_DEPRECATED`, so OBS hides it from the dropdown (and its start-up encoder log) but profiles that already use it still work; OBS labels such a selected encoder "(Deprecated)". Toggles need an OBS restart because encoders register at load. Build: Qt6 (obs-deps qt6 2026-10-23, hash in buildspec.json, ~260 MB download into .deps) and obs-frontend-api; no Q_OBJECT classes, so no moc. Tested in the test OBS: menu item present, dialog defaults right, OK saves, restart lists OxideAV, a hidden encoder still records.
- Rename to StudioEncoders (0.9.0): chosen to keep "ProRes" out of the product name (Apple's trademark guidelines bar using its marks in product names; "ProRes" now appears only in descriptions). Module/folder/DLL/log prefix are `obs-studioencoders`; Plugin Manager and Tools menu say "StudioEncoders". Encoder IDs (`obs_prores_*`) and the Rust crate names are unchanged on purpose, so existing OBS profiles keep working. Settings now live in `plugin_config\obs-studioencoders\settings.json` (the 0.8.0 file under `obs-prores` is not migrated; 0.8.0 was never installed on user machines). `scripts/install.ps1` deletes an old `plugins\obs-prores` folder. The GitHub repo and the project folder still use the old name; renaming the repo needs the owner's OK. Anyone updating by hand must delete the old `obs-prores` plugin folder (the desktop's portable OBS still has it).
- Hardware check and SE prefix (0.9.1): at start-up `pr_nvenc_h264_422_probe()` (encoder-ffmpeg.c) opens `h264_nvenc` in the same 10-bit 4:2:2 mode on a 256x256 frame and closes it (about 90 ms on the laptop). If it fails, the NVENC encoder is hidden from OBS's list (same DEPRECATED-cap mechanism) and shown grayed out in the StudioEncoders window; the saved choice is left untouched, there is no manual override, and the check runs again on every start. The log line says `hardware check passed` or `not available on this system, hidden` with the driver's reason. Laptop (RTX 4070): hidden, as expected. Verified on both machines (2026-11-03): offered on the desktop (RTX 5080), hidden and grayed out on the laptop (RTX 4070). All encoder names in OBS start with `SE: `.
- AW+ (0.10.0): `src/prores-aw.c` is a standalone port of FFmpeg 8.0's prores_aw (proresenc_anatoliy.c, jfdctint 10-bit, proresdata tables), progressive 4:2:2 only. The Proxy blur came from `level = coef / step` truncating toward zero (matrix and quantiser are identical to Apple's: both use q8 on 100% of slices). AW+ rounds with a per-profile offset (`pa_default_round8`: Proxy 4, LT 3, 422 3, HQ 2, eighths of a step; best quality per bit; full rounding at HQ made the rate control raise the quantiser). round8=0 is byte-identical to FFmpeg on all four profiles (`tools/prores-aw-test.c`, build target `prores-aw-test`). Speed: exact reciprocal multiply instead of division and a 32-bit bit writer; single-thread 4K Proxy 41 ms vs FFmpeg 52, HQ 79 vs 81. AW+ took over the ID `obs_prores_ffmpeg_aw` so existing profiles get it; FFmpeg's original moved to `obs_prores_ffmpeg_aw_original` (experimental). Test OBS, 4K: AW+ Proxy 11.4% CPU 94 Mbit/s, original 12.5% 74 Mbit/s; AW+ LT 13.0% 197 Mbit/s; no repeats. Visual check (eye crop) matches Apple Proxy.
- AW+ live test (2026-11-03, laptop, AverMedia + ZV-E1, YUY2, 4K 30, Proxy, 1 min each): AW+ 103 Mbit/s (0.78 GB), 41 ms per frame, OBS CPU 7%; original prores_aw 68 Mbit/s (0.51 GB), 48 ms, CPU 8%. No repeated frames in either; both `apco`, Rec. 709, decode clean. FFmpeg blurdetect over 60 frames: AW+ 8.0, original 9.7 (lower is sharper); a hair close-up shows individual hairs with AW+ and a smear with the original.
- 0.10.1 fixes: the per-frame packet buffer was width x height + 64 KB (8 MB at 4K); pure noise reaches 8.8 MB (Proxy) to 20 MB (HQ) per 4K frame, so very noisy footage could fail the encode. Now 3 bytes per pixel + 1 MB (about 26 MB at 4K, physical memory only for what is written). Verified: a looped 4K noise clip at HQ with AW+ recorded 500 frames of 10-15 MB, decode clean. The overload warning no longer names prores_aw ("Use a lighter profile or a lower resolution; SE: ProRes (AW+, fast) is the lightest ProRes encoder"). Code review findings that did not lead to changes: precise float DCT gains at most 0.15 dB for half the speed; low/high-frequency rounding splits were all worse than uniform rounding; Apple's quantisation matrices equal AW's in all profiles; rate control re-encodes 1.00-1.06 times per slice. Time split per 4K Proxy frame: DCT about 20 ms, quantise + entropy coding about 25 ms (HQ: 55 ms).
- AVX2 DCT (0.10.2): `fdct_get_avx2` in prores-aw.c runs fdct_10's integer algorithm on 8 lines at once (32-bit lanes, mullo/srai, 16-bit truncation where fdct_10 stores int16), chosen at the first pa_create when the CPU has AVX2 (cpuid + xgetbv), plain C otherwise; the log says `aw+: DCT uses AVX2`. Verified: `prores-aw-test selftest 4000000` = 0 mismatching blocks (random, all-0/1023, checkerboards); whole files with AVX2 still byte-identical to FFmpeg at round8 0 (4 profiles at 4K, 1920x1080, 1918x1082) and identical to the C path at the shipped rounding (incl. 4K noise at HQ); `PA_NO_SIMD=1` forces C in the test tool. Speed, single thread 4K: Proxy 40 -> 31 ms, LT 47 -> 39, 422 63 -> 55, HQ 74 -> 67. In the test OBS (11 frames in flight): Proxy 38.6 -> 33.8 ms per frame, CPU 11.7% -> 11.0%; LT 47.1 -> 42.7 ms, 12.6% -> 12.1%. Smaller than the 35% estimated: the AVX2 DCT still costs about half the C one (transposes, slow 32-bit multiplies). The next lever would be entropy coding, now the larger share (about 25 ms of a Proxy frame).
- OxideAV ProRes encoder (0.7.0): `oxideav-prores` 0.1.1 + `oxideav-core` 0.1.42 (both MIT, pinned with `=` in rust/Cargo.toml), wrapped in `rust/src/lib.rs` (`ox_create/ox_encode/ox_destroy`) and `src/encoder-oxideav.c`, id `obs_prores_oxideav`. Uses `make_encoder_with_config` so the frame header carries colour tags and frame-rate code (the simple `encode_frame_*` functions leave them blank). Fixed quantiser per profile (no rate control), one frame per thread, so it relies on the frame-parallel pipeline: laptop 4K422 about 69 fps at 22 frames at once, 328 Mbit/s, 65.6 dB (benchmark in BENCHMARK-APV-OXIDEAV.md). Tested in the test OBS: all four profiles give the right FourCC, Rec. 709 tags, decode clean. Live test on the laptop with the AverMedia card (1 min each, 422): OxideAV 519 Mbit/s, 199 ms per frame, OBS CPU 30%; prores_aw 387 Mbit/s, 74 ms per frame, CPU 12%; no repeated frames in either. Quality per bit equal when re-encoding the same camera frames, so OxideAV has no advantage; prores_aw stays the recommended encoder.
- CPU comparison of the ProRes encoders (2026-11-03, laptop, test OBS, looped 4K clip, profile 422, 30 s window, share of 22 threads; scene alone costs 5.2%): prores_aw 15.2% (65 ms per frame, 1 repeat at start), OxideAV 31.2% (178 ms, no repeats), FilmCraft 66.0% (502 ms, 21.6% of frames repeated, overloaded). Encoder-only share: about 10%, 26% and 61%. Measured with `scripts/test-record.ps1` (prints "OBS CPU"; `-NoRecord` measures the scene alone).
- OxideAV live on the desktop (2026-11-03, plugin 0.7.1, Magewell + GH5S, 4K 29.97, profile 422, 1 min): 591 Mbit/s (4.5 GB), 161.5 ms per frame with 16 frames in flight (up to 32), OBS CPU 16% of 32 threads, no repeated frames; ProRes 422 (`apcn`), Rec. 709 tags, decodes clean. For comparison on the desktop: prores_aw about 8% and FilmCraft about 22% (earlier versions, different scene), so OxideAV is about 2x prores_aw there (2.6x on the laptop).
- Linux later: keep the code portable where practical (pthreads, libobs threading). Install paths differ.

## Test results (laptop: Core Ultra 9 185H, 16 cores / 22 threads, RTX 4070 Laptop)

Setup: real 4K capture (AverMedia Live Gamer Ultra 2.1), OBS 33.0.1 portable, Standard MOV, P216.

- **prores_aw**: passes. LT, 422 and HQ at 1 minute each; a 10-minute soak; 13 quick start/stop cycles. No repeated frames anywhere. Worker time per frame is about 72 ms (LT), 105 ms (422) and 196 ms (HQ), with about 5 ms on the OBS thread. CPU about 13%, the same as OBS's own Custom Output with the same encoder.
- **Rust**: overloaded on this CPU. About 54% of frames repeated; about 62% CPU.
- **prores_ks**: overloaded. About 95% of frames repeated; 54% CPU in the plugin, 87% in OBS's own Custom Output, where it also fails.
- The Custom Output "non monotonically increasing dts" error (stale packets after a quick restart) never appeared with the plugin.
- Files open in DaVinci Resolve. Premiere is not tested yet.

## Test results (desktop: Ryzen 9 9950X3D, 16 cores / 32 threads, RTX 5080)

Setup: Magewell Pro Capture HDMI 4K with a Panasonic GH5S, portable OBS 33.0.1, plugin 0.4.1, 4K 29.97, 422.

- **prores_aw**: keeps up. OBS thread 2.2 ms per frame; about 8% CPU.
- **Rust**: keeps up. OBS thread 2.5 ms per frame; about 22% CPU.
- **prores_ks**: overloaded under 0.4.1 (OBS reported 88.8% skipped; growth had stopped at 17 workers). OBS's own Custom Output prores_ks kept up on this machine at 67% CPU. Re-test with 0.4.2.
- Every desktop file, from any encoder and from Custom Output alike, has about 17–18% single-frame repeats (a repeat every 5–6 frames, about 24.7 unique fps). That comes from the camera signal, not the encoders: it looks like a 25p feed into a 29.97 canvas. Fix it at the camera (29.97p) or in OBS (25 fps).
- A fragmented MOV (Custom Muxer Settings `movflags=frag_keyframe+empty_moov+delay_moov`) froze Resolve on the timeline. Regular MOV is fine. The fragmented setting was a profile setting on that machine, not something the plugin sets.

## Open items

1. **NVENC files in Resolve**: import and play an NV200 recording from the desktop test (not checked yet).
2. Premiere check.
3. Hybrid MOV with these encoders: untested.
4. Linux build: not started.
5. Rust encoder speed: worth profiling; it is young code.

## Working rules agreed with Mike

- Don't commit or push without his go-ahead.
- Don't run the test scripts against his own OBS installs; use the `_test` copy.
- Judge recordings by OBS's repeated-frame counter (or by comparing frame hashes), not by frame counts or timestamps. Gapless timestamps don't prove that no frames were repeated.
- Keep outward-facing actions (publishing, creating repos, posting) behind explicit confirmation.
