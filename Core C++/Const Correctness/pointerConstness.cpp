#include <iostream>

using namespace std;

int main() {
    int x = 10;

    // Case 1: Pointer to const
    const int* p;
    // Cannot modify value, Can change pointer
    
    // Case 2: Const pointer
    int* const p = &x;
    // Can modify value, Cannot change pointer
    
    // Case 3: Const pointer to const
    const int* const p= &x;
    
    return 0;
}
