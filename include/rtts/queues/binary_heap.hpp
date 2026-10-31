// binary_heap.hpp
// Indexed binary min-heap (Modules 2 and 3, binary version).
//
// A normal binary heap stored in an array:
//     parent of i   = (i - 1) / 2
//     children of i = 2i + 1 and 2i + 2
//
// "Indexed" means we also keep a map  task id -> position in the array.
// That map lets us find, remove or re-prioritise ANY task in O(log N)
// instead of scanning the whole array in O(N).

#pragma once

#include <unordered_map>
#include <vector>

#include "rtts/queues/task_queue.hpp"

namespace rtts {

class BinaryHeap : public TaskQueue {
public:
    explicit BinaryHeap(KeyFn key) : TaskQueue(key) {}

    void insert(Task* task) override;           // O(log N)
    Task* peek() const override;                // O(1)
    Task* extractMin() override;                // O(log N)
    void remove(Task* task) override;           // O(log N), any task
    void update(Task* task) override;           // O(log N)
    void decreaseKey(Task* task) override;      // O(log N), only sifts up
    bool contains(const Task* task) const override;
    std::size_t size() const override { return items_.size(); }
    std::vector<Task*> tasks() const override { return items_; }
    QueueView view() const override;
    std::string kind() const override { return "binary"; }

private:
    void swapItems(std::size_t i, std::size_t j);
    void siftUp(std::size_t i);
    void siftDown(std::size_t i);
    NodeView viewAt(std::size_t i) const;

    std::vector<Task*> items_;                       // the heap array
    std::unordered_map<int, std::size_t> position_;  // task id -> index (the "index")
};

}  // namespace rtts
