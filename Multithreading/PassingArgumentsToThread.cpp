#include <iostream>
#include <thread>

void printSum(int a, int b) {
    std::cout << "Sum = " << a + b << std::endl;
}

int main() {

    std::thread t(printSum, 10, 20);

    t.join();
}