//! `pandora` — host-side tooling for ps5-pandora.
//!
//! Nothing here touches a console except `pandora send`, and nothing here needs
//! the PS5 Payload SDK. That is deliberate: the release pipeline must be
//! reproducible on a machine that has never seen a console.

use anyhow::{Context, Result};
use clap::{Parser, Subcommand};
use pandora_core::{validate_title_id, ParamJson};
use sha2::{Digest, Sha256};
use std::io::Write;
use std::path::{Path, PathBuf};

/// Host-side tooling for ps5-pandora.
#[derive(Debug, Parser)]
#[command(
    name = "pandora",
    version,
    about = "Validate title metadata, package a catalog-ready title, send payloads."
)]
struct Cli {
    #[command(subcommand)]
    command: Command,
}

#[derive(Debug, Subcommand)]
enum Command {
    /// Validate a PS5 title ID.
    TitleId {
        #[command(subcommand)]
        command: TitleIdCommand,
    },
    /// Inspect, validate and bump sce_sys/param.json.
    Param {
        #[command(subcommand)]
        command: ParamCommand,
    },
    /// Print the SHA-256 of a file, as lowercase hex.
    Digest {
        /// File to hash.
        file: PathBuf,
    },
    /// Build the catalog-ready <TITLEID>.zip from a title folder.
    Package {
        /// Title folder to package (the one holding eboot.bin and sce_sys/).
        #[arg(long, default_value = "pkg")]
        source: PathBuf,
        /// Output directory.
        #[arg(long, default_value = "dist")]
        out: PathBuf,
        /// Metadata to read the title ID from.
        #[arg(long)]
        param: Option<PathBuf>,
    },
    /// Check the build environment and report what is missing.
    Doctor,
    /// Send an ELF payload to a console over TCP.
    Send {
        /// ELF file to send.
        file: PathBuf,
        /// Console address.
        #[arg(long, default_value = "192.168.1.100")]
        host: String,
        /// Port a payload loader is listening on.
        #[arg(long, default_value_t = 9020)]
        port: u16,
    },
}

#[derive(Debug, Subcommand)]
enum TitleIdCommand {
    /// Check a title ID for shape, prefix and boilerplate rules.
    Check {
        /// Title ID, e.g. PPSA77010.
        id: String,
    },
}

#[derive(Debug, Subcommand)]
enum ParamCommand {
    /// Validate a param.json file.
    Validate {
        /// Path to sce_sys/param.json.
        file: PathBuf,
    },
    /// Print the fields this tool understands.
    Show {
        /// Path to sce_sys/param.json.
        file: PathBuf,
    },
    /// Raise contentVersion, the step a release must never forget.
    Bump {
        /// Path to sce_sys/param.json.
        file: PathBuf,
        /// Raise the major field instead of the patch field.
        #[arg(long)]
        major: bool,
        /// Raise the minor field instead of the patch field.
        #[arg(long)]
        minor: bool,
    },
}

