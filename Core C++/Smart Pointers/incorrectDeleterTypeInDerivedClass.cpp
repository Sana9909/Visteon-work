#include <iostream>
#include <memory>
using namespace std;

class Base
{
public:
    Base()
    {
        cout << "Base constructor\n";
    }

    // FIX: virtual destructor
    // ~Base() // Wrong: Non-virtual destructor
    virtual ~Base() //Correct: Virtual destructor
    {
        cout << "Base destructor\n";
    }
};

class Derived : public Base
{
private:
    int* data;

public:
    Derived()
    {
        data = new int[5];
        cout << "Derived constructor (allocated memory)\n";
    }

    ~Derived()
    {
        delete[] data;
        cout << "Derived destructor (freed memory)\n";
    }
};

int main()
{
    cout << "Creating shared_ptr<Base> pointing to Derived\n";

    shared_ptr<Base> ptr = make_shared<Derived>();

    cout << "Exiting main...\n";
}