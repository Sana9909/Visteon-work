#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {10,20,30};

    auto it = v.end();
    //--it; // Uncommenting this line will make it valid

    cout << *it << endl;   // Undefined behavior
}