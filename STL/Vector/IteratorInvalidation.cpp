#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {1,2,3,4};

    auto it = v.begin();   // iterator to first element
    cout << *it << endl;   

    v.push_back(5);        // may reallocate memory

    cout << *it << endl;   // Undefined behavior (iterator invalid)
}

//OUTPUT:
//1
// 196930

//Only access the iterator after modification