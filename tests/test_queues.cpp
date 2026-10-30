// test_queues.cpp
// All three queues must behave like a correct priority queue.
// The same checks run for every queue type.

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

}  // namespace

TEST(queues_extract_in_sorted_order) {
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
        CHECK(q->extractMin() == nullptr);
    });
}

TEST(queues_remove_any_task) {
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

TEST(queues_update_after_key_change) {
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

TEST(fibonacci_decrease_key_and_lazy_insert) {
    Random rng(4);
    std::vector<Task> tasks = makeTasks(64, rng);
    FibonacciHeap heap(testKey);
    for (Task& t : tasks) heap.insert(&t);

    // Insert is lazy: 64 inserts give 64 one-node trees.
    CHECK_EQ(heap.rootCount(), 64u);

    // The first extractMin consolidates: afterwards all root degrees differ,
    // so there can be at most log2(63) + 1 roots.
    heap.extractMin();
    CHECK(heap.rootCount() <= 7u);

    // decreaseKey moves a task straight to the top.
    Task* last = heap.tasks().back();
    last->release = -1;
    heap.decreaseKey(last);
    CHECK(heap.peek() == last);
    CHECK(heap.extractMin() == last);
}

TEST(queues_count_comparisons) {
    // The sorted list needs ~N comparisons per insert, the heaps far fewer.
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
    CHECK(binary.comparisons() > fib.comparisons());   // Fibonacci insert: 1 comparison each
    CHECK_EQ(fib.comparisons(), 499LL);   // the very first insert has no min to compare with
}
