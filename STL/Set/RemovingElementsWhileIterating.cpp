#include <iostream>
#include <set>
using namespace std;

int main()
{
    set<int> s = {1,2,3,4};

    for(auto it = s.begin(); it != s.end(); it++)
    {
        if(*it == 2)
            s.erase(it);   // invalid iterator
    }
}