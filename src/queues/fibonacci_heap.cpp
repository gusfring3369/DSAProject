// fibonacci_heap.cpp
// See fibonacci_heap.hpp for the big picture.

#include "rtts/queues/fibonacci_heap.hpp"

#include <algorithm>
#include <stdexcept>

namespace rtts {

void FibonacciHeap::insert(Task* task) {
    // Lazy: make a one-node tree, add it to the root list, fix the min pointer.
    auto node = std::make_unique<Node>();
    node->task = task;
    Node* raw = node.get();
    nodes_[task->id] = std::move(node);

    roots_.push_back(raw);
    if (min_ == nullptr || before(raw, min_)) min_ = raw;
}

Task* FibonacciHeap::peek() const {
    return min_ ? min_->task : nullptr;
}

Task* FibonacciHeap::extractMin() {
    Node* z = min_;
    if (z == nullptr) return nullptr;

    // 1. Every child of the minimum becomes a root.
    for (Node* child : z->children) {
        child->parent = nullptr;
        child->marked = false;
        roots_.push_back(child);
    }
    z->children.clear();

    // 2. Remove the minimum itself.
    eraseFromRoots(z);
    Task* task = z->task;
    nodes_.erase(task->id);   // frees the node
    min_ = nullptr;

    // 3. Tidy up. This is where all the postponed work happens.
    if (!roots_.empty()) consolidate();
    return task;
}

void FibonacciHeap::decreaseKey(Task* task) {
    // If the node is now smaller than its parent, cut it out and make it a
    // root. Then walk up and cut any parent that has lost two children
    // (cascading cut). This keeps the trees bushy, which is what guarantees
    // the O(log N) degree bound.
    Node* node = nodes_.at(task->id).get();
    Node* parent = node->parent;
    if (parent != nullptr && before(node, parent)) {
        cut(node);
        cascadingCut(parent);
    }
    if (before(node, min_)) min_ = node;
}

void FibonacciHeap::remove(Task* task) {
    // Textbook version: decrease the key to -infinity, then extractMin.
    // We do the same without a fake key: move the node up to the root list
    // and point min_ at it, so extractMin removes exactly this node.
    auto it = nodes_.find(task->id);
    if (it == nodes_.end()) throw std::runtime_error("FibonacciHeap::remove: task not in heap");
    Node* node = it->second.get();
    if (node->parent != nullptr) {
        Node* parent = node->parent;
        cut(node);
        cascadingCut(parent);
    }
    min_ = node;
    extractMin();
}

void FibonacciHeap::update(Task* task) {
    // Fibonacci heaps are only fast for decrease-key. For a general key
    // change (it could also go UP) we simply remove and re-insert.
    remove(task);
    insert(task);
}

bool FibonacciHeap::contains(const Task* task) const {
    return nodes_.count(task->id) > 0;
}

std::vector<Task*> FibonacciHeap::tasks() const {
    std::vector<Task*> out;
    for (const auto& entry : nodes_) out.push_back(entry.second->task);
    return out;
}

// ------------------------------------------------------------------ helpers

void FibonacciHeap::consolidate() {
    // Link roots of equal degree until all root degrees are different.
    // byDegree[d] = the root of degree d we have seen so far (the array A in
    // the textbook). Degrees are O(log N), so the array stays small.
    std::vector<Node*> byDegree;
    for (Node* node : roots_) {
        while (node->degree() < byDegree.size() && byDegree[node->degree()] != nullptr) {
            Node* other = byDegree[node->degree()];
            byDegree[node->degree()] = nullptr;
            node = link(node, other);
        }
        if (node->degree() >= byDegree.size()) byDegree.resize(node->degree() + 1, nullptr);
        byDegree[node->degree()] = node;
    }

    // Rebuild the root list and find the new minimum.
    roots_.clear();
    min_ = nullptr;
    for (Node* root : byDegree) {
        if (root == nullptr) continue;
        roots_.push_back(root);
        if (min_ == nullptr || before(root, min_)) min_ = root;
    }
}

FibonacciHeap::Node* FibonacciHeap::link(Node* a, Node* b) {
    // The root with the larger key becomes a child of the other one.
    if (before(b, a)) std::swap(a, b);
    b->parent = a;
    b->marked = false;
    a->children.push_back(b);
    return a;
}

void FibonacciHeap::cut(Node* node) {
    // Detach node from its parent and move it to the root list.
    auto& siblings = node->parent->children;
    siblings.erase(std::find(siblings.begin(), siblings.end(), node));
    node->parent = nullptr;
    node->marked = false;
    roots_.push_back(node);
}

void FibonacciHeap::cascadingCut(Node* node) {
    Node* parent = node->parent;
    if (parent == nullptr) return;   // roots are never marked
    if (!node->marked) {
        node->marked = true;          // first child lost: just remember it
    } else {
        cut(node);                    // second child lost: cut this node too
        cascadingCut(parent);
    }
}

void FibonacciHeap::eraseFromRoots(Node* node) {
    roots_.erase(std::find(roots_.begin(), roots_.end(), node));
}

// ------------------------------------------------------------- visualisation

NodeView FibonacciHeap::viewOf(const Node* node) const {
    NodeView v;
    v.taskId = node->task->id;
    v.key = keyOf(node->task);
    v.marked = node->marked;
    for (const Node* child : node->children) v.children.push_back(viewOf(child));
    return v;
}

QueueView FibonacciHeap::view() const {
    QueueView v;
    v.kind = kind();
    for (const Node* root : roots_) v.roots.push_back(viewOf(root));
    v.topId = min_ ? min_->task->id : -1;
    return v;
}

}  // namespace rtts
