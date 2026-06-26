#include <iostream>
#include <thread>

thread_local int value = 0;

void task() {

    value++;
    std::cout << "Thread " << std::this_thread::get_id()<< " Value = "<< value << std::endl;

}

int main() {

    std::thread t1(task);
    std::thread t2(task);
    std::thread t3(task);

    t1.join();
    t2.join();
    t3.join();
}