#include <iostream>

using namespace std;

int main() {
    // 1. Dereferencing a NULL pointer --> Leads to segmentation fault
    // int* ptr = nullptr;
    // cout << "Dereferencing NULL pointer: " << *ptr << endl;

    // 2. Modifying a const pointer --> Leads to compilation error
    // const int* constPtr = new int(10);
    // *constPtr = 20; // Error: assignment of read-only location

    // 3. Returning a reference to a local variable --> Leads to undefined behavior
    // int& getLocalRef() {
    //     int localVar = 42;
    //     return localVar; // Warning: reference to local variable 'localVar' returned
    // }
    // int& ref = getLocalRef();

    // 4. Using an uninitialized pointer --> Leads to undefined behavior
    // int* uninitPtr; // Uninitialized pointer
    // cout << "Dereferencing uninitialized pointer: " << *uninitPtr << endl;

    // 5. Memory leak due to not deleting dynamically allocated memory
    // int* leakPtr = new int(5);
    // delete leakPtr; // This line is missing, causing a memory leak

    // 6. Double deletion of a pointer --> Leads to undefined behavior
    // int* doubleDeletePtr = new int(10);
    // delete doubleDeletePtr;
    // delete doubleDeletePtr; // Error: double free or corruption

    // 7. Accessing out-of-bounds array elements --> Leads to undefined behavior
    // int arr[5] = {1, 2, 3, 4, 5};
    // cout << "Accessing out-of-bounds element: " << arr[10] << endl; // Warning: array subscript is above array bounds

    // 8. Using a dangling pointer --> Leads to undefined behavior
    // int* danglingPtr;
    // {
    //     int localVar = 100;
    //     danglingPtr = &localVar; // danglingPtr points to localVar which goes out of scope after this block
    // }    

    // 9. Modifying a string literal through a pointer --> Leads to undefined behavior
    // char* strLiteral = "Hello"; // String literals are stored in read-only memory
    // strLiteral[0] = 'h'; // Error: assignment of read-only location
    
    // 10. Using a pointer after it has been deleted --> Leads to undefined behavior
    // int* deletedPtr = new int(50);
    // delete deletedPtr;
    // cout << "Using pointer after deletion: " << *deletedPtr << endl; //
    
    // 11. Returning a pointer to a local variable --> Leads to undefined behavior
    // int* getLocalPtr() {
    //     int localVar = 42;
    //     return &localVar; // Warning: address of local variable 'localVar'
    // }
    // int* ptr = getLocalPtr();

    // 12. Using a pointer to a deleted object --> Leads to undefined behavior
    // int* deletedObjPtr = new int(30);
    // delete deletedObjPtr;
    // cout << "Using pointer to deleted object: " << *deletedObjPtr << endl; // Warning: dereferencing pointer to deleted object

    // 13. Modifying a pointer to const data --> Leads to compilation error
    // const int* ptrToConst = new int(10);
    // ptrToConst = new int(20); // Error: assignment of read-only variable 'ptrToConst'

    // 14. Using a pointer to a non-static local variable in a lambda function --> Leads to undefined behavior
    // auto lambda = []() {
    //     int localVar = 50;
    //     int* ptr = &localVar; // Warning: address of local variable 'localVar' returned
    //     return *ptr; // Warning: dereferencing pointer to local variable 'localVar'
    // };

    // 15. Using a pointer to a temporary object --> Leads to undefined behavior
    // int* tempPtr = new int(10);
    // cout << "Using pointer to temporary object: " << *tempPtr << endl; // Warning: dereferencing pointer to temporary object
    // delete tempPtr;

    // 16. Using a pointer to a deleted array --> Leads to undefined behavior
    // int* deletedArrayPtr = new int[5];
    // delete[] deletedArrayPtr;
    // cout << "Using pointer to deleted array: " << deletedArrayPtr[0] << endl; // Warning: dereferencing pointer to deleted array

    return 0;
}