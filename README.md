# Real-Time Task Scheduler Simulator using Dual Indexed Min-Heaps (Binary and Fibonacci)

**CS603 Advanced Data Structures and Algorithms, NIT Tiruchirappalli**
Sriram Paranthaman (206126034) · Tejas Mishra (206126035)

Review slides: [docs/RTTS_First_Review.pptx](docs/RTTS_First_Review.pptx) (23 slides, including the results and a demo slide).

A C++17 simulator of a real-time CPU scheduler. Tasks wait in a **Sleep queue** until their next release, then move to a **Ready queue**; every tick the CPU runs the top job. Both queues are built **twice**, as an **indexed binary min-heap** and as a **Fibonacci heap**, and compared with a **sorted list** (how FreeRTOS does it). The scheduler supports **Rate-Monotonic (RM)** and **Earliest Deadline First (EDF)**, and every result is checked against exact real-time theory.

The project is built to be **seen**, not just run:

* a **terminal view** with a colour Gantt chart and the heaps drawn as trees, tick by tick;
* a **browser viewer** (no install, no server: just open `viewer/index.html`) that animates the schedule and both heaps, and plots the experiments.

![Simulation view](docs/images/simulation_fibonacci.png)

---

## 1. Quick start

You need a C++17 compiler (g++ 8+, clang 7+ or Visual Studio 2019+) and, ideally, CMake 3.14+.

```bash
git clone https://github.com/gusfring3369/DSAProject.git
```

```bash
cd DSAProject
```

```bash
cmake -S . -B build
```

```bash
cmake --build build --config Release
```

```bash
./rtts demo
```

Then open **`viewer/index.html`** in any browser (double-click it). The repository already contains viewer data for the sample task set and the full experiments, so the viewer works even before you build anything.

The executables `rtts` and `rtts_tests` are placed in the project root, so all commands below are run from there.

### Building without CMake

One command is enough:

```bash
g++ -std=c++17 -O2 -Iinclude src/*.cpp src/queues/*.cpp -o rtts
```

### Windows

* **Visual Studio:** *File → Open → Folder…*, pick the project folder, wait for CMake to configure, choose the **Release** configuration and build. `rtts.exe` appears in the project folder. Run it from a terminal opened in that folder: `.\rtts demo`.
* **MinGW / MSYS2:** use the `g++` line above, then `.\rtts.exe demo`.
* Colours and box characters need Windows Terminal or PowerShell on Windows 10+. If the output looks garbled, add `--no-color`.

---

## 2. Commands

| Command | What it shows |
|---|---|
| `./rtts demo` | Task table, the four theory tests, a colour Gantt chart, metrics and the verdict for one task set. Also writes `viewer/data/trace.js` for the browser viewer. |
| `./rtts step` | Walks through the simulation **one tick at a time**: events (release, preemption, finish), the Sleep and Ready heaps drawn as trees, and the Gantt chart so far. Press Enter for the next tick, `q` to stop. |
| `./rtts compare` | Same task set with a sorted list, a binary heap and a Fibonacci heap: work per tick and a check that all three produce the same schedule. |
| `./rtts dataset` | Writes the synthetic dataset (UUniFast) to `data/tasksets.csv`. |
| `./rtts verdict` | **Module 8** over the full dataset: 33,000 task sets × RM/EDF × binary/Fibonacci = 132,000 simulations, compared with theory (≈ 7 s). |
| `./rtts benchmark` | Cost per tick of the three queues for 5 … 1000 tasks (< 1 s). |
| `./rtts all` | `demo` + `verdict` + `benchmark`. |
| `./rtts help` | All options. |

Useful options:

```text
--policy RM|EDF          scheduling policy
--queue binary|fibonacci|sorted_list
--n 8 --u 0.85 --seed 3  use a random task set instead of the sample set from the slides
--delay 300              step: auto-play, 300 ms per tick
--sets 100               verdict: 100 sets per (n, U) pair instead of 1000 (quick run)
--in data/tasksets.csv   verdict: use a dataset file made by "rtts dataset"
--no-color               plain text (for saving output to a file)
```

