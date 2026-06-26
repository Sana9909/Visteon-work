#include <iostream>
using namespace std;

class A {
public:
    void show() {
        cout << "A show()" << endl;
    }
};

class B {
public:
    void show() {
        cout << "B show()" << endl;
    }
};

class C : public A, public B {};

int main() {

    C obj;

    // obj.show(); // ERROR

    obj.A::show();
    obj.B::show();

    return 0;
}