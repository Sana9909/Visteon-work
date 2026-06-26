#include <iostream>

using namespace std;

int main() {
    const int constVar = 10;
    constVar = 20; // Error: assignment of read-only variable 'constVar'
    return 0;
}
