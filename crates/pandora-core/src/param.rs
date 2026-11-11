//! `sce_sys/param.json`: the metadata that makes a folder a native PS5 title.

use crate::error::{PandoraError, Result};
use crate::version::ContentVersion;
use serde::{Deserialize, Serialize};

/// `PPSA99999` ships as the default in the native-app boilerplate and is not a
/// usable identity.
pub const RESERVED_TITLE_ID: &str = "PPSA99999";

/// The subset of `sce_sys/param.json` this project reads and writes.
///
/// Unknown keys are ignored on read so a file produced by other tooling still
/// loads; only the fields we care about are preserved on write.
#[derive(Debug, Clone, PartialEq, Eq, Serialize, Deserialize)]
pub struct ParamJson {
    /// The title's permanent identity, e.g. `PPSA77010`.
    #[serde(rename = "titleId")]
    pub title_id: String,

    /// The version the console compares to decide whether an update exists.
    #[serde(rename = "contentVersion")]
    pub content_version: String,

    /// Optional master version, `NN.NN`.
    #[serde(
        rename = "masterVersion",
        default,
        skip_serializing_if = "Option::is_none"
    )]
    pub master_version: Option<String>,
}

impl ParamJson {
    /// Parses a `param.json` document.
    pub fn parse(json: &str) -> Result<Self> {
        Ok(serde_json::from_str(json)?)
    }

    /// Serialises back to pretty-printed JSON with a trailing newline.
    pub fn to_pretty_json(&self) -> Result<String> {
        let mut out = serde_json::to_string_pretty(self)?;
        out.push('\n');
        Ok(out)
    }

    /// Checks every invariant this project relies on.
    ///
    /// Call this before packaging or publishing: a bad title ID or a malformed
    /// content version produces an artifact a console cannot install properly.
    pub fn validate(&self) -> Result<()> {
        validate_title_id(&self.title_id)?;
        self.parsed_content_version()?;
        Ok(())
    }

    /// Returns the content version as a comparable value.
    pub fn parsed_content_version(&self) -> Result<ContentVersion> {
        self.content_version.parse()
    }
}

/// Validates a title ID: four uppercase letters, then five digits.
///
/// Rejects `CUSA` (PS4 titles), `NPXS` (system titles) and the boilerplate
/// default. See `docs/TITLE-ID.md` for why each rule exists.
pub fn validate_title_id(id: &str) -> Result<()> {
    let bytes = id.as_bytes();
    if bytes.len() != 9 {
        return Err(PandoraError::TitleId(id.to_owned()));
    }
    let letters_ok = bytes[..4].iter().all(u8::is_ascii_uppercase);
    let digits_ok = bytes[4..].iter().all(u8::is_ascii_digit);
    if !letters_ok || !digits_ok {
        return Err(PandoraError::TitleId(id.to_owned()));
    }
    if id.starts_with("CUSA") || id.starts_with("NPXS") {
        return Err(PandoraError::TitleIdPrefix(id.to_owned()));
    }
    if id == RESERVED_TITLE_ID {
        return Err(PandoraError::TitleIdReserved(id.to_owned()));
    }
    Ok(())
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn accepts_a_normal_title_id() {
        for good in ["PPSA77010", "FAKE77031", "PRWB00001"] {
            assert!(validate_title_id(good).is_ok(), "rejected {good:?}");
        }
    }

    #[test]
    fn rejects_bad_shapes() {
        for bad in [
            "PPSA7701",   // too short
            "PPSA770100", // too long
            "ppsa77010",  // lowercase
            "PPSA7701a",  // letter in the digits
            "PP5A77010",  // digit in the letters
            "PPSA 7010",  // whitespace
            "",           // empty
        ] {
            assert!(validate_title_id(bad).is_err(), "accepted {bad:?}");
        }
    }

    #[test]
    fn rejects_reserved_prefixes() {
        assert!(matches!(
            validate_title_id("CUSA12345"),
            Err(PandoraError::TitleIdPrefix(_))
        ));
        assert!(matches!(
            validate_title_id("NPXS12345"),
            Err(PandoraError::TitleIdPrefix(_))
        ));
    }

    #[test]
    fn rejects_the_boilerplate_default() {
        assert!(matches!(
            validate_title_id("PPSA99999"),
            Err(PandoraError::TitleIdReserved(_))
        ));
    }

    #[test]
    fn parses_and_validates_a_document() {
        let doc = r#"{
            "titleId": "PPSA77010",
            "contentVersion": "01.000.070",
            "masterVersion": "01.00"
        }"#;
        let p = ParamJson::parse(doc).unwrap();
        p.validate().unwrap();
        assert_eq!(p.title_id, "PPSA77010");
        assert_eq!(
            p.parsed_content_version().unwrap(),
            ContentVersion::new(1, 0, 70)
        );
    }

    #[test]
    fn ignores_unknown_keys() {
        let doc = r#"{"titleId":"PPSA77010","contentVersion":"01.000.001","somethingElse":42}"#;
        let p = ParamJson::parse(doc).unwrap();
        p.validate().unwrap();
    }

    #[test]
    fn round_trips_to_pretty_json() {
        let p = ParamJson {
            title_id: "PPSA77010".to_owned(),
            content_version: "01.000.070".to_owned(),
            master_version: Some("01.00".to_owned()),
        };
        let text = p.to_pretty_json().unwrap();
        assert!(text.ends_with('\n'));
        assert_eq!(ParamJson::parse(&text).unwrap(), p);
    }

    #[test]
    fn omits_absent_master_version() {
        let p = ParamJson {
            title_id: "PPSA77010".to_owned(),
            content_version: "01.000.070".to_owned(),
            master_version: None,
        };
        assert!(!p.to_pretty_json().unwrap().contains("masterVersion"));
    }
}
