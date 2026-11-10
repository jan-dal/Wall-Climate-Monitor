# Humidity monitor frontend

A Svelte 5, TypeScript, and Vite dashboard for the ESP32's `/api/data` readings.

## Develop

```sh
cd frontend
npm ci
cp .env.example .env.local
# Edit .env.local to point ESP32_URL at your ESP32's HTTPS server.
npm run dev
```

Open <http://localhost:5173>. Vite proxies `/api` requests to `ESP32_URL`.
For a local device with a self-signed certificate, set `ESP32_INSECURE_TLS=true`
to disable certificate verification in the development proxy. These environment
settings apply only to development; the embedded dashboard uses its own origin.

## Check and test

```sh
npm run check  # Svelte and TypeScript checks
npm test      # Vitest tests for calculations and charts
```

## Build

```sh
npm run build
```

This writes a self-contained `index.html` and `index.html.gz` directly to
`../main/web/`, replacing that directory's contents. Rebuild the firmware to embed
the updated gzip file. No `.env.local` is needed for this build.

`npm run preview` serves the built frontend locally, but does not include the
development API proxy; use `npm run dev` to work with live device data.
Sampling and gap-handling settings are in `src/lib/config.ts`.
