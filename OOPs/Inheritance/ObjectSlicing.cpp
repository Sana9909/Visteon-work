#include <iostream>
using namespace std;

class Base {
public:
    // void show() { // This would cause slicing if we were to assign a Derived object to a Base object
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
    void call(){
        Base::show(); // Call the Base class version of show()
    }
};

int main() {
    Derived d;
    Base b;
    Base* ptr = &d;  // pointer avoids slicing
    ptr->show(); // Calls Derived's show() due to virtual function
    ((Base *)ptr)->show(); // Still calls Derived's show() because of virtual function
    (&b)->show(); // Calls Base's show()
    (&d)->call(); // Calls Derived's call(), which in turn calls Base's show()

    return 0;
}