import type { ChartPoint, Measurement } from '../../types/measurement';
import {
  EXPECTED_INTERVAL_SECONDS, FALLBACK_MAX_GAP_SECONDS,
  GAP_MULTIPLIER, MIN_GAP_SECONDS,
} from '../config';

function median(numbers: number[]): number {
  const sorted = numbers.slice().sort((a, b) => a - b);
  const middle = Math.floor(sorted.length / 2);
  return sorted.length % 2 === 1
    ? sorted[middle]
    : (sorted[middle - 1] + sorted[middle]) / 2;
}

export function getGapLimit(measurements: Measurement[]): number {
  if (EXPECTED_INTERVAL_SECONDS !== null && Number.isFinite(EXPECTED_INTERVAL_SECONDS) &&
      EXPECTED_INTERVAL_SECONDS > 0) {
    return Math.max(MIN_GAP_SECONDS, GAP_MULTIPLIER * EXPECTED_INTERVAL_SECONDS);
  }

  const intervals: number[] = [];
  const start = Math.max(1, measurements.length - 2000);
  for (let i = start; i < measurements.length; i++) {
    const delta = measurements[i].timestamp - measurements[i - 1].timestamp;
    if (delta > 0) intervals.push(delta);
  }
  if (intervals.length < 4) return FALLBACK_MAX_GAP_SECONDS;
  return Math.max(MIN_GAP_SECONDS, GAP_MULTIPLIER * median(intervals));
}

export function insertGaps(measurements: Measurement[], maxGapSeconds: number): ChartPoint[] {
  const result: ChartPoint[] = [];
  for (let i = 0; i < measurements.length; i++) {
    if (i > 0) {
      const previous = measurements[i - 1];
      const current = measurements[i];
      if (current.timestamp - previous.timestamp > maxGapSeconds) {
        result.push({
          time: new Date((previous.timestamp + current.timestamp) / 2 * 1000),
          ds18b20_temperature: null,
          aht20_temperature: null,
          aht20_humidity: null,
          surface_humidity: null,
          dew_point: null,
          condensation_margin: null,
        });
      }
    }
    result.push(measurements[i]);
  }
  return result;
}
