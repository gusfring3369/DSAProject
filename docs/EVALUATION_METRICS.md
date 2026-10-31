# Evaluation metrics

The project is about the **data structure** behind the scheduler, so the main metrics measure the queues. The correctness metrics show that the comparison is fair: every queue must produce the same, correct schedule.

## 1. Data-structure metrics (main)

| # | Metric | What it tells us | How we measure it |
|---|---|---|---|
| M1 | **Key comparisons per operation** (average) | How much work one insert / extract-min / decrease-key costs | Every queue counts each call of its compare function. Exact and the same on every computer. |
| M2 | **Worst-case comparisons in one operation** (and in one scheduler tick) | Predictability. A real-time scheduler is judged by its slowest decision, not the average one. | Largest value of M1 seen in a run |
| M3 | **Time per operation / per tick** (ns or µs, average) | Real cost, including memory allocation and pointer chasing that M1 does not see | `std::chrono::steady_clock` around many operations, Release build (-O2), median of 3 runs |
| M4 | **Growth with N** | Does the cost follow O(1), O(log N) or O(N)? | M1 – M3 for N = 10 … 100,000 elements |

Operations measured: **insert**, **extract-min**, **decrease-key**, and **hold** (extract-min followed by insert with a later key: exactly what the Sleep queue does every time a job finishes).

Why comparisons *and* time: comparisons follow the textbook analysis, while time also shows constant factors. A Fibonacci heap, for example, needs fewer comparisons but allocates a node for every insert.

## 2. Correctness metrics (must be 100%)

| # | Metric | Target |
|---|---|---|
| M5 | **Verdict agreement**: share of task sets where the simulation and the exact test (RTA for RM, U ≤ 1 for EDF) agree | 100% |
| M6 | **Schedule equality**: share of task sets where every data structure gives the identical Gantt chart | 100% |

## 3. Scheduling metrics (reported for every run)

| # | Metric | Meaning |
|---|---|---|
| M7 | Deadline misses | Jobs that finished after their deadline |
| M8 | Acceptance ratio | Share of task sets a test or a simulation calls schedulable, per utilization level |
| M9 | Response time (average, worst) | Release to finish; the worst value must equal RTA's Rᵢ |
| M10 | Context switches, CPU busy % | How often the running task changes; how much of the time the CPU works |
