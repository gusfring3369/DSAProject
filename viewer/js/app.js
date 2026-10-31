// app.js - wires the data from the C++ program to the page.
//
// window.RTTS_TRACE      written by  ./rtts demo       (one task set, every tick)
// window.RTTS_VERDICT    written by  ./rtts verdict    (Module 8 over the dataset)
// window.RTTS_BENCHMARK  written by  ./rtts benchmark  (queue cost vs task count)

const TRACE = window.RTTS_TRACE;
const VERDICT = window.RTTS_VERDICT;
const BENCH = window.RTTS_BENCHMARK;
const OPS = window.RTTS_OPERATIONS;

const $ = id => document.getElementById(id);

// ------------------------------------------------------------------- tabs

const drawnTabs = new Set();

function showTab(name) {
  document.querySelectorAll('#tabs button').forEach(b => b.classList.toggle('active', b.dataset.tab === name));
  document.querySelectorAll('.tab').forEach(t => t.classList.toggle('active', t.id === 'tab-' + name));
  // Charts are drawn the first time their tab is opened.
  if (!drawnTabs.has(name)) {
    drawnTabs.add(name);
    if (name === 'compare') drawCompare();
    if (name === 'verdict') drawVerdict();
    if (name === 'benchmark') drawBenchmark();
  }
}

document.querySelectorAll('#tabs button').forEach(b => b.addEventListener('click', () => showTab(b.dataset.tab)));

// ============================================================ Simulation tab

const sim = { policy: 'RM', queue: 'binary', tick: 0, timer: null };

function currentRun() {
  return TRACE.runs.find(r => r.policy === sim.policy && r.queue === sim.queue);
}

function drawTaskSet() {
  $('set-title').textContent = TRACE.title;
  fillTable($('task-table'), ['Task', 'T (period)', 'C (run time)', 'D (deadline)', 'u = C/T'],
    TRACE.tasks.map(t => [
      `<span class="swatch" style="background:${taskColor(t.id)}"></span>T${t.id}`,
      t.T, t.C, t.D, t.u.toFixed(3),
    ]));

  const th = TRACE.theory;
  const R = Object.entries(th.rta.R).map(([id, r]) => `T${id}: ${r < 0 ? 'miss' : r}`).join(', ');
  const rows = [
    ['Liu-Layland (RM, sufficient)', `U = ${th.utilization.toFixed(3)} ≤ ${th.liuLayland.bound.toFixed(3)}`, th.liuLayland.pass],
    ['Hyperbolic (RM, sufficient)', `Π(Uᵢ + 1) = ${th.hyperbolic.product.toFixed(3)} ≤ 2`, th.hyperbolic.pass],
    ['RTA (RM, exact)', `R = ${R}`, th.rta.pass],
    ['EDF (exact, D = T)', `U = ${th.utilization.toFixed(3)} ≤ 1`, th.edf.pass],
  ];
  $('theory').innerHTML = rows.map(([name, what, pass]) =>
    `<div class="theory-row"><div><b>${name}</b><div class="what">${what}</div></div><div>${badge(pass)}</div></div>`).join('') +
    `<p class="hint">Hyperperiod H = ${TRACE.hyperperiod} ticks: the schedule repeats after that.</p>`;
}

function drawSimulation() {
  const run = currentRun();
  const t = sim.tick;
  const snap = run.ticks[t];

  $('tick-label').textContent = `${t} / ${run.ticks.length - 1}`;
  $('tick').value = t;
  $('ready-key').textContent = sim.policy === 'RM' ? 'key = period (RM)' : 'key = absolute deadline (EDF)';

  ganttChart($('gantt'), TRACE.tasks, run, t);
  drawQueue($('sleep-tree'), snap.sleep);
  drawQueue($('ready-tree'), snap.ready);
  drawLiveStats(run, t);
  drawEventLog(run, t);
}

