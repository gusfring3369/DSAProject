// test_queues.cpp
// TC01 - TC10: the data structures.
// TC01 - TC05 run the same checks on every queue type; TC06 - TC10 check
// things that only make sense for one structure (heap shape, Fibonacci cuts).

#include <algorithm>
#include <functional>

#include "rtts/generator.hpp"
#include "rtts/queues/queue_factory.hpp"
#include "test_framework.hpp"

using namespace rtts;

namespace {

// The key is stored in 'release' just for these tests.
long testKey(const Task& t) { return t.release; }

std::vector<Task> makeTasks(int count, Random& rng) {
    std::vector<Task> tasks;
    for (int i = 0; i < count; ++i) {
        Task t(i, 10, 1, 10);
        t.release = static_cast<int>(rng.index(50));   // lots of equal keys on purpose
        tasks.push_back(t);
    }
    return tasks;
}

// Expected order: by key, then by id (the tie-break rule).
bool expectedOrder(const Task* a, const Task* b) {
    return a->release != b->release ? a->release < b->release : a->id < b->id;
}

void forEachQueue(const std::function<void(const std::string&)>& body) {
    for (const std::string& kind : allQueueKinds()) body(kind);
}

// Heap order: every child comes after its parent (same rule as expectedOrder).
bool heapOrdered(const NodeView& node) {
    for (const NodeView& child : node.children) {
        bool after = child.key > node.key || (child.key == node.key && child.taskId > node.taskId);
        if (!after || !heapOrdered(child)) return false;
    }
    return true;
}

std::size_t countNodes(const std::vector<NodeView>& nodes) {
    std::size_t n = 0;
    for (const NodeView& node : nodes) n += 1 + countNodes(node.children);
    return n;
}

const NodeView* findNode(const std::vector<NodeView>& nodes, int id) {
    for (const NodeView& node : nodes) {
        if (node.taskId == id) return &node;
        if (const NodeView* found = findNode(node.children, id)) return found;
    }
    return nullptr;
}

}  // namespace

TEST(TC01_every_queue_extracts_in_sorted_order) {
    forEachQueue([](const std::string& kind) {
        Random rng(1);
        std::vector<Task> tasks = makeTasks(200, rng);
        auto q = makeQueue(kind, testKey);
        for (Task& t : tasks) q->insert(&t);
        CHECK_EQ(q->size(), 200u);

        std::vector<Task*> expected;
        for (Task& t : tasks) expected.push_back(&t);
        std::sort(expected.begin(), expected.end(), expectedOrder);

        for (Task* want : expected) {
            CHECK(q->peek() == want);
            CHECK(q->extractMin() == want);
        }
        CHECK(q->empty());
    });
}

TEST(TC02_every_queue_removes_any_task) {
    forEachQueue([](const std::string& kind) {
        Random rng(2);
        std::vector<Task> tasks = makeTasks(100, rng);
        auto q = makeQueue(kind, testKey);
        for (Task& t : tasks) q->insert(&t);

        // Pop a few first, so the Fibonacci heap has real trees, then
        // remove every third task from the middle of the structure.
        std::vector<Task*> remaining;
        for (Task& t : tasks) remaining.push_back(&t);
        std::sort(remaining.begin(), remaining.end(), expectedOrder);
        for (int i = 0; i < 5; ++i) q->extractMin();
        remaining.erase(remaining.begin(), remaining.begin() + 5);

        std::vector<Task*> kept;
        for (std::size_t i = 0; i < remaining.size(); ++i) {
            if (i % 3 == 0) {
                q->remove(remaining[i]);
                CHECK(!q->contains(remaining[i]));
            } else {
                kept.push_back(remaining[i]);
            }
        }
        CHECK_EQ(q->size(), kept.size());
        for (Task* want : kept) CHECK(q->extractMin() == want);
    });
}

