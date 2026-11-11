//! Console-side entry point.

use pandora_core::config::Config;
use pandora_http::{serve, AppState};
use tracing_subscriber::EnvFilter;

#[tokio::main]
async fn main() -> anyhow::Result<()> {
    let config = Config::default();

    let filter =
        EnvFilter::try_from_default_env().unwrap_or_else(|_| EnvFilter::new(&config.log_level));
    tracing_subscriber::fmt().with_env_filter(filter).init();

    tracing::info!(
        product = pandora_core::PRODUCT_NAME,
        version = pandora_core::VERSION,
        "starting"
    );

    serve(AppState::new(config)).await
}
