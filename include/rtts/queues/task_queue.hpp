// task_queue.hpp
// The common interface for every priority queue in the project.
//
// The scheduler only ever talks to a TaskQueue, so the exact same scheduler
// code runs with a binary heap, a Fibonacci heap or a sorted list.
//
// Ordering: every queue is created with a KeyFn. Smaller key = higher
// priority, and the task id breaks ties so the order is always deterministic.
//     Sleep queue:  key = wake-up time
//     Ready queue:  key = period (RM) or absolute deadline (EDF)

#pragma once

#include <string>
#include <vector>

#include "rtts/task.hpp"

namespace rtts {

using KeyFn = long (*)(const Task&);

// A plain picture of a queue, used to draw it (terminal and HTML viewer).
// Every queue is shown as a forest:
//   binary heap    -> one tree (the heap array drawn as a tree)
//   Fibonacci heap -> the root list, one tree per root
//   sorted list    -> a chain, every element is a single-node "tree"
struct NodeView {
    int taskId = -1;
    long key = 0;
    bool marked = false;            // only used by the Fibonacci heap
    std::vector<NodeView> children;
};

struct QueueView {
    std::string kind;
    std::vector<NodeView> roots;
    int topId = -1;                 // task at the top of the queue, -1 if empty
};

class TaskQueue {
public:
    explicit TaskQueue(KeyFn key) : key_(key) {}
    virtual ~TaskQueue() = default;

    virtual void insert(Task* task) = 0;        // add a task
    virtual Task* peek() const = 0;             // top task, nullptr if empty
    virtual Task* extractMin() = 0;             // remove and return the top task
    virtual void remove(Task* task) = 0;        // remove ANY task
    virtual void update(Task* task) = 0;        // call after a task's key changed
    virtual bool contains(const Task* task) const = 0;
    virtual std::size_t size() const = 0;
    virtual std::vector<Task*> tasks() const = 0;
    virtual QueueView view() const = 0;
    virtual std::string kind() const = 0;

    bool empty() const { return size() == 0; }

    // How many key comparisons this queue has done. This is our
    // machine-independent measure of "work per tick".
    long long comparisons() const { return comparisons_; }

protected:
    // True if task a should come out before task b. Counts the comparison.
    bool before(const Task* a, const Task* b) const {
        ++comparisons_;
        long ka = key_(*a), kb = key_(*b);
        return ka < kb || (ka == kb && a->id < b->id);
    }

    long keyOf(const Task* t) const { return key_(*t); }

private:
    KeyFn key_;
    mutable long long comparisons_ = 0;
};

}  // namespace rtts
