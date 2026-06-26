#include <iostream>
#include <vector>
using namespace std;

int main() {
    vector<int> v = {1,2,3,4,5};

    for(auto it = v.begin(); it != v.end(); )
    {
        if(*it % 2 == 0)
            it = v.erase(it);
        // else
        //     ++it; // Uncommenting this line will make it valid
    }

    for(int x : v)
        cout << x << " ";

    return 0;
}