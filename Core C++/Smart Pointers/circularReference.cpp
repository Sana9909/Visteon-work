#include <iostream> 
#include <memory> 
using namespace std; 

struct B; // Forward declaration

struct A {
    shared_ptr<B> b;
    ~A() { cout << "A destroyed\n"; }
};

struct B {
    // shared_ptr<A> a; // This creates a circular reference
    weak_ptr<A> a; // Changed to weak_ptr
    ~B() { cout << "B destroyed\n"; }
};

int main() {
    shared_ptr<A> a = make_shared<A>();
    shared_ptr<B> b = make_shared<B>();
    
    a->b = b;
    b->a = a;
    
    cout << "End of main\n"; 
}
