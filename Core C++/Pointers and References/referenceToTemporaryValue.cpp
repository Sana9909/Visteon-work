#include <iostream>

using namespace std;

int main() {
    int& ref = 5;  // Error (non-const lvalue ref cannot bind to rvalue)    
    return 0;
}
