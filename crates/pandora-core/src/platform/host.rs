//! Development implementation, backed by `std::fs`.
//!
//! This is not a stub: it is what runs when you `cargo run` on your laptop, and
//! it is what the integration tests exercise. Behaviour here and on the console
//! must match, which is why the differences are kept to path roots only.

use super::{DirEntry, Platform};
use crate::error::Result;
use std::path::{Path, PathBuf};

/// A platform backed by the local filesystem.
#[derive(Debug, Clone)]
pub struct HostPlatform {
    root: PathBuf,
}

impl HostPlatform {
    /// Creates a platform rooted at `./data`, the conventional host location.
    #[must_use]
    pub fn new() -> Self {
        Self::with_root(PathBuf::from("./data"))
    }

    /// Creates a platform rooted at an explicit directory.
    #[must_use]
    pub fn with_root(root: PathBuf) -> Self {
        Self { root }
    }
}

impl Default for HostPlatform {
    fn default() -> Self {
        Self::new()
    }
}

impl Platform for HostPlatform {
    fn name(&self) -> &'static str {
        "host"
    }

    fn data_root(&self) -> PathBuf {
        self.root.clone()
    }

    fn list_dir(&self, path: &Path) -> Result<Vec<DirEntry>> {
        let mut entries = Vec::new();
        for entry in std::fs::read_dir(path)? {
            let entry = entry?;
            let metadata = entry.metadata()?;
            entries.push(DirEntry {
                name: entry.file_name().to_string_lossy().into_owned(),
                path: entry.path().to_string_lossy().into_owned(),
                is_dir: metadata.is_dir(),
                size: if metadata.is_dir() { 0 } else { metadata.len() },
            });
        }
        entries.sort_by(|a, b| b.is_dir.cmp(&a.is_dir).then_with(|| a.name.cmp(&b.name)));
        Ok(entries)
    }

    fn read_file(&self, path: &Path) -> Result<Vec<u8>> {
        Ok(std::fs::read(path)?)
    }

    fn write_file(&self, path: &Path, data: &[u8]) -> Result<()> {
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent)?;
        }
        std::fs::write(path, data)?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn tempdir(tag: &str) -> PathBuf {
        let dir = std::env::temp_dir().join(format!("pandora-host-{tag}"));
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(&dir).unwrap();
        dir
    }

    #[test]
    fn writes_and_reads_back() {
        let dir = tempdir("rw");
        let platform = HostPlatform::with_root(dir.clone());
        let target = dir.join("nested").join("file.txt");

        platform.write_file(&target, b"pandora").unwrap();
        assert_eq!(platform.read_file(&target).unwrap(), b"pandora");

        let _ = std::fs::remove_dir_all(&dir);
    }

    #[test]
    fn lists_directories_before_files() {
        let dir = tempdir("list");
        std::fs::create_dir(dir.join("subdir")).unwrap();
        std::fs::write(dir.join("b.txt"), b"bb").unwrap();
        std::fs::write(dir.join("a.txt"), b"a").unwrap();

        let platform = HostPlatform::with_root(dir.clone());
        let entries = platform.list_dir(&dir).unwrap();
        let names: Vec<&str> = entries.iter().map(|e| e.name.as_str()).collect();
        assert_eq!(names, vec!["subdir", "a.txt", "b.txt"]);

        let _ = std::fs::remove_dir_all(&dir);
    }

    #[test]
    fn reports_missing_paths_as_errors() {
        let platform = HostPlatform::new();
        let missing = std::env::temp_dir().join("pandora-definitely-missing-xyz");
        assert!(platform.read_file(&missing).is_err());
        assert!(platform.list_dir(&missing).is_err());
    }
}
