// charts.js - a small line chart and the Gantt chart, drawn with plain SVG.

// ----------------------------------------------------------- line chart
//
// lineChart(container, {
//   series: [{ name, color, points: [[x, y], ...], dashed }],
//   xLabel, yLabel, logX, logY, yMax, percent
// })
function lineChart(container, opts) {
  // Draw at the card's real width so text keeps the same size in wide cards.
  const W = Math.max(480, container.clientWidth || 640), H = 330;
  const m = { left: 64, right: 18, top: 40, bottom: 48 };
  const svg = makeSvg(container, W, H);

  const all = opts.series.flatMap(s => s.points);
  if (all.length === 0) return;
  const tx = v => (opts.logX ? Math.log10(v) : v);
  const ty = v => (opts.logY ? Math.log10(Math.max(v, 1e-3)) : v);

  let xMin = Math.min(...all.map(p => tx(p[0]))), xMax = Math.max(...all.map(p => tx(p[0])));
  let yMin = opts.logY ? Math.min(...all.map(p => ty(p[1]))) : 0;
  let yMax = opts.yMax !== undefined ? opts.yMax : Math.max(...all.map(p => ty(p[1])));
  if (opts.logY) { yMin = Math.floor(yMin); yMax = Math.ceil(yMax); }
  if (xMax === xMin) xMax = xMin + 1;
  if (yMax === yMin) yMax = yMin + 1;

  const sx = v => m.left + (tx(v) - xMin) / (xMax - xMin) * (W - m.left - m.right);
  const sy = v => H - m.bottom - (ty(v) - yMin) / (yMax - yMin) * (H - m.top - m.bottom);

  // Grid and y labels.
  const yTicks = [];
  if (opts.logY) for (let e = yMin; e <= yMax; e++) yTicks.push(10 ** e);
  else {
    // Round steps (1, 2 or 5 times a power of ten) so labels read 0, 5, 10 ... not 0, 5.8, 11.6
    if (opts.yMax === undefined) {
      const raw = (yMax - yMin) / 5, power = 10 ** Math.floor(Math.log10(raw));
      const step = [1, 2, 5, 10].map(k => k * power).find(s => s >= raw);
      yMax = Math.ceil(yMax / step) * step;
      for (let v = yMin; v <= yMax + 1e-9; v += step) yTicks.push(v);
    } else {
      for (let i = 0; i <= 5; i++) yTicks.push(yMin + (yMax - yMin) * i / 5);
    }
  }
  for (const v of yTicks) {
    svg.appendChild(el('line', { x1: m.left, x2: W - m.right, y1: sy(v), y2: sy(v), stroke: '#eceff4' }));
    const label = opts.percent ? Math.round(v * 100) + '%' : formatNumber(v);
    svg.appendChild(el('text', { x: m.left - 8, y: sy(v) + 4, 'text-anchor': 'end', fill: '#6b7280', 'font-size': 12 }, label));
  }

  // x labels at the actual data x values (they are few).
  const xs = [...new Set(all.map(p => p[0]))].sort((a, b) => a - b);
  const every = Math.ceil(xs.length / 12);
  xs.forEach((v, i) => {
    if (i % every) return;
    svg.appendChild(el('text', { x: sx(v), y: H - m.bottom + 18, 'text-anchor': 'middle', fill: '#6b7280', 'font-size': 12 }, formatNumber(v)));
  });

  // Axes and axis titles.
  svg.appendChild(el('line', { x1: m.left, x2: W - m.right, y1: H - m.bottom, y2: H - m.bottom, stroke: '#9ca3af' }));
  svg.appendChild(el('line', { x1: m.left, x2: m.left, y1: m.top, y2: H - m.bottom, stroke: '#9ca3af' }));
  svg.appendChild(el('text', { x: (m.left + W - m.right) / 2, y: H - 8, 'text-anchor': 'middle', fill: '#374151', 'font-size': 13 }, opts.xLabel || ''));
  svg.appendChild(el('text', { x: 14, y: (m.top + H - m.bottom) / 2, 'text-anchor': 'middle', fill: '#374151', 'font-size': 13,
                               transform: `rotate(-90 14 ${(m.top + H - m.bottom) / 2})` }, opts.yLabel || ''));

  // The lines themselves, plus a hover tooltip on every point.
  for (const s of opts.series) {
    const d = s.points.map((p, i) => `${i ? 'L' : 'M'} ${sx(p[0])} ${sy(p[1])}`).join(' ');
    svg.appendChild(el('path', { d, fill: 'none', stroke: s.color, 'stroke-width': s.width || 2.5,
                                 'stroke-dasharray': s.dashed ? '7 5' : 'none' }));
    if (s.points.length <= 40) {
      for (const p of s.points) {
        const dot = el('circle', { cx: sx(p[0]), cy: sy(p[1]), r: s.dashed ? 5 : 3.5, fill: s.dashed ? 'none' : s.color, stroke: s.color, 'stroke-width': 2 });
        dot.appendChild(el('title', {}, `${s.name}: x = ${formatNumber(p[0])}, y = ${opts.percent ? (p[1] * 100).toFixed(1) + '%' : formatNumber(p[1])}`));
        svg.appendChild(dot);
      }
    }
  }

  // Legend across the top.
  let lx = m.left;
  for (const s of opts.series) {
    svg.appendChild(el('line', { x1: lx, x2: lx + 22, y1: 16, y2: 16, stroke: s.color, 'stroke-width': 3, 'stroke-dasharray': s.dashed ? '6 4' : 'none' }));
    const label = el('text', { x: lx + 28, y: 20, fill: '#374151', 'font-size': 12 }, s.name);
    svg.appendChild(label);
    lx += 40 + s.name.length * 6.6;
  }
}

