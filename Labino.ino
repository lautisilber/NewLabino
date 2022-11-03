#include "Sensors.h"
#include "Credentials.h"

////////////// CONFIG //////////////

AnalogSensor sensors[] = {
    // AnalogSensor(pin, m, b)
    AnalogSensor(34, 2),
    AnalogSensor(32, -3, 4),
    AnalogSensor(35, 0, 5)
};
const size_t nSensors = sizeof(sensors) / sizeof(AnalogSensor);

uint32_t readInterval = 3000;

#define DHT_PIN 27

////////////////////////////////////


#ifdef ESP32
    #include <WiFi.h>
    #include <AsyncTCP.h>
#elif defined(ESP8266)
    #include <ESP8266WiFi.h>
    #include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebServer.h>

AsyncWebServer server(80);

#include <DHT.h>

DHT dht(DHT_PIN, DHT22);

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

const char index_html[] PROGMEM = R"rawliteral(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Labino</title>
</head>
<body>
    <a href="/download">Download</a>
    <a href="/show">Show</a>
    <a href="/delete">Delete</a>
</body>)rawliteral";

void setup() {
    Serial.begin(115200);
    BeginFS(true);
    DeleteFile();
    
    Serial.print("begin with "); Serial.print(nSensors); Serial.println(" Sensors");

    dht.begin();

//    WiFi.mode(WIFI_STA);
//    WiFi.begin(ssid, password);
//    if (WiFi.waitForConnectResult() != WL_CONNECTED) {
//        Serial.println("WiFi Failed!");
//        return;
//    }
//    Serial.print("IP Address: ");
//    Serial.println(WiFi.localIP());

    WiFi.mode(WIFI_AP);
    WiFi.softAP(ssidAP, passwordAP);
    Serial.println("WiFi AP created");
    
    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam("readinterval")) {
            readInterval = request->getParam("readinterval")->value().toInt();
        }
        request->send_P(200, "text/html", index_html);
    });

    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(FILE_SYSTEM, SAVE_FILENAME, "text/plain", true);
    });

    server.on("/show", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(FILE_SYSTEM, SAVE_FILENAME, "text/plain");
    });

    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request){
        if (DeleteFile()) {
            request->send(200, "text/html", "File deleted");
        } else {
            request->send(200, "text/html", "Couldn't delete file");
        }
    });

    server.on("/lastsense", HTTP_GET, [](AsyncWebServerRequest *request){
        DynamicJsonDocument doc(SENSOR_JSON_SIZE * nSensors);
        const size_t strLen = SENSOR_JSON_SIZE+2 * nSensors;
        char *str = (char *) malloc(strLen);
        memset(str, 0, strLen);
        
        SensorsJson(doc, sensors, nSensors);
        serializeJson(doc, str, strLen);

        request->send_P(200, "application/json", (uint8_t *) str, min(strlen(str), strLen));
        free(str);
    });

    #define READ_INTERVAL_QUERYPARAM "readinterval"
    #define MIN_READ_INTERVAL 2000
    server.on("/config", HTTP_GET [](AsyncWebServerRequest *request){
        if (request->hasParam(READ_INTERVAL_QUERYPARAM)) {
            int newInterval = request->getParam(READ_INTERVAL_QUERYPARAM)->value().toInt();
            if (newInterval > 0)
                readInterval = max(newInterval, MIN_READ_INTERVAL);
        }
        request->redirect("/");
    });

    server.onNotFound(notFound);

    server.begin();
}

void loop() {
    static unsigned long lastReadTime = millis();

    if (abs(millis() - lastReadTime) > readInterval) {
        //SensorsSave(sensors, nSensors);
        float hum = dht.readHumidity();
        float temp = dht.readTemperature();
        SensorsSaveAndDHT(sensors, nSensors, hum, temp);
        lastReadTime = millis();
    }
}
