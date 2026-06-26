#include<iostream>
#include<memory>

using namespace std;

int main(){
    shared_ptr<int> sp = make_shared<int>(5);
    weak_ptr<int> wp = sp;
    
    sp.reset();
    
    auto locked = wp.lock(); // returns nullptr
    cout<<"weak_ptr: "<<locked<<endl; // Ouput : `weak_ptr: 0` (nullptr)
    return 0;
}