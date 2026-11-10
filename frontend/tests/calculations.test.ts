import { describe, expect, it } from 'vitest';
import { calculateDewPoint, calculateSurfaceHumidity } from '../src/lib/calculations/psychrometrics';
import { calculateExposure } from '../src/lib/calculations/exposure';
import { getGapLimit, insertGaps } from '../src/lib/calculations/gaps';
import { parseMeasurement, processMeasurements } from '../src/lib/api/measurements';
import type { Measurement } from '../src/types/measurement';

const NOW = 1_800_000_000;
const sample = (time: number, airRh = 50): Measurement => ({
  timestamp: time,
  time: new Date(time * 1000),
  ds18b20_temperature: 18,
  aht20_temperature: 20,
  aht20_humidity: airRh,
  dew_point: 9,
  surface_humidity: airRh,
  condensation_margin: 9,
});

describe('psychrometrics', () => {
  it('computes the expected dew point and rejects impossible humidity', () => {
    expect(calculateDewPoint(20, 50)).toBeCloseTo(9.255, 2);
    expect(calculateDewPoint(20, 0)).toBeNull();
  });
  it('caps surface humidity at saturation', () => {
    expect(calculateSurfaceHumidity(25, 95, 10)).toBe(100);
  });
});

describe('data cleaning', () => {
  it('accepts Unix milliseconds', () => {
    const result = parseMeasurement({
      timestamp: (NOW - 60) * 1000,
      ds18b20_temperature: 18,
      aht20_temperature: 20,
      aht20_humidity: 50,
    }, NOW);
    expect(result.measurement?.timestamp).toBe(NOW - 60);
    expect(result.convertedMilliseconds).toBe(true);
  });
  it('keeps the last duplicate timestamp and counts rejected rows', () => {
    const rows = [
      { timestamp: NOW - 120, ds18b20_temperature: 18, aht20_temperature: 20, aht20_humidity: 50 },
      { timestamp: NOW - 120, ds18b20_temperature: 19, aht20_temperature: 20, aht20_humidity: 50 },
      { timestamp: NOW - 60, ds18b20_temperature: 18, aht20_temperature: 20, aht20_humidity: -1 },
    ];
    const result = processMeasurements(rows, NOW);
    expect(result.measurements).toHaveLength(1);
    expect(result.measurements[0].ds18b20_temperature).toBe(19);
    expect(result.duplicateCount).toBe(1);
    expect(result.invalidCount).toBe(1);
  });
});

describe('gap-aware charts and exposure', () => {
  it('inserts missing chart segments and ignores long gaps in exposure', () => {
    const measurements = [sample(NOW - 3600), sample(NOW - 60)];
    expect(getGapLimit(measurements)).toBe(180);
    const chartPoints = insertGaps(measurements, 180);
    expect(chartPoints).toHaveLength(3);
    expect(chartPoints[1].surface_humidity).toBeNull();
    expect(calculateExposure(measurements, 7, NOW, 180).coverage).toBe(0);
  });
  it('integrates a continuous interval', () => {
    const measurements = [sample(NOW - 60, 70), sample(NOW, 90)];
    const result = calculateExposure(measurements, 7, NOW, 180);
    expect(result.avg).toBeCloseTo(80);
    expect(result.above80).toBeCloseTo(30);
  });
});
