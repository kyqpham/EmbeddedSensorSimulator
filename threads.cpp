#include <iostream>
#include <mutex>
#include <thread>

// a mutex makes one specific section of code
// run one thread at a time
// both threads will run at the same time
// but one at a time can enter through a piece of code
std::mutex printLock;
std::mutex counterLock;

void work(int id) {
    std::lock_guard<std::mutex> guard(printLock);
    std::cout << "hello from thread " << id <<  std::endl;
}

// test build to make race conditions
// without the mutex, race conditions will make
// output through this loop inaccurate

//test for a race conditoin: two threads each 
// increment the counter 100,000 times.
// the correct total is 200,000
// counter++ is read-modify-write
// without a mutex, both threads can read the same value
// before either writes,
// so updates get lost
// and the total come sout below 200,000
// differing run from run.
// a mutex makes the threads take turns on counter++
// so no update is lost
int counter = 0;
void increment() {
    for (int i = 0; i < 100000; i++) {
        std::lock_guard<std::mutex> guard(counterLock);
        counter++;
    }
}
int main() {    
    // starting two threads that both run increment
    std::thread t1(increment);
    std::thread t2(increment);

    // join method makes main wait until
    // that thread is finished.
    // essentially waiting until both threads
    // are finished before reading.
    t1.join();
    t2.join();

    std::cout << "counter: " << counter << std::endl;
    return 0;
}
