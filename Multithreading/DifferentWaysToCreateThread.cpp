#include <iostream>
#include <thread>

void functionThread() {
    std::cout << "Thread created using function\n";
}

class Functor {
public:
    void operator()() {
        std::cout << "Thread created using functor\n";
    }
};

int main() {

    std::thread t1(functionThread);

    std::thread t2([](){
        std::cout << "Thread created using lambda\n";
    });

    std::thread t3(Functor{});

    t1.join();
    t2.join();
    t3.join();
}