#include <iostream>
#include <thread>
#include <mutex>
#include <condition_variable>

std::mutex mtx;
std::condition_variable cv;
bool ready = false;

void worker() {

    std::unique_lock<std::mutex> lock(mtx);

    cv.wait(lock, []{ return ready; });

    std::cout << "Worker thread running\n";
}

void signal() {

    std::lock_guard<std::mutex> lock(mtx);

    ready = true;

    cv.notify_one();
}

int main() {

    std::thread t(worker);

    std::this_thread::sleep_for(std::chrono::seconds(2));

    signal();

    t.join();
}