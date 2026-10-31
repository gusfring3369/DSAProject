// rb_tree_queue.cpp

#include "rtts/queues/rb_tree_queue.hpp"

#include <stdexcept>

namespace rtts {

void RedBlackTreeQueue::insert(Task* task) {
    // std::set walks down the tree (O(log N) comparisons) and rebalances.
    auto result = tree_.insert(Entry{keyOf(task), task->id, task});
    where_[task->id] = result.first;
}

Task* RedBlackTreeQueue::peek() const {
    return tree_.empty() ? nullptr : tree_.begin()->task;
}

Task* RedBlackTreeQueue::extractMin() {
    if (tree_.empty()) return nullptr;
    Task* top = tree_.begin()->task;
    where_.erase(top->id);
    tree_.erase(tree_.begin());   // erasing by iterator needs no comparisons
    return top;
}

void RedBlackTreeQueue::remove(Task* task) {
    auto it = where_.find(task->id);
    if (it == where_.end()) throw std::runtime_error("RedBlackTreeQueue::remove: task not in tree");
    tree_.erase(it->second);
    where_.erase(it);
}

void RedBlackTreeQueue::update(Task* task) {
    remove(task);
    insert(task);
}

bool RedBlackTreeQueue::contains(const Task* task) const {
    return where_.count(task->id) > 0;
}

std::vector<Task*> RedBlackTreeQueue::tasks() const {
    std::vector<Task*> out;
    for (const Entry& e : tree_) out.push_back(e.task);
    return out;
}

QueueView RedBlackTreeQueue::view() const {
    // std::set does not show us its internal tree, so we draw it in order,
    // like the sorted list.
    QueueView v;
    v.kind = kind();
    for (const Entry& e : tree_) {
        NodeView node;
        node.taskId = e.id;
        node.key = e.key;
        v.roots.push_back(node);
    }
    v.topId = tree_.empty() ? -1 : tree_.begin()->id;
    return v;
}

}  // namespace rtts
