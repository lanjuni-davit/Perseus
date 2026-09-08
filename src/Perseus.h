#ifndef PERSEUS_H
#define PERSEUS_H

#include <Arduino.h>

#if !defined(ARDUINO_ARCH_ESP32)
#error "Perseus requires the Arduino ESP32 core; select an ESP32-S3 board."
#endif

/**
 * IR sensor array connected to a CD74HC4067, for Arduino on ESP32.
 *
 * Channels are C0 through C(numSensors - 1).
 * Tie the multiplexer EN pin LOW externally. No dynamic allocation is used.
 * Use from one task, outside interrupts; access to the mux/ADC is not locked.
 */
class Perseus {
public:
    static constexpr uint8_t MAX_SENSORS = 16;
    static constexpr uint8_t DEFAULT_SENSOR_COUNT = 12;
    static constexpr uint16_t DEFAULT_MAX_RANGE = 4095;

    Perseus();

    // Configure GPIOs and a 12-bit ADC. Successful begin resets calibration,
    // but preserves the output range selected with setSensitivity().
    // Invalid counts, duplicate pins, unsuitable output pins or a non-ADC SIG
    // return false without changing an existing configuration or touching GPIOs.
    bool begin();
    bool begin(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3, uint8_t sig,
               uint8_t numSensors = DEFAULT_SENSOR_COUNT);

    // Set the OUTPUT scale to 0..maxRange, not the electrical sensitivity.
    // Accepts 1..65535; 0 is clamped to 1. Calibration remains valid.
    void setSensitivity(uint16_t maxRange);
    uint16_t getSensitivity() const { return _maxRange; }
    uint8_t getSensorCount() const { return _numSensors; }

    // Invalid/inactive channels and reads before begin return 0 without I/O.
    uint16_t readRaw(uint8_t channel);

    // All buffer APIs need at least getSensorCount() uint16_t elements.
    // A null pointer is ignored; the caller owns the buffer and its capacity.
    void readAllRaw(uint16_t* buffer);

    // One scan per call. Call repeatedly while sweeping EVERY sensor over
    // both line and background. Extrema are kept in native 0..4095 ADC units.
    void calibrate();
    // True when every active channel has a nonzero measured span.
    // Calibration records extrema only; it does not change raw readings.
    bool isCalibrated() const;

private:
    static constexpr uint16_t ADC_MAX_VALUE = 4095;
    static constexpr uint8_t SETTLING_DELAY_US = 10;

    uint8_t _selectPins[4];
    uint8_t _sigPin;
    uint8_t _numSensors;
    uint16_t _maxRange;
    uint16_t _minValues[MAX_SENSORS];
    uint16_t _maxValues[MAX_SENSORS];
    bool _initialized;

    void clearCalibration();
    void selectChannel(uint8_t channel);
    uint16_t readAdc(uint8_t channel);
    bool isValidChannel(uint8_t channel) const;
};

#endif // PERSEUS_H
