import type { Measurement, MeasurementValueKey, Stats } from '../../types/measurement';

export function calculateStats(measurements: Measurement[], key: MeasurementValueKey): Stats {
  let min = Infinity;
  let max = -Infinity;
  let sum = 0;
  let count = 0;

  for (const measurement of measurements) {
    const value = measurement[key];
    if (!Number.isFinite(value)) continue;
    min = Math.min(min, value);
    max = Math.max(max, value);
    sum += value;
    count++;
  }

  return count === 0
    ? { min: null, max: null, avg: null }
    : { min, max, avg: sum / count };
}
