#include <iostream>
#include <memory>
using namespace std;

int main()
{
    cout << "Creating original shared_ptr...\n";

    shared_ptr<int> sp1 = make_shared<int>(10);

    cout << "sp1 value = " << *sp1 << endl;
    cout << "sp1 use_count = " << sp1.use_count() << endl;

    cout << "\nCreating alias shared_ptr with NEW memory (WRONG)...\n";

    // shared_ptr<int> sp2(sp1, new int(20)); // MEMORY LEAK
    int num=20;
    shared_ptr<int> sp2(sp1, &num); // Correct way to use aliasing constructor with existing memory

    cout << "sp1 use_count after alias = " << sp1.use_count() << endl;
    cout << "sp2 use_count = " << sp2.use_count() << endl;

    cout << "sp1 points to = " << *sp1 << endl;
    cout << "sp2 points to = " << *sp2 << endl;

    cout << "\nResetting both pointers...\n";

    sp1.reset();
    sp2.reset();

    cout << "Program exiting...\n";
}