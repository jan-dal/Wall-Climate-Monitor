<script lang="ts">
  import { onMount } from 'svelte';
  import uPlot from 'uplot';
  import 'uplot/dist/uPlot.min.css';
  import type { Measurement } from '../types/measurement';
  import type { ChartKind } from '../lib/charts/figures';
  import { getChartSpec, makeChartData } from '../lib/charts/figures';
  import { chartDecorations } from '../lib/charts/decorations';
  import { insertGaps } from '../lib/calculations/gaps';

  interface Props {
    kind: ChartKind;
    measurements: Measurement[];
    gapLimitSeconds: number;
    showDewPoint?: boolean;
  }

  let { kind, measurements, gapLimitSeconds, showDewPoint = $bindable(false) }: Props = $props();
  const spec = getChartSpec(kind);

  let host: HTMLDivElement;
  let plot: uPlot | null = null;
  let ready = $state(false);
  let status = $state('');
  let mode = $state<'zoom' | 'pan'>('zoom');
  let visible = $state<boolean[]>(spec.lines.map((_, i) => kind !== 'temperature' || i !== 2));

  interface HoverRow { label: string; color: string; value: number | null }
  interface HoverData { timestamp: string; left: number; top: number; rows: HoverRow[] }
  let hover = $state<HoverData | null>(null);

  const CHART_HEIGHT = 285;

  function isVisible(seriesIndex: number): boolean {
    return kind === 'temperature' && seriesIndex === 3
      ? showDewPoint
      : visible[seriesIndex - 1];
  }

  function toggleSeries(seriesIndex: number): void {
    if (!plot) return;
    if (kind === 'temperature' && seriesIndex === 3) {
      showDewPoint = !showDewPoint;
    } else {
      const next = visible.slice();
      next[seriesIndex - 1] = !next[seriesIndex - 1];
      visible = next;
      plot.setSeries(seriesIndex, { show: next[seriesIndex - 1] });
    }
  }

  function updateHover(chart: uPlot): void {
    const idx = chart.cursor.idx;
    if (idx == null || !chart.data[0]?.length) {
      hover = null;
      return;
    }
    const seconds = chart.data[0][idx];
    if (seconds == null) { hover = null; return; }
    const ratio = chart.over ? chart.bbox.width / Math.max(chart.over.clientWidth, 1) : 1;
    let left = chart.bbox.left / ratio + chart.cursor.left + 15;
    // Keep the unified hover tooltip in the available chart width.
    if (left + 225 > host.clientWidth) left -= 250;
    left = Math.max(2, left);
    const top = Math.max(6, chart.bbox.top / ratio + chart.cursor.top - 10);
    hover = {
      timestamp: new Date(seconds * 1000).toLocaleString(),
      left,
      top,
      rows: spec.lines.flatMap((line, i) => {
        if (!chart.series[i + 1].show) return [];
        const value = chart.data[i + 1][idx];
        return [{ label: line.label, color: line.color, value: value ?? null }];
      }),
    };
  }

  function resetZoom(): void {
    if (!plot) return;
    // Re-ranging resets the X selection without changing the dataset.
    plot.setData(plot.data, true);
  }

  function downloadPNG(): void {
    if (!plot) return;
    const canvas = plot.ctx.canvas;
    const ratio = canvas.width / plot.width;
    const headingHeight = Math.ceil(58 * ratio);
    const output = document.createElement('canvas');
    output.width = canvas.width;
    output.height = canvas.height + headingHeight;
    const ctx = output.getContext('2d');
    if (!ctx) return;
    ctx.fillStyle = '#ffffff';
    ctx.fillRect(0, 0, output.width, output.height);
    ctx.drawImage(canvas, 0, headingHeight);
    ctx.save();
    ctx.scale(ratio, ratio);
    ctx.fillStyle = '#344154';
    ctx.font = 'bold 16px sans-serif';
    ctx.fillText(spec.title, 12, 23);
    ctx.font = '12px sans-serif';
    let x = 12;
    spec.lines.forEach((line, i) => {
      if (!isVisible(i + 1)) return;
      ctx.fillStyle = line.color;
      ctx.fillRect(x, 39, 14, 3);
      ctx.fillStyle = '#344154';
      ctx.fillText(line.label, x + 20, 44);
      x += ctx.measureText(line.label).width + 40;
    });
    ctx.restore();
    output.toBlob(blob => {
      if (!blob) return;
      const url = URL.createObjectURL(blob);
      const link = document.createElement('a');
      link.href = url;
      link.download = `esp32-${kind}-chart.png`;
      link.click();
      setTimeout(() => URL.revokeObjectURL(url), 1000);
    }, 'image/png');
  }

  onMount(() => {
    if (measurements.length === 0) {
      status = 'No valid data to plot.';
    }
    try {
      const points = insertGaps(measurements, gapLimitSeconds);
      const data = makeChartData(points, spec);
      const width = Math.max(220, Math.round(host.clientWidth));
      const options: uPlot.Options = {
        width,
        height: CHART_HEIGHT,
        legend: { show: false }, // Render a Plotly-style horizontal legend ourselves.
        cursor: { drag: { x: true, y: kind !== 'humidity' } },
        select: { show: true },
        scales: { x: { time: true }, y: spec.yRange ? { range: spec.yRange } : {} },
        axes: [
          { label: 'Time', stroke: '#344154', grid: { stroke: '#e5eaf0' } },
          { label: spec.yLabel, stroke: '#344154', grid: { stroke: '#e5eaf0' }, size: 60 },
        ],
        series: [
          {},
          ...spec.lines.map((line, i) => ({
            label: line.label,
            stroke: line.color,
            width: 2,
            spanGaps: false,
            show: kind !== 'temperature' || i !== 2 || showDewPoint,
          })),
        ],
        plugins: [chartDecorations(spec)],
        hooks: { setCursor: [updateHover] },
      };
      plot = new uPlot(options, data, host);
      ready = true;
    } catch (error) {
      status = `Charts unavailable: ${error instanceof Error ? error.message : String(error)}`;
      return;
    }

    const chart = plot as uPlot;
    const observer = new ResizeObserver(entries => {
      const width = Math.round(entries[0]?.contentRect.width ?? host.clientWidth);
      if (width > 0 && width !== chart.width) chart.setSize({ width, height: CHART_HEIGHT });
    });
    observer.observe(host);

    // uPlot handles left-drag selection/zoom; intercept it only while Pan is selected.
    const overlay = chart.over;
    let dragStart: { x: number; min: number; max: number } | null = null;
    const endPan = () => {
      dragStart = null;
      window.removeEventListener('mousemove', movePan, true);
      window.removeEventListener('mouseup', endPan, true);
    };
    const movePan = (event: MouseEvent) => {
      if (!dragStart || !plot) return;
      event.stopPropagation();
      const plotWidth = plot.over?.clientWidth ?? plot.width;
      const delta = (event.clientX - dragStart.x) / Math.max(1, plotWidth) *
        (dragStart.max - dragStart.min);
      plot.setScale('x', { min: dragStart.min - delta, max: dragStart.max - delta });
    };
    const clearHover = () => { hover = null; };
    const startPan = (event: MouseEvent) => {
      if (mode !== 'pan' || event.button !== 0 || !plot) return;
      const xScale = plot.scales.x;
      if (xScale.min == null || xScale.max == null) return;
      dragStart = { x: event.clientX, min: xScale.min, max: xScale.max };
      event.stopImmediatePropagation();
      event.preventDefault();
      window.addEventListener('mousemove', movePan, true);
      window.addEventListener('mouseup', endPan, true);
    };
    overlay?.addEventListener('mousedown', startPan, true);
    overlay?.addEventListener('dblclick', resetZoom);
    overlay?.addEventListener('mouseleave', clearHover);

    return () => {
      endPan();
      overlay?.removeEventListener('mousedown', startPan, true);
      overlay?.removeEventListener('dblclick', resetZoom);
      overlay?.removeEventListener('mouseleave', clearHover);
      observer.disconnect();
      chart.destroy();
      plot = null;
    };
  });

  // Data refreshes recreate aligned columns, but do not recreate the chart instance.
  $effect(() => {
    const snapshot = measurements;
    const maxGap = gapLimitSeconds;
    if (!ready || !plot) return;
    plot.setData(makeChartData(insertGaps(snapshot, maxGap), spec), true);
    status = snapshot.length ? '' : 'No valid data to plot.';
    hover = null;
  });

  // The external dew-point checkbox only toggles series visibility, preserving zoom.
  $effect(() => {
    const show = showDewPoint;
    if (ready && plot && kind === 'temperature') plot.setSeries(3, { show });
  });
