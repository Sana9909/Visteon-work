#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v;

    cout << v.front();   // error / undefined behavior
}

//OUTPUT:
//Segmentation fault 