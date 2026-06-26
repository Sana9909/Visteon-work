#include <iostream>

using namespace std;

int main() {
    int a = 5, b = 10;
    int& ref = a;
    ref = b;   
    cout<<&a<<" "<<&b<<" "<<&ref<<endl; //Output: 0x7ffd26a9c464 0x7ffd26a9c460 0x7ffd26a9c464
    // "reference reseating" refers to the act of trying to change a reference to point 
    //to a different object after it has already been initialized. This is not possible 
    //in C++; a reference is a permanent alias to the original object it was bound to.
    return 0;
}
