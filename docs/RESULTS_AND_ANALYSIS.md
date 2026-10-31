# Results and analysis

All numbers come from the files the program writes (`results/*.csv`, also shown in the viewer). Comparison counts are exact and identical on every computer; times depend on the machine.

**Machine:** Intel Xeon (4 cores), Linux, g++ 13.3, Release build (-O3). Times are the median of 3 runs.

Re-create everything with:

```bash
./rtts verdict && ./rtts benchmark && ./rtts operations
```

---

## 1. Four data structures compared

| Structure | Used by | Insert | Extract-min | Decrease-key | Worst single operation |
|---|---|---|---|---|---|
| Sorted list | FreeRTOS (delayed-task list) | O(N) | O(1) | O(N) | O(N) |
| Red-black tree | Linux scheduler (CFS, EEVDF) | O(log N) | O(1) amortized | O(log N) | O(log N) |
| **Indexed binary heap** (ours) | — | O(log N) | O(log N) | O(log N) | O(log N) |
| **Fibonacci heap** (ours) | — | O(1) | O(log N) amortized | O(1) amortized | **O(N)** |

---

## 2. The data structures on their own (`./rtts operations`)

Each operation is measured at least 100,000 times for N = 10 … 100,000 elements (sorted list up to 10,000: beyond that it takes minutes).

### Comparisons per operation at N = 100,000 (average / worst)

| Operation | Red-black tree | Binary heap | Fibonacci heap |
|---|---|---|---|
| Insert | 17.6 / 22 | 2.3 / 15 | **1.0 / 1** |
| Extract-min | 0 / 0 | 28.3 / 32 | 21.2 / **99,998** |
| Decrease-key | 19.6 / 23 | **1.4** / 9 | 2.0 / **2** |
| Hold (extract + insert) | **19.4 / 23** | 31.9 / 48 | 26.6 / **99,999** |

### Time per operation at N = 100,000 (ns)

| Operation | Red-black tree | Binary heap | Fibonacci heap |
|---|---|---|---|
| Insert | 250.5 | **19.3** | 34.5 |
| Extract-min | **92.2** | 210.3 | 815.5 |
| Decrease-key | 383.3 | **10.5** | 22.9 |
| Hold | 444.0 | **254.0** | 754.5 |

The sorted list at N = 10,000 needs **36,478 ns** per hold, against 158 ns for the binary heap (about 230 times slower).

### What the numbers show

1. **Every structure follows its textbook bound.** The Fibonacci heap always inserts with exactly 1 comparison and decreases a key with at most 2. The sorted list needs about N/4 comparisons per insert (2,512 at N = 10,000). The red-black tree and the binary heap grow like log N.
2. **The binary heap is better on average than its bound.** With random keys, an insert or decrease-key climbs only one or two levels on average (2.3 and 1.4 comparisons), even though the worst case is log₂N.
3. **The Fibonacci heap's average is good, its worst case is not.** It does the fewest comparisons for insert, extract-min and hold on average. But it postpones its work, so one extract-min right after N inserts has to link all N trees: 99,998 comparisons at N = 100,000, against 32 for the binary heap. In a scheduler this happens at t = 0, when all tasks are released together.
4. **Fewer comparisons do not mean less time.** The Fibonacci heap is slower than the binary heap in every operation at every size we measured; at N = 100,000 it is 1.8 – 3.9 times slower. It allocates a node for every insert and follows pointers spread over memory, while the binary heap is one array.
5. **Red-black tree.** It removes the first element with 0 comparisons and has a small, steady worst case (23). Each insert costs about log N comparisons plus rebalancing, so it is 1.75 times slower than the binary heap in the hold test at N = 100,000.
6. **Sorted list.** With 10 elements it beats the red-black tree and the Fibonacci heap and nearly matches the binary heap (hold: 33.0 ns vs 30.8 ns). With 100 elements it is already 2.4 times slower than the binary heap, and the gap keeps growing.

---

## 3. Inside the scheduler (`./rtts benchmark`)

EDF, target U = 0.7, 5 random task sets per size, one hyperperiod each.

| N tasks | Sorted list | Red-black tree | Binary heap | Fibonacci heap |
|---|---|---|---|---|
| Average µs per tick, N = 1000 | 1.087 | 0.330 | **0.325** | 0.514 |
| Average comparisons per tick, N = 1000 | 197.3 | 9.6 | 11.9 | **6.4** |
| Worst comparisons in one tick, N = 1000 | 296,696 | 13,935 | 16,882 | **10,444** |

