// rb_tree_queue.hpp
// Red-black tree queue: the "existing work" comparison.
//
// The Linux kernel's task scheduler (CFS, and EEVDF since Linux 6.6) keeps
// its runnable tasks in a red-black tree ordered by a key, and caches the
// leftmost node so the next task is found in O(1). This class does the same
// with std::set, which is a red-black tree in every major C++ library.
//
//     insert       O(log N)
//     peek         O(1)        (begin() is the leftmost node)
//     extractMin   O(1) amortized (erase the leftmost node)
//     remove       O(1) amortized (we keep an iterator to every task)
//
// The key is copied into the set when a task is inserted, so the tree stays
// valid even after the caller changes the task's key; update() then removes
// the old entry and inserts a new one.

#pragma once

#include <set>
#include <unordered_map>

#include "rtts/queues/task_queue.hpp"

namespace rtts {

class RedBlackTreeQueue : public TaskQueue {
public:
    explicit RedBlackTreeQueue(KeyFn key) : TaskQueue(key), tree_(ByKey{this}) {}

    // Not copyable: the tree's comparison object points back to this queue.
    RedBlackTreeQueue(const RedBlackTreeQueue&) = delete;
    RedBlackTreeQueue& operator=(const RedBlackTreeQueue&) = delete;

    void insert(Task* task) override;           // O(log N)
    Task* peek() const override;                // O(1)
    Task* extractMin() override;                // O(1) amortized
    void remove(Task* task) override;           // O(1) amortized
    void update(Task* task) override;           // O(log N): remove + insert
    bool contains(const Task* task) const override;
    std::size_t size() const override { return tree_.size(); }
    std::vector<Task*> tasks() const override;
    QueueView view() const override;
    std::string kind() const override { return "rb_tree"; }

private:
    struct Entry {
        long key;      // copy of the key at insert time
        int id;        // tie-break, same rule as the other queues
        Task* task;
    };

    // The comparison std::set uses. It counts every call, like the other queues.
    struct ByKey {
        const RedBlackTreeQueue* owner;
        bool operator()(const Entry& a, const Entry& b) const {
            owner->countComparison();
            return a.key < b.key || (a.key == b.key && a.id < b.id);
        }
    };

    std::set<Entry, ByKey> tree_;
    std::unordered_map<int, std::set<Entry, ByKey>::iterator> where_;   // task id -> its tree node
};

}  // namespace rtts
