//! PlayStation `contentVersion` handling.
//!
//! A `contentVersion` is the version the console itself reports for an installed
//! title, and it is how a console decides whether an installed copy is outdated.
//! Release tags cannot do that job: they have no common format and no defined
//! order. The format is fixed at `NN.NNN.NNN` and is compared as three numbers,
//! left to right.

use crate::error::{PandoraError, Result};
use std::fmt;
use std::str::FromStr;

/// Largest value the three-digit fields can hold.
const MAX_FIELD: u16 = 999;
/// Largest value the two-digit major field can hold.
const MAX_MAJOR: u16 = 99;

/// A PlayStation content version, always rendered `NN.NNN.NNN`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, PartialOrd, Ord, Hash, Default)]
pub struct ContentVersion {
    /// Two-digit field.
    pub major: u16,
    /// Three-digit field.
    pub minor: u16,
    /// Three-digit field.
    pub patch: u16,
}

impl ContentVersion {
    /// The value many templates ship and never raise: `01.000.000`.
    pub const INITIAL: Self = Self {
        major: 1,
        minor: 0,
        patch: 0,
    };

    /// Builds a version from its three fields.
    #[must_use]
    pub const fn new(major: u16, minor: u16, patch: u16) -> Self {
        Self {
            major,
            minor,
            patch,
        }
    }

    /// Returns the next patch version, the usual choice for a new release.
    ///
    /// Clamped at the field's maximum, `999`: the format has exactly three
    /// digits, so `1000` would render as an unparseable `01.000.1000`. The
    /// clamp is on the field width, not on `u16`, which is why
    /// `saturating_add` alone is not enough.
    #[must_use]
    pub fn bump_patch(self) -> Self {
        Self {
            patch: self.patch.saturating_add(1).min(MAX_FIELD),
            ..self
        }
    }

    /// Returns the next minor version: minor plus one, patch reset to `0`.
    #[must_use]
    pub fn bump_minor(self) -> Self {
        Self {
            minor: self.minor.saturating_add(1).min(MAX_FIELD),
            patch: 0,
            ..self
        }
    }

    /// Returns the next major version: major plus one, the rest reset to `0`.
    #[must_use]
    pub fn bump_major(self) -> Self {
        Self {
            major: self.major.saturating_add(1).min(MAX_MAJOR),
            minor: 0,
            patch: 0,
        }
    }
}

impl FromStr for ContentVersion {
    type Err = PandoraError;

    /// Parses `NN.NNN.NNN`, rejecting anything else.
    ///
    /// Field widths are enforced exactly: `1.0.0` and `01.0.0` are rejected,
    /// because the console treats a malformed value as unknown.
    fn from_str(s: &str) -> Result<Self> {
        let invalid = || PandoraError::ContentVersion(s.to_owned());

        let mut parts = s.split('.');
        let (Some(major), Some(minor), Some(patch)) = (parts.next(), parts.next(), parts.next())
        else {
            return Err(invalid());
        };
        if parts.next().is_some() {
            return Err(invalid());
        }
        if major.len() != 2 || minor.len() != 3 || patch.len() != 3 {
            return Err(invalid());
        }
        let all_digits = |field: &str| field.bytes().all(|b| b.is_ascii_digit());
        if !all_digits(major) || !all_digits(minor) || !all_digits(patch) {
            return Err(invalid());
        }

        Ok(Self {
            major: major.parse().map_err(|_| invalid())?,
            minor: minor.parse().map_err(|_| invalid())?,
            patch: patch.parse().map_err(|_| invalid())?,
        })
    }
}

impl fmt::Display for ContentVersion {
    fn fmt(&self, f: &mut fmt::Formatter<'_>) -> fmt::Result {
        write!(f, "{:02}.{:03}.{:03}", self.major, self.minor, self.patch)
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn parses_a_well_formed_version() {
        let v: ContentVersion = "01.000.070".parse().unwrap();
        assert_eq!(v, ContentVersion::new(1, 0, 70));
    }

    #[test]
    fn round_trips_through_display() {
        let v = ContentVersion::new(2, 15, 999);
        assert_eq!(v.to_string(), "02.015.999");
        assert_eq!(v.to_string().parse::<ContentVersion>().unwrap(), v);
    }

    #[test]
    fn rejects_wrong_field_widths() {
        for bad in ["1.0.0", "01.0.070", "01.000.70", "001.000.070"] {
            assert!(bad.parse::<ContentVersion>().is_err(), "accepted {bad:?}");
        }
    }

    #[test]
    fn rejects_non_numeric_and_malformed() {
        for bad in [
            "",
            "01.000",
            "01.000.070.1",
            "aa.bbb.ccc",
            "01-000-070",
            " 01.000.070",
        ] {
            assert!(bad.parse::<ContentVersion>().is_err(), "accepted {bad:?}");
        }
    }

    #[test]
    fn orders_numerically_not_lexically() {
        let a: ContentVersion = "01.000.009".parse().unwrap();
        let b: ContentVersion = "01.000.010".parse().unwrap();
        let c: ContentVersion = "01.001.000".parse().unwrap();
        let d: ContentVersion = "02.000.000".parse().unwrap();
        assert!(a < b && b < c && c < d);
    }

    #[test]
    fn bump_patch_advances_and_clamps_to_the_field_width() {
        assert_eq!(
            ContentVersion::new(1, 0, 70).bump_patch(),
            ContentVersion::new(1, 0, 71)
        );
        let max = ContentVersion::new(99, 999, 999);
        assert_eq!(max.bump_patch(), max, "must clamp, not render 1000");
    }

    #[test]
    fn bump_minor_and_major_reset_lower_fields() {
        assert_eq!(
            ContentVersion::new(1, 0, 70).bump_minor(),
            ContentVersion::new(1, 1, 0)
        );
        assert_eq!(
            ContentVersion::new(1, 5, 70).bump_major(),
            ContentVersion::new(2, 0, 0)
        );
        assert_eq!(
            ContentVersion::new(99, 999, 999).bump_major(),
            ContentVersion::new(99, 0, 0),
            "major clamps at 99"
        );
    }

    #[test]
    fn bumped_versions_always_reparse() {
        let start = ContentVersion::new(1, 0, 998);
        let after = start.bump_patch();
        assert!(
            after.to_string().parse::<ContentVersion>().is_ok(),
            "{after} must stay parseable"
        );
        let at_max = ContentVersion::new(99, 999, 999);
        assert!(at_max.to_string().parse::<ContentVersion>().is_ok());
    }
}
