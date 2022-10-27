#include "Sensors.h"

AnalogSensor sensors[] = {
    // AnalogSensor(pin, m, b)
    AnalogSensor(34, 2),
    AnalogSensor(32, -3, 4),
    AnalogSensor(35, 0, 5)
};
const size_t nSensors = sizeof(sensors) / sizeof(AnalogSensor);

#define READ_INTERVAL_MS 3000
#define PRINT_INTERVAL_MS 9050

void setup() {
    Serial.begin(115200);
    BeginFS(true);
    DeleteFile();
    Serial.print("begin with "); Serial.print(nSensors); Serial.println(" Sensors");
}

void loop() {
    static unsigned long lastReadTime = millis();
    static unsigned long lastPrintInterval = millis();

    if (abs(millis() - lastReadTime) > READ_INTERVAL_MS) {
        SensorsSave(sensors, nSensors);
        lastReadTime = millis();
    }

    if (abs(millis() - lastPrintInterval) > PRINT_INTERVAL_MS) {
        ReadFile();
        lastPrintInterval = millis();
    }
}
