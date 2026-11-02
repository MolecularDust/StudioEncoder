//! Phase 1 harness: encode planar yuv422p10le frames with the FilmCraft ProRes crate and report speed.
//!
//! usage: prores-bench <input.yuv> <width> <height> <proxy|lt|422|hq> <in_flight> <out.mov> [fps]
//! in_flight = 1 uses slice-parallel encoding; N > 1 encodes N frames at once, each single-threaded.

mod mov;
mod psnr;

use filmcraft_prores::{ChromaFormat, Encoder, EncoderConfig, Frame, Profile};
use rayon::prelude::*;
use std::time::Instant;

fn load_frame(buf: &[u8], w: usize, h: usize) -> Frame {
    let cw = w / 2;
    let (ny, nc) = (w * h, cw * h);
    let rd = |off: usize, n: usize| -> Vec<u16> {
        buf[off * 2..(off + n) * 2].chunks_exact(2).map(|b| u16::from_le_bytes([b[0], b[1]])).collect()
    };
    let mut f = Frame::new(w as u32, h as u32, ChromaFormat::Yuv422, 10, false);
    f.y = rd(0, ny);
    f.cb = rd(ny, nc);
    f.cr = rd(ny + nc, nc);
    f
}

fn main() {
    let a: Vec<String> = std::env::args().collect();
    if a.len() == 6 && a[1] == "psnr" {
        psnr::run(&a[2], &a[3], a[4].parse().unwrap(), a[5].parse().unwrap());
        return;
    }
    if a.len() < 7 {
        eprintln!("usage: {} <input.yuv> <w> <h> <proxy|lt|422|hq> <in_flight> <out.mov> [fps]", a[0]);
        std::process::exit(2);
    }
    let (w, h): (usize, usize) = (a[2].parse().unwrap(), a[3].parse().unwrap());
    let profile = match a[4].as_str() {
        "proxy" => Profile::Proxy,
        "lt" => Profile::Lt,
        "422" => Profile::Standard,
        "hq" => Profile::Hq,
        p => panic!("unknown profile {p}"),
    };
    let in_flight: usize = a[5].parse().unwrap();
    let fps: u32 = a.get(7).map(|s| s.parse().unwrap()).unwrap_or(30);
    let data = std::fs::read(&a[1]).expect("read input");
    let frame_bytes = w * h * 2 * 2;
    let n = data.len() / frame_bytes;

    let mut cfg = EncoderConfig::new(profile, w as u32, h as u32);
    cfg.threads = in_flight == 1;
    let pool = rayon::ThreadPoolBuilder::new().build().unwrap();

    let frames: Vec<Frame> = (0..n).map(|i| load_frame(&data[i * frame_bytes..(i + 1) * frame_bytes], w, h)).collect();
    drop(data);

    let t0 = Instant::now();
    let packets: Vec<Vec<u8>> = if in_flight == 1 {
        frames.iter().map(|f| Encoder::with_config(cfg.clone()).encode(f).expect("encode")).collect()
    } else {
        pool.install(|| {
            frames
                .par_iter()
                .with_max_len(1)
                .map(|f| Encoder::with_config(cfg.clone()).encode(f).expect("encode"))
                .collect()
        })
    };
    let dt = t0.elapsed().as_secs_f64();
    let total: usize = packets.iter().map(Vec::len).sum();
    println!(
        "frames={n} time={dt:.2}s ms/frame={:.1} fps={:.1} avg_bytes={} Mbit/s@{fps}fps={:.0}",
        dt * 1000.0 / n as f64,
        n as f64 / dt,
        total / n,
        (total / n) as f64 * 8.0 * fps as f64 / 1e6
    );
    mov::write(&a[6], &packets, w as u32, h as u32, fps, profile.fourcc());
}