function drawLiveStats(run, t) {
  let finished = 0, misses = 0, switches = 0, work = 0, last = -1;
  for (let i = 0; i <= t; i++) {
    const tick = run.ticks[i];
    work += tick.cost;
    for (const e of tick.events) {
      if (e.includes('finished')) finished++;
      if (e.includes('MISSED')) misses++;
    }
    if (run.gantt[i] >= 0 && run.gantt[i] !== last) { switches++; last = run.gantt[i]; }
  }
  // On the last tick also count jobs that never finished (they are misses too).
  if (t === run.ticks.length - 1) misses = run.summary.misses;

  const running = run.ticks[t].running;
  const stats = [
    ['running now', running < 0 ? 'idle' : 'T' + running, running < 0 ? '' : ''],
    ['jobs finished', finished, ''],
    ['deadline misses', misses, misses ? 'bad' : 'ok'],
    ['context switches', switches, ''],
    ['comparisons this tick', run.ticks[t].cost, ''],
    ['comparisons so far', work, ''],
  ];
  $('live-stats').innerHTML = stats.map(([label, value, cls]) =>
    `<div class="stat ${cls}"><div class="value"${label === 'running now' && running >= 0 ? ` style="color:${taskColor(running)}"` : ''}>${value}</div><div class="label">${label}</div></div>`).join('');
}

function drawEventLog(run, t) {
  const items = [];
  for (let i = t; i >= 0; i--) {
    for (const e of run.ticks[i].events) {
      const cls = e.includes('MISSED') ? 'miss' : e.includes('preempts') ? 'preempt' : '';
      items.push(`<li class="${i === t ? 'current' : ''}"><span class="t">t = ${i}</span><span class="${cls}">${e}</span></li>`);
    }
  }
  $('event-log').innerHTML = items.join('');
}

function goTo(tick) {
  const last = currentRun().ticks.length - 1;
  sim.tick = Math.max(0, Math.min(last, tick));
  drawSimulation();
}

function togglePlay() {
  if (sim.timer) return stopPlaying();
  if (sim.tick >= currentRun().ticks.length - 1) sim.tick = -1;   // replay from the start
  $('btn-play').innerHTML = '&#10074;&#10074; Pause';
  const stepOnce = () => {
    if (sim.tick >= currentRun().ticks.length - 1) return stopPlaying();
    goTo(sim.tick + 1);
    sim.timer = setTimeout(stepOnce, Number($('speed').value));
  };
  stepOnce();
}

function stopPlaying() {
  clearTimeout(sim.timer);
  sim.timer = null;
  $('btn-play').innerHTML = '&#9654; Play';
}

function setUpSimulation() {
  if (!TRACE) {
    $('no-trace').classList.remove('hidden');
    return;
  }
  $('tick').max = TRACE.runs[0].ticks.length - 1;
  drawTaskSet();
  drawSimulation();

  $('policy').addEventListener('change', e => { sim.policy = e.target.value; drawSimulation(); });
  $('queue').addEventListener('change', e => { sim.queue = e.target.value; drawSimulation(); });
  $('tick').addEventListener('input', e => { stopPlaying(); goTo(Number(e.target.value)); });
  $('btn-start').addEventListener('click', () => { stopPlaying(); goTo(0); });
  $('btn-prev').addEventListener('click', () => { stopPlaying(); goTo(sim.tick - 1); });
  $('btn-next').addEventListener('click', () => { stopPlaying(); goTo(sim.tick + 1); });
  $('btn-end').addEventListener('click', () => { stopPlaying(); goTo(Infinity); });
  $('btn-play').addEventListener('click', togglePlay);

  // Keyboard: left / right arrows step, space plays.
  document.addEventListener('keydown', e => {
    if (!$('tab-simulation').classList.contains('active') || e.target.tagName === 'SELECT') return;
    if (e.key === 'ArrowRight') { stopPlaying(); goTo(sim.tick + 1); }
    if (e.key === 'ArrowLeft') { stopPlaying(); goTo(sim.tick - 1); }
    if (e.key === ' ') { e.preventDefault(); togglePlay(); }
  });
}

// ================================================== Binary vs Fibonacci tab

