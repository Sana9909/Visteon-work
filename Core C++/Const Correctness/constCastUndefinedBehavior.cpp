#include <iostream>
#include <vector>

using namespace std;

int main(){
    const int x = 10;
    int* p = const_cast<int*>(&x);
    *p = 20;   // Undefined Behavior
    cout<<p<<" "<<*p<<endl;
    cout<<&x<<" "<<x<<endl; //Same address different values
    return 0;
}