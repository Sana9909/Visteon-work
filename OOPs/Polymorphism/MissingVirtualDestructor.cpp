#include <iostream>
using namespace std;

class Base {
public:
    Base() {
        cout << "Base Constructor" << endl;
    }

    // virtual ~Base() {
    ~Base() {   // Missing virtual keyword, should be virtual ~Base()
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

    delete ptr; // Only Base Destructor will be called, Derived Destructor will not be called

    return 0;
}

// OUTPUT:
// Base Constructor
// Derived Constructor
// Base Destructor