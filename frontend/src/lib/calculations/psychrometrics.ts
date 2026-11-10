/** Magnus approximation (degrees Celsius). */
const A = 17.62;
const B = 243.12;

function saturationFactor(temperature: number): number {
  return Math.exp((A * temperature) / (B + temperature));
}

export function calculateDewPoint(airTemperature: number, humidity: number): number | null {
  if (![airTemperature, humidity].every(Number.isFinite) || humidity <= 0 || humidity > 100) {
    return null;
  }
  const gamma = Math.log(humidity / 100) + (A * airTemperature) / (B + airTemperature);
  const dewPoint = (B * gamma) / (A - gamma);
  return Number.isFinite(dewPoint) ? dewPoint : null;
}

export function calculateSurfaceHumidity(
  airTemperature: number,
  humidity: number,
  wallTemperature: number
): number | null {
  if (![airTemperature, humidity, wallTemperature].every(Number.isFinite) ||
      humidity <= 0 || humidity > 100) {
    return null;
  }
  const theoreticalRH = humidity * saturationFactor(airTemperature) /
    saturationFactor(wallTemperature);
  if (!Number.isFinite(theoreticalRH)) return null;
  // >100% theoretical supersaturation is displayed as saturation (not liquid water).
  return Math.max(0, Math.min(100, theoreticalRH));
}
