#include <iostream>
#include <memory>
using namespace std;

struct MyData
{
    int a;
    int b;

    MyData(int x, int y) : a(x), b(y)
    {
        cout << "MyData constructor\n";
    }

    ~MyData()
    {
        cout << "MyData destructor\n";
    }
};

int main()
{
    shared_ptr<MyData> sp1 = make_shared<MyData>(10, 20);

    // Alias pointer to member 'b'
    shared_ptr<int> sp2(sp1, &sp1->b);

    cout << "sp1 use_count = " << sp1.use_count() << endl;
    cout << "sp2 use_count = " << sp2.use_count() << endl;

    cout << "Value of a = " << sp1->a << endl;
    cout << "Value of b via alias = " << *sp2 << endl;

    cout << "Resetting sp1...\n";
    sp1.reset();

    cout << "sp2 still valid = " << *sp2 << endl;

    cout << "Resetting sp2...\n";
    sp2.reset();

    cout << "Program exiting...\n";
}