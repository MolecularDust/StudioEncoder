//! C ABI around the FilmCraft ProRes encoder, used by the OBS plugin.
//!
//! Input is planar 10-bit 4:2:2 (`u16`, low-aligned): Y is `width*height`, Cb and Cr are
//! `width/2*height` each. No panic crosses the boundary.

use filmcraft_prores::{ChromaFormat, ColorInfo, Encoder, EncoderConfig, Frame, Profile};
use std::cell::RefCell;
use std::ffi::{c_char, c_int, c_void, CString};
use std::panic::{catch_unwind, AssertUnwindSafe};

thread_local! {
    static LAST_ERROR: RefCell<CString> = RefCell::new(CString::default());
}

fn set_error(msg: impl Into<Vec<u8>>) {
    let msg = CString::new(msg).unwrap_or_else(|_| CString::new("error message contained NUL").unwrap_or_default());
    LAST_ERROR.with(|e| *e.borrow_mut() = msg);
}

struct Handle {
    enc: Encoder,
    frame: Frame,
}

fn profile_from(p: c_int) -> Option<Profile> {
    match p {
        0 => Some(Profile::Proxy),
        1 => Some(Profile::Lt),
        2 => Some(Profile::Standard),
        3 => Some(Profile::Hq),
        _ => None,
    }
}

/// Create an encoder. `profile`: 0 Proxy, 1 LT, 2 422, 3 HQ. Colour values are H.273 codes.
/// Returns NULL on failure (see `pr_last_error`).
#[no_mangle]
pub extern "C" fn pr_create(
    width: u32,
    height: u32,
    profile: c_int,
    slice_threads: c_int,
    primaries: u8,
    transfer: u8,
    matrix: u8,
) -> *mut c_void {
    let r = catch_unwind(AssertUnwindSafe(|| {
        let Some(profile) = profile_from(profile) else {
            set_error(format!("unsupported profile {profile}"));
            return std::ptr::null_mut();
        };
        if width == 0 || height == 0 || width % 2 != 0 {
            set_error(format!("unsupported frame size {width}x{height}"));
            return std::ptr::null_mut();
        }
        let mut cfg = EncoderConfig::new(profile, width, height);
        cfg.threads = slice_threads != 0;
        cfg.color = ColorInfo { primaries, transfer, matrix };
        let frame = Frame::new(width, height, ChromaFormat::Yuv422, 10, false);
        Box::into_raw(Box::new(Handle { enc: Encoder::with_config(cfg), frame })) as *mut c_void
    }));
    r.unwrap_or_else(|_| {
        set_error("panic in pr_create");
        std::ptr::null_mut()
    })
}

/// Encode one frame. Returns the number of bytes written to `out`, or -1 on error.
///
/// # Safety
/// `h` must come from `pr_create`; plane pointers must cover the sizes described above and
/// `out` must be valid for `out_cap` bytes.
#[no_mangle]
pub unsafe extern "C" fn pr_encode(
    h: *mut c_void,
    y: *const u16,
    cb: *const u16,
    cr: *const u16,
    out: *mut u8,
    out_cap: usize,
) -> i64 {
    if h.is_null() || y.is_null() || cb.is_null() || cr.is_null() || out.is_null() {
        set_error("null argument");
        return -1;
    }
    let r = catch_unwind(AssertUnwindSafe(|| {
        let handle = &mut *(h as *mut Handle);
        let f = &mut handle.frame;
        let (ny, nc) = (f.y.len(), f.cb.len());
        f.y.copy_from_slice(std::slice::from_raw_parts(y, ny));
        f.cb.copy_from_slice(std::slice::from_raw_parts(cb, nc));
        f.cr.copy_from_slice(std::slice::from_raw_parts(cr, nc));
        match handle.enc.encode(&handle.frame) {
            Ok(bytes) => {
                if bytes.len() > out_cap {
                    set_error(format!("packet of {} bytes exceeds buffer of {out_cap}", bytes.len()));
                    return -1;
                }
                std::slice::from_raw_parts_mut(out, bytes.len()).copy_from_slice(&bytes);
                bytes.len() as i64
            }
            Err(e) => {
                set_error(e.to_string());
                -1
            }
        }
    }));
    r.unwrap_or_else(|_| {
        set_error("panic in pr_encode");
        -1
    })
}

/// # Safety
/// `h` must come from `pr_create` (or be NULL) and must not be used afterwards.
#[no_mangle]
pub unsafe extern "C" fn pr_destroy(h: *mut c_void) {
    if !h.is_null() {
        let _ = catch_unwind(AssertUnwindSafe(|| drop(Box::from_raw(h as *mut Handle))));
    }
}

/// Last error on this thread. The pointer stays valid until the next call on the same thread.
#[no_mangle]
pub extern "C" fn pr_last_error() -> *const c_char {
    LAST_ERROR.with(|e| e.borrow().as_ptr())
}

// ---------------------------------------------------------------------------------------------
// OxideAV ProRes encoder (oxideav-prores). Same planar 10-bit 4:2:2 input as above.
// ---------------------------------------------------------------------------------------------

mod oxide {
    use super::{profile_from, set_error};
    use oxideav_core::{CodecId, CodecParameters, Encoder, Error, Frame, PixelFormat, Rational, VideoFrame, VideoPlane};
    use oxideav_prores::encoder::{make_encoder_with_config, EncoderConfig};
    use oxideav_prores::frame::{FrameMeta, Profile};
    use std::ffi::{c_int, c_void};
    use std::panic::{catch_unwind, AssertUnwindSafe};