TEST(TC03_every_queue_handles_key_changes) {
    forEachQueue([](const std::string& kind) {
        Random rng(3);
        std::vector<Task> tasks = makeTasks(60, rng);
        auto q = makeQueue(kind, testKey);
        for (Task& t : tasks) q->insert(&t);
        q->extractMin();   // build Fibonacci trees

        // Change some keys up and some down.
        for (Task& t : tasks) {
            if (!q->contains(&t)) continue;
            if (t.id % 4 == 0) { t.release += 30; q->update(&t); }
            if (t.id % 4 == 1) { t.release = std::max(0, t.release - 30); q->update(&t); }
        }
        std::vector<Task*> expected = q->tasks();
        std::sort(expected.begin(), expected.end(), expectedOrder);
        for (Task* want : expected) CHECK(q->extractMin() == want);
    });
}

TEST(TC04_every_queue_decrease_key_moves_task_to_top) {
    forEachQueue([](const std::string& kind) {
        Random rng(4);
        std::vector<Task> tasks = makeTasks(50, rng);
        auto q = makeQueue(kind, testKey);
        for (Task& t : tasks) q->insert(&t);
        q->extractMin();

        // Give 20 tasks a key smaller than everything else, one after another.
        int smallest = -1;
        for (Task& t : tasks) {
            if (!q->contains(&t) || t.id % 2 == 0 || smallest < -20) continue;
            t.release = smallest--;
            q->decreaseKey(&t);
            CHECK(q->peek() == &t);
        }
        std::vector<Task*> expected = q->tasks();
        std::sort(expected.begin(), expected.end(), expectedOrder);
        for (Task* want : expected) CHECK(q->extractMin() == want);
    });
}

TEST(TC05_every_queue_empty_case) {
    forEachQueue([](const std::string& kind) {
        auto q = makeQueue(kind, testKey);
        CHECK(q->empty());
        CHECK_EQ(q->size(), 0u);
        CHECK(q->peek() == nullptr);
        CHECK(q->extractMin() == nullptr);
        CHECK(q->view().roots.empty());
        CHECK_EQ(q->view().topId, -1);

        // One element in, one element out, empty again.
        Task t(7, 10, 1, 10);
        q->insert(&t);
        CHECK(q->peek() == &t);
        CHECK(q->extractMin() == &t);
        CHECK(q->empty());
    });
}

TEST(TC06_binary_heap_shape_and_index_stay_correct) {
    // 2000 random operations; after each one the tree must be heap-ordered
    // and the index (task id -> position) must still find every task.
    Random rng(6);
    std::vector<Task> tasks = makeTasks(100, rng);
    BinaryHeap heap(testKey);
    for (int step = 0; step < 2000; ++step) {
        Task& t = tasks[rng.index(tasks.size())];
        int op = static_cast<int>(rng.index(3));
        if (!heap.contains(&t)) {
            heap.insert(&t);
        } else if (op == 0) {
            heap.remove(&t);
        } else if (op == 1) {
            t.release = static_cast<int>(rng.index(50));
            heap.update(&t);
        } else {
            heap.extractMin();
        }
        QueueView v = heap.view();
        CHECK(v.roots.size() <= 1u);
        if (!v.roots.empty()) CHECK(heapOrdered(v.roots[0]));
        CHECK_EQ(countNodes(v.roots), heap.size());
    }
    for (Task* t : heap.tasks()) CHECK(heap.contains(t));
}

TEST(TC07_fibonacci_lazy_insert_then_consolidate) {
    Random rng(7);
    std::vector<Task> tasks = makeTasks(64, rng);
    FibonacciHeap heap(testKey);
    for (Task& t : tasks) heap.insert(&t);

    // Insert is lazy: 64 inserts give 64 one-node trees.
    CHECK_EQ(heap.rootCount(), 64u);

    // The first extractMin consolidates: afterwards every root has a
    // different degree, so 63 nodes fit in at most 6 trees (63 = 32+16+8+4+2+1).
    heap.extractMin();
    QueueView v = heap.view();
    CHECK(v.roots.size() <= 6u);
    std::vector<std::size_t> degrees;
    for (const NodeView& root : v.roots) degrees.push_back(root.children.size());
    std::sort(degrees.begin(), degrees.end());
    CHECK(std::adjacent_find(degrees.begin(), degrees.end()) == degrees.end());
}

