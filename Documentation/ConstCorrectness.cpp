#include <iostream>
#include <string>

using namespace std;

int main() {

    // 1. Modifying a const variable --> Leads to compilation error
    // const int constVar = 10;
    // constVar = 20; // Error: assignment of read-only variable 'constVar'

    2. const pointer to non-const data --> Leads to compilation error
    int data = 5;   
    int* const constPtr = &data; // const pointer to non-const data
    // constPtr = &data; // Error: assignment of read-only variable 'constPtr'
    *constPtr = 10; // Allowed: modifying the data through const pointer

    3. const pointer to const data --> Leads to compilation error
    const int* const constPtrToConstData = &data; // const pointer to const data
    // constPtrToConstData = &data; // Error: assignment of read-only variable 'constPtrToConstData'
    // *constPtrToConstData = 10; // Error: assignment of read-only location    

    4. const

    return 0;
}