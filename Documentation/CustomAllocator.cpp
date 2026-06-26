#include <iostream>
#include <vector>
#include <memory>

template <typename T>
class SimplePoolAllocator {
public:
    using value_type = T;

    SimplePoolAllocator() = default;

    // This allows the allocator to be used for different types (e.g., internal tree nodes)
    template <typename U>
    SimplePoolAllocator(const SimplePoolAllocator<U>&) noexcept {}

    // The actual allocation logic
    T* allocate(std::size_t n) {
        if (n > std::size_t(-1) / sizeof(T)) throw std::bad_alloc();
        
        std::cout << "[Alloc] Requesting space for " << n << " objects (" 
                  << n * sizeof(T) << " bytes).\n";
        
        if (auto p = static_cast<T*>(std::malloc(n * sizeof(T)))) {
            return p;
        }
        
        throw std::bad_alloc();
    }

    // The deallocation logic
    void deallocate(T* p, std::size_t n) noexcept {
        std::cout << "[Free] Releasing " << n * sizeof(T) << " bytes.\n";
        std::free(p);
    }
};

// Comparison operators are required for allocators
template <class T, class U>
bool operator==(const SimplePoolAllocator<T>&, const SimplePoolAllocator<U>&) { return true; }

template <class T, class U>
bool operator!=(const SimplePoolAllocator<T>&, const SimplePoolAllocator<U>&) { return false; }


int main() {
    // We tell the vector to use OUR allocator instead of the default one
    std::vector<int, SimplePoolAllocator<int>> myVec;

    std::cout << "--- Pushing elements ---\n";
    myVec.push_back(10); // Triggering our allocator
    myVec.push_back(20);
    
    std::cout << "--- Vector going out of scope ---\n";
    return 0;
}