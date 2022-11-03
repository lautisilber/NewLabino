#if !defined(_SENSORS_H)
#define _SENSORS_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <math.h>

#include "FS.h"
#ifdef ESP32
    #include <LITTLEFS.h>
    #define FILE_SYSTEM LITTLEFS
#elif defined(ESP8266)
    #include <LittleFS.h>
    #define FILE_SYSTEM LittleFS
#endif


#if !defined(SENS_AVG)
#define SENS_AVG 50
#endif


class AnalogSensor {
private:
  byte _pin;
    float _m, _b;
    float _lastVal;
    public:
    AnalogSensor(byte pin, float m=0, float b=0) {
        _pin = pin;
        _m = m;
        _b = b;
    }

    public:
    float read(bool update=true){
        if (update) _read();
        return _lastVal;
    }

private:
    void _read() {
        float val = 0;
        for (int i = 0; i < SENS_AVG; i++) {
            int raw = analogRead(_pin);
            val += (float)raw;
        }
        val /= SENS_AVG;
        _lastVal = (_m * ((float)val)) + _b;
    }
};


#define SENSOR_JSON_SIZE 64
#define SAVE_FILENAME "/save.txt"

//inline float _Round(float f, int decs) {
//    // only for positive numbers
//    float mult = pow(10.0, (float)decs);
//    int multiplied = (int)(f * mult + .5);
//    float rounded = ((float)multiplied) / mult;
//    if (f > 0 && rounded < 0) {
//        return _Round(f, decs-1);
//    }
//    return rounded;
//}

void SensorsJson(JsonDocument &doc, AnalogSensor *sensors, size_t len) {
    const size_t keyLen = 16;
    for (size_t i = 0; i < len; i++) {
        char key[keyLen]{0};
        snprintf(key, keyLen, "%d", i);
        JsonObject sens = doc.createNestedObject(key);
        sens["val"] = sensors[i].read(false);
    }
}

bool _JsonToFile(JsonDocument &doc, const char *fname=SAVE_FILENAME){
    File file = FILE_SYSTEM.open(fname, FILE_APPEND);
    if(!file){
        Serial.println("Failed to open file for writing");
        return false;
    }
    if (serializeJson(doc, file) == 0) {
        Serial.println("Failed to write to file");
        return false;
    }
    if (!file.println("")) {
        Serial.println("Failed to write to file");
        return false;
    }
    
    file.close();
    return true;
}

bool SensorsSave(AnalogSensor *sensors, size_t len, const char *fname=SAVE_FILENAME) {
    DynamicJsonDocument doc(SENSOR_JSON_SIZE * len);

    SensorsJson(doc, sensors, len);
    
    return _JsonToFile(doc, fname);
}

bool SensorsSaveAndDHT(AnalogSensor *sensors, size_t len, float hum, float temp, const char *fname=SAVE_FILENAME) {
    DynamicJsonDocument doc(SENSOR_JSON_SIZE * len + 128);

    SensorsJson(doc, sensors, len);
    //doc["hum"] = _Round(hum, 2);
    //doc["temp"] = _Round(temp, 2);
    doc["hum"] = hum;
    doc["temp"] = temp;
    
    return _JsonToFile(doc, fname);
}

bool DeleteFile(const char *fname=SAVE_FILENAME) {
    if(!FILE_SYSTEM.remove(fname)){
        Serial.println("File deletion failed");
        return false;
    }
    return true;
}

bool BeginFS(bool format=false) {
    if(!FILE_SYSTEM.begin(format)){
        Serial.println("FS Mount Failed");
        return false;
    }
    return true;
}

void ReadFile(const char *fname=SAVE_FILENAME) {
    File file = FILE_SYSTEM.open(fname);
    if(!file || file.isDirectory()){
        Serial.println("Failed to open file for reading");
        return;
    }

    while(file.available()){
        Serial.write(file.read());
    }
    file.close();
    Serial.println();
}


#endif
