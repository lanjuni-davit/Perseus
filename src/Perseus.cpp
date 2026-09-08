#include "Perseus.h"

// Definitions also allow the constants to be used by reference in C++11.
constexpr uint8_t Perseus::MAX_SENSORS;
constexpr uint8_t Perseus::DEFAULT_SENSOR_COUNT;
constexpr uint16_t Perseus::DEFAULT_MAX_RANGE;
constexpr uint16_t Perseus::ADC_MAX_VALUE;
constexpr uint8_t Perseus::SETTLING_DELAY_US;

Perseus::Perseus()
    : _selectPins{37, 36, 35, 34},
      _sigPin(7),
      _numSensors(DEFAULT_SENSOR_COUNT),
      _maxRange(DEFAULT_MAX_RANGE),
      _initialized(false) {
    // Construction is safe for global objects: no hardware calls here.
    clearCalibration();
}

bool Perseus::begin() {
    return begin(37, 36, 35, 34, 7, DEFAULT_SENSOR_COUNT);
}

bool Perseus::begin(uint8_t s0, uint8_t s1, uint8_t s2, uint8_t s3,
                    uint8_t sig, uint8_t numSensors) {
    if (numSensors == 0 || numSensors > MAX_SENSORS) {
        return false;
    }

    const uint8_t pins[5] = {s0, s1, s2, s3, sig};
    for (uint8_t i = 0; i < 5; ++i) {
        for (uint8_t j = 0; j < i; ++j) {
            if (pins[i] == pins[j]) {
                return false;
            }
        }
    }

    for (uint8_t i = 0; i < 4; ++i) {
        if (!digitalPinCanOutput(pins[i])) {
            return false;
        }
    }
    if (!digitalPinIsValid(sig) || digitalPinToAnalogChannel(sig) < 0) {
        return false;
    }

    for (uint8_t i = 0; i < 4; ++i) {
        _selectPins[i] = pins[i];
        pinMode(_selectPins[i], OUTPUT);
    }
    _sigPin = sig;
    _numSensors = numSensors;
    pinMode(_sigPin, INPUT);
    selectChannel(0);

    // Keep the ADC source scale separate from the requested output scale.
    // This resolution setting is shared by analogRead() users in the sketch.
    analogReadResolution(12);
    analogSetPinAttenuation(_sigPin, ADC_11db);

    _initialized = true;
    clearCalibration();
    return true;
}

void Perseus::setSensitivity(uint16_t maxRange) {
    _maxRange = (maxRange == 0) ? 1 : maxRange;
}

bool Perseus::isValidChannel(uint8_t channel) const {
    return _initialized && channel < _numSensors;
}

void Perseus::selectChannel(uint8_t channel) {
    // S0 is the least significant address bit, S3 the most significant.
    // Right-shift brings the chosen bit to bit 0; & 1 extracts only that bit.
    // Example: channel 5 (0101 binary) sets S0=1, S1=0, S2=1, S3=0.
    for (uint8_t bit = 0; bit < 4; ++bit) {
        digitalWrite(_selectPins[bit], ((channel >> bit) & 0x01U) ? HIGH : LOW);
    }
}

uint16_t Perseus::readAdc(uint8_t channel) {
    // Private callers have already checked initialization and channel bounds.
    selectChannel(channel);

    // Wait after the LAST address write so the switched analog path settles
    // before sampling. Also wait on repeat reads of the same channel.
    delayMicroseconds(SETTLING_DELAY_US);
    const uint16_t value = ::analogRead(_sigPin);
    return (value > ADC_MAX_VALUE) ? ADC_MAX_VALUE : value;
}

uint16_t Perseus::readRaw(uint8_t channel) {
    if (!isValidChannel(channel)) {
        return 0;
    }

    // Multiply before dividing; use 32 bits to avoid 16-bit overflow.
    // Integer division maps endpoints exactly and truncates fractional counts.
    return static_cast<uint16_t>(
        (static_cast<uint32_t>(readAdc(channel)) * _maxRange) / ADC_MAX_VALUE);
}

void Perseus::readAllRaw(uint16_t* buffer) {
    if (buffer == nullptr) {
        return;
    }
    for (uint8_t channel = 0; channel < _numSensors; ++channel) {
        buffer[channel] = readRaw(channel);
    }
}

void Perseus::clearCalibration() {
    for (uint8_t channel = 0; channel < MAX_SENSORS; ++channel) {
        // Reversed extrema let the first sample initialize both endpoints.
        _minValues[channel] = ADC_MAX_VALUE;
        _maxValues[channel] = 0;
    }
}

void Perseus::calibrate() {
    if (!_initialized) {
        return;
    }
    for (uint8_t channel = 0; channel < _numSensors; ++channel) {
        // Never calibrate scaled/quantized values: changing setSensitivity()
        // later must not distort or discard the measured calibration range.
        const uint16_t value = readAdc(channel);
        if (value < _minValues[channel]) {
            _minValues[channel] = value;
        }
        if (value > _maxValues[channel]) {
            _maxValues[channel] = value;
        }
    }
}

bool Perseus::isCalibrated() const {
    if (!_initialized) {
        return false;
    }
    for (uint8_t channel = 0; channel < _numSensors; ++channel) {
        if (_maxValues[channel] <= _minValues[channel]) {
            return false;
        }
    }
    return true;
}
