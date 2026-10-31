// binary_heap.cpp
// See binary_heap.hpp for the idea. Every swap also updates heapPos,
// otherwise a task would point to the wrong slot.

#include "rtts/queues/binary_heap.hpp"

#include <stdexcept>

namespace rtts {

void BinaryHeap::insert(Task* task) {
    // Put the task at the end and let it float up to its place.
    items_.push_back(task);
    task->heapPos = static_cast<int>(items_.size()) - 1;
    siftUp(items_.size() - 1);
}

Task* BinaryHeap::peek() const {
    return items_.empty() ? nullptr : items_.front();
}

Task* BinaryHeap::extractMin() {
    if (items_.empty()) return nullptr;
    Task* top = items_.front();
    remove(top);
    return top;
}

void BinaryHeap::remove(Task* task) {
    if (!contains(task)) throw std::runtime_error("BinaryHeap::remove: task not in heap");

    // Trick: swap with the last element, pop the last element, then fix the
    // element that moved into the hole. It may need to go up OR down.
    std::size_t i = task->heapPos;
    std::size_t last = items_.size() - 1;
    swapItems(i, last);
    items_.pop_back();
    task->heapPos = -1;

    if (i < items_.size()) {
        Task* moved = items_[i];
        siftUp(i);
        siftDown(moved->heapPos);
    }
}

void BinaryHeap::update(Task* task) {
    siftUp(task->heapPos);
    siftDown(task->heapPos);
}

void BinaryHeap::decreaseKey(Task* task) {
    // A smaller key can only move the task towards the root.
    siftUp(task->heapPos);
}

bool BinaryHeap::contains(const Task* task) const {
    // Check the slot really holds this task: a stale heapPos left over from
    // another heap must not count.
    return task->heapPos >= 0 && static_cast<std::size_t>(task->heapPos) < items_.size() &&
           items_[task->heapPos] == task;
}

void BinaryHeap::swapItems(std::size_t i, std::size_t j) {
    std::swap(items_[i], items_[j]);
    items_[i]->heapPos = static_cast<int>(i);
    items_[j]->heapPos = static_cast<int>(j);
}

void BinaryHeap::siftUp(std::size_t i) {
    while (i > 0) {
        std::size_t parent = (i - 1) / 2;
        if (!before(items_[i], items_[parent])) break;
        swapItems(i, parent);
        i = parent;
    }
}

void BinaryHeap::siftDown(std::size_t i) {
    const std::size_t n = items_.size();
    while (true) {
        std::size_t left = 2 * i + 1, right = 2 * i + 2, smallest = i;
        if (left < n && before(items_[left], items_[smallest])) smallest = left;
        if (right < n && before(items_[right], items_[smallest])) smallest = right;
        if (smallest == i) break;
        swapItems(i, smallest);
        i = smallest;
    }
}

// ------------------------------------------------------------- visualisation

NodeView BinaryHeap::viewAt(std::size_t i) const {
    NodeView node;
    node.taskId = items_[i]->id;
    node.key = keyOf(items_[i]);
    for (std::size_t c : {2 * i + 1, 2 * i + 2})
        if (c < items_.size()) node.children.push_back(viewAt(c));
    return node;
}

QueueView BinaryHeap::view() const {
    QueueView v;
    v.kind = kind();
    if (!items_.empty()) {
        v.roots.push_back(viewAt(0));
        v.topId = items_.front()->id;
    }
    return v;
}

}  // namespace rtts
