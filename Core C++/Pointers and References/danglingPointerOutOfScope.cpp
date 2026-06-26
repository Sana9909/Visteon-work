#include <iostream>

using namespace std;

int main() {
    int* danglingPtr=nullptr;
    cout<<"Dangling Ptr: "<<danglingPtr<<endl;
    {
        int localVar = 100;
        danglingPtr = &localVar; // danglingPtr points to localVar which goes out of scope after this block
    } 
    // cout<<"Local Var Address: "<<&localVar<<endl; // localVar doesn't exist anymore
    cout<<"Dangling Ptr: "<<danglingPtr<<endl; 
}
