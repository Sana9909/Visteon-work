#include <iostream>
#include <vector>

using namespace std;

void f(int* p) {
    cout << "Value pointed to: " << *p << endl;
}

int main(){
    int a=10;
    int* const p=&a;
    f(p);   // OK (top-level const ignored)
    return 0;
}