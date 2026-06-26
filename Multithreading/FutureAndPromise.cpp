#include <iostream>
#include <thread>
#include <future>

void calculate(std::promise<int> p) {

    int result = 10 + 20;
    std::this_thread::sleep_for(std::chrono::seconds(2));
    p.set_value(result);
}

int main() {
    std::cout<<"Main Thread"<<std::endl;
    std::promise<int> p;

    std::future<int> f = p.get_future();

    std::thread t(calculate, std::move(p));

    std::cout << "Result = " << f.get() << std::endl;

    t.join();
}