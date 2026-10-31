// queue_factory.hpp
// Creates a queue by name, so the type can be chosen on the command line.
//
//     "binary"      - indexed binary min-heap  (our main design)
//     "fibonacci"   - Fibonacci heap           (the second version we compare)
//     "sorted_list" - sorted list              (existing work: FreeRTOS)
//     "rb_tree"     - red-black tree           (existing work: Linux CFS / EEVDF)

#pragma once

#include <memory>
#include <stdexcept>
#include <string>
#include <vector>

#include "rtts/queues/binary_heap.hpp"
#include "rtts/queues/fibonacci_heap.hpp"
#include "rtts/queues/rb_tree_queue.hpp"
#include "rtts/queues/sorted_list.hpp"

namespace rtts {

inline const std::vector<std::string>& allQueueKinds() {
    static const std::vector<std::string> kinds = {"sorted_list", "rb_tree", "binary", "fibonacci"};
    return kinds;
}

inline std::unique_ptr<TaskQueue> makeQueue(const std::string& kind, KeyFn key) {
    if (kind == "binary") return std::make_unique<BinaryHeap>(key);
    if (kind == "fibonacci") return std::make_unique<FibonacciHeap>(key);
    if (kind == "sorted_list") return std::make_unique<SortedList>(key);
    if (kind == "rb_tree") return std::make_unique<RedBlackTreeQueue>(key);
    throw std::invalid_argument("unknown queue type '" + kind + "' (use binary, fibonacci, sorted_list or rb_tree)");
}

inline std::string queueLabel(const std::string& kind) {
    if (kind == "binary") return "Binary heap";
    if (kind == "fibonacci") return "Fibonacci heap";
    if (kind == "sorted_list") return "Sorted list";
    if (kind == "rb_tree") return "Red-black tree";
    return kind;
}

}  // namespace rtts
