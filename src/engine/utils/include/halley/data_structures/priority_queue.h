#include <algorithm>
#include <vector>

namespace Halley {
    template <typename T, typename Comparator>
    class PriorityQueue {
    public:
        PriorityQueue(Comparator comparator)
            : comparator(comparator)
        {
        }
        
        void push(T value)
        {
            heap.push_back(std::move(value));
            std::push_heap(heap.begin(), heap.end(), comparator);
        }

        void pop()
        {
            std::pop_heap(heap.begin(), heap.end(), comparator);
            heap.pop_back();
        }

        const T& top() const
        {
            return heap.front();
        }

        void update(const T& value)
        {
            const auto iter = std::find(heap.begin(), heap.end(), value);
            if (iter != heap.end()) {
                std::push_heap(heap.begin(), iter + 1, comparator);
            }
        }

        bool empty() const
        {
            return heap.empty();
        }

        void reserve(size_t size)
        {
	        heap.reserve(size);
        }

    private:
        std::vector<T> heap;
        Comparator comparator;
    };
}
