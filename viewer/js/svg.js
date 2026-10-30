// svg.js - small helpers shared by the tree and chart drawing code.

// One colour per task, in the same order as the terminal colours.
const TASK_COLORS = ['#1f77d0', '#e8772e', '#4a9a1f', '#b65cc9', '#d6336c',
                     '#13a3a3', '#c9a227', '#6a5acd', '#a0522d', '#7f8c8d'];

const QUEUE_LABELS = { binary: 'Binary heap', fibonacci: 'Fibonacci heap', sorted_list: 'Sorted list' };
const QUEUE_COLORS = { sorted_list: '#9ca3af', binary: '#1f77d0', fibonacci: '#e8772e' };

function taskColor(id) {
  return TASK_COLORS[id % TASK_COLORS.length];
}

const SVG_NS = 'http://www.w3.org/2000/svg';

// el('rect', {x: 1, y: 2}) creates an SVG element with those attributes.
function el(tag, attrs = {}, text) {
  const node = document.createElementNS(SVG_NS, tag);
  for (const [name, value] of Object.entries(attrs)) node.setAttribute(name, value);
  if (text !== undefined) node.textContent = text;
  return node;
}

// An empty <svg> with a viewBox. Charts stretch to the card width; trees pass
// naturalSize = true so their nodes keep the same size however wide the card is.
function makeSvg(container, width, height, naturalSize = false) {
  container.innerHTML = '';
  const attrs = { viewBox: `0 0 ${width} ${height}`, preserveAspectRatio: 'xMinYMin meet' };
  if (naturalSize) Object.assign(attrs, { width, height });
  const svg = el('svg', attrs);
  container.appendChild(svg);
  return svg;
}

// Small helper for HTML tables: rows is an array of arrays of HTML strings.
function fillTable(table, headers, rows) {
  const head = '<tr>' + headers.map(h => `<th>${h}</th>`).join('') + '</tr>';
  const body = rows.map(r => '<tr>' + r.map(c => `<td>${c}</td>`).join('') + '</tr>').join('');
  table.innerHTML = head + body;
}

function badge(pass) {
  return pass ? '<span class="badge pass">PASS</span>' : '<span class="badge fail">FAIL</span>';
}
