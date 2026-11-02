# Handoff: OBS ProRes plugin

For a new Claude Code session on another machine. Read this first, then `README.md`, `docs/phase0-design.md` and `docs/phase1-report.md`.

## What this is

A Windows OBS 33 plugin (`obs-prores`) that adds three ProRes video encoders for Standard recording, 4K 10-bit 4:2:2, from OBS's P216 output format:

1. **ProRes (FFmpeg prores_aw, fast)**: FFmpeg's `prores_aw` (the same code as FFmpeg's default `prores`). Recommended.
2. **ProRes (FFmpeg prores_ks)**: FFmpeg's `prores_ks`. Heavy CPU use.
3. **ProRes-compatible (Rust / FilmCraft)**: Rust encoder from the FilmCraft project (MIT OR Apache-2.0), pinned to commit `68919ed87aa6a097a4d21414ec7c438883f5fe22`. Heavy CPU use.

Unofficial; not Apple ProRes. Keep that wording in any user-facing text.

Current version: **0.4.1**.

## Layout

- `src/`: C glue. `encoder-common.c/.h` has the shared pipeline, P216 conversion and overload check. `encoder-ffmpeg.c` has both FFmpeg encoders. Also `encoder-rust.c`, `prores-rs.h` and `plugin-main.c`.
- `rust/`: Rust C-ABI staticlib (`src/lib.rs`) wrapping the FilmCraft crate. Built by CMake through cargo.
- `harness/`: Rust benchmark CLI used in phase 1 (`prores-bench`).
- `scripts/install.ps1`: installs into a portable OBS (`-Portable <folder>`), or into the system OBS at `C:\ProgramData\obs-studio\plugins`. Removes legacy-layout copies.
- `scripts/package.ps1`: builds `obs-prores-<version>-windows-x64.zip` for portable OBS.
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
- Automatic frames in flight is the default. It starts at half the logical CPUs, grows while the oldest frame is late, and stops when another worker no longer raises throughput.
- Stopping costs no frames. OBS keeps feeding the encoder until a packet past the stop point arrives.
- Overload detection reads OBS's repeated-frame counter (`video_output_get_skipped_frames`) once a second. It warns every overloaded second, writes a summary at the end, and can optionally stop the recording after 3 overloaded seconds in a row.
- Linux later: keep the code portable where practical (pthreads, libobs threading). Install paths differ.

## Test results (laptop: Core Ultra 9 185H, 16 cores / 22 threads, RTX 4070 Laptop)

Setup: real 4K capture (AverMedia Live Gamer Ultra 2.1), OBS 33.0.1 portable, Standard MOV, P216.

- **prores_aw**: passes. LT, 422 and HQ at 1 minute each; a 10-minute soak; 13 quick start/stop cycles. No repeated frames anywhere. Worker time per frame is about 72 ms (LT), 105 ms (422) and 196 ms (HQ), with about 5 ms on the OBS thread. CPU about 13%, the same as OBS's own Custom Output with the same encoder.
- **Rust**: overloaded on this CPU. About 54% of frames repeated; about 62% CPU.
- **prores_ks**: overloaded. About 95% of frames repeated; 54% CPU in the plugin, 87% in OBS's own Custom Output, where it also fails.
- The Custom Output "non monotonically increasing dts" error (stale packets after a quick restart) never appeared with the plugin.
- Files open in DaVinci Resolve. Premiere is not tested yet.

## Open items

1. **Desktop (Ryzen 9950X3D, 32 threads, RTX 5080)**: system-wide OBS 33 install. Put the plugin in `C:\ProgramData\obs-studio\plugins\obs-prores\`. It wasn't loading at the time of this handoff. Check the OBS version, check whether the log says "Portable mode: true", and search the log for `obs-prores`. Then run all three encoders at 422 (and HQ) to see whether Rust and prores_ks keep up there.
2. Premiere check.
3. Hybrid MOV with these encoders: untested.
4. Linux build: not started.
5. Rust encoder speed: worth profiling; it is young code.

## Working rules agreed with Mike

- Don't commit or push without his go-ahead.
- Don't run the test scripts against his own OBS installs; use the `_test` copy.
- Judge recordings by OBS's repeated-frame counter (or by comparing frame hashes), not by frame counts or timestamps. Gapless timestamps don't prove that no frames were repeated.
- Keep outward-facing actions (publishing, creating repos, posting) behind explicit confirmation.
