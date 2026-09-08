#include <Perseus.h>

Perseus perseus;
uint16_t sensorValues[Perseus::MAX_SENSORS];

void setup() {
    Serial.begin(115200);

    // S0=37, S1=36, S2=35, S3=34, SIG=7, active channels C0..C11.
    if (!perseus.begin()) {
        while (true) {
            Serial.println("Perseus: invalid pins or sensor count.");
            delay(1000);
        }
    }

    // Custom wiring/count: replace the begin() call above with, for example,
    // perseus.begin(37, 36, 35, 34, 7, 16).

    // Output range: 0..4095. Change to 255 or 1023 for a smaller scale.
    // This changes numeric scaling, not sensor gain or IR LED power.
    perseus.setSensitivity(4095);
}

void loop() {
    // Read each active mux channel once. Raw reads need no calibration.
    perseus.readAllRaw(sensorValues);

    for (uint8_t channel = 0; channel < perseus.getSensorCount(); ++channel) {
        Serial.print('C');
        Serial.print(channel);
        Serial.print(':');
        Serial.print(sensorValues[channel]);
        Serial.print('\t');
    }
    Serial.println();

    // Slow serial reporting for readability.
    delay(500);
}
