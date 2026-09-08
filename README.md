# Perseus

**Perseus** is the official Arduino C++ library for the **Perseus Forge hardware ecosystem**.

The library is designed to provide a simple and consistent interface for Perseus Forge development boards, sensors, modules, and future robotics hardware.

Instead of using separate libraries with completely different APIs for every Perseus Forge product, the goal of Perseus is to provide one unified library:

```cpp
#include <Perseus.h>
```

Current hardware support includes the **Perseus IR Sensor Array** based on the ESP32-S3 and CD74HC4067 multiplexer.

Future versions of the library are planned to support additional Perseus Forge hardware, including motor control and other sensors and modules.

> **Current release:** IR Sensor Array support
> **Planned:** Motor control and additional Perseus Forge modules

---

## Current Hardware Support

### Perseus IR Sensor Array

The current implementation supports a Perseus Forge IR sensor array using a **CD74HC4067 16-channel analog multiplexer**.

By default, the library is configured for a 12-sensor array connected to channels **C0–C11**.

| Connection                         | Default |
| ---------------------------------- | ------- |
| S0 — least significant address bit | GPIO 37 |
| S1                                 | GPIO 36 |
| S2                                 | GPIO 35 |
| S3 — most significant address bit  | GPIO 34 |
| SIG / analog input                 | GPIO 7  |
| Active sensor inputs               | C0–C11  |
| EN / E                             | LOW     |
| Multiplexer settling time          | 10 µs   |

The default configuration targets the **ESP32-S3**.

Custom pins and sensor counts can also be supplied during initialization.

---

## Installation

### Arduino IDE

Download the Perseus library as a ZIP file.

In Arduino IDE select:

**Sketch → Include Library → Add .ZIP Library**

Then select `Perseus.zip`.

The included example can be opened from:

**File → Examples → Perseus → BasicRead**

Select your ESP32-S3 board and open Serial Monitor at:

```text
115200 baud
```

No additional Arduino libraries are required.

---

## PlatformIO

Extract the `Perseus` folder into your project's:

```text
lib/
```

directory.

Use the Arduino framework for your ESP32-S3 and include the library with:

```cpp
#include <Perseus.h>
```

---

# Quick Start — IR Sensor Array

```cpp
#include <Perseus.h>

Perseus perseus;

void setup() {
    Serial.begin(115200);

    perseus.begin();
    perseus.setSensitivity(255);
}

void loop() {
    for (uint8_t i = 0; i < perseus.getSensorCount(); ++i) {
        Serial.print(perseus.readRaw(i));
        Serial.print('\t');
    }

    Serial.println();
    delay(500);
}
```

With the default configuration, this reads IR sensors **C0 through C11** every 500 ms.

---

# Initialization

For the standard Perseus IR Sensor Array configuration:

```cpp
perseus.begin();
```

Default pins:

```text
S0  = GPIO 37
S1  = GPIO 36
S2  = GPIO 35
S3  = GPIO 34
SIG = GPIO 7
```

The default number of active sensors is:

```text
12
```

You can provide a custom configuration:

```cpp
perseus.begin(s0, s1, s2, s3, sig, numSensors);
```

For example:

```cpp
perseus.begin(37, 36, 35, 34, 7, 16);
```

This enables all 16 CD74HC4067 channels.

If the sixth argument is omitted, 12 sensors are used.

`begin()` returns `true` when the configuration is accepted and `false` when the requested configuration is invalid.

---

# Reading Sensors

### Read one sensor

```cpp
uint16_t value = perseus.readRaw(0);
```

Channels are zero-based.

For the standard 12-sensor configuration:

```text
0 ... 11
```

are valid.

---

### Read all sensors

```cpp
uint16_t values[Perseus::MAX_SENSORS];

perseus.readAllRaw(values);
```

The library scans every currently active sensor.

You can then access individual values:

```cpp
Serial.println(values[0]);
Serial.println(values[1]);
```

---

# Sensitivity / Output Range

The output range can be configured using:

```cpp
perseus.setSensitivity(255);
```

For example:

```cpp
perseus.setSensitivity(255);
```

produces values from:

```text
0 ... 255
```

while:

```cpp
perseus.setSensitivity(1023);
```

produces:

```text
0 ... 1023
```

