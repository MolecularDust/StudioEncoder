# Phase 1 report: validation harness (first pass)

Status: **first pass, synthetic clip only. The go criteria are not met on this clip.** Real footage is needed before a final decision.

## Setup

- CPU: Intel Core Ultra 9 185H (16 cores, 22 threads), Windows 11.
- Harness: `harness/` (Rust CLI `prores-bench`). Encodes planar yuv422p10le frames with the FilmCraft `filmcraft-prores` crate, writes a MOV, and has its own PSNR check (ffmpeg's psnr filter gave impossible values on my first, invalid clip, so I did not use it).
- FilmCraft pinned at commit `68919ed87aa6a097a4d21414ec7c438883f5fe22` (`storytold/filmcraft`, MIT OR Apache-2.0). The crate depends on `filmcraft-bitstream`, which re-exports `craft-media` from `storytold/craft-libs` rev `c5a1cbfb5f9049f9270955caffb5bee5997aebf7`. No standalone FilmCraft ProRes repo exists that I could find; `safewords/rivet-prores` is a different project with a non-standard licence (`NOASSERTION`).
- FFmpeg comparison: ffmpeg 9.0.2 (Gyan full build) CLI, `prores_ks`, `-vendor apl0`, default quality settings. This is only a test tool, not shipped.
- Clip: 30 frames, 3840×2160, yuv422p10le, limited range (max sample 940). Synthetic: a zooming Mandelbrot with temporal noise. It is very detailed and noisy, so it is a worst case for speed and not representative of camera or screen footage.

**Correction:** my first run used an invalid clip (16-bit samples labelled as 10-bit). Those numbers were wrong and are discarded. All results below use the corrected clip.

## Results (4K, 30 frames)

Rust (FilmCraft), encode time only, best of 1 to 16 frames in flight:

| Profile | ms/frame | fps | Size | Rate at 30 fps |
|---|---|---|---|---|
| Proxy | 93 | 10.8 | 0.74 MB | 179 Mbit/s |
| 422 | 69 | 14.5 | 2.43 MB | 584 Mbit/s |
| HQ | 70 | 14.3 | 3.64 MB | 875 Mbit/s |

More frames in flight did not help: the crate already uses all cores for one frame, so throughput stays at about 70 ms/frame.

`prores_ks` via ffmpeg CLI (includes file read and write, so slightly pessimistic): Proxy 67 ms, LT 80 ms, 422 77 ms, HQ 86 ms per frame. Sizes 0.57, 1.79, 2.56, 3.86 MB/frame.

Quality (my PSNR against the source, all 30 frames, dB for Y / Cb / Cr):

| Profile | Rust | prores_ks | Size (Rust / ks) |
|---|---|---|---|
| Proxy | 49.7 / 50.3 / 49.8 | 37.2 / 36.5 / 36.4 | 0.74 / 0.57 MB |
| LT | 54.6 / 56.6 / 56.2 | 37.8 / 37.2 / 37.1 | 1.69 / 1.79 MB |
| 422 | 56.1 / 57.4 / 57.1 | 40.0 / 39.4 / 39.3 | 2.43 / 2.56 MB |
| HQ | 58.5 / 59.1 / 58.9 | 43.8 / 43.2 / 43.1 | 3.64 / 3.86 MB |

Rust sizes match Apple's nominal rates scaled to 4K30 (about 180, 408, 588, 880 Mbit/s).

All outputs decode in ffmpeg with no errors. ffprobe reports the correct profile and FourCC and BT.709 primaries, transfer and matrix from the `colr` atom my MOV writer adds.

## Against the go criteria

| Criterion | Result |
|---|---|
| At least 1.5x real-time at 4K30 (needs 22 ms/frame or better) | **Not met** by either encoder on this clip: Rust about 70 ms, `prores_ks` about 77 ms. |
| Quality at least equal to `prores_ks` at the same bitrate | Met on this clip, by a wide margin. This gap is larger than I would expect, so I do not trust it yet. |
| Zero decode errors | Met in ffmpeg. Not yet tested in Resolve or Premiere. |

## Caveats

- The clip is a worst case. Real footage would likely encode faster, but I cannot say whether by 3x.
- The 10 to 19 dB quality gap may come from this clip. `prores_ks` uses one quantiser per frame, while the Rust crate adapts per slice, which matters most on very uneven content. Do not read it as a general result until it is repeated on real footage.
- Not done yet: `prores_ks` with a tuned `qscale`, SSIM and VMAF, the Resolve and Premiere open tests, and any test on x86 with a different thread count.
- Both encoders being about 14 fps at 4K on a 16-core CPU is the main risk to real-time 4K30 recording. Options: lower resolution or frame rate, a lighter profile on real content, or profiling the Rust crate for speedups.

## Real footage (second pass)

Source: Mike's own OBS recording, `Downloads\2026-11-02 14-34-28.mov`: 3840×2160, ProRes LT, 10-bit 4:2:2, BT.709 limited, 29.97 fps, about 203 Mbit/s. A talking-head shot. I decoded 60 frames (from 1 s in) to raw yuv422p10le and re-encoded them with both encoders. Same CPU as before.

| Profile | Rust ms/frame (fps) | Rust size, Mbit/s @30 | prores_ks ms/frame* | prores_ks size, Mbit/s @30 |
|---|---|---|---|---|
| Proxy | 52 (19) | 179 | 32 | 123 |
| LT | 35 (28) | 405 | 52 | 278 |
| 422 | 33 (31) | 462 | 38 | 305 |
| HQ | 35 (28) | 462 | 38 | 341 |

*`prores_ks` timing is wall time of the ffmpeg command line, including process start, reading the raw file and writing the MOV, so it is a little pessimistic. Rust timing is encode only.

Findings:
- **Speed:** both encoders sit at roughly 30 fps for LT, 422 and HQ at 4K. That is about 1.0x real time, not the 1.5x headroom target. This matches your experience: OBS records 4K30 in real time but hits the occasional error.
- **Rust file sizes are fixed by profile** (it targets Apple's nominal rates), so on easy content it produces files about 1.4 to 2x larger than `prores_ks` for the same profile. Rust 422 and HQ came out byte-identical, so it looks like the encoder hit its finest quantiser before using the HQ budget. Worth checking in the crate.
- **Quality:** PSNR is 70+ dB for both. That figure is not informative here because the source is already ProRes LT, so re-encoding reproduces it almost exactly. Rust is a few dB higher, but it also uses more bits.
- **Not a clear win:** on real content the Rust crate is about as fast as `prores_ks` and not faster, with larger files. Its advantage on the noisy synthetic clip came from per-slice rate control, which matters little on clean footage.

Go criteria on real footage:

| Criterion | Result |
|---|---|
| 1.5x real time at 4K30 | Not met by either encoder (about 1.0x). |
| Quality at least equal to `prores_ks` at the same bitrate | Not shown. Rust is higher quality but at a higher bitrate. |
| Zero decode errors | Met in ffmpeg. Resolve and Premiere untested. |

## Next steps

1. Decide whether to continue given the numbers above (see the summary message).
2. Add a P216 input path to the harness, since that is what the plugin will receive.
3. Open the Rust MOV in Resolve and Premiere.
4. If continuing: profile the Rust crate, and check why 422 and HQ match.
