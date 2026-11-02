# Phase 0 design: OBS ProRes encoder plugin (Windows, OBS 33)

Status: DRAFT for approval. No plugin code written.

Reference sources: `_ref/obs-studio` at tag `33.0.1` (blobless clone), `_ref/obs-plugintemplate` at `3e7d7ac` (Dec 2025).
Installed locally: OBS Studio **33.0.0** (winget). 33.0.1 was not offered by winget ("no newer package versions"). Needs a manual install from obsproject.com to match the reference tree.

## 0. Decisions (from Mike, 2026-11-02)

- FFmpeg backend: **option (a)**, build against the obs-deps FFmpeg headers and import libs; keep a backend seam so option (b) can replace it.
- Target OBS **33.0.0** (installed). 33.0.1 not required.
- Input format **P216** (OBS 33 has no I210 or P210 output); the plugin implements the P216 → 10-bit 4:2:2 conversion itself. Mike confirmed P216 works with OBS's built-in FFmpeg in Advanced mode and produced 4K 10-bit 4:2:2 ProRes files.
- Linux build is a possible later goal: keep the code free of Windows-only calls. Rust, CMake, Ninja and the plugin template are cross-platform; MSVC is not.
- Toolchain: VS 2026 Build Tools (generator "Visual Studio 18 2026", SDK 10.0.26100.0, from OBS 33 `CMakePresets.json`), CMake, Rust (rustup), Ninja.

## 1. 4K 10-bit 4:2:2 recording: is it supported?

**Yes, through P216. Not through I210.** This is the most important finding in Phase 0.

| Format | Shows in Settings → Advanced → Color Format | OBS 33.0.1 output pipeline | Use |
|---|---|---|---|
| `VIDEO_FORMAT_P216` | yes (`OBSBasicSettings.cpp:1057`) | yes: conversion techs `P216_{SRGB,PQ,HLG}_{Y,UV}` (`obs.c:109-121`), CPU plane copy in `obs-video.c:710` | **Primary** |
| `VIDEO_FORMAT_I210` | **no** | **no**: `obs-video.c:745` lists it under `/* unimplemented */`; no conversion tech in `obs.c`. Only exists as an input format (`format_conversion.effect` `I210_Reverse`) | Not usable as output |

So the prompt's preference (I210, fall back to P216) collapses to **P216 only**. Consequences:

- The user must set Color Format = **P216**, Color Space = Rec. 709, Range = Limited. The GUI already shows a precision warning for P216 (`FormatWarningPreciseSdr`).
- The encoder receives P216 directly with no scaling mix, because Standard recording does not call `obs_encoder_set_preferred_video_format` (only streaming paths do: `SimpleOutput.cpp:369`, `AdvancedOutput.cpp:257`). The plugin should **not** set a preferred format, so the user's choice is what it gets.
- Plugin gets two planes: plane 0 = Y, 16 bit per sample (row = `width*2` bytes); plane 1 = interleaved U,V pairs, 16 bit each, full height, half width (row = `width*2` bytes). Confirm with a test frame dump before relying on it.
- **Open:** whether the 10-bit value is stored in the top 10 bits (value `<<6`) or in full 16-bit normalised scale. The shader writes normalised floats, so the plugin must check this on a real frame (e.g. a known grey ramp) before encoding. Mark as **[verify on hardware]**.
- **Performance risk (4K):** P216 is 2 × 2 bytes/pixel luma-plus-chroma for 4:2:2, i.e. 4 bytes/pixel. At 3840×2160 that is about 33 MB per frame, about 1 GB/s at 30 fps, for CPU readback and for the encoder queue. Frames-in-flight limits must be sized against this.

**Confirmed for 4K recording:** I found no resolution cap in the output path for P216 in the source I read. Verify the actual 4K30 P216 path by running OBS (Phase 2 acceptance), not just by reading code.

## 2. Plugin loading and install layout (OBS 33.0.1 source)

Read from `frontend/plugin-manager/PluginModuleLoader_Windows.cpp`:

