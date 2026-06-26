#include <iostream>

using namespace std;

class A {
public:
    void modify() { } //error: passing 'const A' as 'this' argument discards qualifiers [-fpermissive]
    // void modify() const { } // Correct implementation
};

int main() {
    const A obj;
    obj.modify();   // Error
    return 0;
}
