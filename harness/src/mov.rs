//! Minimal QuickTime writer for ProRes frames (one sample per frame).

use std::io::Write;

fn atom(tag: &[u8; 4], body: &[u8]) -> Vec<u8> {
    let mut v = ((body.len() + 8) as u32).to_be_bytes().to_vec();
    v.extend_from_slice(tag);
    v.extend_from_slice(body);
    v
}

fn full(tag: &[u8; 4], ver_flags: u32, body: &[u8]) -> Vec<u8> {
    let mut b = ver_flags.to_be_bytes().to_vec();
    b.extend_from_slice(body);
    atom(tag, &b)
}

fn u32s(v: &[u32]) -> Vec<u8> {
    v.iter().flat_map(|x| x.to_be_bytes()).collect()
}

pub fn write(path: &str, packets: &[Vec<u8>], w: u32, h: u32, fps: u32, fourcc: [u8; 4]) {
    let ftyp = atom(b"ftyp", &[b"qt  ".as_slice(), &[0, 0, 2, 0], b"qt  "].concat());
    let mdat_size: usize = packets.iter().map(Vec::len).sum();
    let mdat_hdr_len = 8u64;
    let first_offset = ftyp.len() as u64 + mdat_hdr_len;

    let n = packets.len() as u32;
    let ts = fps * 1000;
    let dur = n * 1000;

    // sample description: ProRes visual sample entry with fiel + colr(nclc, BT.709)
    let mut e = vec![0u8; 6];
    e.extend_from_slice(&1u16.to_be_bytes()); // data ref index
    e.extend_from_slice(&[0, 0, 0, 0]); // version, revision
    e.extend_from_slice(b"apl0");
    e.extend_from_slice(&0u32.to_be_bytes()); // temporal quality
    e.extend_from_slice(&1023u32.to_be_bytes()); // spatial quality
    e.extend_from_slice(&(w as u16).to_be_bytes());
    e.extend_from_slice(&(h as u16).to_be_bytes());
    e.extend_from_slice(&0x0048_0000u32.to_be_bytes());
    e.extend_from_slice(&0x0048_0000u32.to_be_bytes());
    e.extend_from_slice(&0u32.to_be_bytes());
    e.extend_from_slice(&1u16.to_be_bytes()); // frame count
    e.extend_from_slice(&[0u8; 32]); // compressor name
    e.extend_from_slice(&24u16.to_be_bytes()); // depth
    e.extend_from_slice(&0xFFFFu16.to_be_bytes()); // colour table id
    e.extend(atom(b"fiel", &[1, 0]));
    e.extend(atom(b"colr", &[b"nclc".as_slice(), &1u16.to_be_bytes(), &1u16.to_be_bytes(), &1u16.to_be_bytes()].concat()));
    let stsd = full(b"stsd", 0, &[u32s(&[1]), atom(&fourcc, &e)].concat());

    let stts = full(b"stts", 0, &u32s(&[1, n, 1000]));
    let stsc = full(b"stsc", 0, &u32s(&[1, 1, n, 1]));
    let mut sz = vec![0u32, n];
    sz.extend(packets.iter().map(|p| p.len() as u32));
    let stsz = full(b"stsz", 0, &u32s(&sz));
    let stco = full(b"stco", 0, &u32s(&[1, first_offset as u32]));
    let stbl = atom(b"stbl", &[stsd, stts, stsc, stsz, stco].concat());

    let dinf = atom(b"dinf", &full(b"dref", 0, &[u32s(&[1]), full(b"url ", 1, &[])].concat()));
    let vmhd = full(b"vmhd", 1, &[0u8; 8]);
    let minf = atom(b"minf", &[vmhd, dinf, stbl].concat());
    let hdlr = full(b"hdlr", 0, &[u32s(&[0]), b"vide".to_vec(), vec![0u8; 12], b"VideoHandler\0".to_vec()].concat());
    let mdhd = full(b"mdhd", 0, &u32s(&[0, 0, ts, dur, 0x55c4_0000]));
    let mdia = atom(b"mdia", &[mdhd, hdlr, minf].concat());
    let mut tk = u32s(&[0, 0, 1, 0, dur]);
    tk.extend_from_slice(&[0u8; 8 + 2 + 2 + 2 + 2]);
    tk.extend(u32s(&[0x10000, 0, 0, 0, 0x10000, 0, 0, 0, 0x4000_0000]));
    tk.extend(u32s(&[w << 16, h << 16]));
    let tkhd = full(b"tkhd", 3, &tk);
    let trak = atom(b"trak", &[tkhd, mdia].concat());
    let mut mv = u32s(&[0, 0, ts, dur, 0x10000]);
    mv.extend_from_slice(&[0x01, 0x00, 0, 0]);
    mv.extend_from_slice(&[0u8; 8]);
    mv.extend(u32s(&[0x10000, 0, 0, 0, 0x10000, 0, 0, 0, 0x4000_0000]));
    mv.extend_from_slice(&[0u8; 24]);
    mv.extend(u32s(&[2]));
    let mvhd = full(b"mvhd", 0, &mv);
    let moov = atom(b"moov", &[mvhd, trak].concat());

    let mut f = std::io::BufWriter::new(std::fs::File::create(path).expect("create mov"));
    f.write_all(&ftyp).unwrap();
    f.write_all(&((mdat_size + 8) as u32).to_be_bytes()).unwrap();
    f.write_all(b"mdat").unwrap();
    for p in packets {
        f.write_all(p).unwrap();
    }
    f.write_all(&moov).unwrap();
}