Examples:

```bash
./rtts step --queue fibonacci --policy EDF
```

```bash
./rtts demo --n 8 --u 0.85 --seed 3 --policy EDF
```

```bash
./rtts verdict --sets 100
```

---

## 3. The browser viewer

Open `viewer/index.html`. It reads the files the program writes into `viewer/data/`:

| Tab | Content | Re-create with |
|---|---|---|
| **Simulation** | Task set and theory tests; play/pause/step through the schedule; Gantt chart filling in as time moves; the Sleep and Ready queues drawn live (binary heap as a tree plus its array, Fibonacci heap as its root list of trees with the min pointer and marked nodes, sorted list as a chain); event log. Switch policy and queue at any time. | `./rtts demo` (any task-set options) |
| **Binary vs Fibonacci** | Key comparisons per tick for the three queues on the same task set, and a check that the schedules are identical. | `./rtts demo` |
| **Verdict check** | Share of task sets accepted by Liu-Layland, hyperbolic, RTA, the U ≤ 1 test and the two simulations, per number of tasks. The exact tests lie exactly on the simulation curves. | `./rtts verdict` |
| **Benchmark** | Average and worst comparisons per tick, and time per tick, against the number of tasks (log-log). | `./rtts benchmark` |

Keyboard on the Simulation tab: **← / →** step one tick, **Space** play/pause.

| | |
|---|---|
| ![Binary heap view](docs/images/binary_heap.png) | ![Verdict check](docs/images/verdict.png) |
| ![Binary vs Fibonacci](docs/images/compare.png) | ![Benchmark](docs/images/benchmark.png) |

---

## 4. Suggested demo for a review (about 5 minutes)

1. `./rtts demo`: the sample set from the slides. Point out that T2 is preempted at t = 10 and finishes at **28**, exactly the R₂ that RTA predicts.
2. `./rtts step --queue fibonacci`: press Enter a few times. At t = 0 the Fibonacci Ready heap has **three separate one-node trees** (lazy insert); after the first ExtractMin they are **consolidated**.
3. Open `viewer/index.html`, press **Play**, then switch *Queue* between *Binary heap* and *Fibonacci heap* on the same tick to compare their shapes.
4. *Binary vs Fibonacci* tab: same schedule, different work per tick.
5. *Verdict check* tab: 33,000 task sets, **0 mismatches**.
6. *Benchmark* tab: the sorted list grows linearly; the Fibonacci heap does the fewest comparisons but is not the fastest in real time.

---

## 5. Project structure

```text
DSAProject/
├── CMakeLists.txt
├── README.md
├── include/rtts/                  headers (one per module)
│   ├── task.hpp                   Task model: T, C, D and the current job's state
│   ├── queues/
│   │   ├── task_queue.hpp         common interface + QueueView (used for drawing)
│   │   ├── binary_heap.hpp        Modules 2/3, binary version (indexed)
│   │   ├── fibonacci_heap.hpp     Modules 2/3, Fibonacci version
│   │   ├── sorted_list.hpp        baseline (FreeRTOS style)
│   │   └── queue_factory.hpp      pick a queue by name
│   ├── generator.hpp              Module 1: UUniFast, hyperperiod, sample set
│   ├── scheduler.hpp              Modules 2–5: release, dispatch, completion
│   ├── metrics.hpp                Module 6: Gantt, misses, response times, cost per tick
│   ├── theory.hpp                 Module 7: Liu-Layland, hyperbolic, RTA, EDF
│   ├── verdict.hpp                Module 8: simulator vs theory
│   ├── dataset.hpp                33,000-set dataset, CSV in/out
│   ├── experiments.hpp            verdict experiment + heap benchmark
│   ├── terminal_view.hpp          colour Gantt chart, heap trees, tables
│   └── json_export.hpp            data files for the browser viewer
├── src/                           the matching .cpp files, plus main.cpp (the CLI)
├── tests/                         unit tests (tiny built-in framework, no dependencies)
├── viewer/                        browser viewer: index.html, css/, js/, data/
└── docs/images/                   screenshots used in this README
```

