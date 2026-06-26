#include <iostream>

using namespace std;

int* getLocalPtr() {
    int localVar = 42;
    return &localVar; // Warning: address of local variable 'localVar'
}

int main() {
    int* ptr = getLocalPtr();
    cout<<"Local pointer: "<<ptr<<endl;
}
