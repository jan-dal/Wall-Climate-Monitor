<script lang="ts">
  import type { Measurement, StatsSpec } from '../types/measurement';
  import { calculateStats } from '../lib/calculations/statistics';
  import { formatValue } from '../lib/utils/format';

  let { measurements, specs }: { measurements: Measurement[]; specs: readonly StatsSpec[] } = $props();
</script>

<aside class="stats-panel">
  {#each specs as spec (spec.key)}
    {@const stats = calculateStats(measurements, spec.key)}
    <div class="stats-group">
      <h3>{spec.title}</h3>
      <p>Minimum <strong>{formatValue(stats.min, spec.unit)}</strong></p>
      <p>Maximum <strong>{formatValue(stats.max, spec.unit)}</strong></p>
      <p>Sample average <strong>{formatValue(stats.avg, spec.unit)}</strong></p>
    </div>
  {/each}
</aside>
