#include <iostream>
using namespace std;

class Base {
public:
    virtual void show() {
        cout << "Base show" << endl;
    }
};

class Derived : public Base {
public:
    void show() {
        cout << "Derived show" << endl;
    }
};

int main() {

    Derived d;
    Base b;

    b = d;   // Object slicing

    b.show();

    //CORRECT CODE
    // Derived d;

    // Base* ptr = &d;

    // ptr->show();

    return 0;
}

// OUTPUT:
// Base show