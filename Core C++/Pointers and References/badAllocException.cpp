#include <iostream>

using namespace std;

int main() {
    int* ptr = new int[1000000000000000];  //throws std::bad_alloc 
}
