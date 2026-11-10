/** Fields as emitted by /api/data. The parsing layer still validates unknown input. */
export interface RawMeasurement {
  timestamp: number;
  ds18b20_temperature: number;
  aht20_temperature: number;
  aht20_humidity: number;
}

/** Validated sensor reading, extended with browser-side calculations. */
export interface Measurement extends RawMeasurement {
  time: Date;
  dew_point: number;
  surface_humidity: number;
  condensation_margin: number;
}

export type MeasurementValueKey =
  | 'ds18b20_temperature'
  | 'aht20_temperature'
  | 'aht20_humidity'
  | 'dew_point'
  | 'surface_humidity'
  | 'condensation_margin';

/** Null values intentionally break chart lines across missing-data intervals. */
export type ChartPoint = Pick<Measurement, 'time'> & {
  [K in MeasurementValueKey]: number | null;
};

export interface StatsSpec {
  title: string;
  key: MeasurementValueKey;
  unit: string;
}

export interface Stats {
  min: number | null;
  max: number | null;
  avg: number | null;
}

export interface Exposure {
  avg: number | null;
  above80: number | null;
  above95: number | null;
  condensation: number | null;
  minMargin: number | null;
  coverage: number;
}

export interface ParseResult {
  measurement: Measurement | null;
  reason: string | null;
  convertedMilliseconds: boolean;
}

export interface ProcessedMeasurements {
  measurements: Measurement[];
  rawCount: number;
  invalidCount: number;
  duplicateCount: number;
  convertedMsCount: number;
  diagnostics: string;
}
