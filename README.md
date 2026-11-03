# StudioEncoders: editing-friendly encoders for OBS Studio 33

An OBS Studio plugin for Windows. It adds ProRes-compatible video encoders and an all-intra H.264 encoder to **Standard** recording mode, for 4K, 10-bit, 4:2:2 recording to MOV.

*Renamed in 0.9.0 (it was called obs-prores). The plugin folder is now `obs-studioencoders`; delete the old `obs-prores` folder when you update.*

> **Not Apple ProRes.** This plugin is unofficial and is not affiliated with or endorsed by Apple. Apple ProRes is a licensed, certified codec. The encoders here are independent implementations of the publicly documented format (SMPTE RDD 36): FFmpeg's `prores_aw` and `prores_ks`, the FilmCraft Rust encoder and the OxideAV Rust encoder. Their output plays in common editors, such as DaVinci Resolve, but it is not certified by Apple.

## Encoders

| Encoder in OBS | Implementation | Notes |
|---|---|---|
| ProRes (FFmpeg prores_aw, fast) | FFmpeg `prores_aw` | **Recommended.** Light on CPU; real-time 4K30 on a laptop CPU at LT, 422 and HQ. |
| ProRes (FFmpeg prores_ks) (experimental) | FFmpeg `prores_ks` | Much heavier. Needs a lot of CPU at 4K. |
| ProRes-compatible (Rust / FilmCraft) (experimental) | [FilmCraft](https://github.com/storytold/filmcraft) Rust encoder | Experimental. Heavy CPU use at 4K. |
| ProRes (OxideAV, pure Rust) (experimental) | [OxideAV](https://github.com/oxideav/oxideav) `oxideav-prores` | Experimental. About 2.5 times the CPU of prores_aw at 4K (30% against 12% of a 22-thread laptop) and about a third bigger files at profile 422 on live camera footage, with no quality advantage per bit. Very young code (0.1). |
| H.264 10-bit 4:2:2 Intra (NVIDIA NVENC, RTX 50 series) | FFmpeg `h264_nvenc` | Smaller files on the GPU. Needs an **RTX 50-series** card. See below. |

Profiles: Proxy (`apco`), LT (`apcs`), 422 (`apcn`), HQ (`apch`).

### Choosing which encoders show up (Tools > StudioEncoders)

OBS's encoder list stays short by default. **Tools > StudioEncoders** opens a window listing every encoder in this plugin, split into **Recommended** (prores_aw and the NVENC H.264 encoder, both on) and **Experimental** (prores_ks, FilmCraft and OxideAV, all off). Tick the ones you want and press OK; the change takes effect after you restart OBS.

An encoder that is switched off is only hidden from the dropdown. A profile that already uses it keeps working. All encoders from this plugin start with "SE:" in OBS's encoder list so they are easy to spot.

The NVENC H.264 encoder checks your graphics card when OBS starts. If the card cannot encode H.264 10-bit 4:2:2 (anything older than an RTX 50 series), the encoder is hidden from the list and shown grayed out in this window.

### H.264 10-bit 4:2:2 Intra (NVIDIA NVENC)

A space-saving alternative to ProRes, encoded on the GPU. Every frame is a keyframe (all-intra), High 4:2:2 profile, 10-bit, constant bitrate, so it edits like ProRes but is smaller at a small cost in quality.

- **Needs an NVIDIA RTX 50-series GPU** (Blackwell) and a current driver. Older NVIDIA cards can't encode H.264 10-bit or 4:2:2; on those the recording won't start and OBS shows an error. Use ProRes (prores_aw) there.
- **Bitrate**: 100 Mbit/s (small, about ProRes LT quality), **200 (default**, better than ProRes LT, about half the size of ProRes 422), 300 (high). Above 400 adds nothing.
- **Speed**: Quality (p5, default) or Fast (p1, slightly lower quality).
- Frames go to the GPU as P216 without conversion. At most 2 frames are in flight, because consumer NVIDIA cards limit the number of encoder sessions.
- In an FFmpeg test on an RTX 5080 (4K 29.97 camera footage) this ran at about 77 fps at p5 and 101 fps at p1.

## Requirements

- Windows 10/11 x64
- OBS Studio 33.0.x (built against 33.0.0; tested on 33.0.1)

## Install

Close OBS, then copy the `obs-studioencoders` folder from the release zip:

- **Installed OBS:** to `C:\ProgramData\obs-studio\plugins\obs-studioencoders\`. `ProgramData` is hidden; type the path into File Explorer.
- **Portable OBS:** to `<OBS folder>\plugins\obs-studioencoders\`, next to `bin`, `data` and `config`.

If you had an older version, delete its `obs-prores` folder from the same place first, or OBS will load two copies.

The folder must contain `obs-studioencoders.dll` and `data\locale\`. In OBS, Tools → Plugin Manager should list "StudioEncoders", and Tools → StudioEncoders opens its settings.

Do not use the old `obs-plugins\64bit` or `data\obs-plugins` folders. OBS 33 marks those locations as legacy.

## Settings

1. Settings → Advanced: **Color Format P216**, Color Space Rec. 709, Color Range Limited.
2. Settings → Output: Output Mode **Advanced**, Recording tab, Type **Standard**, Recording Format **MOV**.
3. Video Encoder: one of the encoders above. In its settings:
   - **Profile** (ProRes encoders): Proxy, LT, 422 or HQ. **Bitrate** and **Speed** (NVENC encoder): see above.
   - **Frames in flight**: 0 (automatic, recommended). Frames are encoded in parallel, one per CPU core, as needed.
   - **Automatic sizing** (only with Frames in flight 0): *Grow when frames are late* (default) or *Size from measured frame cost* (experimental). The second starts with 4 workers, times a frame, then sets the number of workers needed in one step. That uses less memory for light encoders and ramps up faster for heavy ones. CPU load is the same either way.
   - **On overload**: *Warn in the log* or *Stop recording*.
   - **Log verbosity**: Info shows timing and automatic tuning.

## Overload

If an encoder cannot keep up, OBS repeats the previous frame to keep the timeline, and the recording looks frozen in those places. The plugin counts those repeated frames for each recording (it notices when OBS hands it the same frame twice in a row):
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
- `oxideav-prores` and `oxideav-core` (MIT), the OxideAV ProRes encoder.
