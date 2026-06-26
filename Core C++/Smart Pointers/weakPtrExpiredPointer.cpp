#include <iostream>
#include <memory>
using namespace std;

int main(){
    weak_ptr<int> wp;
    
    {
        shared_ptr<int> sp = make_shared<int>(500);
        wp = sp;
        cout << "Inside block: " << *wp.lock() << endl;
        cout << "use_count: " << wp.use_count() << endl;
    }

    if (wp.expired())
        cout << "Pointer expired\n";
}