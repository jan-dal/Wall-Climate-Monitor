import type uPlot from 'uplot';
import type { ChartPoint, MeasurementValueKey } from '../../types/measurement';

export type ChartKind = 'temperature' | 'humidity' | 'margin';

export interface ChartLine {
  key: MeasurementValueKey;
  label: string;
  color: string;
}

export interface ChartSpec {
  title: string;
  yLabel: string;
  lines: ChartLine[];
  /** If present, prevents automatic Y scaling (matching the original RH chart). */
  yRange?: [number, number];
  /** Drawn underneath the data lines, within the plotting area. */
  bands?: Array<{ from: number; to: number; color: string }>;
  /** Horizontal dashed reference lines, drawn above the data. */
  references?: Array<{ y: number; color: string }>;
}

const SPECS: Record<ChartKind, ChartSpec> = {
  temperature: {
    title: 'Wall and Air Temperature',
    yLabel: 'Temperature (°C)',
    lines: [
      { key: 'ds18b20_temperature', label: 'Wall', color: '#d62728' },
      { key: 'aht20_temperature', label: 'Air', color: '#ff7f0e' },
      { key: 'dew_point', label: 'Dew Point', color: '#2563eb' },
    ],
  },
  humidity: {
    title: 'Air and Wall-Surface Humidity',
    yLabel: 'Relative Humidity (%)',
    yRange: [0, 100],
    lines: [
      { key: 'aht20_humidity', label: 'Air RH', color: '#1f77b4' },
      { key: 'surface_humidity', label: 'Estimated Surface RH', color: '#9467bd' },
    ],
    bands: [
      { from: 80, to: 95, color: 'rgba(255,193,7,0.08)' },
      { from: 95, to: 100, color: 'rgba(220,53,69,0.09)' },
    ],
    references: [
      { y: 80, color: '#c68b20' },
      { y: 95, color: '#c44751' },
    ],
  },
  margin: {
    title: 'Condensation Margin',
    yLabel: 'Wall − Dew Point (°C)',
    lines: [
      { key: 'condensation_margin', label: 'Condensation Margin', color: '#238b57' },
    ],
    references: [{ y: 0, color: '#d62728' }],
  },
};

export function getChartSpec(kind: ChartKind): ChartSpec {
  return SPECS[kind];
}

/** uPlot requires sorted, unique Unix-second X values and one aligned Y column per series. */
export function makeChartData(points: ChartPoint[], spec: ChartSpec): uPlot.AlignedData {
  return [
    points.map(point => point.time.getTime() / 1000),
    ...spec.lines.map(line => points.map(point => point[line.key])),
  ] as uPlot.AlignedData;
}
