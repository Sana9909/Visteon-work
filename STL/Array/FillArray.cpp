#include <iostream>
#include <array>

using namespace std;

int main()
{
    array<int,5> arr;

    arr.fill(7);

    for(int x : arr)
        cout<<x<<" ";
}