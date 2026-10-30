// sorted_list.hpp
// Sorted list queue: the baseline.
//
// This is how many small RTOS kernels (FreeRTOS, for example) keep their
// delayed-task list: a list kept in sorted order. Peek is O(1), but every
// insert walks the list to find its spot, so insert is O(N).
// It is here only so the benchmark can compare it with the two heaps.

#pragma once

#include <list>

#include "rtts/queues/task_queue.hpp"

namespace rtts {

class SortedList : public TaskQueue {
public:
    explicit SortedList(KeyFn key) : TaskQueue(key) {}

    void insert(Task* task) override;           // O(N)
    Task* peek() const override;                // O(1)
    Task* extractMin() override;                // O(1)
    void remove(Task* task) override;           // O(N)
    void update(Task* task) override;           // O(N)
    bool contains(const Task* task) const override;
    std::size_t size() const override { return items_.size(); }
    std::vector<Task*> tasks() const override;
    QueueView view() const override;
    std::string kind() const override { return "sorted_list"; }

private:
    std::list<Task*> items_;   // always sorted, highest priority first
};

}  // namespace rtts
