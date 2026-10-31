// fibonacci_heap.hpp
// Fibonacci heap (Modules 2 and 3, Fibonacci version).
// Reference: Fredman & Tarjan, "Fibonacci heaps and their uses in improved
// network optimization algorithms", Journal of the ACM, 1987.
//
// The idea in one paragraph:
//   A Fibonacci heap is a list of min-ordered trees (the "root list").
//   insert is lazy: it just drops a one-node tree into the root list, O(1).
//   All tidying up is postponed until extractMin, which "consolidates" the
//   root list by linking trees of equal degree until every degree appears at
//   most once. Averaged over many operations (amortized) that is O(log N),
//   but ONE extractMin after many inserts can touch O(N) roots. For a
//   real-time scheduler that worst case matters, which is exactly what our
//   benchmark measures.
//
// To keep the code readable, children and roots are stored in std::vector
// instead of the classic circular doubly linked lists. Removing a child
// costs O(degree), and the degree is O(log N), so the bounds still hold.

#pragma once

#include <memory>
#include <unordered_map>
#include <vector>

#include "rtts/queues/task_queue.hpp"

namespace rtts {

class FibonacciHeap : public TaskQueue {
public:
    explicit FibonacciHeap(KeyFn key) : TaskQueue(key) {}

    void insert(Task* task) override;           // O(1)
    Task* peek() const override;                // O(1)
    Task* extractMin() override;                // O(log N) amortized, O(N) worst case
    void remove(Task* task) override;           // O(log N) amortized
    void update(Task* task) override;           // general change: remove + insert
    void decreaseKey(Task* task) override;      // O(1) amortized, key got SMALLER
    bool contains(const Task* task) const override;
    std::size_t size() const override { return nodes_.size(); }
    std::vector<Task*> tasks() const override;
    QueueView view() const override;
    std::string kind() const override { return "fibonacci"; }

    std::size_t rootCount() const { return roots_.size(); }

private:
    struct Node {
        Task* task = nullptr;
        Node* parent = nullptr;
        std::vector<Node*> children;
        // A node is "marked" once it has lost a child since it became a child
        // itself. Losing a second child triggers a cascading cut.
        bool marked = false;
        std::size_t degree() const { return children.size(); }
    };

    bool before(const Node* a, const Node* b) const { return TaskQueue::before(a->task, b->task); }
    void consolidate();
    Node* link(Node* a, Node* b);
    void cut(Node* node);
    void cascadingCut(Node* node);
    void eraseFromRoots(Node* node);
    NodeView viewOf(const Node* node) const;

    std::vector<Node*> roots_;                                // the root list
    Node* min_ = nullptr;                                     // root with the smallest key
    std::unordered_map<int, std::unique_ptr<Node>> nodes_;    // task id -> node (the handle)
};

}  // namespace rtts
