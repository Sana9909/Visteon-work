#include <iostream>
#include <memory>
using namespace std;

int main()
{
    weak_ptr<int> wp;
    shared_ptr<int> sp1 = make_shared<int>(42);
    weak_ptr<int> wp = sp1;
    
    if (auto sp1 = wp.lock()) {   // lock returns shared_ptr or empty
        cout << "Value: " << *sp1 << endl;
    } else {
        cout << "Object no longer exists" << endl;
    }
    // try
    // {
    //     shared_ptr<int> sp(wp); // throws exception
    // }
    // catch (const bad_weak_ptr& e)
    // {
    //     cout << "Exception caught: bad_weak_ptr\n";
    // }
}