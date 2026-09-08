#ifndef PERSEUS_TEST_ARDUINO_H
#define PERSEUS_TEST_ARDUINO_H

#include <cstdint>
#include <sstream>

constexpr uint8_t INPUT = 0x01;
constexpr uint8_t OUTPUT = 0x03;
constexpr uint8_t LOW = 0;
constexpr uint8_t HIGH = 1;
enum adc_attenuation_t { ADC_0db, ADC_2_5db, ADC_6db, ADC_11db };

void pinMode(uint8_t pin, uint8_t mode);
void digitalWrite(uint8_t pin, uint8_t value);
bool digitalPinCanOutput(uint8_t pin);
bool digitalPinIsValid(uint8_t pin);
int8_t digitalPinToAnalogChannel(uint8_t pin);
void delayMicroseconds(uint32_t us);
void delay(uint32_t ms);
uint16_t analogRead(uint8_t pin);
void analogReadResolution(uint8_t bits);
void analogSetPinAttenuation(uint8_t pin, adc_attenuation_t attenuation);

struct FakeSerial {
    uint32_t baud = 0;
    std::ostringstream output;
    void begin(uint32_t value) { baud = value; }
    void print(uint8_t value) { output << static_cast<unsigned int>(value); }
    template <typename T> void print(const T& value) { output << value; }
    void println() { output << '\n'; }
    template <typename T> void println(const T& value) { print(value); println(); }
};
extern FakeSerial Serial;

#endif
