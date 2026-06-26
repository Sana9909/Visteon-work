#include <iostream>
#include <vector>

using namespace std;

class A {
public:
    void show() { cout << "Non-const"<<endl; }
    void show() const { cout << "Const"<<endl; }
};

int main(){
    A a;
    const A b;
    
    a.show();  // Calls non-const
    b.show();  // Calls const
    return 0;
}