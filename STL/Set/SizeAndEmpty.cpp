#include <iostream>
#include <set>
using namespace std;

int main()
{
    set<int> s = {1,2,3};

    cout << "Size: " << s.size() << endl;

    if(s.empty())
        cout << "Set is empty";
    else
        cout << "Set is not empty";
}

//OUTPUT:
//Size: 3
//Set is not empty