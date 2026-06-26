#include <iostream>
#include <memory>
using namespace std;

int main()
{
    unique_ptr<int> p1 = make_unique<int>(10);
    // unique_ptr<int> p2 = p1; // ERROR: copy not allowed
    unique_ptr<int> p2 = move(p1);

    if (!p1)
        cout << "p1 is now NULL\n";

    cout << "p2 value = " << *p2 << endl;
}