#include <iostream>
using namespace std;

class Base {
public:
    // virtual void show() {       
    void show() {   // Missing virtual keyword, should be virtual void show()
        cout << "Base show function" << endl;
    }
};

class Derived : public Base {
public:
    void show() override {   // override is optional but good practice
        cout << "Derived show function" << endl;
    }
};

int main() {

    Base* ptr;
    Derived d;

    ptr = &d;

    ptr->show();   // Calls Base version

    return 0;
}