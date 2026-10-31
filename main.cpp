#include <iostream>
#include <string>
#include <vector>
#include <memory>

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
};

// creating subclass, thermometer
class Thermometer : public Sensor {
    public:
        std::string name() const override {
            return "thermometer";
        }

        // overriding the parent read() method
        double read() override {
            return 21.5;
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
std::unique_ptr<Sensor> Sensor::create(const std:: string& type) {
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

int main() {
    std::cout << "count: " << Sensor::count << std::endl;

    // creating a list of sensors using unique pointers
    // unique pointers delete the pointed object when its deleted
    // unique = one object at a time, no copies
    std::vector<std::unique_ptr<Sensor>> sensors;
    sensors.push_back(Sensor::create("temp"));
    sensors.push_back(Sensor::create("pressure"));
    sensors.push_back(Sensor::create("imu"));

    std::cout << "count: " << Sensor::count << std::endl;

    // because unique pointers only have one, & operator needed
    // have to use -> instead of dot notation
    // because of the pointers
    for (const auto& s: sensors){
        std::cout << s->name() << ": " << s->read() << std::endl;
    }
    return 0;
}