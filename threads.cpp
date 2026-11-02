#include <iostream>
#include <mutex>
#include <thread>

std:: mutex printLock;
void work(int id) {
    std::lock_guard<std::mutex> guard(printLock);
    std::cout << "hello from thread " << id <<  std::endl;
}

int main() {    
    // adding a lock guard mutex
    // starting a thread running work(1)
    std::thread t1(work, 1);

    // starting another thread to run work(2)
    std::thread t2(work, 2);
    
    // waiting for t1 to finish
    t1.join();
    // waiting for t2 to finish
    t2.join();
    
    return 0;
}
