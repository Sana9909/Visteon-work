#include <iostream>
#include <vector>

using namespace std;

int main(){
    // const vector<int> v = {1,2,3};
    // v.push_back(4);   // Error
    int a=0;
    const vector<int*> v={&a};
// v[0] = nullptr;  // ❌
    *v[0] = 10;      // ✅ if pointer not const
    cout<<v[0]<<endl;
    cout<<*v[0]<<endl;
    cout<<&a<<endl;
    cout<<a<<endl;
    return 0;
}