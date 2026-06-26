#include <iostream>
#include <array>

using namespace std;

int main()
{
    array<int,3> a = {1,2,3};
    array<int,3> b = {10,20,30};

    a.swap(b);

    cout<<"Array a:\n";
    for(int x : a)
        cout<<x<<" ";
}

//OUTPUT:
//Array a:
//10 20 30