</script>

<div class="history-figure">
  <h3 class="history-figure-title">{spec.title}</h3>
  <div class="history-figure-topbar">
    <div class="history-legend" aria-label="Chart series">
      {#each spec.lines as line, i}
        <button type="button" class="history-legend-item" class:inactive={!isVisible(i + 1)}
          onclick={() => toggleSeries(i + 1)} title={`Toggle ${line.label}`}>
          <span class="history-legend-line" style:background-color={line.color}></span>{line.label}
        </button>
      {/each}
    </div>
    <div class="history-chart-tools" aria-label="Chart tools">
      <button type="button" class:active={mode === 'zoom'} onclick={() => mode = 'zoom'} title="Drag to zoom">Zoom</button>
      <button type="button" class:active={mode === 'pan'} onclick={() => mode = 'pan'} title="Drag to pan">Pan</button>
      <button type="button" onclick={resetZoom} title="Reset zoom">Reset</button>
      <button type="button" onclick={downloadPNG} title="Download chart as PNG">PNG</button>
    </div>
  </div>
  <div class="history-plot-container">
    <div class="history-plot" bind:this={host}></div>
    {#if hover}
      <div class="history-tooltip" style:left={`${hover.left}px`} style:top={`${hover.top}px`}>
        <strong>{hover.timestamp}</strong>
        {#each hover.rows as row}
          <div class="history-tooltip-row">
            <span class="history-tooltip-key"><i style:background-color={row.color}></i>{row.label}</span>
            <strong>{row.value === null ? '—' : row.value.toFixed(1)}</strong>
          </div>
        {/each}
      </div>
    {/if}
  </div>
</div>
{#if status}
  <p class="status-line" class:warning={status.startsWith('Charts unavailable')} role="status">{status}</p>
{/if}
