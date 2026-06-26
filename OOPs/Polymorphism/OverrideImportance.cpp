#include <iostream>
using namespace std;

class Base {
public:
    virtual void show(int x) {
        cout << "Base show: " << x << endl;
    }
};

class Derived : public Base {
public:
    void show(double x) override {   // Mistake! Different signature, compiler error due to signature mismatch, should be void show(int x) override
        cout << "Derived show: " << x << endl;
    }
};

int main() {
    Base* ptr = new Derived();
    ptr->show(5);
    return 0;
}