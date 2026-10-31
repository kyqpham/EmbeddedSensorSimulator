#include <iostream>
#include <string>

// creating a parent class, sensor
class Sensor {
    public:
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
    
        // creating a destructor
        virtual ~Sensor() {
            std::cout << "Sensor destroyed" << std::endl;
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
    ~Thermometer() {
        std::cout << "Thermometer destroyed" << std::endl;
    }
};

void report (Sensor& S) {
    std::cout << S.read() << std::endl;
}

int main() {
    Thermometer t;
    t.describe();
    report(t);

    Sensor* p = new Thermometer;
    delete p;

    return 0;
}