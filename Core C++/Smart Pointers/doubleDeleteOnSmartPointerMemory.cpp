#include <iostream>
#include <memory>
using namespace std;

int main()
{
    shared_ptr<int> sp = make_shared<int>(10);

    int* raw = sp.get();

    cout << "Value = " << *raw << endl;

    // delete raw;  // NEVER DO THIS
}