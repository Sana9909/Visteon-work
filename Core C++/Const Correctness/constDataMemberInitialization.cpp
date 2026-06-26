#include <iostream>

using namespace std;

class A {
    const int x;
public:
    // A() { x = 5; }  // Error
    A() : x(5) { }  // Correct
};

int main(){
    return 0;
}