#include <iostream>

using namespace std;

int main() {
    int arr[5] = {1, 2, 3, 4, 5};
    int *ptr=&arr[0];
    for(int i=0;i<5;i++){
        cout << "Value at index " << i << " is: " << *(ptr+i) << endl;
    }
    return 0;
    // cout << "Accessing out-of-bounds element: " << arr[10] << endl; // Warning: array subscript is above array bounds
}
