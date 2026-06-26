#include <iostream>
#include <memory>
using namespace std;

class Test : public enable_shared_from_this<Test> {
public:
    shared_ptr<Test> getPtr(){
        return shared_from_this();
    }
};

int main(){
    // Test* t = new Test;
    // try{
    //     auto p = t->getPtr();
    // }
    // catch(const bad_weak_ptr&){
    //     cout << "bad_weak_ptr exception\n";
    // }
    // delete t;

    shared_ptr<Test> t = make_shared<Test>();
    shared_ptr<Test> p = t->getPtr();
    cout << "use_count = " << p.use_count() << endl;
    
    return 0;
}