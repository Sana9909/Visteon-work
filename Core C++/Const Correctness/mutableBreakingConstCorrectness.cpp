#include <iostream>
#include <vector>

using namespace std;

class A {
    public:
    mutable int cache;
    void update() const {
        cache = 10;   // Allowed
    }
};

int main(){
    A* a=new A();
    a->update();
    cout<<a->cache<<endl;
    return 0;
}