    pub struct Handle {
        enc: Box<dyn Encoder>,
        frame: VideoFrame,
        next_pts: i64,
    }

    fn profile_for(p: c_int) -> Option<Profile> {
        match profile_from(p)? {
            filmcraft_prores::Profile::Proxy => Some(Profile::Proxy),
            filmcraft_prores::Profile::Lt => Some(Profile::Lt),
            filmcraft_prores::Profile::Standard => Some(Profile::Standard),
            filmcraft_prores::Profile::Hq => Some(Profile::Hq),
            _ => None,
        }
    }

    /// Create an OxideAV encoder. `profile`: 0 Proxy, 1 LT, 2 422, 3 HQ. Colour values are H.273 codes.
    #[no_mangle]
    pub extern "C" fn ox_create(
        width: u32,
        height: u32,
        profile: c_int,
        fps_num: u32,
        fps_den: u32,
        primaries: u8,
        transfer: u8,
        matrix: u8,
    ) -> *mut c_void {
        let r = catch_unwind(AssertUnwindSafe(|| {
            let Some(profile) = profile_for(profile) else {
                set_error(format!("unsupported profile {profile}"));
                return std::ptr::null_mut();
            };
            if width == 0 || height == 0 || width % 2 != 0 {
                set_error(format!("unsupported frame size {width}x{height}"));
                return std::ptr::null_mut();
            }
            let mut params = CodecParameters::video(CodecId::new("prores"));
            params.width = Some(width);
            params.height = Some(height);
            params.pixel_format = Some(PixelFormat::Yuv422P10Le);
            if fps_num > 0 && fps_den > 0 {
                params.frame_rate = Some(Rational::new(fps_num as i64, fps_den as i64));
            }
            let mut cfg = EncoderConfig::default().with_profile(profile);
            let rate_code = oxideav_prores::frame::frame_rate_code_from_rational(params.frame_rate.unwrap_or(Rational::new(0, 1)));
            cfg = cfg.with_meta(FrameMeta {
                aspect_ratio_information: 0,
                frame_rate_code: rate_code,
                color_primaries: primaries,
                transfer_characteristic: transfer,
                matrix_coefficients: matrix,
            });
            let enc = match make_encoder_with_config(&params, cfg) {
                Ok(e) => e,
                Err(e) => {
                    set_error(e.to_string());
                    return std::ptr::null_mut();
                }
            };
            let (w, h) = (width as usize, height as usize);
            let frame = VideoFrame {
                pts: None,
                planes: vec![
                    VideoPlane { stride: w * 2, data: vec![0; w * 2 * h] },
                    VideoPlane { stride: w, data: vec![0; w * h] },
                    VideoPlane { stride: w, data: vec![0; w * h] },
                ],
            };
            Box::into_raw(Box::new(Handle { enc, frame, next_pts: 0 })) as *mut c_void
        }));
        r.unwrap_or_else(|_| {
            set_error("panic in ox_create");
            std::ptr::null_mut()
        })
    }

    unsafe fn copy_plane(dst: &mut [u8], src: *const u16, samples: usize) {
        // samples are little-endian u16 on every supported target
        std::ptr::copy_nonoverlapping(src as *const u8, dst.as_mut_ptr(), samples * 2);
    }

    /// Encode one frame. Returns the number of bytes written to `out`, or -1 on error.
    ///
    /// # Safety
    /// `h` must come from `ox_create`; plane pointers must cover the sizes described at the top of the
    /// file and `out` must be valid for `out_cap` bytes.
    #[no_mangle]
    pub unsafe extern "C" fn ox_encode(
        h: *mut c_void,
        y: *const u16,
        cb: *const u16,
        cr: *const u16,
        out: *mut u8,
        out_cap: usize,
    ) -> i64 {
        if h.is_null() || y.is_null() || cb.is_null() || cr.is_null() || out.is_null() {
            set_error("null argument");
            return -1;
        }
        let r = catch_unwind(AssertUnwindSafe(|| {
            let handle = &mut *(h as *mut Handle);
            let f = &mut handle.frame;
            let ny = f.planes[0].data.len() / 2;
            let nc = f.planes[1].data.len() / 2;
            copy_plane(&mut f.planes[0].data, y, ny);
            copy_plane(&mut f.planes[1].data, cb, nc);
            copy_plane(&mut f.planes[2].data, cr, nc);
            f.pts = Some(handle.next_pts);
            handle.next_pts += 1;
            if let Err(e) = handle.enc.send_frame(&Frame::Video(f.clone())) {
                set_error(e.to_string());
                return -1;
            }
            match handle.enc.receive_packet() {
                Ok(pkt) => {
                    let bytes = &pkt.data;
                    if bytes.len() > out_cap {
                        set_error(format!("packet of {} bytes exceeds buffer of {out_cap}", bytes.len()));
                        return -1;
                    }
                    std::slice::from_raw_parts_mut(out, bytes.len()).copy_from_slice(bytes);
                    bytes.len() as i64
                }
                Err(Error::NeedMore) => {
                    set_error("encoder returned no packet");
                    -1
                }
                Err(e) => {
                    set_error(e.to_string());
                    -1
                }
            }
        }));
        r.unwrap_or_else(|_| {
            set_error("panic in ox_encode");
            -1
        })
    }

    /// # Safety
    /// `h` must come from `ox_create` (or be NULL) and must not be used afterwards.
    #[no_mangle]
    pub unsafe extern "C" fn ox_destroy(h: *mut c_void) {
        if !h.is_null() {
            let _ = catch_unwind(AssertUnwindSafe(|| drop(Box::from_raw(h as *mut Handle))));
        }
    }
}
