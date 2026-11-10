<script lang="ts">
  import type { Measurement } from '../types/measurement';
  import { calculateExposure } from '../lib/calculations/exposure';
  import ExposureCard from './ExposureCard.svelte';

  interface Props {
    measurements: Measurement[];
    nowSeconds: number;
    gapLimitSeconds: number;
  }
  let { measurements, nowSeconds, gapLimitSeconds }: Props = $props();
  let week = $derived(calculateExposure(measurements, 7, nowSeconds, gapLimitSeconds));
  let month = $derived(calculateExposure(measurements, 30, nowSeconds, gapLimitSeconds));
</script>

<section aria-labelledby="exposure-title">
  <h2 class="section-heading" id="exposure-title">Surface humidity exposure</h2>
  <div class="exposure-grid">
    <ExposureCard days={7} exposure={week} />
    <ExposureCard days={30} exposure={month} />
  </div>
  <p class="note">
    Exposure durations use linear interpolation between consecutive valid measurements,
    excluding gaps longer than three times the typical measurement interval.
    Coverage is the percentage of each full 7- or 30-day window actually represented by
    those intervals. A limited coverage percentage means the exposure metrics are
    incomplete, not that conditions were safe during missing periods.
  </p>
</section>
