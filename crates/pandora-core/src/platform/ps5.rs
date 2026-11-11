//! Console implementation.
//!
//! **UNVALIDATED.** This module has never been executed on hardware. It is the
//! first milestone in `docs/ROADMAP.md`, and the FFI below is written against the
//! `libkernel` symbols exposed by the PS5 Payload SDK. Do not assume any of it
//! works until a hardware test report says so.
//!
//! Why it lives behind `platform` at all: when this module is wrong, the failure
//! should be a compile error or a single failing test here, not corruption
//! somewhere in the HTTP layer.

use super::{DirEntry, Platform};
use crate::error::Result;
use std::ffi::CString;
use std::os::raw::{c_char, c_int};
use std::path::{Path, PathBuf};

// `libkernel`: the subset used by this module. Signatures follow the public
// PS5 Payload SDK headers (`ps5-payload-dev/sdk`), not Sony's SDK.
extern "C" {
    fn sceKernelOpen(path: *const c_char, flags: c_int, mode: c_int) -> c_int;
    fn sceKernelClose(fd: c_int) -> c_int;
    fn sceKernelRead(fd: c_int, buf: *mut u8, nbyte: usize) -> isize;
    fn sceKernelWrite(fd: c_int, buf: *const u8, nbyte: usize) -> isize;
    fn sceKernelGetdents(fd: c_int, buf: *mut c_char, nbyte: usize) -> c_int;
}

const O_RDONLY: c_int = 0o0000;
const O_WRONLY: c_int = 0o0001;
const O_CREAT: c_int = 0o0100;
const O_TRUNC: c_int = 0o1000;

/// A platform backed by the console's own filesystem.
#[derive(Debug, Clone)]
pub struct Ps5Platform {
    root: PathBuf,
}

impl Ps5Platform {
    /// Creates the console platform, rooted at `/data/pandora`.
    #[must_use]
    pub fn new() -> Self {
        Self {
            root: PathBuf::from("/data/pandora"),
        }
    }
}

impl Default for Ps5Platform {
    fn default() -> Self {
        Self::new()
    }
}

/// Converts a path into a NUL-terminated C string.
fn c_path(path: &Path) -> Result<CString> {
    let bytes = path.to_string_lossy().into_owned();
    CString::new(bytes).map_err(|_| {
        crate::error::PandoraError::Io(std::io::Error::new(
            std::io::ErrorKind::InvalidInput,
            "path contains an interior NUL byte",
        ))
    })
}

impl Platform for Ps5Platform {
    fn name(&self) -> &'static str {
        "ps5"
    }

    fn data_root(&self) -> PathBuf {
        self.root.clone()
    }

    fn list_dir(&self, path: &Path) -> Result<Vec<DirEntry>> {
        let cpath = c_path(path)?;
        // SAFETY: `cpath` is a valid NUL-terminated string that outlives the call.
        let fd = unsafe { sceKernelOpen(cpath.as_ptr(), O_RDONLY, 0) };
        if fd < 0 {
            return Err(std::io::Error::from_raw_os_error(-fd).into());
        }

        let mut entries = Vec::new();
        let mut buffer = vec![0u8; 4096];
        loop {
            // SAFETY: `fd` is open, and `buffer` is writable for `buffer.len()` bytes.
            let read = unsafe {
                sceKernelGetdents(fd, buffer.as_mut_ptr().cast::<c_char>(), buffer.len())
            };
            if read <= 0 {
                break;
            }
            // Record framing follows the `dirent` layout of the target's libc:
            // name bytes terminated by NUL, then a type byte.
            let mut offset = 0usize;
            let read = read as usize;
            while offset < read {
                let name_end = buffer[offset..read]
                    .iter()
                    .position(|&b| b == 0)
                    .map_or(read, |p| offset + p);
                let name = String::from_utf8_lossy(&buffer[offset..name_end]).into_owned();
                if name.is_empty() || name == "." || name == ".." {
                    offset = (name_end + 1) & !0x3;
                    continue;
                }
                let entry_path = path.join(&name);
                let metadata = std::fs::metadata(&entry_path);
                let (is_dir, size) = metadata
                    .map(|m| (m.is_dir(), if m.is_dir() { 0 } else { m.len() }))
                    .unwrap_or((false, 0));
                entries.push(DirEntry {
                    name,
                    path: entry_path.to_string_lossy().into_owned(),
                    is_dir,
                    size,
                });
                offset = (name_end + 1) & !0x3;
            }
        }

        // SAFETY: `fd` came from `sceKernelOpen` and is closed exactly once here.
        unsafe { sceKernelClose(fd) };

        entries.sort_by(|a, b| b.is_dir.cmp(&a.is_dir).then_with(|| a.name.cmp(&b.name)));
        Ok(entries)
    }

    fn read_file(&self, path: &Path) -> Result<Vec<u8>> {
        let cpath = c_path(path)?;
        // SAFETY: `cpath` is a valid NUL-terminated string that outlives the call.
        let fd = unsafe { sceKernelOpen(cpath.as_ptr(), O_RDONLY, 0) };
        if fd < 0 {
            return Err(std::io::Error::from_raw_os_error(-fd).into());
        }

        let mut data = Vec::new();
        let mut chunk = vec![0u8; 64 * 1024];
        loop {
            // SAFETY: `fd` is open and `chunk` is writable for `chunk.len()` bytes.
            let read = unsafe { sceKernelRead(fd, chunk.as_mut_ptr(), chunk.len()) };
            if read < 0 {
                // SAFETY: closing an open descriptor exactly once.
                unsafe { sceKernelClose(fd) };
                return Err(std::io::Error::from_raw_os_error(-(read as i32)).into());
            }
            if read == 0 {
                break;
            }
            data.extend_from_slice(&chunk[..read as usize]);
        }

        // SAFETY: `fd` is open and is closed exactly once here.
        unsafe { sceKernelClose(fd) };
        Ok(data)
    }

    fn write_file(&self, path: &Path, data: &[u8]) -> Result<()> {
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent)?;
        }
        let cpath = c_path(path)?;
        // SAFETY: `cpath` is a valid NUL-terminated string that outlives the call.
        let fd = unsafe { sceKernelOpen(cpath.as_ptr(), O_WRONLY | O_CREAT | O_TRUNC, 0o644) };
        if fd < 0 {
            return Err(std::io::Error::from_raw_os_error(-fd).into());
        }

        let mut written = 0usize;
        while written < data.len() {
            // SAFETY: `fd` is open and the pointer is valid for the remaining length.
            let n = unsafe { sceKernelWrite(fd, data[written..].as_ptr(), data.len() - written) };
            if n <= 0 {
                // SAFETY: closing an open descriptor exactly once.
                unsafe { sceKernelClose(fd) };
                return Err(std::io::Error::from_raw_os_error(-(n as i32)).into());
            }
            written += n as usize;
        }

        // SAFETY: `fd` is open and is closed exactly once here.
        unsafe { sceKernelClose(fd) };
        Ok(())
    }
}
