# Perseus

Arduino C++ library for the Perseus ESP32-S3 IR array and a CD74HC4067
multiplexer. The implementation is written for this hardware and API request;
it has no dependency on Octoliner or Wire.

The requested files are `src/Perseus.h`, `src/Perseus.cpp`, and
`examples/BasicRead/BasicRead.ino`. The package also includes Arduino metadata
and reproducible host tests. The code targets the Arduino ESP32 core.

In Arduino IDE, select **Sketch > Include Library > Add .ZIP Library**, choose
`Perseus.zip`, and open **File > Examples > Perseus > BasicRead**. Select your
ESP32-S3 board configuration and open Serial Monitor at **115200 baud**. The
example prints labeled C0 through C11 readings every 500 ms and needs no
calibration. The ZIP follows the [Arduino library layout](https://docs.arduino.cc/arduino-cli/library-specification/).

For PlatformIO, extract the `Perseus` folder into your project's `lib` folder,
use the Arduino framework for your ESP32-S3, and copy the example into
`src/main.cpp`. No extra library dependencies are required.

| Connection | Default |
| --- | --- |
| S0, least significant address bit | GPIO 37 |
| S1 | GPIO 36 |
| S2 | GPIO 35 |
| S3, most significant address bit | GPIO 34 |
| SIG / common analog terminal | GPIO 7 |
| Active sensor inputs | C0 through C11 |
| EN / E | Hold LOW externally |
| Wait after the last address write | 10 microseconds before every ADC read |

Use GPIO numbering and a common ground. For a direct 3.3 V ESP32 interface,
power the **HC** multiplexer at 3.3 V and keep its analog signals within that
supply. E must be LOW to connect a channel. The
[TI datasheet](https://www.ti.com/lit/ds/symlink/cd74hc4067.pdf) specifies the
control truth table and supply/analog limits. `begin()` checks chip-level pin
capabilities and duplicates, but cannot verify wiring, module-reserved pins,
or whether a sensor is connected. Custom pin choices must suit your board.

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

To change the wiring or count, replace `begin()` with
`begin(s0, s1, s2, s3, sig, numSensors)`, for example
`perseus.begin(37, 36, 35, 34, 7, 16)`. Omitting the sixth argument selects
12 sensors. Both overloads return `bool`; false means the proposed pin/count
configuration was rejected without hardware changes. An already valid
configuration remains active if a later `begin()` fails.

`setSensitivity(maxRange)` changes the numeric output scale to `0..maxRange`.
Its default is 4095; 255 and 1023 are also supported. Values 1 through 65535
are accepted, and 0 is clamped to 1. Changing this setting preserves
calibration. It does not change the sensor's gain, emitter power, or ADC input
voltage range. Octoliner's identically named method controls its board's
electrical sensitivity, so that method's meaning differs from this requested
API. See [Octoliner's API](https://github.com/amperka/Octoliner/blob/master/API.md).

`begin()` sets `analogReadResolution(12)` and sets SIG attenuation to
`ADC_11db`. ESP32-S3's documented measurable range at that attenuation is
approximately 0 to 3.1 V. A value of 4095 is an ADC count, not a guaranteed
3.3 V measurement. See the [Espressif ADC reference](https://docs.espressif.com/projects/arduino-esp32/en/latest/api/adc.html).
The resolution setting is shared with other `analogRead()` users. Keep it
at 12 bits while using this library; changing it elsewhere breaks scaling.
If you change attenuation after `begin()`, calibrate again.

| Method | Behavior |
| --- | --- |
| `readRaw(channel)` | Read one active channel and scale its ADC count to `0..maxRange`. |
| `readAllRaw(buffer)` | Read each active channel once into the buffer. |
| `calibrate()` | Perform one scan and extend each channel's native ADC min/max. |
| `isCalibrated()` | True when every active channel has a nonzero calibration span. |
| `getSensorCount()` / `getSensitivity()` | Query the active count and output scale. |

All channels are zero-based and must be below `getSensorCount()`. Single
reads of an invalid channel, or before initialization, return 0 without
hardware access. Every array must contain at least `getSensorCount()` writable
`uint16_t` elements; use `uint16_t values[Perseus::MAX_SENSORS]` to allow
any count. `readAllRaw(nullptr)` is ignored.

Calibration requires repeated calls while **every sensor** passes over both
the line and background at the intended working height. One stationary scan
cannot establish the contrast range. Extrema use native 12-bit ADC counts,
which prevents a smaller output scale from throwing away calibration detail.
A successful `isCalibrated()` only proves that two different ADC values were
seen; noise alone is not a useful calibration. Calibration lives in RAM and
is cleared by a successful `begin()`.

`calibrate()` and the no-argument `isCalibrated()` collect extrema and report
whether all active sensors have a measured span. Calibration does not change
`readRaw()` or `readAllRaw()`; their values remain scaled ADC counts.

For example, add this after a successful `begin()` in `setup()`:

```cpp
Serial.println("Sweep all sensors across the line and background for 5 seconds.");
const uint32_t started = millis();
while (static_cast<uint32_t>(millis() - started) < 5000U) {
    perseus.calibrate();
    delay(5);
}
if (!perseus.isCalibrated()) {
    Serial.println("Incomplete calibration: move every sensor across both surfaces.");
}
```

Calls are synchronous and perform one `analogRead()` per sampled channel.
The requested 10 microsecond wait occurs after all four address writes,
including repeated channel reads. Actual scan time also includes GPIO and
ADC overhead; analog settling must be checked with your source impedance
and capacitance. Use this object from one task outside interrupts, and
serialize all access if other code uses the same mux or ADC configuration.

Validation: the source and actual BasicRead sketch compile as C++11 with
strict warnings against a simulated Arduino interface. Host checks passed
for all 16 mux addresses, settling-call order, every active count from 1 to
16, configuration failures, buffer boundaries, scaling, calibration status,
and constant/uncalibrated channels. AddressSanitizer and UndefinedBehaviorSanitizer
reported no errors. These are host tests, not an ESP32 target build or
measurements on a physical board; those remain necessary before a production
release.

Run the included checks with `bash tests/run_tests.sh` using a Linux C++
compiler supporting those sanitizers. In environments where LeakSanitizer
cannot inspect processes, use
`ASAN_OPTIONS=detect_leaks=0 bash tests/run_tests.sh`; address and undefined
behavior checks remain enabled. That is the command used for this package's
host verification.
