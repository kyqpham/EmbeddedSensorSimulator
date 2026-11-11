#pragma once
#include <iostream>
#include <string>
#include <memory>
#include <array>

class RingBuffer {
    public:
        // how many readings are stored
        int size() const {
            return count;
        }

        // true if nothing is stored
        bool empty() const {
            return count == 0;
        }

        // true if every slot is used
        bool full() const{
            return count == capacity;
        }

        void push(double value) {
            int slot;
            if (!full()) {
                slot = (head + count) % capacity;
                data[slot] = value;
                count++;
            } 
            else{
                data[head] = value;
                head = (head + 1) % capacity;
            }
        }

        //prints each slow in array order
        // plus head + count for debugging
        void printRaw() const {
            for (double x : data) {
                std::cout << x << " ";
            }
            std::cout << "| head =" << head << " count=" << count << std::endl;
        }

        // prints readings from oldest to newest
        void print() const {
            for (int i = 0; i < count; i++) {
                int slot = (head + i) % capacity;
                std::cout << data[slot] << " ";
            }
            std::cout << std::endl;
        }

    private:
        // seting the number of slots
        static constexpr int capacity = 4;
        std::array<double, capacity> data {};

        // slot holding the oldest reading
        int head = 0;
        
        // counting how many slots are filled
        int count = 0;
};
// creating a parent class, sensor
class Sensor {
    public:
        // declaring static function
        // static functiion implemented later
        // helps create a list of sensors
        // with input text, not the class name itself
        // ie: a factory
        static std::unique_ptr<Sensor> create (const std:: string& type);
        inline static int count = 0;

        std::string category() const {
            return "sensor";
        }

        // virtual method to allow override
        // with a body, the class isn't abstract
        // this makes an abstract class

        // subclasses must implement their
        // own version.
        virtual double read() = 0;
        virtual std::string name() const = 0;
    
        // adding the constructor
        Sensor() {
            count ++;
        }

        // creating a destructor
        virtual ~Sensor() {
            std::cout << "Sensor destroyed" << std::endl;
            count --;
        }

        // takes one reading + stores it into sensor's logs
        void sample() {
            log.push(read());
        }

        // print the sensor's name + logged readings
        void printLog() const {
            std::cout << name() << ": ";
            log.print();
        }
        

    private:
        // each sensor gets their own log
        // to distinguish readings from each sensor
        RingBuffer log;
};

// creating subclass, thermometer
class Thermometer : public Sensor {
    public:
        std::string name() const override {
            return "thermometer";
        }

        // overriding the parent read() method
        double read() override {
            return temp += 0.5;
        }

        void describe() {
            std::cout << name() << " reads " << read() << std::endl;
            std::cout << category() << std::endl; 
            std::cout << read() << std::endl;
        }
        // adding a destructor
        // note that activating a subclass destructors
        // also activates the parent class destructor
        ~Thermometer() {
            std::cout << "Thermometer destroyed" << std::endl;
        }

    private:
        // current fake temperature
        double temp = 20.0;
};

class PressureSensor : public Sensor {
    public:
        std::string name() const override {
            return "pressure";
        }

        double read() override {
            return 101.3;
        }
    
    ~PressureSensor() {
        std::cout << "PressureSensor destroyed" << std::endl;
    }
};

class IMUSensor : public Sensor {
    public:
        std::string name() const override {
            return "imu";
        }

        double read() override {
            return 9.8;
        }
    
    ~IMUSensor() {
        std::cout << "IMUSensor destroyed" << std::endl;
    }
};

void report (Sensor& S) {
    std::cout << S.name() << ": " << S.read() << std::endl;
}

// implementing a static function declared in Sensor
inline std::unique_ptr<Sensor> Sensor::create(const std:: string& type) {
    // creating the mappings as arg types
    if (type == "temp") {
        return std::make_unique<Thermometer>();
    }
    else if (type == "pressure") {
        return std::make_unique<PressureSensor>();
    }
    else if (type == "imu") {
        return std::make_unique<IMUSensor>();
    }
    else {
        return nullptr;
    }
}
