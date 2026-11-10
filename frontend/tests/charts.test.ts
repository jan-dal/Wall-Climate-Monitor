import { describe, expect, it } from 'vitest';
import { getChartSpec, makeChartData } from '../src/lib/charts/figures';
import type { ChartPoint } from '../src/types/measurement';

const points: ChartPoint[] = [
  { time: new Date(1_800_000_000_000), ds18b20_temperature: 18, aht20_temperature: 20,
    aht20_humidity: 50, dew_point: 9, surface_humidity: 57, condensation_margin: 9 },
  { time: new Date(1_800_000_060_000), ds18b20_temperature: null, aht20_temperature: null,
    aht20_humidity: null, dew_point: null, surface_humidity: null, condensation_margin: null },
  { time: new Date(1_800_000_120_000), ds18b20_temperature: 17, aht20_temperature: 20,
    aht20_humidity: 55, dew_point: 11, surface_humidity: 61, condensation_margin: 6 },
];

describe('uPlot configuration', () => {
  it('retains the old humidity thresholds and colors', () => {
    const spec = getChartSpec('humidity');
    expect(spec.yRange).toEqual([0, 100]);
    expect(spec.references?.map(reference => reference.y)).toEqual([80, 95]);
    expect(spec.lines.map(line => line.color)).toEqual(['#1f77b4', '#9467bd']);
  });
  it('uses aligned Unix seconds with null gaps', () => {
    const data = makeChartData(points, getChartSpec('temperature'));
    expect(data[0]).toEqual([1_800_000_000, 1_800_000_060, 1_800_000_120]);
    expect(data[1]).toEqual([18, null, 17]);
    expect(data).toHaveLength(4);
  });
});
