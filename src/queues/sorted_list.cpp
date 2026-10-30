// sorted_list.cpp

#include "rtts/queues/sorted_list.hpp"

#include <algorithm>
#include <stdexcept>

namespace rtts {

void SortedList::insert(Task* task) {
    // Walk from the front until we find a task that should come after us.
    auto it = items_.begin();
    while (it != items_.end() && !before(task, *it)) ++it;
    items_.insert(it, task);
}

Task* SortedList::peek() const {
    return items_.empty() ? nullptr : items_.front();
}

Task* SortedList::extractMin() {
    if (items_.empty()) return nullptr;
    Task* top = items_.front();
    items_.pop_front();
    return top;
}

void SortedList::remove(Task* task) {
    auto it = std::find(items_.begin(), items_.end(), task);
    if (it == items_.end()) throw std::runtime_error("SortedList::remove: task not in list");
    items_.erase(it);
}

void SortedList::update(Task* task) {
    remove(task);
    insert(task);
}

bool SortedList::contains(const Task* task) const {
    return std::find(items_.begin(), items_.end(), task) != items_.end();
}

std::vector<Task*> SortedList::tasks() const {
    return {items_.begin(), items_.end()};
}

QueueView SortedList::view() const {
    // Drawn as a flat row of single nodes, in order.
    QueueView v;
    v.kind = kind();
    for (Task* t : items_) {
        NodeView node;
        node.taskId = t->id;
        node.key = keyOf(t);
        v.roots.push_back(node);
    }
    v.topId = items_.empty() ? -1 : items_.front()->id;
    return v;
}

}  // namespace rtts