* The binary heap gives the lowest time per tick. The red-black tree is almost the same; the Fibonacci heap is 1.6 times slower and the sorted list 3.3 times slower.
* The worst **tick** is t = 0, when all N tasks are released. Here the Fibonacci heap has the smallest total, because its one expensive consolidation is shared by the N extract-mins of that tick. Its worst **single operation** is still O(N) (section 2).

---

## 4. Correctness (`./rtts verdict`)

* 33,000 task sets × RM/EDF × 4 data structures = **264,000 simulations**, in about 10 s.
* **0 mismatches**: whenever RTA (RM) or U ≤ 1 (EDF) says a set is schedulable, the simulation has no missed deadline, and the other way round. Worst simulated response times equal RTA's Rᵢ.
* All four data structures produce **exactly the same schedule** on every task set. So the performance comparison is fair: the structures differ only in cost, never in behaviour.

Acceptance ratio (n = 5 tasks):

| Target U | Liu-Layland | Hyperbolic | RTA = RM simulation | U ≤ 1 = EDF simulation |
|---|---|---|---|---|
| 0.70 | 68.6% | 84.5% | 100% | 100% |
| 0.75 | 19.0% | 49.0% | 100% | 100% |
| 0.80 | 0.9% | 11.0% | 99.5% | 100% |
| 0.90 | 0% | 0% | 90.0% | 96.3% |
| 1.00 | 0% | 0% | 26.4% | 37.4% |

---

## 5. Comparison with existing work

### Existing systems

| System | Structure | What our measurements say |
|---|---|---|
| FreeRTOS | Sorted linked list (delayed tasks) | Fine for ~10 tasks; O(N) insert makes it about 230 times slower than a binary heap at 10,000 elements |
| Linux (CFS / EEVDF) | Red-black tree with cached leftmost node | Steady O(log N) worst case; about 1.75 times slower than our binary heap at 100,000 elements, close to it at small N |
| C++ `std::priority_queue` | Binary heap without an index | Cannot remove or re-prioritise a given task; our indexed heap can, in O(log N) |

### Published results

| Work | Their claim | Our result |
|---|---|---|
| Liu & Layland (1973) | U ≤ n(2^(1/n) − 1) is sufficient for RM | Confirmed as only sufficient: at U = 0.80 it accepts 0.9% of the sets, while 99.5% are schedulable |
| Bini, Buttazzo & Buttazzo (2003) | The hyperbolic bound accepts more sets than Liu-Layland | Confirmed: 49.0% vs 19.0% at U = 0.75 (n = 5) |
| Fredman & Tarjan (1987) | Fibonacci heap: O(1) insert and decrease-key, O(log N) amortized extract-min | Confirmed: 1 comparison per insert, ≤ 2 per decrease-key; but amortized means one extract-min can cost O(N) (99,998 comparisons) |
| Cormen et al., *Introduction to Algorithms* (Ch. 19) | Fibonacci heaps' constant factors make binary heaps preferable in most applications | Confirmed: 1.8 – 3.9 times slower than the binary heap at N = 100,000, despite fewer comparisons |
| Baudis et al. (2017) | Simple linear queues often beat tree-based queues up to about 500 elements (GPU) | Same trend on a CPU, but the crossover is much earlier: at 10 elements the sorted list beats the red-black tree; at 100 it is already 1.8 – 2.4 times slower than the red-black tree and the binary heap |

---

## 6. Conclusion

* For a **real-time** scheduler the **indexed binary heap** is the best choice among the four: the fastest in the hold test from 100 elements up and per scheduler tick, a small worst case (at most about 2 log₂N comparisons per operation), one simple array, and any task can be removed or re-prioritised through its stored position.
* The **Fibonacci heap** wins on comparison counts, but its O(N) worst-case extract-min and its higher constant factors make it unsuitable when every single decision must be fast.
* The **red-black tree** is a good second choice with a steady O(log N) worst case. The **sorted list** only makes sense for very small task counts.

**Future work:** wake-up times and deadlines are small integers, so integer-key structures (a timing wheel for the Sleep queue, a bitmap of priorities for the Ready queue) could give O(1) worst-case operations.
