#include <iostream>
using namespace std;

class Base {
public:
    virtual void show() = 0;
};

class Derived : public Base {
    // public:
    // void show() override {
    //     cout << "Derived show()" << endl;
    // }    // Missing implementation of pure virtual function show() in Derived class, should be implemented to avoid compilation error
};

int main() {

    Derived d;   // ERROR
    d.show();
    return 0;
}