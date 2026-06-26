#include <iostream>
using namespace std;

class Base {
public:
    Base() {
        cout << "Base Constructor" << endl;
    }
    // ~Base() { // Non-virtual destructor -> leads to undefined behavior when deleting a Derived object through a Base pointer
    virtual ~Base() {
        cout << "Base Destructor" << endl;
    }
};

class Derived : public Base {
public:
    Derived() {
        cout << "Derived Constructor" << endl;
    }
    ~Derived() {
        cout << "Derived Destructor" << endl;
    }
};

int main() {
    Base* ptr = new Derived();
    delete ptr;  
    return 0;
}

// OUTPUT:
// Base Constructor
// Derived Constructor
// Derived Destructor
// Base Destructor