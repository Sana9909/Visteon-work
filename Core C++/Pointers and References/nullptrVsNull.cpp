#include <iostream>

using namespace std;

void func(int* p){
    cout << "Pointer overload called with: " << p << endl;
}
void func(int x){
    cout << "Integer overload called with: " << x << endl;
}

int main(){
    func(NULL);  // May call wrong overload
    func(nullptr);  // Correct pointer overload
    return 0;
}