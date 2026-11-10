import type { Measurement } from '../../types/measurement';

const CSV_KEYS = [
  'timestamp',
  'ds18b20_temperature',
  'aht20_temperature',
  'aht20_humidity',
  'dew_point',
  'surface_humidity',
  'condensation_margin',
] as const satisfies readonly (keyof Measurement)[];

export function buildCSV(measurements: Measurement[]): string {
  return [
    CSV_KEYS.join(','),
    ...measurements.map(m => CSV_KEYS.map(key => m[key]).join(',')),
  ].join('\r\n') + '\r\n';
}

export function downloadCSV(measurements: Measurement[]): void {
  if (measurements.length === 0) return;
  const blob = new Blob([buildCSV(measurements)], { type: 'text/csv;charset=utf-8' });
  const url = URL.createObjectURL(blob);
  const link = document.createElement('a');
  link.href = url;
  link.download = 'esp32-wall-surface-measurements.csv';
  document.body.appendChild(link);
  link.click();
  link.remove();
  setTimeout(() => URL.revokeObjectURL(url), 1000);
}
