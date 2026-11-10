# ESP32-S3 humidity monitor — Svelte 5 migration

The `frontend/` directory is a replacement for the original ~950-line standalone
HTML dashboard. It keeps the original `/api/data` JSON contract and all user-visible
features: latest readings, malformed-data diagnostics, sorting/deduplication,
timestamp normalization, dew point / wall-surface RH, 7-/30-day exposure,
three interactive Plotly charts, missing-gap handling, dew-point visibility,
refresh and CSV download.

It is a client-side Svelte 5 + TypeScript + Vite app, **not SvelteKit**.

## Quick start

```bash
cd frontend
cp .env.example .env.local
# Set ESP32_URL to your ESP32's HTTP server URL in .env.local.
npm install
npm run dev
```

Open http://localhost:5173. Calls to `/api/data` are proxied to `ESP32_URL`
in development, while the production HTML calls the same path on the ESP32.

## Build and embed

```bash
cd frontend
npm run check
npm run test
npm run build
```

`frontend/dist/index.html` and `frontend/dist/index.html.gz` are emitted. The
first is convenient for troubleshooting; the second should be embedded in the
firmware (copy into your own `main/web/` or similar). Example ESP-IDF CMake entry:

```cmake
idf_component_register(
    SRCS "main.c"  # plus your existing C sources
    INCLUDE_DIRS "."
    EMBED_FILES "web/index.html.gz"
)
```

Example HTTP handler (the gzip response assumes a gzip-capable client):

```c
extern const uint8_t index_html_gz_start[] asm("_binary_web_index_html_gz_start");
extern const uint8_t index_html_gz_end[] asm("_binary_web_index_html_gz_end");

static esp_err_t index_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "text/html; charset=utf-8");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Vary", "Accept-Encoding");
    return httpd_resp_send(req, (const char *)index_html_gz_start,
                           index_html_gz_end - index_html_gz_start);
}
```

For clients without gzip support, check `Accept-Encoding` or serve an
uncompressed fallback.

`plotly.js-gl2d-dist-min` is bundled **locally** (no CDN). It supports the original
`scattergl` WebGL line traces. All three charts, including annotations and
hover information, remain supported;
the compressed HTML will be larger than the CDN-dependent original page.

Change sensor interval and limits in `src/lib/config.ts`. Keep `index.html`
small: it should just mount the Svelte application.

The source project deliberately excludes `node_modules/` and `dist/` from the
archive; install and build locally to generate both HTML artifacts.
