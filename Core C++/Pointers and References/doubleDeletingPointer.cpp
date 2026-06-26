#include <iostream>

using namespace std;

int main() {
    int* doubleDeletePtr = new int(10);
    delete doubleDeletePtr;
    delete doubleDeletePtr; // Error: double free or corruption
}