fn main() -> Result<()> {
    let cli = Cli::parse();

    match cli.command {
        Command::TitleId {
            command: TitleIdCommand::Check { id },
        } => {
            validate_title_id(&id).context("title id rejected")?;
            println!("{id}: ok");
        }

        Command::Param { command } => match command {
            ParamCommand::Validate { file } => {
                let param = read_param(&file)?;
                param.validate().context("metadata rejected")?;
                let version = param.parsed_content_version()?;
                println!("{}: ok", file.display());
                println!("  titleId        {}", param.title_id);
                println!("  contentVersion {version}");
            }
            ParamCommand::Show { file } => {
                let param = read_param(&file)?;
                println!("titleId        {}", param.title_id);
                println!("contentVersion {}", param.content_version);
                match &param.master_version {
                    Some(v) => println!("masterVersion  {v}"),
                    None => println!("masterVersion  (absent)"),
                }
            }
            ParamCommand::Bump { file, major, minor } => {
                let mut param = read_param(&file)?;
                let current = param.parsed_content_version()?;
                let next = if major {
                    current.bump_major()
                } else if minor {
                    current.bump_minor()
                } else {
                    current.bump_patch()
                };
                param.content_version = next.to_string();
                std::fs::write(&file, param.to_pretty_json()?)
                    .with_context(|| format!("writing {}", file.display()))?;
                println!("contentVersion {current} -> {next}");
            }
        },

        Command::Digest { file } => {
            println!("{}", sha256_of(&file)?);
        }

        Command::Package { source, out, param } => {
            let param_path = param.unwrap_or_else(|| source.join("sce_sys").join("param.json"));
            let path = package(&source, &out, &param_path)?;
            println!("wrote {}", path.display());
            println!("sha256 {}", sha256_of(&path)?);
        }

        Command::Doctor => doctor(),

        Command::Send { file, host, port } => {
            let data =
                std::fs::read(&file).with_context(|| format!("reading {}", file.display()))?;
            let mut stream = std::net::TcpStream::connect((host.as_str(), port))
                .with_context(|| format!("connecting to {host}:{port}"))?;
            stream.write_all(&data)?;
            stream.flush()?;
            println!("sent {} bytes to {host}:{port}", data.len());
        }
    }

    Ok(())
}

fn read_param(path: &Path) -> Result<ParamJson> {
    let text =
        std::fs::read_to_string(path).with_context(|| format!("reading {}", path.display()))?;
    ParamJson::parse(&text).with_context(|| format!("parsing {}", path.display()))
}

fn sha256_of(path: &Path) -> Result<String> {
    let mut file =
        std::fs::File::open(path).with_context(|| format!("opening {}", path.display()))?;
    let mut hasher = Sha256::new();
    std::io::copy(&mut file, &mut hasher)?;
    Ok(format!("{:x}", hasher.finalize()))
}

/// Archives `source` as `<TITLEID>/...` at the top of the ZIP.
///
/// The folder at the top of the archive is what the console and the homebrew
/// catalog both expect: extract, then copy the folder to the homebrew directory.
fn package(source: &Path, out_dir: &Path, param_path: &Path) -> Result<PathBuf> {
    let param = read_param(param_path)?;
    param
        .validate()
        .with_context(|| format!("{} failed validation", param_path.display()))?;

    if !source.is_dir() {
        anyhow::bail!("{} is not a directory", source.display());
    }

    std::fs::create_dir_all(out_dir).with_context(|| format!("creating {}", out_dir.display()))?;

    let title_id = param.title_id.clone();
    let out_path = out_dir.join(format!("{title_id}.zip"));
    let file = std::fs::File::create(&out_path)
        .with_context(|| format!("creating {}", out_path.display()))?;

    let mut zip = zip::ZipWriter::new(file);
    let options = zip::write::SimpleFileOptions::default()
        .compression_method(zip::CompressionMethod::Deflated);

    zip.add_directory(format!("{title_id}/"), options)?;

    let mut stack = vec![source.to_path_buf()];
    while let Some(dir) = stack.pop() {
        let entries =
            std::fs::read_dir(&dir).with_context(|| format!("listing {}", dir.display()))?;
        for entry in entries {
            let entry = entry?;
            let path = entry.path();
            if path == out_path {
                continue;
            }
            let relative: Vec<String> = path
                .strip_prefix(source)?
                .components()
                .map(|c| c.as_os_str().to_string_lossy().into_owned())
                .collect();
            let name = format!("{title_id}/{}", relative.join("/"));
            let metadata = entry.metadata()?;

            if metadata.is_dir() {
                zip.add_directory(format!("{name}/"), options)?;
                stack.push(path);
            } else if metadata.is_file() {
                zip.start_file(name, options)?;
                let mut f = std::fs::File::open(&path)?;
                std::io::copy(&mut f, &mut zip)?;
            }
        }
    }

    zip.finish()?;
    Ok(out_path)
}

