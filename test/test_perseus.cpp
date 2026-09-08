#include "Perseus.h"

#include <array>
#include <cstdlib>
#include <iostream>
#include <string>

namespace {
unsigned int assertions = 0;

void require(bool condition, const char* expression, int line) {
    ++assertions;
    if (!condition) {
        std::cerr << "FAIL at line " << line << ": " << expression << '\n';
        std::exit(1);
    }
}
#define CHECK(expression) require((expression), #expression, __LINE__)

struct Hardware {
    std::array<uint8_t, 4> selectPins{{37, 36, 35, 34}};
    uint8_t sigPin = 7;
    std::array<uint8_t, 256> levels{};
    std::array<uint8_t, 256> modes{};
    std::array<uint16_t, 16> samples{};
    unsigned int gpioCalls = 0;
    unsigned int writes = 0;
    unsigned int reads = 0;
    unsigned int settlingCalls = 0;
    unsigned int resolutionCalls = 0;
    unsigned int attenuationCalls = 0;
    uint32_t elapsedAfterWrite = 0;
    uint32_t lastDelayMs = 0;
    uint8_t resolution = 0;
} hw;

void freshHardware() {
    hw = Hardware{};
}

void fill(uint16_t value) {
    hw.samples.fill(value);
}

void calibrateEndpoints(Perseus& sensor, uint16_t low = 0, uint16_t high = 4095) {
    fill(low);
    sensor.calibrate();
    fill(high);
    sensor.calibrate();
    CHECK(sensor.isCalibrated());
}

void testStartupAndFailures() {
    freshHardware();
    Perseus sensor;
    CHECK(hw.gpioCalls == 0);
    CHECK(sensor.getSensorCount() == 12);
    CHECK(sensor.getSensitivity() == 4095);
    CHECK(sensor.readRaw(0) == 0);
    std::array<uint16_t, 17> data;
    data.fill(12345);
    sensor.readAllRaw(data.data());
    for (unsigned int i = 0; i < 12; ++i) { CHECK(data[i] == 0); }
    CHECK(data[12] == 12345);
    sensor.calibrate();
    sensor.readAllRaw(nullptr);
    CHECK(hw.reads == 0 && hw.gpioCalls == 0);

    CHECK(!sensor.begin(37, 36, 35, 34, 7, 0));
    CHECK(!sensor.begin(37, 36, 35, 34, 7, 17));
    CHECK(!sensor.begin(37, 36, 35, 34, 7, 255));
    CHECK(!sensor.begin(37, 37, 35, 34, 7));
    CHECK(!sensor.begin(37, 36, 35, 34, 34));
    CHECK(!sensor.begin(46, 36, 35, 34, 7));
    CHECK(!sensor.begin(255, 36, 35, 34, 7));
    CHECK(!sensor.begin(37, 36, 35, 34, 33));
    CHECK(!sensor.begin(37, 36, 35, 34, 255));
    CHECK(hw.gpioCalls == 0 && hw.resolutionCalls == 0);

    sensor.setSensitivity(255);
    CHECK(sensor.begin());
    CHECK(sensor.getSensitivity() == 255);
    CHECK(hw.modes[37] == OUTPUT && hw.modes[36] == OUTPUT);
    CHECK(hw.modes[35] == OUTPUT && hw.modes[34] == OUTPUT);
    CHECK(hw.modes[7] == INPUT);
    CHECK(hw.resolution == 12 && hw.attenuationCalls == 1);
    calibrateEndpoints(sensor);
    const auto calls = hw.gpioCalls;
    CHECK(!sensor.begin(37, 36, 35, 34, 7, 17));
    CHECK(hw.gpioCalls == calls);
    CHECK(sensor.isCalibrated());
    CHECK(sensor.getSensorCount() == 12);
    CHECK(sensor.begin());
    CHECK(!sensor.isCalibrated());
    CHECK(sensor.getSensitivity() == 255);
}

void testMuxAndBuffers() {
    freshHardware();
    Perseus sensor;
    CHECK(sensor.begin(37, 36, 35, 34, 7, 16));
    for (uint16_t channel = 0; channel < 16; ++channel) {
        hw.samples[channel] = static_cast<uint16_t>(channel * 257);
    }
    std::array<uint16_t, 18> storage;
    storage.fill(60000);
    const auto writes = hw.writes;
    sensor.readAllRaw(storage.data() + 1);
    CHECK(storage.front() == 60000 && storage.back() == 60000);
    for (uint16_t channel = 0; channel < 16; ++channel) {
        CHECK(storage[channel + 1] == channel * 257);
    }
    CHECK(hw.reads == 16 && hw.settlingCalls == 16);
    CHECK(hw.writes - writes == 64);
    CHECK(sensor.readRaw(15) == 3855);
    CHECK(hw.reads == 17 && hw.settlingCalls == 17);
    CHECK(sensor.readRaw(16) == 0);
    CHECK(sensor.readRaw(255) == 0);
    CHECK(hw.reads == 17);

    hw.selectPins = {{10, 11, 12, 13}};
    hw.sigPin = 1;
    CHECK(sensor.begin(10, 11, 12, 13, 1, 3));
    CHECK(sensor.getSensorCount() == 3);
    CHECK(sensor.readRaw(2) == 514);
    CHECK(sensor.readRaw(3) == 0);
    CHECK(sensor.begin(10, 11, 12, 13, 1));
    CHECK(sensor.getSensorCount() == 12);

    hw.selectPins = {{37, 36, 35, 34}};
    hw.sigPin = 7;
    CHECK(sensor.begin());
    CHECK(sensor.readRaw(11) == 2827);
}

void testScaling() {
    freshHardware();
    Perseus sensor;
    CHECK(sensor.begin());
    struct Case { uint16_t range; uint16_t half; };
    const Case cases[] = {{1, 0}, {255, 127}, {1023, 511}, {4095, 2048}, {65535, 32775}};
    for (const auto& item : cases) {
        sensor.setSensitivity(item.range);
        fill(0);
        CHECK(sensor.readRaw(0) == 0);
        fill(4095);
        CHECK(sensor.readRaw(0) == item.range);
        fill(2048);
        CHECK(sensor.readRaw(0) == item.half);
        fill(65535); // Defensive saturation of an out-of-contract ADC result.
        CHECK(sensor.readRaw(0) == item.range);
    }
    sensor.setSensitivity(0);
    CHECK(sensor.getSensitivity() == 1);
    fill(4095);
    CHECK(sensor.readRaw(0) == 1);
}

void testCalibration() {
    freshHardware();
    Perseus sensor;
    CHECK(!sensor.isCalibrated());
    sensor.calibrate();
    CHECK(hw.reads == 0);
    CHECK(sensor.begin());
    CHECK(!sensor.isCalibrated());

    fill(2031);
    sensor.setSensitivity(1);
    sensor.calibrate();
    CHECK(!sensor.isCalibrated());
    sensor.calibrate();
    CHECK(!sensor.isCalibrated());
    hw.samples[0] = 2033;
    sensor.calibrate();
    CHECK(!sensor.isCalibrated()); // Every active channel needs a span.

    sensor.setSensitivity(255);
    fill(2033);
    sensor.calibrate();
    CHECK(sensor.isCalibrated()); // Native counts survive output quantization.
    sensor.setSensitivity(4095);
    fill(2032);
    CHECK(sensor.readRaw(0) == 2032); // Calibration does not normalize raw reads.
    CHECK(sensor.isCalibrated());
    CHECK(sensor.begin());
    CHECK(!sensor.isCalibrated());
    CHECK(sensor.getSensitivity() == 4095);
}

void testCounts() {
    for (uint8_t count = 1; count <= 16; ++count) {
        freshHardware();
        Perseus sensor;
        CHECK(sensor.begin(37, 36, 35, 34, 7, count));
        CHECK(sensor.getSensorCount() == count);
        calibrateEndpoints(sensor);
        sensor.setSensitivity(65535);
        uint16_t buffer[17]{};
        buffer[count] = 60000;
        fill(4095);
        const auto reads = hw.reads;
        sensor.readAllRaw(buffer);
        CHECK(hw.reads - reads == count);
        for (uint8_t channel = 0; channel < count; ++channel) {
            CHECK(buffer[channel] == 65535);
        }
        CHECK(buffer[count] == 60000);
        CHECK(sensor.readRaw(count) == 0);
        CHECK(hw.reads - reads == count);
    }
}
} // namespace