function drawCompare() {
  if (!TRACE) return;
  const policy = $('compare-policy').value;
  const runs = QUEUE_KINDS.map(q => TRACE.runs.find(r => r.policy === policy && r.queue === q)).filter(Boolean);

  lineChart($('compare-chart'), {
    series: runs.map(r => ({
      name: QUEUE_LABELS[r.queue], color: QUEUE_COLORS[r.queue],
      points: r.ticks.map(t => [t.t, t.cost]), width: 2,
    })),
    xLabel: 'tick', yLabel: 'key comparisons',
  });

  const reference = JSON.stringify(runs.find(r => r.queue === 'binary').gantt);
  fillTable($('compare-table'),
    ['Queue', 'Total comparisons', 'Average per tick', 'Worst tick', 'Deadline misses', 'Same schedule as binary heap?'],
    runs.map(r => [
      `<span class="swatch" style="background:${QUEUE_COLORS[r.queue]}"></span>${QUEUE_LABELS[r.queue]}`,
      r.ticks.reduce((s, t) => s + t.cost, 0),
      r.summary.avgCost.toFixed(2),
      r.summary.worstCost,
      r.summary.misses,
      JSON.stringify(r.gantt) === reference ? badge(true).replace('PASS', 'YES') : badge(false).replace('FAIL', 'NO'),
    ]));
}

$('compare-policy').addEventListener('change', drawCompare);

// ========================================================== Verdict tab

let verdictN = null;

function drawVerdict() {
  if (!VERDICT) { $('no-verdict').classList.remove('hidden'); return; }

  const ns = [...new Set(VERDICT.rows.map(r => r.n))];
  if (verdictN === null) verdictN = ns[0];

  $('verdict-stats').innerHTML = [
    ['task sets checked', VERDICT.totalSets.toLocaleString('en-US'), ''],
    ['sets per (n, U) pair', VERDICT.setsPerPoint, ''],
    ['simulations run', (VERDICT.simulations ?? VERDICT.totalSets * 4).toLocaleString('en-US'), ''],
    ['mismatches', VERDICT.mismatches, VERDICT.mismatches ? 'bad' : 'ok'],
  ].map(([label, value, cls]) => `<div class="stat ${cls}"><div class="value">${value}</div><div class="label">${label}</div></div>`).join('');

  $('verdict-n').innerHTML = ns.map(n => `<button class="${n === verdictN ? 'selected' : ''}" data-n="${n}">n = ${n}</button>`).join('');
  $('verdict-n').querySelectorAll('button').forEach(b => b.addEventListener('click', () => { verdictN = Number(b.dataset.n); drawVerdict(); }));

  const rows = VERDICT.rows.filter(r => r.n === verdictN);
  const pts = key => rows.map(r => [r.u, r[key]]);
  const common = { xLabel: 'target utilization U', yLabel: 'task sets accepted', yMax: 1, percent: true };

  lineChart($('verdict-rm'), { ...common, series: [
    { name: 'Liu-Layland', color: '#9ca3af', points: pts('liuLayland') },
    { name: 'Hyperbolic', color: '#c9a227', points: pts('hyperbolic') },
    { name: 'RTA (exact)', color: '#1f77d0', points: pts('rta') },
    { name: 'RM simulation', color: '#111827', points: pts('rmSim'), dashed: true },
  ]});
  lineChart($('verdict-edf'), { ...common, series: [
    { name: 'U ≤ 1 test (exact)', color: '#e8772e', points: pts('edf') },
    { name: 'EDF simulation', color: '#111827', points: pts('edfSim'), dashed: true },
  ]});

  const pct = x => (x * 100).toFixed(1) + '%';
  fillTable($('verdict-table'),
    ['n', 'target U', 'real U', 'sets', 'Liu-Layland', 'Hyperbolic', 'RTA', 'RM sim', 'EDF test', 'EDF sim', 'mismatches'],
    rows.map(r => [r.n, r.u.toFixed(2), (r.realU ?? r.u).toFixed(3), r.sets, pct(r.liuLayland), pct(r.hyperbolic),
                   pct(r.rta), pct(r.rmSim), pct(r.edf), pct(r.edfSim),
                   r.mismatches ? `<b style="color:#c62828">${r.mismatches}</b>` : '0']));
}

// ======================================================== Benchmark tab

