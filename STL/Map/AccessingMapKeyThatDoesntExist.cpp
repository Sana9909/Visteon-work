#include <iostream>
#include <map>
using namespace std;

int main() {
    map<int,string> m;

    cout << m[10] << endl;  // key 10 is automatically created

    cout << "Size: " << m.size() << endl;
}

//OUTPUT:
// (empty string)
//Size: 1

//Correct

// #include <iostream>
// #include <map>
// using namespace std;

// int main() {
//     map<int,string> m;

//     if(m.find(10) == m.end())
//         cout << "Key not found\n";
// }