- Root path: `C:\ProgramData\obs-studio\plugins\<Plugin>\<Plugin>.dll`, data at `...\<Plugin>\data`. Confirmed.
- Portable: `<OBS root>\..\..\plugins\<Plugin>\` (relative to `bin\64bit`). Confirmed.
- Legacy types still scanned: `...\obs-studio\plugins\<Plugin>\bin\<Plugin>.dll` (ProgramData), `obs-plugins\64bit\` (portable/legacy). Loaded as `OBS_MODULE_TYPE_LEGACY_PLUGIN`, flagged Legacy in Plugin Manager (`InstalledPluginRow.cpp:95`), and removed in a later major version.
- Duplicate handling (`libobs/obs-module.c:518`): if a plugin name is already loaded, later copies are skipped with a log warning. The new location is loaded first, so a stale legacy copy would be skipped, not loaded. The install script must still remove legacy copies to avoid confusion.
- Safe Mode: `is_safe_module()` and related checks run before load, so third-party plugins are skipped when Safe Mode is on. Matches the prompt.

## 3. DLL search path (answers prompt item 5)

`libobs/util/platform-windows.c:92`: when a plugin is loaded by full path, OBS calls
`LoadLibraryExW(fullpath, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS)`.

So the plugin's own folder is searched for the plugin's direct dependencies. Bundled FFmpeg DLLs placed next to `<Plugin>.dll` should resolve.
**[verify on hardware]:** transitive dependencies (bundled `avcodec` → `avutil`) use the same folder, but this is not documented. Test with a clean OBS install.

Also: `obs-ffmpeg-mux.exe` runs with `SetDllDirectoryW(L"")`, a separate process. Keep the Rust crate statically linked so it needs no DLL.

## 4. FFmpeg version (answers prompt item 4)

The installed OBS 33.0.0 ships `avcodec-62.dll` (62.28.102), `avformat-62`, `avutil-60`, `avfilter-11`, `swscale-9`.
libavcodec major 62 corresponds to the FFmpeg 8.x series. **[verify]** the exact FFmpeg release from obs-deps 2026-10-23 (not in the tree I cloned).
The OBS 33 `FindFFmpeg.cmake` finder expects the obs-deps layout, so headers and import libs must come from the same obs-deps release.

## 5. OBS 33 encoder API (answers prompt item 7)

**[verify, partially done]:** A full 32.2.2 → 33.0.1 diff of `obs-encoder.h` and `video-io.h` is not yet run. Found so far:
- `OBS_ENCODER_CAP_*` flags unchanged in the list I read (`obs-encoder.h:35-41`).
- `get_video_info` callback exists (`obs-encoder.h:316`), as the prompt describes.
- `obs_encoder_set_preferred_video_format` exists (`obs-encoder.c:1926`).

Next step: run the diff and report any deprecations.

## 6. Mux codec_type / FourCC (prompt section 3)

Read from `plugins/obs-ffmpeg/obs-ffmpeg-mux.c:141`: `codec_type` is an int in the video encoder's settings. The mux byte-swaps it on little-endian, then writes it as the container codec tag.

So the plugin must store each FourCC as the big-endian reading of its four characters: `apcn` = `0x6170636E`, `apco` = `0x6170636F`, `apcs` = `0x61706373`, `apch` = `0x61706368`. Derived from the swap, not yet run. **[verify on hardware]** by checking the tag in the output MOV with ffprobe.

Also confirmed: OBS 33 `obs-encoder.h` and `video-io.h` are unchanged from 32.2.2 to 33.0.1 (empty diff). `obs-module.h` changed only the `obs_module_unload` comment: it now says libobs objects are still valid during unload, and the module must not call libobs APIs after returning. That matches the prompt's item 6.

## 7. Template status

`obs-plugintemplate` `main` (Dec 2025) pins **OBS 31.1.1** sources and obs-deps `2025-07-11` in `buildspec.json`. It does **not** target OBS 33 out of the box. The plugin will need the template's CMake structure, with updated buildspec pins for 33.0.1 and obs-deps 2026-10-23 (the `CMakePresets.json` in OBS 33 uses that release).

## 8. FFmpeg option (prompt item 6)

Two ways to get `prores_ks` into the plugin:

**(a) Build against obs-deps FFmpeg headers and import libs (the DLLs OBS already ships).**
- Pro: the plugin uses the same FFmpeg build as OBS, so no DLL duplication and no second FFmpeg in the process.
- Pro: no extra FFmpeg binaries to ship or license-audit.
- Con: tied to one OBS version's FFmpeg. OBS 34 may use FFmpeg 9 and break the ABI.
- Con: you must read obs-deps internals, which OBS does not promise to keep stable for plugins.
- Con: the plugin can only use what that FFmpeg build compiled in (prores_ks is standard, so this is fine).

**(b) Bundle a separate LGPL shared FFmpeg build next to the plugin DLL.**
- Pro: you control the FFmpeg version and config; it can be pinned and tested independently of OBS.
- Pro: no ABI coupling to OBS's internal FFmpeg.
- Con: a second FFmpeg in the same process as OBS's own FFmpeg. Symbol clashes are possible if both are loaded in one address space (usually avoided by loading only your copy inside your own code, but this needs care).
- Con: ships several large DLLs (~20 to 40 MB). LGPL obligations apply: provide source offer, keep it dynamically linked, allow DLL replacement.
- Con: the DLL search-path behaviour in section 3 must be tested with bundled DLLs.

**My recommendation for Phase 2:** start with (a), since it matches the OBS 33 FFmpeg and avoids a second FFmpeg in process, and keep the backend behind a seam so (b) can replace it if OBS changes its FFmpeg. Decide after the section 5 diff and a test that `prores_ks` loads from OBS's own `avcodec-62.dll`.

## 9. Architecture (unchanged from prompt, with P216 change)

- `src/plugin-main.c`: module load, registers both encoders.
- `src/encoder-common.*`: properties, codec_type, frame queue, logging, colour tags. Input is always P216.
- `src/encoder-rust.c`: Rust backend via C ABI.
- `src/encoder-ffmpeg.c`: `prores_ks` backend.
- `rust/`: `staticlib`, FFI limited to `pr_create`, `pr_encode`, `pr_destroy`, `pr_last_error`. `catch_unwind` at boundary.
- P216 → backend input: the Rust backend takes Y and UV planes plus a de-interleave step if needed. FFmpeg takes `yuv422p10le`, so the plugin de-interleaves UV into planes and shifts to 10-bit.

## 10. Risks

1. P216 bit alignment unknown (section 1). Mitigation: frame dump test in Phase 1.
2. 4K P216 readback bandwidth (~1 GB/s). Mitigation: frames-in-flight limit, drop-and-log.
3. FFmpeg ABI coupling to OBS (option a). Mitigation: backend seam.
4. Rust encoder is young and unbenchmarked on x86 (prompt section 3). Phase 1 gates it.
5. Template is stale (OBS 31.1.1). Mitigation: update buildspec pins before first build.
6. Hybrid MOV is untested on Windows third-party encoders.
7. Installed OBS is 33.0.0, not 33.0.1.

## 11. Test plan

- Phase 1: encode a reference clip as P216 frames with (a) FilmCraft crate, (b) `prores_ks` default, (c) `prores_ks` with `vendor=apl0`. Measure ms/frame at 4K30, quality (PSNR/SSIM/VMAF), Resolve/Premiere open.
- Phase 2: OBS 33 acceptance tests from prompt section 3a, plus a P216 4K30 MOV and MKV recording.
- Phase 3: 1-hour soak, overload, remux, colour tag check.

## Open verify items

| Item | Status |
|---|---|
| Installed OBS version vs 33.0.1 | open: install 33.0.1 manually |
| I210 unsupported as output | resolved: P216 only |
| P216 bit alignment | open: hardware test |
| P216 4K output has no resolution cap | partly: no cap found in source; test in OBS |
| Plugin DLL search, transitive deps | open: test |
| FFmpeg version of obs-deps 2026-10-23 | open |
| 32.2.2 → 33.0.1 encoder header diff | resolved: no change in obs-encoder.h / video-io.h |
| `codec_type` / FourCC in obs-ffmpeg-mux | resolved from source; hardware check open |
| Mockup | done: docs/phase0-mockup.html |
| Hybrid MOV on Windows | open (Phase 3) |
| Rust backend x86 performance | open (Phase 1) |
| `obs-plugintemplate` for OBS 33 | open: update pins |
