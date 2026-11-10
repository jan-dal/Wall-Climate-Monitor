import { calculateDewPoint, calculateSurfaceHumidity } from '../calculations/psychrometrics';
import { MAX_TEMPERATURE_C, MIN_TEMPERATURE_C, MIN_VALID_UNIX_TIMESTAMP, SECONDS_PER_DAY } from '../config';
import type { ParseResult, ProcessedMeasurements } from '../../types/measurement';

/** Do not trust JSON type assertions here: invalid records are reported, not hidden. */
export async function fetchMeasurements(): Promise<unknown[]> {
  const response = await fetch('/api/data', { cache: 'no-store' });
  if (!response.ok) throw new Error(`HTTP ${response.status}`);
  const raw: unknown = await response.json();
  if (!Array.isArray(raw)) throw new Error('Invalid API response: expected an array');
  return raw;
}

function asFiniteNumber(value: unknown): number | null {
  if (typeof value === 'number') return Number.isFinite(value) ? value : null;
  if (typeof value !== 'string' || value.trim() === '') return null;
  const number = Number(value);
  return Number.isFinite(number) ? number : null;
}

export function parseMeasurement(record: unknown, nowSeconds: number): ParseResult {
  const invalid = (reason: string): ParseResult => ({
    measurement: null,
    reason,
    convertedMilliseconds: false,
  });

  if (!record || typeof record !== 'object' || Array.isArray(record)) {
    return invalid('record is not an object');
  }
  const row = record as Record<string, unknown>;
  const rawTimestamp = asFiniteNumber(row.timestamp);
  if (rawTimestamp === null) return invalid('missing or nonnumeric timestamp');

  const convertedMilliseconds = rawTimestamp > 1e11;
  const timestamp = convertedMilliseconds ? rawTimestamp / 1000 : rawTimestamp;
  if (timestamp < MIN_VALID_UNIX_TIMESTAMP) {
    return invalid('timestamp before year 2000 (check ESP32 clock or timestamp units)');
  }
  if (timestamp > nowSeconds + SECONDS_PER_DAY) {
    return invalid('timestamp more than 24 hours in the future (check clock)');
  }

  const wall = asFiniteNumber(row.ds18b20_temperature);
  const air = asFiniteNumber(row.aht20_temperature);
  const rh = asFiniteNumber(row.aht20_humidity);
  if (wall === null) return invalid('missing/invalid ds18b20_temperature');
  if (air === null) return invalid('missing/invalid aht20_temperature');
  if (rh === null) return invalid('missing/invalid aht20_humidity');
  if (wall < MIN_TEMPERATURE_C || wall > MAX_TEMPERATURE_C) {
    return invalid('wall temperature outside permitted range');
  }
  if (air < MIN_TEMPERATURE_C || air > MAX_TEMPERATURE_C) {
    return invalid('air temperature outside permitted range');
  }
  if (rh <= 0 || rh > 100) return invalid('air humidity not between 0 and 100%');

  const time = new Date(timestamp * 1000);
  if (!Number.isFinite(time.getTime())) return invalid('invalid Date from timestamp');
  const dewPoint = calculateDewPoint(air, rh);
  const surfaceRH = calculateSurfaceHumidity(air, rh, wall);
  if (dewPoint === null || surfaceRH === null) {
    return invalid('dew-point or surface-RH calculation failed');
  }

  return {
    reason: null,
    convertedMilliseconds,
    measurement: {
      timestamp,
      time,
      ds18b20_temperature: wall,
      aht20_temperature: air,
      aht20_humidity: rh,
      dew_point: dewPoint,
      surface_humidity: surfaceRH,
      condensation_margin: wall - dewPoint,
    },
  };
}

/** Preserves original behavior: reject bad data, last duplicate wins, sort ascending. */
export function processMeasurements(raw: unknown[], nowSeconds: number): ProcessedMeasurements {
  const invalidReasons = new Map<string, number>();
  const byTimestamp = new Map<number, NonNullable<ParseResult['measurement']>>();
  let invalidCount = 0;
  let duplicateCount = 0;
  let convertedMsCount = 0;
  let firstInvalid: { record: unknown; reason: string } | null = null;

  for (const record of raw) {
    const result = parseMeasurement(record, nowSeconds);
    if (!result.measurement) {
      invalidCount++;
      const reason = result.reason ?? 'unknown validation error';
      invalidReasons.set(reason, (invalidReasons.get(reason) ?? 0) + 1);
      if (!firstInvalid) firstInvalid = { record, reason };
      continue;
    }
    if (result.convertedMilliseconds) convertedMsCount++;
    const m = result.measurement;
    if (byTimestamp.has(m.timestamp)) duplicateCount++;
    byTimestamp.set(m.timestamp, m);
  }

  const details: string[] = [];
  if (raw.length === 0) {
    details.push('/api/data returned an empty array ([]). Check ESP32 logging and storage.');
  }
  if (invalidCount > 0) {
    const breakdown = [...invalidReasons.entries()]
      .sort((a, b) => b[1] - a[1])
      .map(([reason, count]) => `${count} x ${reason}`).join('; ');
    details.push(`Invalid records: ${invalidCount}. Reasons: ${breakdown}.`);
    if (firstInvalid) {
      const example = JSON.stringify(firstInvalid.record) ?? String(firstInvalid.record);
      details.push('Example rejected record: ' +
        (example.length > 260 ? example.slice(0, 260) + '...' : example) +
        ` (reason: ${firstInvalid.reason}).`);
    }
  }
  if (duplicateCount > 0) {
    details.push(`Duplicate timestamps: ${duplicateCount} (last record kept).`);
  }
  if (convertedMsCount > 0) {
    details.push(`Normalized ${convertedMsCount} millisecond Unix timestamps to seconds.`);
  }

  return {
    measurements: [...byTimestamp.values()].sort((a, b) => a.timestamp - b.timestamp),
    rawCount: raw.length,
    invalidCount,
    duplicateCount,
    convertedMsCount,
    diagnostics: details.join(' '),
  };
}
