#include <iostream>
using namespace std;

class A {
public:
    int x = 5;
    A(){
        cout<<"Constructor A"<<endl;
    }
    ~A(){
        cout<<"Destructor A"<<endl;
    }
};

// class B : public A {};
// class C : public A {}; // This will cause the "Diamond Problem" because both B and C inherit from A, and D inherits from both B and C.
// To resolve the "Diamond Problem", we can use virtual inheritance.

class B :virtual public A {
    public:
    B(){
        cout<<"Constructor B"<<endl;
        x=10;
    }
    ~B(){
        cout<<"Destructor B"<<endl;
    }
};

class C :virtual public A {
    public:
    C(){
        cout<<"Constructor C"<<endl;
        x=15;
    }
    ~C(){
        cout<<"Destructor C"<<endl;
    }
};

class D : public C, public B {
    public:
    D(){
        cout<<"Constructor D"<<endl;
    }
    ~D(){
        cout<<"Destructor D"<<endl;
    }
    
};

int main() {

    D obj;

    // cout << obj.x;  // ERROR: ambiguous

    cout << "B : "<< obj.B::x << endl;
    cout <<"C : " << obj.C::x << endl;

    return 0;
}

// OUTPUT:
// Constructor A
// Constructor C
// Constructor B
// Constructor D
// B : 10
// C : 10
// Destructor D
// Destructor B
// Destructor C
// Destructor A

