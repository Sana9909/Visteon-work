#include <iostream>
#include <array>

using namespace std;

int main()
{
    array<int,4> arr = {1,2,3,4};

    cout<<"Size: "<<arr.size()<<endl;

    if(arr.empty())
        cout<<"Array is empty";
    else
        cout<<"Array is not empty";
}