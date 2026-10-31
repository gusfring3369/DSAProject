# Test cases

Run them with `./rtts_tests` (or `ctest --test-dir build`). Each test prints its id, for example `[ OK ] TC09_fibonacci_cascading_cut`.

Every test was also checked the other way round: we broke the binary heap (ignored the right child in sift-down) and the Fibonacci heap (never marked a node) on purpose, and the matching tests failed.

## Data structures (TC01 – TC10)

TC01 – TC05 run on **every** queue type (sorted list, red-black tree, binary heap, Fibonacci heap).

| ID | What is checked | Input | Expected result |
|---|---|---|---|
| TC01 | Tasks come out in priority order | 200 tasks, keys 0–49 (many equal keys) | Sorted by key; equal keys by smaller task id |
| TC02 | Removing **any** task, not just the top | 100 tasks, remove every third one | Removed tasks are gone; the rest still come out sorted |
| TC03 | Key changes (up and down) | 60 tasks, change 50% of the keys | Still extracted in correct order |
| TC04 | Decrease-key | 20 tasks get a key smaller than all others | Each one becomes the top immediately |
| TC05 | Empty queue and single element | — | `peek()` / `extractMin()` return nothing; one in, one out |
| TC06 | Binary heap shape and index | 2,000 random insert / remove / update / extract | After every step: heap-ordered, one tree, index finds every task |
| TC07 | Fibonacci lazy insert and consolidation | 64 inserts, then 1 extract-min | 64 roots before; at most 6 roots with different degrees after |
| TC08 | Fibonacci heap order | 1,500 random operations | Every tree heap-ordered, no marked roots, min pointer correct |
| TC09 | Fibonacci cascading cut | Node X loses two children | After the 1st cut X is marked; after the 2nd, X is cut and becomes an unmarked root |
| TC10 | Comparison counts | 500 inserts into each structure | Sorted list > binary heap > Fibonacci; Fibonacci = exactly 499 (1 per insert) |

## Theory and generator (TC11 – TC16)

| ID | What is checked | Input | Expected result |
|---|---|---|---|
| TC11 | Numbers on the slides | T = 10, 20, 40; C = 2, 5, 12 | U = 0.75, bound 0.780, product 1.95, R = 2, 7, 28, all tests pass |
| TC12 | RTA finds a miss | (T, C) = (4, 2), (10, 5) | R₁ = 11 > D = 10 → RM fails; EDF passes (U = 1) |
| TC13 | Liu-Layland is only sufficient | (10, 5), (20, 10), U = 1.0 | Liu-Layland fails, RTA passes |
| TC14 | EDF test exact at U = 1 | U = 1.0 and U = 1.02 | Pass and fail |
| TC15 | UUniFast | n = 2, 5, 20; U = 0.8 | All u_i ≥ 0 and they sum to 0.8 |
| TC16 | Hyperperiod | periods 10, 20, 40 and 25, 40 | 40 and 200 |

## Simulator (TC17 – TC21)

| ID | What is checked | Input | Expected result |
|---|---|---|---|
| TC17 | Worked example (slide) | Sample set, RM, every queue | Gantt `0011111222002222222200111112..00........`, T2 finishes at 28, 0 misses |
| TC18 | EDF vs RM | (4, 2), (10, 5), U = 1.0 | RM misses a deadline; EDF has 0 misses and 0 idle ticks |
| TC19 | Overload | (10, 6), (20, 10), U = 1.1 | Misses detected with every queue |
| TC20 | Heap snapshots | Sample set, Fibonacci heap | Tick 0: Sleep empty, Ready has 3 one-node trees, T0 on top |
| TC21 | Same schedule with every queue | 50 random sets, RM and EDF, all four structures | Identical Gantt charts |

## Verdict check and dataset (TC22 – TC24)

| ID | What is checked | Input | Expected result |
|---|---|---|---|
| TC22 | Verdict on the sample set | Sample set | Simulator and theory agree |
| TC23 | Verdict on random sets | 330 sets (10 per n, U pair), all four data structures | 0 mismatches |
| TC24 | Dataset file | Write and read back the CSV | Same task sets |

## Added for the final review (TC25 – TC26)

| ID | What is checked | Input | Expected result |
|---|---|---|---|
| TC25 | Red-black tree costs | 1,000 inserts, 1 remove, 100 extract-mins | ≤ 2 log₂N + 2 comparisons per insert; remove and extract-min need 0 comparisons |
| TC26 | Benchmark follows the theory | Operation benchmark, N = 1,000 | Fibonacci insert = 1 comparison; binary insert ≤ log₂N + 1; sorted-list worst insert > N/2; Fibonacci worst extract-min > N/2 (O(N)) but average < 3 log₂N; Fibonacci decrease-key ≤ 2; binary decrease-key ≤ log₂N + 1 |

Result: **26 / 26 passed.** (24 / 24 at the second review.)
