#include <iostream>
#include <thread>

void task() {
    std::cout << "Hello from worker thread\n";
}

int main() {
    std::thread t(task);

    std::cout << "Hello from main thread\n";

    t.join();
    return 0;
}