Folders created when you run the program (not in git): `build/`, `data/` (dataset CSV), `results/` (CSV results and `mismatches.txt`).

---

## 6. How it works

### One tick of the simulation (`src/scheduler.cpp`)

```text
for t = 0 .. H-1:
    while Sleep.top.wakeUpTime <= t:         # Module 2 -> 3: release
        task = Sleep.extractMin()
        start a new job (release, absolute deadline, remaining = C)
        Ready.insert(task)
    if Ready is empty: CPU idle               # Module 4: dispatch
    else:
        run Ready.top for one tick
        if its job is done:                   # Module 5: completion
            Ready.extractMin(); check deadline
            task.wakeUpTime = release + period; Sleep.insert(task)
```

Preemption needs no extra code: a more urgent job simply becomes the new top of the Ready heap. Switching RM ↔ EDF only changes the Ready key (period ↔ absolute deadline). Switching binary ↔ Fibonacci only changes which class `makeQueue()` creates.

### The queues

| Operation | Sorted list | Indexed binary heap | Fibonacci heap |
|---|---|---|---|
| Insert | O(N) | O(log N) | **O(1)** |
| Top (peek) | O(1) | O(1) | O(1) |
| ExtractMin | O(1) | O(log N) | O(log N) amortized, **O(N) worst case** |
| Decrease key | O(N) | O(log N) | **O(1)** amortized |
| Remove any task | O(N) | O(log N) | O(log N) amortized |
| Find a task | O(N) | O(1) via `position_` map | O(1) via `nodes_` map |

* **Indexed binary heap** (`binary_heap.cpp`): array heap plus a map `task id → array index`, updated on every swap. That index is what makes "remove any task" O(log N).
* **Fibonacci heap** (`fibonacci_heap.cpp`): a root list of trees. Insert just adds a one-node tree; `extractMin` consolidates trees of equal degree; `decreaseKey` uses cut and cascading cut. Children and roots are kept in `std::vector` to keep the code readable.
* Every queue counts its **key comparisons**, which is the machine-independent "work per tick" used in the benchmark.

### The theory tests (`src/theory.cpp`)

| Test | Policy | Type | Condition |
|---|---|---|---|
| Liu-Layland | RM | sufficient | U ≤ n(2^(1/n) − 1) |
| Hyperbolic | RM | sufficient | Π(Uᵢ + 1) ≤ 2 |
| RTA | RM | exact | Rᵢ = Cᵢ + Σ⌈Rᵢ/Tⱼ⌉Cⱼ ≤ Dᵢ |
| EDF | EDF | exact (D = T) | Σ Cᵢ·(H/Tᵢ) ≤ H, i.e. U ≤ 1 in whole numbers |

### The verdict check (`src/verdict.cpp`)

For every task set: run RM and EDF with **both** heaps, then require that
1. RM has no misses ⇔ RTA passes, and EDF has no misses ⇔ U ≤ 1;
2. if Liu-Layland or the hyperbolic bound passes, RM has no misses;
3. when RM is schedulable, each task's worst simulated response time **equals** its RTA Rᵢ;
4. the binary and Fibonacci heaps produce **exactly the same** Gantt chart.

---

## 7. Results

Produced on a normal laptop-class CPU; your timings will differ slightly, the comparison counts will not.

**Verdict check:** 33,000 task sets (n = 5, 10, 20; U = 0.50 … 1.00; 1,000 sets each; seed 2026), 132,000 simulations, **0 mismatches**. Excerpt for n = 5:

