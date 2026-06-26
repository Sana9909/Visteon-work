#include <iostream>
#include <vector>

using namespace std;

class B {
public:
    B(){
        cout<<"B Constructor"<<endl;
    }
    ~B(){
        cout<<"B Destructor"<<endl;
    }
};
class C { 
public:
    C(){
        cout<<"C Constructor"<<endl;
    }
    ~C(){
        cout<<"C Destructor"<<endl;
    }
};

class A {
public:
    A(){
        cout<<"A Constructor"<<endl;
    }
    ~A(){
        cout<<"A Destructor"<<endl;
    }
    B b; // Object b is instantiated when A is instantiated
    C c; // Object c is instantiated when A is instantiated
};

int main(){
    A* a=new A();
    delete a;
    return 0;
}


// OUTPUT:
// B Constructor
// C Constructor
// A Constructor
// A Destructor
// C Destructor
// B Destructor