The default is:

```text
0 ... 4095
```

You can query the current setting with:

```cpp
perseus.getSensitivity();
```

`setSensitivity()` controls the **numeric output scale**. It does not change IR emitter power, sensor gain, or the ESP32 ADC voltage range.

---

# Calibration

The IR sensor array can collect minimum and maximum ADC values for every active sensor.

Call:

```cpp
perseus.calibrate();
```

repeatedly while moving the sensor array across both the line and the background.

Example:

```cpp
Serial.println("Move the sensors across the line and background.");

const uint32_t started = millis();

while (static_cast<uint32_t>(millis() - started) < 5000U) {
    perseus.calibrate();
    delay(5);
}

if (perseus.isCalibrated()) {
    Serial.println("Calibration complete.");
} else {
    Serial.println("Calibration incomplete.");
}
```

Every sensor should see both surfaces during calibration.

`isCalibrated()` returns `true` when every active sensor has recorded a non-zero measurement span.

Calibration data is stored in RAM and is reset after a successful `begin()`.

Calibration currently does not modify the values returned by `readRaw()` or `readAllRaw()`.

---

# IR Sensor API

| Method                              | Description                                                        |
| ----------------------------------- | ------------------------------------------------------------------ |
| `begin()`                           | Initialize using the default Perseus IR sensor configuration.      |
| `begin(s0, s1, s2, s3, sig, count)` | Initialize with custom multiplexer pins and sensor count.          |
| `readRaw(channel)`                  | Read one sensor.                                                   |
| `readAllRaw(buffer)`                | Read all active sensors.                                           |
| `setSensitivity(range)`             | Set the numeric output range.                                      |
| `getSensitivity()`                  | Return the current output range.                                   |
| `getSensorCount()`                  | Return the number of active sensors.                               |
| `calibrate()`                       | Update calibration extrema for every sensor.                       |
| `isCalibrated()`                    | Check whether all active sensors have a measured calibration span. |

---

# ADC Configuration

For the current ESP32-S3 IR sensor implementation, `begin()` configures:

```cpp
analogReadResolution(12);
```

and uses:

```cpp
ADC_11db
```

attenuation for the SIG input.

The library therefore works internally with 12-bit ADC measurements before applying the selected output range.

A raw ADC count of `4095` should not be interpreted as exactly 3.3 V.

---

# Multiplexer Operation

The IR Sensor Array uses the **CD74HC4067** analog multiplexer.

Perseus automatically selects the required channel using S0–S3 and waits **10 µs** after setting the multiplexer address before performing the ADC read.

The multiplexer enable input **E / EN must be LOW** for a channel to be connected.

For a direct ESP32-S3 interface, the multiplexer should be powered appropriately for the ESP32's 3.3 V logic and the analog input must remain within the permitted voltage range.

---

# Perseus Forge Ecosystem

The IR Sensor Array is the first hardware family supported by the Perseus library.

The library is intended to grow together with the Perseus Forge ecosystem so that additional hardware can use the same Arduino package rather than requiring an unrelated library for every board.

Planned areas include:

* IR and line-tracking sensors
* Motor control
* Additional sensor modules
* Perseus Forge development boards
* Future Perseus Forge robotics modules

The API will expand as those devices are released.

Functionality documented in this README refers only to hardware supported by the current release.

---

# Library Structure

```text
Perseus/
├── src/
│   ├── Perseus.h
│   └── Perseus.cpp
├── examples/
│   └── BasicRead/
│       └── BasicRead.ino
├── tests/
├── library.properties
└── README.md
```

The library targets the Arduino ESP32 core and has no dependency on Octoliner or Wire.

---

# Testing

The current IR Sensor Array implementation includes reproducible host tests covering:

* all 16 multiplexer addresses
* channel selection
* settling-call order
* sensor counts from 1 to 16
* invalid configurations
* output scaling
* calibration state
* buffer boundaries
* constant and uncalibrated channels

Run the tests with:

```bash
bash tests/run_tests.sh
```

The tests use a simulated Arduino interface and do not replace testing on a physical ESP32-S3 and Perseus Forge sensor board.

---

## License

See the repository license for usage and distribution terms.

---

**Perseus Forge**
Hardware built for robotics.
