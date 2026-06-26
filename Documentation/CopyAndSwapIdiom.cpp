#include <algorithm> // for std::swap

class SmartBuffer {
    size_t size;
    int* data;

public:
    // ... Constructors as defined before ...

    // High-performance, exception-safe Swap function
    friend void swap(SmartBuffer& first, SmartBuffer& second) noexcept {
        using std::swap; // ADL (Argument Dependent Lookup)
        swap(first.size, second.size);
        swap(first.data, second.data);
    }

    // The Unified Assignment Operator
    // Note: We take 'other' BY VALUE. This handles both Copy and Move!
    SmartBuffer& operator=(SmartBuffer other) noexcept {
        swap(*this, other);
        return *this;
    } 
    // When 'other' goes out of scope here, the OLD memory 
    // held by *this is automatically deleted.
};