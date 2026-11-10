<script lang="ts">
  import type { Exposure } from '../types/measurement';
  import { formatCoverage, formatDuration, formatValue } from '../lib/utils/format';

  let { days, exposure }: { days: 7 | 30; exposure: Exposure } = $props();
</script>

<div class="exposure-card">
  <h3>Last {days} days</h3>
  <p>Time-weighted surface RH <strong>{formatValue(exposure.avg, '%')}</strong></p>
  <p>Time at or above 80% RH <strong>{formatDuration(exposure.above80)}</strong></p>
  <p>Time at or above 95% RH <strong>{formatDuration(exposure.above95)}</strong></p>
  <p>Time at/below dew point <strong>{formatDuration(exposure.condensation)}</strong></p>
  <p>Lowest condensation margin <strong>{formatValue(exposure.minMargin, '°C')}</strong></p>
  <p>Time with usable coverage <strong>{formatCoverage(exposure.coverage)}</strong></p>
</div>
