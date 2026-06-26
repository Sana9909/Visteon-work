#include <iostream>
using namespace std;

class Base {
public:
    virtual void show(int x) {
        cout << "Base show with int: " << x << endl;
    }
};

class Derived : public Base {
public:
    void show() {
        cout << "Derived show without parameter" << endl;
    }
};

int main() {

    Base* ptr;
    Derived d;

    ptr = &d;

    ptr->show(10);

    return 0;
}

// OUTPUT:
// Base show with int: 10