#include <iostream>
#include <thread>
#include <mutex>

void worker(){
    std::cout << "Thread ID: "<< std::this_thread::get_id() << std::endl;
}

int main() {
    std::thread t1(worker);
    std::thread t2(worker);
    t1.join();
    t2.join();
}