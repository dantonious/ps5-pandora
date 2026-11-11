//! Error types shared across the workspace.

use thiserror::Error;

/// Every fallible operation in this project returns a [`PandoraError`].
///
/// Variants are typed rather than stringly so callers can react to a specific
/// failure (a 404 for a missing file, a 400 for bad metadata, and so on).
#[derive(Debug, Error)]
pub enum PandoraError {
    /// An I/O operation failed.
    #[error("i/o error: {0}")]
    Io(#[from] std::io::Error),

    /// JSON could not be decoded or encoded.
    #[error("json error: {0}")]
    Json(#[from] serde_json::Error),

    /// A title ID is not four uppercase letters followed by five digits.
    #[error(
        "invalid title id {0:?}: expected four uppercase letters then five digits, e.g. PPSA01234"
    )]
    TitleId(String),

    /// A title ID uses a prefix this project refuses to build.
    #[error(
        "title id {0:?} uses a reserved prefix (CUSA belongs to PS4 titles, NPXS to system titles)"
    )]
    TitleIdPrefix(String),

    /// A title ID is the boilerplate default.
    #[error("title id {0:?} is reserved by the native-app boilerplate; choose your own")]
    TitleIdReserved(String),

    /// A content version is not `NN.NNN.NNN`.
    #[error("invalid content version {0:?}: expected NN.NNN.NNN, e.g. 01.000.070")]
    ContentVersion(String),
}

/// Convenience alias used throughout the crate.
pub type Result<T> = std::result::Result<T, PandoraError>;
