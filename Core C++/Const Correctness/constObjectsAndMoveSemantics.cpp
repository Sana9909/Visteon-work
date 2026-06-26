#include <iostream>
#include <vector>

using namespace std;

string f() {
    const string s = "hello";
    return s;   // ❌ Move won't happen
}

int main(){
    string str=f();
    cout<<str<<endl;
    return 0;
}