function drawBenchmark() {
  drawOperations();
  if (!BENCH) { $('no-benchmark').classList.remove('hidden'); return; }
  $('benchmark-info').textContent =
    `${BENCH.policy} scheduling, target U = ${BENCH.utilization}, ${BENCH.setsPerSize} random task sets per size, one hyperperiod each. Both axes are logarithmic.`;

  const queues = QUEUE_KINDS.filter(q => BENCH.rows.some(r => r.queue === q));
  const series = key => queues.map(q => ({
    name: QUEUE_LABELS[q], color: QUEUE_COLORS[q],
    points: BENCH.rows.filter(r => r.queue === q).map(r => [r.n, r[key]]),
  }));

  lineChart($('bench-avg'), { series: series('avgCmp'), xLabel: 'number of tasks N', yLabel: 'comparisons', logX: true, logY: true });
  lineChart($('bench-worst'), { series: series('worstCmp'), xLabel: 'number of tasks N', yLabel: 'comparisons', logX: true, logY: true });
  lineChart($('bench-time'), { series: series('avgUs'), xLabel: 'number of tasks N', yLabel: 'µs per tick', logX: true, logY: true });

  fillTable($('bench-table'),
    ['Queue', 'N', 'real U', 'avg comparisons / tick', 'worst comparisons / tick', 'avg µs / tick', 'worst µs / tick'],
    BENCH.rows.map(r => [
      `<span class="swatch" style="background:${QUEUE_COLORS[r.queue]}"></span>${QUEUE_LABELS[r.queue]}`,
      r.n, r.u.toFixed(2), r.avgCmp.toFixed(2), r.worstCmp.toLocaleString('en-US'), r.avgUs.toFixed(3), r.worstUs.toFixed(1),
    ]));
}

// ----------------------------------------- data structures on their own

const OP_INFO = {
  insert: 'N inserts into an empty queue, random keys.',
  extract_min: 'Then N extract-mins until the queue is empty. Sorted list and red-black tree take the first element: 0 comparisons, so they are not on the log-scale comparison charts.',
  decrease_key: 'N − 1 decrease-keys on a full queue.',
  hold: 'Extract-min, then insert the same task with a later key: exactly what the Sleep queue does when a job finishes. 100,000 per run.',
};
const OP_LABELS = { insert: 'Insert', extract_min: 'Extract-min', decrease_key: 'Decrease-key', hold: 'Hold (extract + insert)' };
let currentOp = 'hold';

function drawOperations() {
  if (!OPS) { $('no-operations').classList.remove('hidden'); return; }

  $('op-buttons').innerHTML = Object.keys(OP_LABELS).map(op =>
    `<button class="${op === currentOp ? 'selected' : ''}" data-op="${op}">${OP_LABELS[op]}</button>`).join('');
  $('op-buttons').querySelectorAll('button').forEach(b =>
    b.addEventListener('click', () => { currentOp = b.dataset.op; drawOperations(); }));
  $('op-info').textContent = OP_INFO[currentOp] + ` Time = median of ${OPS.repeats} runs.`;

  const rows = OPS.rows.filter(r => r.op === currentOp);
  // Log scales cannot show 0, so points with 0 comparisons are left out.
  const series = key => QUEUE_KINDS.map(q => ({
    name: QUEUE_LABELS[q], color: QUEUE_COLORS[q],
    points: rows.filter(r => r.queue === q && r[key] > 0).map(r => [r.n, r[key]]),
  })).filter(s => s.points.length > 0);

  const axes = { xLabel: 'number of elements N', logX: true, logY: true };
  lineChart($('op-avg'), { ...axes, series: series('avgCmp'), yLabel: 'comparisons' });
  lineChart($('op-worst'), { ...axes, series: series('worstCmp'), yLabel: 'comparisons' });
  lineChart($('op-time'), { ...axes, series: series('ns'), yLabel: 'ns per operation' });

  fillTable($('op-table'), ['Queue', 'N', 'avg cmp', 'worst cmp', 'ns / op'],
    rows.map(r => [
      `<span class="swatch" style="background:${QUEUE_COLORS[r.queue]}"></span>${QUEUE_LABELS[r.queue]}`,
      r.n.toLocaleString('en-US'), r.avgCmp.toFixed(2), r.worstCmp.toLocaleString('en-US'), r.ns.toFixed(1),
    ]));
}

// ------------------------------------------------------------------ start

setUpSimulation();

// Charts are drawn at the card's width, so redraw the visible tab after a resize.
let resizeTimer = null;
window.addEventListener('resize', () => {
  clearTimeout(resizeTimer);
  resizeTimer = setTimeout(() => {
    const active = document.querySelector('#tabs button.active').dataset.tab;
    if (active === 'compare') drawCompare();
    if (active === 'verdict') drawVerdict();
    if (active === 'benchmark') drawBenchmark();
  }, 200);
});