FakeSerial Serial;

void pinMode(uint8_t pin, uint8_t mode) {
    ++hw.gpioCalls;
    hw.modes[pin] = mode;
}
void digitalWrite(uint8_t pin, uint8_t value) {
    CHECK(hw.modes[pin] == OUTPUT);
    CHECK(value == LOW || value == HIGH);
    ++hw.gpioCalls;
    ++hw.writes;
    hw.levels[pin] = value;
    hw.elapsedAfterWrite = 0;
}
bool digitalPinIsValid(uint8_t pin) {
    return pin <= 48 && !(pin >= 22 && pin <= 25);
}
bool digitalPinCanOutput(uint8_t pin) {
    return digitalPinIsValid(pin) && pin != 46;
}
int8_t digitalPinToAnalogChannel(uint8_t pin) {
    return (pin >= 1 && pin <= 20) ? static_cast<int8_t>(pin - 1) : -1;
}
void delayMicroseconds(uint32_t us) {
    CHECK(us == 10);
    ++hw.settlingCalls;
    hw.elapsedAfterWrite += us;
}
void delay(uint32_t ms) { hw.lastDelayMs = ms; }
uint16_t analogRead(uint8_t pin) {
    CHECK(pin == hw.sigPin);
    CHECK(hw.modes[pin] == INPUT);
    CHECK(hw.elapsedAfterWrite >= 10);
    CHECK(hw.resolution == 12);
    ++hw.reads;
    const unsigned int channel = hw.levels[hw.selectPins[0]]
        + 2U * hw.levels[hw.selectPins[1]]
        + 4U * hw.levels[hw.selectPins[2]]
        + 8U * hw.levels[hw.selectPins[3]];
    return hw.samples[channel];
}
void analogReadResolution(uint8_t bits) {
    ++hw.resolutionCalls;
    hw.resolution = bits;
}
void analogSetPinAttenuation(uint8_t pin, adc_attenuation_t attenuation) {
    CHECK(pin == hw.sigPin);
    CHECK(attenuation == ADC_11db);
    ++hw.attenuationCalls;
}

// Link the actual .ino as C++ and execute its setup and one reporting loop.
void setup();
void loop();

int main() {
    testStartupAndFailures();
    testMuxAndBuffers();
    testScaling();
    testCalibration();
    testCounts();

    freshHardware();
    for (uint16_t i = 0; i < 16; ++i) { hw.samples[i] = static_cast<uint16_t>(i * 100); }
    setup();
    loop();
    CHECK(Serial.baud == 115200);
    CHECK(Serial.output.str().find("C0:0\tC1:100\t") == 0);
    CHECK(Serial.output.str().find("C11:1100\t\n") != std::string::npos);
    CHECK(Serial.output.str().find("C12:") == std::string::npos);
    CHECK(hw.reads == 12 && hw.lastDelayMs == 500);

    std::cout << "PASS: all Perseus host checks (" << assertions
              << " assertions), including BasicRead setup/loop.\n";
}
