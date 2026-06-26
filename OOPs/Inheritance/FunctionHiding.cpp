#include <iostream>
using namespace std;

class Base {
public:
    void display(int x){
        cout << "Base display int: " << x << endl;
    }
};

class Derived : public Base {
public:
    using Base::display; // This brings Base's display(int) into scope, preventing it from being hidden by Derived's display()
    void display() {
        cout << "Derived display()" << endl;
    }
};

int main() {
    Derived d;
    d.display(10);  // Calls Base's display(int) due to using declaration
    d.display();

    return 0;
}

// OUTPUT:
// Base display int: 10 
// Derived display()