| target U | Liu-Layland | Hyperbolic | RTA = RM sim | U ≤ 1 = EDF sim |
|---|---|---|---|---|
| 0.70 | 68.6% | 84.5% | 100% | 100% |
| 0.80 | 0.9% | 11.0% | 99.5% | 100% |
| 0.90 | 0% | 0% | 90.0% | 96.3% |
| 1.00 | 0% | 0% | 26.4% | 37.4% |

The sufficient tests reject many sets that are actually schedulable; the exact tests match the simulation every time.

> **Note on "target U".** C has to be a whole number of ticks, so it is rounded and at least 1 tick. With 20 tasks and periods as short as 10, many tiny tasks get rounded *up*, so the **real** utilization is often well above the target (see the *real U* column in the viewer). That is why only about 82% of n = 20 sets are schedulable even at target U = 0.50. The verdict check uses the real C and T, so it is not affected.

**Benchmark** (EDF, target U = 0.7, 5 task sets per size, one hyperperiod each):

| N | Sorted list avg / worst | Binary heap avg / worst | Fibonacci heap avg / worst |
|---|---|---|---|
| 20 | 0.10 / 149 | 0.09 / 118 | 0.06 / 86 |
| 200 | 8.06 / 12,527 | 1.70 / 2,468 | 1.01 / 1,700 |
| 1000 | 197.3 / 296,696 | 11.9 / 16,882 | 6.4 / 10,444 |

(comparisons per tick)

* The sorted list's cost grows linearly with N, as expected.
* The Fibonacci heap does the **fewest comparisons** (about half of the binary heap at N = 1000), because insert costs one comparison.
* But in **wall-clock time** the binary heap is faster at large N (≈ 0.39 µs vs 0.67 µs per tick at N = 1000): the Fibonacci heap allocates a node per insert and follows pointers, which costs more than the comparisons it saves.
* The worst tick is t = 0, when all N tasks are released together.
* For small N (≈ 10 or fewer) the plain sorted list is just as good, which matches Baudis et al. (2017).

---

## 8. Tests

```bash
./rtts_tests
```

or, with CMake:

```bash
ctest --test-dir build
```

The 19 tests cover: all three queues against a sorted reference (insert, extract, remove-any, key update); Fibonacci lazy insert, consolidation and decrease-key; the theory numbers from the slides (U = 0.75, bound 0.780, product 1.95, R = 2, 7, 28); the worked example Gantt chart; EDF vs RM at U = 1; overload detection; identical schedules across queues; the verdict experiment; and the CSV round trip.

---

## 9. Assumptions and limits

* Discrete time: one tick is the smallest unit, and C, T and D are whole ticks.
* Single CPU, preemptive, independent periodic tasks, D = T, all released together at t = 0 (the critical instant). Simulating one hyperperiod from there is enough for the exact verdict.
* The scheduler's own run time is measured (benchmark), but it is not added to the simulated task time.
* A job that misses its deadline keeps running; the task's next job is released when the late one finishes.

## 10. References

1. C. L. Liu and J. W. Layland, "Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment," *Journal of the ACM*, 20(1), 1973.
2. N. Baudis, F. Jacob and P. Andelfinger, "Performance Evaluation of Priority Queues for Fine-Grained Parallel Tasks on GPUs," *IEEE MASCOTS*, 2017.
3. I. Zagan and V. G. Găitan, "Designing a Custom CPU Architecture Based on Hardware RTOS and Dynamic Preemptive Scheduler," *Mathematics*, 10(15), 2022.
4. M. L. Fredman and R. E. Tarjan, "Fibonacci Heaps and Their Uses in Improved Network Optimization Algorithms," *Journal of the ACM*, 34(3), 1987.
5. E. Bini and G. C. Buttazzo, "Measuring the Performance of Schedulability Tests," *Real-Time Systems*, 30, 2005 (UUniFast).
6. E. Bini, G. C. Buttazzo and G. M. Buttazzo, "Rate Monotonic Analysis: The Hyperbolic Bound," *IEEE Transactions on Computers*, 52(7), 2003.
