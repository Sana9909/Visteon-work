#include <iostream>
using namespace std;

class Base {
public:
    // void show() { // no overriding possible without virtual function, leads to Base version being called even for Derived objects
    virtual void show() {
        cout << "Base show()" << endl;
    }
};

class Derived : public Base {
public:
    // void show() { // This would cause slicing if we were to assign a Derived object to a Base object
    void show() override {
        cout << "Derived show()" << endl;
    }
};

int main() {

    Base* ptr = new Derived();

    ptr->show();   // Base version called when show() is not virtual, leads to object slicing

    return 0;
}

// OUTPUT:
// Derived show()