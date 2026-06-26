#include <iostream>
#include <vector>
using namespace std;

int main()
{
    vector<int> v = {1,2,3};

    cout << v[10];   // undefined behavior
}

//OUTPUT:
//Segmentation fault

//Correct way to check for out of range access

// #include <iostream>
// #include <vector>
// using namespace std;

// int main()
// {
//     vector<int> v = {1,2,3};
//     cout<<v.at(10);   // throws out_of_range exception
// }