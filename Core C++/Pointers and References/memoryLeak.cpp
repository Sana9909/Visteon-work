#include <iostream>

using namespace std;

int main() {
    int* leakPtr = new int(5);
    // delete leakPtr; // This line is missing, causing a memory leak
}
