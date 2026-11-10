import type { Exposure, Measurement } from '../../types/measurement';
import { SECONDS_PER_DAY } from '../config';

function interpolate(a: number, b: number, fraction: number): number {
  return a + (b - a) * fraction;
}

/** Fraction of time a linearly interpolated interval stays >= threshold. */
function fractionAtOrAbove(a: number, b: number, threshold: number): number {
  if (a >= threshold && b >= threshold) return 1;
  if (a < threshold && b < threshold) return 0;
  const crossing = (threshold - a) / (b - a);
  return a >= threshold ? crossing : 1 - crossing;
}

/** Fraction of time a linearly interpolated interval stays <= threshold. */
function fractionAtOrBelow(a: number, b: number, threshold: number): number {
  if (a <= threshold && b <= threshold) return 1;
  if (a > threshold && b > threshold) return 0;
  const crossing = (threshold - a) / (b - a);
  return a <= threshold ? crossing : 1 - crossing;
}

/**
 * Time-weighted exposure; doesn't interpolate through missing-data gaps.
 * `measurements` must be ascending by timestamp with duplicates removed.
 */
export function calculateExposure(
  measurements: Measurement[],
  days: number,
  nowSeconds: number,
  maxGapSeconds: number
): Exposure {
  const windowStart = nowSeconds - days * SECONDS_PER_DAY;
  let observedSeconds = 0;
  let rhArea = 0;
  let above80Seconds = 0;
  let above95Seconds = 0;
  let condensationSeconds = 0;
  let minMargin: number | null = null;

  for (const m of measurements) {
    if (m.timestamp >= windowStart && m.timestamp <= nowSeconds) {
      minMargin = minMargin === null
        ? m.condensation_margin
        : Math.min(minMargin, m.condensation_margin);
    }
  }

  for (let i = 1; i < measurements.length; i++) {
    const previous = measurements[i - 1];
    const current = measurements[i];
    const originalDuration = current.timestamp - previous.timestamp;
    if (originalDuration <= 0 || originalDuration > maxGapSeconds) continue;

    const from = Math.max(previous.timestamp, windowStart);
    const to = Math.min(current.timestamp, nowSeconds);
    if (to <= from) continue;

    const startFraction = (from - previous.timestamp) / originalDuration;
    const endFraction = (to - previous.timestamp) / originalDuration;
    const duration = to - from;
    const startRH = interpolate(previous.surface_humidity, current.surface_humidity, startFraction);
    const endRH = interpolate(previous.surface_humidity, current.surface_humidity, endFraction);
    const startMargin = interpolate(previous.condensation_margin, current.condensation_margin, startFraction);
    const endMargin = interpolate(previous.condensation_margin, current.condensation_margin, endFraction);

    observedSeconds += duration;
    rhArea += duration * (startRH + endRH) / 2;
    above80Seconds += duration * fractionAtOrAbove(startRH, endRH, 80);
    above95Seconds += duration * fractionAtOrAbove(startRH, endRH, 95);
    condensationSeconds += duration * fractionAtOrBelow(startMargin, endMargin, 0);

    const intervalMin = Math.min(startMargin, endMargin);
    minMargin = minMargin === null ? intervalMin : Math.min(minMargin, intervalMin);
  }

  return {
    avg: observedSeconds > 0 ? rhArea / observedSeconds : null,
    above80: observedSeconds > 0 ? above80Seconds : null,
    above95: observedSeconds > 0 ? above95Seconds : null,
    condensation: observedSeconds > 0 ? condensationSeconds : null,
    minMargin,
    coverage: 100 * observedSeconds / (days * SECONDS_PER_DAY),
  };
}
