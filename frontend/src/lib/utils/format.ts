import type { Measurement } from '../../types/measurement';
import { SECONDS_PER_DAY } from '../config';

export function formatValue(value: number | null | undefined, unit = '°C', digits = 1): string {
  return value == null || !Number.isFinite(value) ? '--' : `${value.toFixed(digits)} ${unit}`;
}

export function formatDuration(seconds: number | null): string {
  if (seconds === null || !Number.isFinite(seconds)) return '--';
  if (seconds < 3600) return (seconds / 60).toFixed(0) + ' min';
  if (seconds < 48 * 3600) return (seconds / 3600).toFixed(1) + ' h';
  return (seconds / SECONDS_PER_DAY).toFixed(1) + ' d';
}

export function formatCoverage(coverage: number): string {
  return coverage > 0 && coverage < 0.1 ? '<0.1%' : coverage.toFixed(1) + '%';
}

export interface Freshness {
  text: string;
  warning: boolean;
  latestTime: string;
}

export function getFreshness(
  measurements: Measurement[],
  nowSeconds: number,
  maxGapSeconds: number
): Freshness {
  if (measurements.length === 0) return { text: '', warning: false, latestTime: '--' };
  const latest = measurements[measurements.length - 1];
  const ageSeconds = nowSeconds - latest.timestamp;
  const staleAfterSeconds = Math.max(300, maxGapSeconds);
  if (ageSeconds > staleAfterSeconds) {
    return {
      text: `Warning: latest reading is ${formatDuration(ageSeconds)} old. Check whether the ESP32 is still recording.`,
      warning: true,
      latestTime: latest.time.toLocaleString(),
    };
  }
  if (ageSeconds < -60) {
    return {
      text: 'Warning: latest record is in the future; check time synchronization.',
      warning: true,
      latestTime: latest.time.toLocaleString(),
    };
  }
  return {
    text: 'Latest reading appears recent.',
    warning: false,
    latestTime: latest.time.toLocaleString(),
  };
}
