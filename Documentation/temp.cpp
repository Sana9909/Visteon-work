#include <iostream>

using namespace std;

void f(int* p);

int main() {
    int a=10;
    int* const p=&a;
    f(p);   // OK (top-level const ignored)  // low-level const
    return 0;
}