import type uPlot from 'uplot';
import type { ChartSpec } from './figures';

/** Match Plotly's translucent RH regions and dashed threshold lines. */
export function chartDecorations(spec: ChartSpec): uPlot.Plugin {
  const clipToPlot = (plot: uPlot): void => {
    const { ctx, bbox } = plot;
    ctx.beginPath();
    ctx.rect(bbox.left, bbox.top, bbox.width, bbox.height);
    ctx.clip();
  };

  return {
    hooks: {
      // The canvas has just been cleared; regions appear behind grid and data.
      drawClear: [plot => {
        if (!spec.bands?.length) return;
        const { ctx, bbox } = plot;
        ctx.save();
        clipToPlot(plot);
        for (const band of spec.bands) {
          const top = plot.valToPos(band.to, 'y', true);
          const bottom = plot.valToPos(band.from, 'y', true);
          if (!Number.isFinite(top) || !Number.isFinite(bottom)) continue;
          ctx.fillStyle = band.color;
          ctx.fillRect(bbox.left, top, bbox.width, bottom - top);
        }
        ctx.restore();
      }],
      // Plotly's reference shapes overlay plotted lines.
      draw: [plot => {
        if (!spec.references?.length) return;
        const { ctx, bbox } = plot;
        const pixelRatio = plot.over ? bbox.width / Math.max(plot.over.clientWidth, 1) : 1;
        ctx.save();
        clipToPlot(plot);
        ctx.lineWidth = 1.5 * pixelRatio;
        ctx.setLineDash([5 * pixelRatio, 4 * pixelRatio]);
        for (const reference of spec.references) {
          const y = plot.valToPos(reference.y, 'y', true);
          if (!Number.isFinite(y)) continue;
          ctx.strokeStyle = reference.color;
          ctx.beginPath();
          ctx.moveTo(bbox.left, y);
          ctx.lineTo(bbox.left + bbox.width, y);
          ctx.stroke();
        }
        ctx.restore();
      }],
    },
  };
}
