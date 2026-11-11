#include <iostream>
#include <string>
#include <vector>
#include <memory>
#include <array>
#include "sensor.h"

int main() {
    std::cout << "count: " << Sensor::count << std::endl;

    // creating a list of sensors using unique pointers
    // unique pointers delete the pointed object when its deleted
    // unique = one object at a time, no copies
    std::vector<std::unique_ptr<Sensor>> sensors;
    sensors.push_back(Sensor::create("temp"));
    sensors.push_back(Sensor::create("pressure"));
    sensors.push_back(Sensor::create("imu"));

    // takes six rounds of readings
    for (int round = 0; round < 6; round++) {
        for (const auto& s : sensors) {
            s->sample();
        }
    }

    // prints each sensor's log
    for (const auto& s : sensors) {
        s->printLog();
    }
    
    std::cout << "count: " << Sensor::count << std::endl;   

    return 0;
}