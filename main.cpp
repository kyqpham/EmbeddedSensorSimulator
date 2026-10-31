#include <iostream>
#include <string>

// creating a parent class, sensor
class Sensor {
    public:
        std::string category() const {
            return "sensor";
        }

        // virtual method to allow override
        virtual double read() {
            return 0.0;
        }
};

// creating subclass, thermometer
class Thermometer : public Sensor {
    public:
        std::string name() const {
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
};

void report (Sensor& S) {
    std::cout << S.read() << std::endl;
}

int main() {
    Thermometer t;
    t.describe();
    report(t);

    return 0;
}