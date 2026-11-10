import { defineConfig, loadEnv } from 'vite';
import { svelte } from '@sveltejs/vite-plugin-svelte';
import { viteSingleFile } from 'vite-plugin-singlefile';
import viteCompression from 'vite-plugin-compression';

export default defineConfig(({ mode }) => {
  const env = loadEnv(mode, process.cwd(), '');

  return {
    plugins: [
      svelte(),
      viteSingleFile(),
      viteCompression({
        algorithm: 'gzip',
        ext: '.gz',
        filter: /index\.html$/,
        threshold: 0,
        deleteOriginFile: false,
        compressionOptions: { level: 9 },
      }),
    ],
    server: {
      port: 5173,
      strictPort: true,
      proxy: {
        '/api': {
          target: env.ESP32_URL || 'http://esp32.local',
          changeOrigin: true,
          followRedirects: true,
          secure: env.ESP32_INSECURE_TLS !== 'true',
        },
      },
    },
    build: {
      target: 'es2020',
      outDir: '../main/web',
      emptyOutDir: true,
      sourcemap: false,
    },
  };
});