TEST(TC08_fibonacci_trees_stay_heap_ordered) {
    Random rng(8);
    std::vector<Task> tasks = makeTasks(80, rng);
    FibonacciHeap heap(testKey);
    for (Task& t : tasks) heap.insert(&t);
    for (int step = 0; step < 1500; ++step) {
        Task& t = tasks[rng.index(tasks.size())];
        int op = static_cast<int>(rng.index(4));
        if (!heap.contains(&t)) heap.insert(&t);
        else if (op == 0) heap.extractMin();
        else if (op == 1) heap.remove(&t);
        else if (op == 2) { t.release -= static_cast<int>(rng.index(20)); heap.decreaseKey(&t); }
        else { t.release += static_cast<int>(rng.index(20)); heap.update(&t); }

        QueueView v = heap.view();
        for (const NodeView& root : v.roots) {
            CHECK(heapOrdered(root));
            CHECK(!root.marked);   // roots are never marked
        }
        CHECK_EQ(countNodes(v.roots), heap.size());
        if (heap.size() > 0) CHECK(heap.peek()->id == v.topId);
    }
}

TEST(TC09_fibonacci_cascading_cut) {
    // Keys 100..115, so the heap builds trees of size 8, 4, 2, 1 after the
    // first extractMin.
    std::vector<Task> tasks;
    for (int i = 0; i < 16; ++i) {
        Task t(i, 10, 1, 10);
        t.release = 100 + i;
        tasks.push_back(t);
    }
    FibonacciHeap heap(testKey);
    for (Task& t : tasks) heap.insert(&t);
    heap.extractMin();

    // Find a node X that is a child of a root and has at least two children.
    QueueView v = heap.view();
    const NodeView* x = nullptr;
    for (const NodeView& root : v.roots)
        for (const NodeView& child : root.children)
            if (x == nullptr && child.children.size() >= 2) x = &child;
    CHECK(x != nullptr);
    if (x == nullptr) return;
    const int xId = x->taskId, a = x->children[0].taskId, b = x->children[1].taskId;

    // X loses its first child: the child moves to the root list, X gets marked.
    tasks[a].release = 0;
    heap.decreaseKey(&tasks[a]);
    v = heap.view();
    CHECK(findNode(v.roots, xId)->marked);
    CHECK(heap.peek() == &tasks[a]);

    // X loses a second child: X itself is cut (cascading cut) and becomes an
    // unmarked root.
    tasks[b].release = 1;
    heap.decreaseKey(&tasks[b]);
    v = heap.view();
    bool xIsRoot = false;
    for (const NodeView& root : v.roots)
        if (root.taskId == xId) xIsRoot = !root.marked;
    CHECK(xIsRoot);
}

TEST(TC10_comparison_counts_match_the_theory) {
    // 500 inserts: the sorted list walks the list (O(N) each), the binary
    // heap sifts up (O(log N) each), the Fibonacci heap does exactly one
    // comparison with the min pointer (O(1) each).
    Random rng(5);
    std::vector<Task> a = makeTasks(500, rng), b = a, c = a;
    SortedList list(testKey);
    BinaryHeap binary(testKey);
    FibonacciHeap fib(testKey);
    for (std::size_t i = 0; i < a.size(); ++i) {
        list.insert(&a[i]);
        binary.insert(&b[i]);
        fib.insert(&c[i]);
    }
    CHECK(list.comparisons() > binary.comparisons());
    CHECK(binary.comparisons() > fib.comparisons());
    CHECK_EQ(fib.comparisons(), 499LL);   // the very first insert has no min to compare with
}
