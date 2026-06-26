#include <iostream>
#include <stdexcept>
#include <memory>

using namespace std;

class Test {
    // int* ptr;
    unique_ptr<int[]> ptr; // Using smart pointer for automatic memory management

public:
    Test() {
        // ptr = new int[5];   // raw allocation
        ptr = make_unique<int[]>(5);
        cout << "Memory allocated\n";
        throw runtime_error("Constructor failed");
    }

    ~Test() {
        // delete[] ptr; // No need to manually delete when using smart pointer
        cout << "Destructor called\n";
    }
};

int main()
{
    try {
        Test obj;
    }
    catch (...) {
        cout << "Exception caught\n";
    }
}