function formatNumber(v) {
  if (v >= 1000) return Math.round(v).toLocaleString('en-US');
  if (v >= 10 || Number.isInteger(v)) return String(Math.round(v * 10) / 10);
  if (v < 0.01) return String(Number(v.toPrecision(2)));   // e.g. 0.001 on a log axis
  return v.toFixed(2);
}

// ---------------------------------------------------------------- Gantt
//
// One row per task. Each run of consecutive ticks becomes one bar.
// Ticks after `currentTick` are drawn faded, so you can watch the schedule fill in.
function ganttChart(container, tasks, run, currentTick) {
  const H_TICKS = run.gantt.length;
  const rowH = 34, labelW = 46, top = 10, axisH = 34;
  const W = 1200;
  const H = top + tasks.length * (rowH + 14) + axisH;
  const svg = makeSvg(container, W, H);
  const tickW = (W - labelW - 20) / H_TICKS;
  const x = t => labelW + t * tickW;
  const rowY = i => top + i * (rowH + 14);

  tasks.forEach((task, i) => {
    const y = rowY(i);
    svg.appendChild(el('text', { x: 4, y: y + rowH / 2 + 5, fill: taskColor(task.id), 'font-size': 15, 'font-weight': 700 }, 'T' + task.id));
    svg.appendChild(el('rect', { x: labelW, y, width: W - labelW - 20, height: rowH, fill: '#f7f8fb', stroke: '#e2e6ee' }));

    // Bars: merge consecutive ticks of the same task.
    let start = null;
    for (let t = 0; t <= H_TICKS; t++) {
      const running = t < H_TICKS && run.gantt[t] === task.id;
      if (running && start === null) start = t;
      if (!running && start !== null) {
        // Split the bar at the current tick so the "done" part is solid.
        const splitAt = Math.min(Math.max(start, currentTick + 1), t);
        if (splitAt > start) svg.appendChild(el('rect', { x: x(start), y: y + 3, width: (splitAt - start) * tickW, height: rowH - 6, fill: taskColor(task.id), rx: 2 }));
        if (t > splitAt) svg.appendChild(el('rect', { x: x(splitAt), y: y + 3, width: (t - splitAt) * tickW, height: rowH - 6, fill: taskColor(task.id), opacity: 0.18, rx: 2 }));
        start = null;
      }
    }

    // Release markers (triangles under the row).
    for (const [t, id] of run.releases) {
      if (id !== task.id) continue;
      const cx = x(t);
      svg.appendChild(el('path', { d: `M ${cx} ${y + rowH + 1} l -5 9 h 10 z`, fill: t <= currentTick ? '#111' : '#cbd0d8' }));
    }

    // Deadline misses: a red cross at the missed deadline.
    for (const miss of run.misses) {
      if (miss.task !== task.id) continue;
      const cx = x(miss.deadline), cy = y + rowH / 2;
      svg.appendChild(el('path', { d: `M ${cx - 7} ${cy - 7} L ${cx + 7} ${cy + 7} M ${cx + 7} ${cy - 7} L ${cx - 7} ${cy + 7}`, stroke: '#c62828', 'stroke-width': 3 }));
    }
  });

  // Time axis.
  const axisY = rowY(tasks.length) + 4;
  const step = H_TICKS <= 60 ? 5 : H_TICKS <= 200 ? 10 : 50;
  for (let t = 0; t <= H_TICKS; t += step) {
    svg.appendChild(el('line', { x1: x(t), x2: x(t), y1: top, y2: axisY, stroke: '#e5e7eb' }));
    svg.appendChild(el('text', { x: x(t), y: axisY + 16, 'text-anchor': 'middle', fill: '#6b7280', 'font-size': 13 }, t));
  }

  // Cursor at the current tick.
  const cx = x(currentTick + 0.5);
  svg.appendChild(el('line', { x1: cx, x2: cx, y1: top - 6, y2: axisY, stroke: '#e0a800', 'stroke-width': 3 }));
}
