#include <iostream>

using namespace std;

void func(const int& x) {
    x = 10;  // Compile error
}

int main() {
    const int a=5;
    func(a);   // error: assignment of read-only reference 'x'
    return 0;
}
