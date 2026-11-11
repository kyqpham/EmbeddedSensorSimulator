#include <iostream>
#include <thread>
#include <mutex>
#include <queue>

// one sensor reading: which sensor, and what value
// recall that structs are similar to classes
// structs have default public access to members
// classes have default private access to members

struct Reading {
    int sensorId;
    double value;
};

// shared queue + corresponding mutex between threads
std::queue<Reading> readings;
std::mutex queueLock;

void producer(int id, double value) {
    for (int i = 0; i < 5; i++) {
        std::lock_guard<std::mutex> guard(queueLock);
        readings.push(Reading{id, value + i});
    }
}

void consumer(int total) {
    int consumed = 0;

    while (consumed < total) {
        std::lock_guard<std::mutex> guard(queueLock);
        if (!readings.empty()) {
            Reading r = readings.front();
            readings.pop();
            
            std::cout << "sensor " << r.sensorId << ": " << r.value << std::endl;
            consumed++;
        }
    }
}
int main() {
    // producer threads, one per sensor
    std::thread p1(producer, 1, 21.5);
    std::thread p2(producer, 2, 101.3);
    std::thread p3(producer, 3, 9.8);

    // one consumer thread
    // three sensors, five readings = 15
    std::thread c(consumer, 15);

    // contineu when all threads are finished
    p1.join();
    p2.join();
    p3.join();
    c.join();

    std::cout << "queue size: " << readings.size() << std::endl;
    return 0;
}