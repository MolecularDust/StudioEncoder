# obs-prores: ProRes-compatible encoders for OBS Studio 33

An OBS Studio plugin for Windows. It adds ProRes-compatible video encoders to **Standard** recording mode, for 4K, 10-bit, 4:2:2 recording to MOV.

> **Not Apple ProRes.** This plugin is unofficial and is not affiliated with or endorsed by Apple. Apple ProRes is a licensed, certified codec. The encoders here are independent implementations of the publicly documented format (SMPTE RDD 36): FFmpeg's `prores_aw` and `prores_ks`, and the FilmCraft Rust encoder. Their output plays in common editors, such as DaVinci Resolve, but it is not certified by Apple.

## Encoders

| Encoder in OBS | Implementation | Notes |
|---|---|---|
| ProRes (FFmpeg prores_aw, fast) | FFmpeg `prores_aw` | **Recommended.** Light on CPU; real-time 4K30 on a laptop CPU at LT, 422 and HQ. |
| ProRes (FFmpeg prores_ks) | FFmpeg `prores_ks` | Much heavier. Needs a lot of CPU at 4K. |
| ProRes-compatible (Rust / FilmCraft) | [FilmCraft](https://github.com/storytold/filmcraft) Rust encoder | Experimental. Heavy CPU use at 4K. |

Profiles: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`).

## Requirements

- Windows 10/11 x64
- OBS Studio 33.0.x (built against 33.0.0; tested on 33.0.1)

## Install

Close OBS, then copy the `obs-prores` folder from the release zip:

- **Installed OBS:** to `C:\ProgramData\obs-studio\plugins\obs-prores\`. `ProgramData` is hidden; type the path into File Explorer.
- **Portable OBS:** to `<OBS folder>\plugins\obs-prores\`, next to `bin`, `data` and `config`.

The folder must contain `obs-prores.dll` and `data\locale\`. In OBS, Tools → Plugin Manager should list "ProRes-compatible encoders".

Do not use the old `obs-plugins\64bit` or `data\obs-plugins` folders. OBS 33 marks those locations as legacy.

## Settings

1. Settings → Advanced: **Color Format P216**, Color Space Rec. 709, Color Range Limited.
2. Settings → Output: Output Mode **Advanced**, Recording tab, Type **Standard**, Recording Format **MOV**.
3. Video Encoder: one of the encoders above. In its settings:
   - **Profile**: Proxy, LT, 422 or HQ.
   - **Frames in flight**: 0 (automatic, recommended). Frames are encoded in parallel, one per CPU core, as needed.
   - **On overload**: *Warn in the log* or *Stop recording*.
   - **Log verbosity**: Info shows timing and automatic tuning.

## Overload

If an encoder cannot keep up, OBS repeats the previous frame to keep the timeline, and the recording looks frozen in those places. The plugin reads OBS's own count of repeated frames:
- It writes an `OVERLOAD` warning to the log for every second in which that happens, and a summary when the recording ends.
- With **On overload: Stop recording**, it ends the recording after 3 overloaded seconds in a row and shows an error.

If you see overload warnings, switch to the prores_aw encoder or a lighter profile.

## Known limits

- Input must be P216. OBS 33 offers no other 10-bit 4:2:2 output format.
- Frames captured after you press Stop, but still encoding, are discarded. This is expected; the recording ends at the stop point.
- Hybrid MOV is untested.
- Windows only for now.

## Build

See `docs/HANDOFF.md` for toolchain and build steps.

## License

GPL-2.0-or-later (see `LICENSE`), as required for OBS plugins. Third-party code:
- FFmpeg (LGPL), as shipped with OBS.
- FilmCraft ProRes crate (MIT OR Apache-2.0, used under MIT).
