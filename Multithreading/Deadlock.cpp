#include <iostream>
#include <thread>
#include <mutex>

std::mutex m1;
std::mutex m2;

void task1() {

    m1.lock();
    std::cout << "Task1 locked m1\n";

    m2.lock();
    std::cout << "Task1 locked m2\n";

    m2.unlock();
    m1.unlock();
}

void task2() {

    m2.lock();
    std::cout << "Task2 locked m2\n";

    m1.lock();
    std::cout << "Task2 locked m1\n";

    m1.unlock();
    m2.unlock();
}

int main() {

    std::thread t1(task1);
    std::thread t2(task2);

    t1.join();
    t2.join();
}