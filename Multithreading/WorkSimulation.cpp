#include <iostream>
#include <thread>
#include <chrono>

void worker() {
    std::cout << "Thread started\n";
    std::this_thread::sleep_for(std::chrono::seconds(2));
    std::cout << "Thread finished\n";
}

int main() {

    std::thread t(worker);

    if(t.joinable()) {
        t.join();
    }

    std::cout << "Main thread finished\n";
}