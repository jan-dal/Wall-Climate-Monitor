<script lang="ts">
  import { onMount } from 'svelte';
  import type { Measurement } from './types/measurement';
  import { fetchMeasurements, processMeasurements } from './lib/api/measurements';
  import { getGapLimit } from './lib/calculations/gaps';
  import { FALLBACK_MAX_GAP_SECONDS, FRONTEND_BUILD } from './lib/config';
  import { downloadCSV } from './lib/utils/csv';
  import { getFreshness } from './lib/utils/format';
  import SensorOverview from './components/SensorOverview.svelte';
  import ExposurePanel from './components/ExposurePanel.svelte';
  import ChartRow from './components/ChartRow.svelte';
  import MethodNote from './components/MethodNote.svelte';

  let measurements = $state<Measurement[]>([]);
  let nowSeconds = $state(Date.now() / 1000);
  let gapLimitSeconds = $state(FALLBACK_MAX_GAP_SECONDS);
  let rawCount = $state(0);
  let invalidCount = $state(0);
  let duplicateCount = $state(0);
  let diagnostics = $state('');
  let loading = $state(false);
  let status = $state('Loading measurements...');
  let statusLevel = $state<'normal' | 'warning' | 'error'>('normal');
  let loadedOnce = $state(false);

  let latest = $derived(measurements.length ? measurements[measurements.length - 1] : null);
  let freshness = $derived(getFreshness(measurements, nowSeconds, gapLimitSeconds));
  let rejectedCount = $derived(invalidCount + duplicateCount);

  async function refreshData(): Promise<void> {
    if (loading) return;
    loading = true;
    status = 'Downloading measurements...';
    statusLevel = 'normal';

    try {
      const raw = await fetchMeasurements();
      const refreshedAt = Date.now() / 1000;
      const result = processMeasurements(raw, refreshedAt);
      const gap = getGapLimit(result.measurements);

      // Only replace the previous view after a successful request and parse.
      measurements = result.measurements;
      nowSeconds = refreshedAt;
      gapLimitSeconds = gap;
      rawCount = result.rawCount;
      invalidCount = result.invalidCount;
      duplicateCount = result.duplicateCount;
      diagnostics = result.diagnostics;
      loadedOnce = true;

      if (measurements.length === 0) {
        status = raw.length === 0
          ? 'No records returned by /api/data.'
          : `The API returned ${raw.length} records, but none passed validation. See diagnostics below.`;
        statusLevel = 'warning';
      } else {
        status = `Loaded ${measurements.length.toLocaleString()} measurements` +
          (rejectedCount > 0
            ? `; excluded ${invalidCount.toLocaleString()} invalid and ${duplicateCount.toLocaleString()} duplicate records.`
            : '.');
      }
    } catch (error) {
      const message = error instanceof Error ? error.message : String(error);
      status = `Refresh failed: ${message}. Previously loaded readings are preserved.`;
      statusLevel = 'error';
    } finally {
      loading = false;
    }
  }

  onMount(() => { void refreshData(); });
</script>

<h1>Wall Surface Humidity Monitor</h1>
<p class="subtitle">
  Estimated wall-surface relative humidity, dew point and condensation exposure
  from a DS18B20 on the wall and an AHT20 in nearby air.
</p>

<SensorOverview {latest} />

<div class="toolbar">
  <button type="button" onclick={refreshData} disabled={loading}>
    {loading ? 'Refreshing...' : 'Refresh Data'}
  </button>
  <button type="button" class="secondary" onclick={() => downloadCSV(measurements)} disabled={measurements.length === 0}>
    Download CSV
  </button>
</div>

<div class="metadata status-line">
  <span>API records: <strong>{rawCount.toLocaleString()}</strong></span>
  <span>Valid measurements: <strong>{measurements.length.toLocaleString()}</strong></span>
  <span>Rejected records: <strong>{rejectedCount.toLocaleString()}</strong></span>
  <span>Latest record: <strong>{freshness.latestTime}</strong></span>
</div>

<p class="status-line" class:warning={statusLevel === 'warning'} class:error={statusLevel === 'error'} role="status" aria-live="polite">
  {status}
</p>
{#if diagnostics}
  <p class="status-line" class:warning={rawCount === 0 || invalidCount > 0} aria-live="polite">{diagnostics}</p>
{/if}
{#if freshness.text}
  <p class="status-line" class:warning={freshness.warning} role="status" aria-live="polite">{freshness.text}</p>
{/if}

<ExposurePanel {measurements} {nowSeconds} {gapLimitSeconds} />

<section aria-label="Measurement charts">
  <h2 class="section-heading">Measurement history</h2>
  {#if loadedOnce}
    <ChartRow kind="temperature" {measurements} {gapLimitSeconds} />
    <ChartRow kind="humidity" {measurements} {gapLimitSeconds} />
    <ChartRow kind="margin" {measurements} {gapLimitSeconds} />
  {:else}
    <p class="status-line">Charts will appear after the first successful data request.</p>
  {/if}
</section>

<p class="note">Frontend build: <strong>{FRONTEND_BUILD}</strong>. If this version is not visible, the browser or ESP32 is serving an older HTML file.</p>
<MethodNote />
