#include <iostream>
#include <stdexcept>
using namespace std;

class Test {
public:
    ~Test() {
        cout << "Destructor called\n";
        throw runtime_error("Error in destructor");
    }
};

int main()
{
    Test obj;
}