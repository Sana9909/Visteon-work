#include <iostream>
#include <memory>
using namespace std;

int main()
{
    int* raw = new int(100);

    shared_ptr<int> p1(raw);
    shared_ptr<int> p2(raw); // WRONG

    cout << "p1 use_count = " << p1.use_count() << endl;
    cout << "p2 use_count = " << p2.use_count() << endl;
}