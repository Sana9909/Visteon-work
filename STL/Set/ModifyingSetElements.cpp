#include <iostream>
#include <set>
using namespace std;

int main()
{
    set<int> s = {1,2,3};

    auto it = s.begin();

    *it = 100;   // ERROR
}