#include<iostream>
#include<memory>

using namespace std;

int main(){
    weak_ptr<int> wp;
    shared_ptr<int> sp = make_shared<int>(100);
    wp = sp;
    // cout<<"Weak Ptr : "<<*wp<<endl; // Undefined Behavior: dereferencing without lock() can lead to accessing a destroyed object
    cout<<"Weak Ptr : "<<*wp.lock()<<endl; // Correct way to access weak_ptr value
    return 0;
}