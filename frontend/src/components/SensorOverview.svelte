<script lang="ts">
  import type { Measurement } from '../types/measurement';
  import SensorCard from './SensorCard.svelte';

  let { latest }: { latest: Measurement | null } = $props();
</script>

<section class="values" aria-label="Latest sensor readings">
  <SensorCard label="Wall temperature" value={latest?.ds18b20_temperature} unit="°C" />
  <SensorCard label="Air temperature" value={latest?.aht20_temperature} unit="°C" />
  <SensorCard label="Air RH" value={latest?.aht20_humidity} unit="%" />
  <SensorCard label="Estimated surface RH" value={latest?.surface_humidity} unit="%" />
  <SensorCard label="Dew point" value={latest?.dew_point} unit="°C" />
  <SensorCard label="Condensation margin" value={latest?.condensation_margin} unit="°C" />
</section>
