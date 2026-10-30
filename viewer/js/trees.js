// trees.js - draws a queue snapshot.
//
// The C++ program exports every queue as a forest (see QueueView in
// task_queue.hpp):
//   binary heap    -> one tree, plus we show the underlying array
//   Fibonacci heap -> the root list: several trees side by side
//   sorted list    -> a row of nodes joined by arrows

const NODE_R = 20;     // node radius
const X_GAP = 92;      // horizontal space per leaf (room for the key label)
const Y_GAP = 74;      // vertical space per level

// Give every node an (x, depth). Leaves get the next free x slot and a parent
// sits in the middle of its children. Simple, and it never overlaps.
function layoutForest(roots) {
  const nodes = [];
  const edges = [];
  let nextX = 0;

  function place(node, depth) {
    let x;
    if (node.children.length === 0) {
      x = nextX++;
    } else {
      const childXs = node.children.map(child => {
        const cx = place(child, depth + 1);
        edges.push({ from: node, to: child });
        return cx;
      });
      x = (childXs[0] + childXs[childXs.length - 1]) / 2;
    }
    node._x = x;
    node._depth = depth;
    nodes.push(node);
    return x;
  }

  roots.forEach((root, i) => {
    if (i > 0) nextX += 0.4;   // a little gap between separate trees
    place(root, 0);
  });

  const maxDepth = nodes.reduce((m, n) => Math.max(m, n._depth), 0);
  return { nodes, edges, width: nextX, depth: maxDepth + 1 };
}

function drawNode(svg, node, x, y, isTop) {
  const color = taskColor(node.id);
  if (isTop) svg.appendChild(el('circle', { cx: x, cy: y, r: NODE_R + 6, fill: 'none', stroke: '#e0a800', 'stroke-width': 3 }));
  svg.appendChild(el('circle', {
    cx: x, cy: y, r: NODE_R, fill: color,
    stroke: node.marked ? '#c62828' : '#fff', 'stroke-width': node.marked ? 4 : 2,
    'stroke-dasharray': node.marked ? '4 3' : 'none',
  }));
  svg.appendChild(el('text', { x, y: y + 5, 'text-anchor': 'middle', fill: '#fff', 'font-size': 14, 'font-weight': 700 }, 'T' + node.id));
  svg.appendChild(el('text', { x: x + NODE_R + 5, y: y - 6, fill: '#555', 'font-size': 12 }, 'key ' + node.key));
}

function drawQueue(container, queue) {
  if (!queue || queue.roots.length === 0) {
    container.innerHTML = '<div class="empty">(empty)</div>';
    return;
  }
  if (queue.kind === 'sorted_list') return drawList(container, queue);

  const layout = layoutForest(JSON.parse(JSON.stringify(queue.roots)));
  const pad = 40;
  const arrayRow = queue.kind === 'binary' ? 70 : 0;
  const width = Math.max(320, layout.width * X_GAP + pad * 2);
  const height = layout.depth * Y_GAP + pad + 10 + arrayRow;
  const svg = makeSvg(container, width, height, true);
  const px = n => pad + n._x * X_GAP + X_GAP / 2;
  const py = n => pad + n._depth * Y_GAP;

  // Fibonacci heap: draw the root list as a dashed line through all roots.
  if (queue.kind === 'fibonacci') {
    const roots = layout.nodes.filter(n => n._depth === 0).sort((a, b) => a._x - b._x);
    if (roots.length > 1) {
      svg.appendChild(el('line', { x1: px(roots[0]), y1: py(roots[0]), x2: px(roots[roots.length - 1]), y2: py(roots[0]),
                                   stroke: '#9ca3af', 'stroke-width': 2, 'stroke-dasharray': '6 5' }));
    }
  }

  for (const e of layout.edges)
    svg.appendChild(el('line', { x1: px(e.from), y1: py(e.from), x2: px(e.to), y2: py(e.to), stroke: '#9ca3af', 'stroke-width': 2 }));
  for (const n of layout.nodes) drawNode(svg, n, px(n), py(n), n.id === queue.top);

  // Binary heap: also show the array, because that is how it is stored.
  if (queue.kind === 'binary') drawHeapArray(svg, queue, height - arrayRow + 10);

  const legend = document.createElement('p');
  legend.className = 'tree-legend';
  legend.textContent = queue.kind === 'fibonacci'
    ? `Root list: ${queue.roots.length} tree(s) joined by the dashed line · gold ring = min pointer · red dashed border = marked node`
    : 'Gold ring = top of the heap (array index 0)';
  container.appendChild(legend);
}

// The binary heap array in level order (index 0 = root).
function drawHeapArray(svg, queue, y) {
  const order = [];
  let level = [queue.roots[0]];
  while (level.length) {
    order.push(...level);
    level = level.flatMap(n => n.children);
  }
  svg.appendChild(el('text', { x: 8, y: y + 4, fill: '#6b7280', 'font-size': 13 }, 'array:'));
  order.forEach((n, i) => {
    const x = 60 + i * 44;
    svg.appendChild(el('rect', { x, y: y - 14, width: 40, height: 28, rx: 4, fill: taskColor(n.id) }));
    svg.appendChild(el('text', { x: x + 20, y: y + 5, 'text-anchor': 'middle', fill: '#fff', 'font-size': 13, 'font-weight': 700 }, 'T' + n.id));
    svg.appendChild(el('text', { x: x + 20, y: y + 30, 'text-anchor': 'middle', fill: '#9ca3af', 'font-size': 11 }, i));
  });
}

function drawList(container, queue) {
  const step = 90;
  const width = Math.max(320, queue.roots.length * step + 60);
  const svg = makeSvg(container, width, 100, true);
  queue.roots.forEach((node, i) => {
    const x = 50 + i * step;
    if (i > 0) {
      svg.appendChild(el('line', { x1: x - step + NODE_R + 40, y1: 50, x2: x - NODE_R - 8, y2: 50, stroke: '#9ca3af', 'stroke-width': 2 }));
      svg.appendChild(el('path', { d: `M ${x - NODE_R - 4} 50 l -9 -5 v 10 z`, fill: '#9ca3af' }));
    }
    drawNode(svg, node, x, 50, node.id === queue.top);
  });
}
