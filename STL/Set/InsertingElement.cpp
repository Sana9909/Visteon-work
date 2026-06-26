#include<iostream>
#include<set>
using namespace std;

int main(){
    set<int> s;
    s.insert(10);
    s.insert(20);
    s.insert(30);
    s.insert(10); // duplicate element, will not be inserted
    
    for(int x : s)
        cout<<x<<" "; 

    return 0;
}

//OUTPUT:
//10 20 30 //Correct, duplicate element is not inserted