//! PSNR between two raw yuv422p10le files (per plane, 10-bit peak 1023).

pub fn run(a: &str, b: &str, w: usize, h: usize) {
    let (da, db) = (std::fs::read(a).expect("read a"), std::fs::read(b).expect("read b"));
    let (ny, nc) = (w * h, (w / 2) * h);
    let fb = (ny + 2 * nc) * 2;
    let n = da.len().min(db.len()) / fb;
    let mut se = [0f64; 3];
    let mut cnt = [0f64; 3];
    let mut maxdiff = 0i32;
    let mut maxval = 0u16;
    for f in 0..n {
        let (fa, fbuf) = (&da[f * fb..(f + 1) * fb], &db[f * fb..(f + 1) * fb]);
        for (p, (off, len)) in [(0, ny), (ny, nc), (ny + nc, nc)].into_iter().enumerate() {
            for i in off..off + len {
                let x = u16::from_le_bytes([fa[i * 2], fa[i * 2 + 1]]);
                let y = u16::from_le_bytes([fbuf[i * 2], fbuf[i * 2 + 1]]);
                maxval = maxval.max(x).max(y);
                let d = x as i32 - y as i32;
                maxdiff = maxdiff.max(d.abs());
                se[p] += (d * d) as f64;
                cnt[p] += 1.0;
            }
        }
    }
    let db_of = |p: usize| 10.0 * (1023.0f64 * 1023.0 / (se[p] / cnt[p]).max(1e-12)).log10();
    println!(
        "frames={n} maxsample={maxval} maxdiff={maxdiff} PSNR Y={:.2} Cb={:.2} Cr={:.2}",
        db_of(0),
        db_of(1),
        db_of(2)
    );
}
