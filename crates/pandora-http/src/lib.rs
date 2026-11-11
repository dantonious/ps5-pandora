//! HTTP API and embedded dashboard.
//!
//! The dashboard is served from inside the binary, so deploying the payload is
//! the only deployment step. Routes are intentionally small and typed: every
//! handler returns either JSON or an [`ApiError`], never a panic.

use axum::{
    extract::{Query, State},
    http::StatusCode,
    response::{Html, IntoResponse, Response},
    routing::get,
    Json, Router,
};
use pandora_core::{
    config::Config,
    platform::{current_platform, DirEntry, Platform},
    PandoraError, VERSION,
};
use serde::{Deserialize, Serialize};
use std::path::PathBuf;
use std::sync::Arc;
use tower_http::cors::CorsLayer;

/// The placeholder page, embedded at compile time.
const INDEX_HTML: &str = include_str!("../../../ui/index.html");

/// Shared state handed to every handler.
#[derive(Clone)]
pub struct AppState {
    /// The machine the requests act on.
    pub platform: Arc<dyn Platform>,
    /// Runtime configuration.
    pub config: Config,
}

impl AppState {
    /// Builds state for the current target, using the configuration given.
    #[must_use]
    pub fn new(config: Config) -> Self {
        Self {
            platform: current_platform().into(),
            config,
        }
    }
}

/// Builds the router. Exposed separately so tests can drive it without a socket.
pub fn router(state: AppState) -> Router {
    Router::new()
        .route("/", get(index))
        .route("/api/health", get(health))
        .route("/api/version", get(version))
        .route("/api/fs/list", get(list_dir))
        .layer(CorsLayer::permissive())
        .with_state(state)
}

/// Serves the dashboard until the process is stopped.
pub async fn serve(state: AppState) -> anyhow::Result<()> {
    let address = format!("{}:{}", state.config.bind_address, state.config.port);
    let listener = tokio::net::TcpListener::bind(&address).await?;
    tracing::info!(address = %address, "dashboard listening");
    axum::serve(listener, router(state)).await?;
    Ok(())
}

async fn index() -> Html<&'static str> {
    Html(INDEX_HTML)
}

#[derive(Debug, Serialize)]
struct Health {
    status: &'static str,
    version: &'static str,
    platform: &'static str,
}

async fn health(State(state): State<AppState>) -> Json<Health> {
    Json(Health {
        status: "ok",
        version: VERSION,
        platform: state.platform.name(),
    })
}

#[derive(Debug, Serialize)]
struct VersionInfo {
    product: &'static str,
    version: &'static str,
    data_root: String,
}

async fn version(State(state): State<AppState>) -> Json<VersionInfo> {
    Json(VersionInfo {
        product: pandora_core::PRODUCT_NAME,
        version: VERSION,
        data_root: state.platform.data_root().to_string_lossy().into_owned(),
    })
}

#[derive(Debug, Deserialize)]
struct ListQuery {
    path: Option<String>,
}

async fn list_dir(
    State(state): State<AppState>,
    Query(query): Query<ListQuery>,
) -> Result<Json<Vec<DirEntry>>, ApiError> {
    let path = query
        .path
        .map(PathBuf::from)
        .unwrap_or_else(|| state.platform.data_root());
    let entries = state.platform.list_dir(&path)?;
    Ok(Json(entries))
}

/// An error rendered as a JSON body with a meaningful status code.
#[derive(Debug)]
pub struct ApiError {
    status: StatusCode,
    message: String,
}

impl IntoResponse for ApiError {
    fn into_response(self) -> Response {
        let body = serde_json::json!({ "error": self.message });
        (self.status, Json(body)).into_response()
    }
}

impl From<PandoraError> for ApiError {
    fn from(error: PandoraError) -> Self {
        let status = match &error {
            PandoraError::Io(io) if io.kind() == std::io::ErrorKind::NotFound => {
                StatusCode::NOT_FOUND
            }
            PandoraError::Io(_) => StatusCode::INTERNAL_SERVER_ERROR,
            _ => StatusCode::BAD_REQUEST,
        };
        Self {
            status,
            message: error.to_string(),
        }
    }
}

#[cfg(test)]
mod tests {
    use super::*;

    fn state() -> AppState {
        AppState::new(Config::default())
    }

    #[test]
    fn builds_a_router() {
        let _: Router = router(state());
    }

    #[test]
    fn index_is_embedded_and_non_empty() {
        assert!(INDEX_HTML.contains("ps5-pandora"));
    }

    #[test]
    fn api_error_maps_missing_files_to_404() {
        let missing = std::io::Error::from(std::io::ErrorKind::NotFound);
        let mapped: ApiError = PandoraError::Io(missing).into();
        assert_eq!(mapped.status, StatusCode::NOT_FOUND);
    }

    #[test]
    fn api_error_maps_bad_metadata_to_400() {
        let mapped: ApiError = PandoraError::TitleId("nope".to_owned()).into();
        assert_eq!(mapped.status, StatusCode::BAD_REQUEST);
    }
}
