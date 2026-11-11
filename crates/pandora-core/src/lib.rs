//! Core domain types, platform abstraction and native title metadata.
//!
//! Nothing in this crate touches a console unless the `ps5` feature is enabled,
//! so the whole workspace builds and tests on a normal development machine.
//! This is deliberate: console logic that cannot be tested off-device rots.

pub mod config;
pub mod error;
pub mod param;
pub mod platform;
pub mod version;

pub use error::{PandoraError, Result};
pub use param::{validate_title_id, ParamJson};
pub use version::ContentVersion;

/// Product name, used in logs and user-facing output.
pub const PRODUCT_NAME: &str = "ps5-pandora";

/// Version of the running binary.
pub const VERSION: &str = env!("CARGO_PKG_VERSION");