fn doctor() {
    println!("ps5-pandora doctor");

    match std::env::var("PS5_PAYLOAD_SDK") {
        Ok(path) if !path.is_empty() => {
            let exists = Path::new(&path).is_dir();
            println!(
                "  PS5_PAYLOAD_SDK  {path} ({})",
                if exists { "found" } else { "MISSING" }
            );
        }
        _ => println!("  PS5_PAYLOAD_SDK  not set (host-only build)"),
    }

    let param = Path::new("pkg/sce_sys/param.json");
    if param.exists() {
        match read_param(param).and_then(|p| {
            p.validate()?;
            Ok(p)
        }) {
            Ok(p) => println!(
                "  param.json       ok ({} / {})",
                p.title_id, p.content_version
            ),
            Err(e) => println!("  param.json       INVALID: {e}"),
        }
    } else {
        println!("  param.json       not found at {}", param.display());
    }

    let icon = Path::new("pkg/sce_sys/icon0.png");
    println!(
        "  icon0.png        {}",
        if icon.exists() { "found" } else { "MISSING" }
    );
    println!("  version          {}", env!("CARGO_PKG_VERSION"));
}

#[cfg(test)]
mod tests {
    use super::*;

    #[test]
    fn sha256_matches_a_known_digest() {
        let dir = std::env::temp_dir().join("pandora-cli-digest");
        std::fs::create_dir_all(&dir).unwrap();
        let file = dir.join("abc.txt");
        std::fs::write(&file, b"abc").unwrap();

        // echo -n abc | sha256sum
        assert_eq!(
            sha256_of(&file).unwrap(),
            "ba7816bf8f01cfea414140de5dae2223b00361a396177a9cb410ff61f20015ad"
        );

        let _ = std::fs::remove_dir_all(&dir);
    }

    #[test]
    fn packages_the_title_folder_at_the_top_of_the_archive() {
        let dir = std::env::temp_dir().join("pandora-cli-package");
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(dir.join("src").join("sce_sys")).unwrap();
        std::fs::write(
            dir.join("src").join("sce_sys").join("param.json"),
            r#"{"titleId":"PPSA77010","contentVersion":"01.000.000","masterVersion":"01.00"}"#,
        )
        .unwrap();
        std::fs::write(dir.join("src").join("eboot.bin"), b"ELFDATA").unwrap();

        let out = package(
            &dir.join("src"),
            &dir.join("dist"),
            &dir.join("src").join("sce_sys").join("param.json"),
        )
        .unwrap();

        assert!(out.ends_with("PPSA77010.zip"));
        assert!(out.exists());

        let mut archive = zip::ZipArchive::new(std::fs::File::open(&out).unwrap()).unwrap();
        let names: Vec<String> = (0..archive.len())
            .map(|i| archive.by_index(i).unwrap().name().to_owned())
            .collect();
        assert!(names.iter().any(|n| n == "PPSA77010/"));
        assert!(names.iter().any(|n| n == "PPSA77010/eboot.bin"));
        assert!(names.iter().any(|n| n == "PPSA77010/sce_sys/param.json"));

        let _ = std::fs::remove_dir_all(&dir);
    }

    #[test]
    fn refuses_to_package_an_invalid_title_id() {
        let dir = std::env::temp_dir().join("pandora-cli-badpkg");
        let _ = std::fs::remove_dir_all(&dir);
        std::fs::create_dir_all(dir.join("src").join("sce_sys")).unwrap();
        std::fs::write(
            dir.join("src").join("sce_sys").join("param.json"),
            r#"{"titleId":"PPSA99999","contentVersion":"01.000.000"}"#,
        )
        .unwrap();

        let result = package(
            &dir.join("src"),
            &dir.join("dist"),
            &dir.join("src").join("sce_sys").join("param.json"),
        );
        assert!(result.is_err());

        let _ = std::fs::remove_dir_all(&dir);
    }
}
