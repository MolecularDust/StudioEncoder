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
