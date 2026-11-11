//! The console abstraction.
//!
//! Everything that touches the operating system goes through [`Platform`]. That
//! keeps console-specific code in one module and lets the rest of the project be
//! written and tested on a development machine.

pub mod host;

#[cfg(all(target_os = "freebsd", feature = "ps5"))]
pub mod ps5;

use crate::error::Result;
use std::path::{Path, PathBuf};

/// One entry of a directory listing.
#[derive(Debug, Clone, PartialEq, Eq, serde::Serialize)]
pub struct DirEntry {
    /// File or directory name, without any parent path.
    pub name: String,
    /// Full path, as the platform reports it.
    pub path: String,
    /// Whether this entry is a directory.
    pub is_dir: bool,
    /// Size in bytes; `0` for directories.
    pub size: u64,
}

/// Operations the rest of the project needs from the machine it runs on.
///
/// Implementations must be `Send + Sync`: the HTTP server calls them from many
/// tasks at once.
pub trait Platform: Send + Sync {
    /// Short name of this implementation, shown in `/api/health`.
    fn name(&self) -> &'static str;

    /// Writable root for configuration and state.
    fn data_root(&self) -> PathBuf;

    /// Lists a directory, directories first, then names in byte order.
    fn list_dir(&self, path: &Path) -> Result<Vec<DirEntry>>;

    /// Reads a whole file.
    fn read_file(&self, path: &Path) -> Result<Vec<u8>>;

    /// Writes a whole file, creating parent directories.
    fn write_file(&self, path: &Path, data: &[u8]) -> Result<()>;
}

/// Returns the implementation for the current target.
///
/// On the console this is `ps5`; everywhere else it is `host`, so a normal
/// `cargo run` works without the SDK.
#[must_use]
pub fn current_platform() -> Box<dyn Platform> {
    #[cfg(all(target_os = "freebsd", feature = "ps5"))]
    {
        Box::new(ps5::Ps5Platform::new())
    }
    #[cfg(not(all(target_os = "freebsd", feature = "ps5")))]
    {
        Box::new(host::HostPlatform::new())
    }
}
