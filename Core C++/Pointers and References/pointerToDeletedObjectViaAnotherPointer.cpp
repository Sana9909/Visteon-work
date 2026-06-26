#include <iostream>

using namespace std;

int main() {
    int* p1 = new int(5);
    int* p2 = p1;
    cout << *p1<<" "<<p1<<endl;
    delete p1;     // It does not destroy the pointer variable itself, but rather the object or data it points to in the heap memory
    cout << *p2<<" "<<p2<<endl;  // Dangling pointer
}
