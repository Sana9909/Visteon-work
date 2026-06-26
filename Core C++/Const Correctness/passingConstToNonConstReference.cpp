#include <iostream>

using namespace std;

void func(int& x) { }

int main() {
    const int a = 5;
    func(a);   // ❌ Error: cannot bind non-const lvalue reference of type 'int&' to an rvalue of type 'int'
}
