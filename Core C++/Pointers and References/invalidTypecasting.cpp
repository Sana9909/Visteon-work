#include <iostream>

using namespace std;

int main() {
    int x = 10;
    string* p = (string*)&x;  
    cout<<x<<" "<<&x<<endl;
    cout<<*p<<" "<<p<<endl; //Segmentation fault
    return 0;
}
