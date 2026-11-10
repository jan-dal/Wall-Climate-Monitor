<script lang="ts">
  interface Props {
    label: string;
    value: number | null | undefined;
    unit: string;
  }
  let { label, value, unit }: Props = $props();
</script>

<div class="value-card">
  <span class="label">{label}</span>
  <span class="reading">
    {value == null || !Number.isFinite(value) ? '--' : value.toFixed(1)}
    <span class="unit">{unit}</span>
  </span>
</div>
