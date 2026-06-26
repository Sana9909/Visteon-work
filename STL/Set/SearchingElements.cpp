#include <iostream>
#include <set>
using namespace std;

int main()
{
    set<int> s = {1,2,3,4,5};

    auto it = s.find(3);

    if(it != s.end())
        cout << "Element found";
    else
        cout << "Element not found";
}

//OUTPUT:
//Element found