#include <iostream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <chrono>
#include <condition_variable>
#include "sensor.h"
#include <vector>

// one sensor reading: which sensor, and what value
// recall that structs are similar to classes
// structs have default public access to members
// classes have default private access to members

struct Reading {
    int sensorId;
    double value;
};

// exactly one logger exists, shared by every thread
// records what's happening while it runs
class Logger{ 
    public:
    // returns the single logger
    static Logger& instance() {
        static Logger logger;
        return logger;
    }

    // making it thread-safe
    // prints one message at a time
    void log(const std::string& msg) {
        std::lock_guard<std::mutex> guard(lock);
        std::cout << msg << std::endl;
    }

    // private constructor
    // nothing else can make a logger
    private:
        Logger() {}
        std::mutex lock;
};

// shared queue + corresponding mutex between threads
// queues are the middle man between producers and consumers
// holds readings that haven't been processed
std::queue<Reading> readings;
std::mutex queueLock;

// producers create data
void producer(Sensor* s, int id) {
    for (int i = 0; i < 5; i++) {
        // adding parentheses to release lock
        {
            // generating + reading in data
            std::lock_guard<std::mutex> guard(queueLock);
            readings.push(Reading{id, s->read() + i});
        }
    
    // sleeps for 10 ms betewen readings
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    }
}

// adding in to flag outliers
bool isOutlier (const Reading& r) {
    if (r.sensorId == 1) {
        return r.value < 20.0 || r.value > 24.0;
    }
    if (r.sensorId == 2) {
        return r.value < 100.0 || r.value > 104.0;
    }
    if (r.sensorId == 3) {
        return r.value < 9.0 || r.value > 12.0;
    }
    else { 
        return false;
    }
}
// consumers use the data
// busy-waits: keeps checking the queue even if empty
void consumer(int total) {
    int consumed = 0;
    int checks = 0;
    int outliers = 0;

    while (consumed < total) {
        // checks every pass, instantly
        checks++;
        std::lock_guard<std::mutex> guard(queueLock);
        if (!readings.empty()) {
            Reading r = readings.front();
            readings.pop();

            if (isOutlier(r)) {
                Logger::instance().log("WARNING: sensor " + std::to_string(r.sensorId) + " out of range: " 
                + std::to_string(r.value));
                outliers++;
            }
            else {
                Logger::instance().log("sensor " + std::to_string(r.sensorId) + ": " + std::to_string(r.value));
            }
            consumed++;
        }
    }
    std::cout << "consumer checked " << checks << " times" << std::endl;
    std::cout << "outliers flagged: " << outliers << std::endl;
}
int main() {
    // producer threads, one per sensor
    std::vector<std::unique_ptr<Sensor>> sensors;
    sensors.push_back(Sensor::create("temp"));
    sensors.push_back(Sensor::create("pressure"));
    sensors.push_back(Sensor::create("imu"));

    std::thread p1(producer, sensors[0].get(), 1);
    std::thread p2(producer, sensors[1].get(), 2);
    std::thread p3(producer, sensors[2].get(), 3);

    // one consumer thread
    // three sensors, five readings = 15
    // if only read 10 times, 5 would be left in queue
    std::thread c(consumer, 15);

    // continue when all threads are finished
    p1.join();
    p2.join();
    p3.join();
    c.join();

    std::cout << "queue size: " << readings.size() << std::endl;
    return 0;
}