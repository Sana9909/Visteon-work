// MOVE SEMANTICS AND THE RULE OF FIVE
// In C++, when a class manages resources (like dynamic memory, file handles, etc.),
// it should follow the Rule of Five to ensure proper resource management and avoid issues like memory leaks, double deletions, and dangling pointers.
// The Rule of Five states that if a class defines one of the following special member functions, it should probably explicitly define all five:
// 1. Destructor
// 2. Copy Constructor
// 3. Copy Assignment Operator
// 4. Move Constructor
// 5. Move Assignment Operator

#include <iostream>
#include <algorithm>

class SmartBuffer {
private:
    size_t size;
    int* data;

public:
    // 1. Constructor
    explicit SmartBuffer(size_t s) : size(s), data(new int[s]) {
        std::cout << "Allocated " << size << " integers.\n";
    }

    // 2. Destructor (The 'R' in RAII)
    ~SmartBuffer() {
        delete[] data;
        std::cout << "Memory freed.\n";
    }

    // 3. Copy Constructor (Deep Copy)
    SmartBuffer(const SmartBuffer& other) : size(other.size), data(new int[other.size]) {
        std::copy(other.data, other.data + size, data);
        std::cout << "Deep Copy performed.\n";
    }

    // 4. Move Constructor (The "Steal")
    // Use noexcept so STL containers can optimize moves
    SmartBuffer(SmartBuffer&& other) noexcept : data(nullptr), size(0) {
        data = other.data;  // Steal the pointer
        size = other.size;
        other.data = nullptr; // Leave 'other' in a valid but empty state
        other.size = 0;
        std::cout << "Move Constructor used.\n";
    }

    // 5. Move Assignment Operator
    SmartBuffer& operator=(SmartBuffer&& other) noexcept {
        if (this != &other) {
            delete[] data;      // Clean up existing resource
            data = other.data;  // Steal
            size = other.size;
            other.data = nullptr;
            other.size = 0;
        }
        return *this;
    }
};

int main() {
    SmartBuffer buf1(10);           // Constructor
    SmartBuffer buf2 = buf1;        // Copy Constructor
    SmartBuffer buf3 = std::move(buf1); // Move Constructor

    SmartBuffer buf4(5);
    buf4 = std::move(buf2);        // Move Assignment Operator

    return 0;
}


// OR

    // // ... Constructors as defined before ...

    // // High-performance, exception-safe Swap function
    // friend void swap(SmartBuffer& first, SmartBuffer& second) noexcept {
    //     using std::swap; // ADL (Argument Dependent Lookup)
    //     swap(first.size, second.size);
    //     swap(first.data, second.data);
    // }

    // // The Unified Assignment Operator
    // // Note: We take 'other' BY VALUE. This handles both Copy and Move!
    // SmartBuffer& operator=(SmartBuffer other) noexcept {
    //     swap(*this, other);
    //     return *this;
    // } 
    // // When 'other' goes out of scope here, the OLD memory 
    // // held by *this is automatically deleted.