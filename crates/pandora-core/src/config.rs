//! Runtime configuration.

use crate::error::Result;
use serde::{Deserialize, Serialize};
use std::path::{Path, PathBuf};

/// Default TCP port of the dashboard.
pub const DEFAULT_PORT: u16 = 7070;

/// Configuration for the console-side service.
///
/// Defaults target the console (`/data/pandora`). The host implementation uses
/// the same struct with a different root, so configuration handling is exercised
/// by ordinary tests.
#[derive(Debug, Clone, Serialize, Deserialize)]
pub struct Config {
    /// Address to bind. `0.0.0.0` keeps the dashboard reachable from the LAN,
    /// which is the point of the product; bind to loopback if you do not want that.
    pub bind_address: String,
    /// TCP port of the dashboard.
    pub port: u16,
    /// Writable root for configuration and state.
    pub data_root: PathBuf,
    /// Where uploaded payloads live.
    pub payload_dir: PathBuf,
    /// Tracing filter, e.g. `info` or `pandora_http=debug`.
    pub log_level: String,
}

impl Default for Config {
    fn default() -> Self {
        let data_root = PathBuf::from("/data/pandora");
        Self {
            bind_address: "0.0.0.0".to_owned(),
            port: DEFAULT_PORT,
            payload_dir: data_root.join("payloads"),
            data_root,
            log_level: "info".to_owned(),
        }
    }
}

impl Config {
    /// Loads configuration from `path`, falling back to defaults when absent.
    pub fn load(path: &Path) -> Result<Self> {
        if !path.exists() {
            return Ok(Self::default());
        }
        let text = std::fs::read_to_string(path)?;
        Ok(serde_json::from_str(&text)?)
    }

    /// Writes configuration to `path`, creating the parent directory.
    pub fn save(&self, path: &Path) -> Result<()> {
        if let Some(parent) = path.parent() {
            std::fs::create_dir_all(parent)?;
        }
        std::fs::write(path, serde_json::to_string_pretty(self)?)?;
        Ok(())
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn defaults_are_consistent() {
        let c = Config::default();
        assert_eq!(c.port, DEFAULT_PORT);
        assert_eq!(c.payload_dir, PathBuf::from("/data/pandora/payloads"));
    }

    #[test]
    fn missing_file_yields_defaults() {
        let dir = std::env::temp_dir().join("pandora-cfg-missing");
        let path = dir.join("config.json");
        let _ = std::fs::remove_file(&path);
        let c = Config::load(&path).unwrap();
        assert_eq!(c.port, DEFAULT_PORT);
    }

    #[test]
    fn save_then_load_round_trips() {
        let dir = std::env::temp_dir().join("pandora-cfg-roundtrip");
        let path = dir.join("nested").join("config.json");
        let c = Config {
            port: 8081,
            bind_address: "127.0.0.1".to_owned(),
            ..Config::default()
        };
        c.save(&path).unwrap();

        let loaded = Config::load(&path).unwrap();
        assert_eq!(loaded.port, 8081);
        assert_eq!(loaded.bind_address, "127.0.0.1");
        let _ = std::fs::remove_dir_all(&dir);
    }
}
