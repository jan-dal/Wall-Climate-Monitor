<script lang="ts">
  import type { Measurement, StatsSpec } from '../types/measurement';
  import type { ChartKind } from '../lib/charts/figures';
  import MeasurementChart from './MeasurementChart.svelte';
  import StatisticsPanel from './StatisticsPanel.svelte';

  interface Props {
    kind: ChartKind;
    measurements: Measurement[];
    gapLimitSeconds: number;
  }
  let { kind, measurements, gapLimitSeconds }: Props = $props();
  let showDewPoint = $state(false);

  const statsByChart: Record<ChartKind, readonly StatsSpec[]> = {
    temperature: [
      { title: 'Wall temperature', key: 'ds18b20_temperature', unit: '°C' },
      { title: 'Air temperature', key: 'aht20_temperature', unit: '°C' },
    ],
    humidity: [
      { title: 'Air relative humidity', key: 'aht20_humidity', unit: '%' },
      { title: 'Estimated surface RH', key: 'surface_humidity', unit: '%' },
    ],
    margin: [
      { title: 'Condensation margin', key: 'condensation_margin', unit: '°C' },
      { title: 'Dew point', key: 'dew_point', unit: '°C' },
    ],
  };
</script>

<div class="graph-row">
  <div class="chart-wrap">
    {#if kind === 'temperature'}
      <div class="chart-controls">
        <label class="chart-toggle">
          <input type="checkbox" bind:checked={showDewPoint} />
          Show dew point
        </label>
      </div>
    {/if}

    <MeasurementChart {kind} {measurements} {gapLimitSeconds} bind:showDewPoint />

    {#if kind === 'humidity'}
      <p class="chart-note">
        The 80% and 95% reference lines apply to the estimated wall-surface RH,
        not to the air RH trace. These are monitoring references, not universal
        mold-risk thresholds.
      </p>
    {/if}
  </div>
  <StatisticsPanel {measurements} specs={statsByChart[kind]} />
</div>
