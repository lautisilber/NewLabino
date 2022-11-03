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

uint32_t readInterval = 3000; // this will be stored in index 0 of nvm storage

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
#include <EEPROM.h>
#define EEPROM_SIZE 64

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
    <style>
        * {
            font-family: Helvetica, Arial, sans-serif;
        }

        :root {
            --color-dark: #425F57;
            --color-main: #749F82;
            --color-accent: #c2e7c2;
        }

        body {
            background-color: var(--color-dark);
        }
        .content {
            width: fit-content;            
            background-color: var(--color-main);
            padding: 0.5rem 1rem;
            padding-bottom: 1rem;
            border-radius: 5px;
            margin: 1rem;
            margin-left:auto;
            margin-right:auto;
        }

        a, button {
            display: inline-block;
            text-decoration: none;
            color: black;
            background-color: var(--color-accent);
            border: 1px solid var(--color-dark);
            border-radius: 4px;
            padding: .2rem .3rem;
            margin: .2rem .3rem;
            transition: .15s;
            cursor: pointer;
        }

        a:hover, button:hover {
            filter: brightness(92%);
        }

        a:active, button:active {
            filter: brightness(83%);
        }

        a#delete {
            color: white;
            background-color: crimson;
        }

        input {
            background-color: var(--color-accent);
            border: 1px solid var(--color-dark);
            border-radius: 5px;
        }

        fieldset {
            padding: 0.7rem 1rem;
            border: 1px solid var(--color-dark);
            border-radius: 5px;
        }
    </style>
</head>
<body>
    <div class="content">
        <h1>Labino</h1>
        <a href="/download">Download</a><br>
        <a href="/show">Show</a><br>
        <a href="/delete" id="delete" onclick="return confirm('Seguro queres eliminar el archivo?')">Delete</a><br><br>

        <form action="/config" method="get">
            <fieldset>
                <legend>Config</legend>
                <label for="readinterval">Read interval (ms): </label>
                <input type="number" name="readinterval" value="%INTERVAL%" min="2000"><br>
                <button type="submit">Set</button>
            </fieldset>
        </form>
    </div>
</body>
</html>)rawliteral";

const char download_html[] PROGMEM = R"rawstring(<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta http-equiv="X-UA-Compatible" content="IE=edge">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Labino</title>
    <style>
        * {
            font-family: Helvetica, Arial, sans-serif;
        }

        :root {
            --color-dark: #425F57;
            --color-main: #749F82;
            --color-accent: #c2e7c2;
        }

        body {
            background-color: var(--color-dark);
        }
        .content {
            width: fit-content;            
            background-color: var(--color-main);
            padding: 0.5rem 1rem;
            padding-bottom: 1rem;
            border-radius: 5px;
            margin: 1rem;
            margin-left:auto;
            margin-right:auto;
        }

        p {
            margin: 1rem;
            padding: 0.9rem;
            border-radius: 5px;
            font-size: 1.1rem;
            background-color: var(--color-accent);
        }
    </style>
</head>
<body>
    <div class="content">
        <h1>Labino</h1>
        <p>File deleted</p>
    </div>
    <script>
        setTimeout(() => { history.back(); }, 2000);
    </script>
</body>
</html>)rawstring";

String indexProcessor(const String &value) {
    if (value == "INTERVAL")
        return String(readInterval);
    return String();
}

void setup() {
    Serial.begin(115200);
    BeginFS(true);
    if (!EEPROM.begin(EEPROM_SIZE)) {
        Serial.println("failed to initialise EEPROM");
    }

    readInterval = EEPROM.readLong(0);
    
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
        request->send_P(200, "text/html", index_html, indexProcessor);
    });

    server.on("/download", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(FILE_SYSTEM, SAVE_FILENAME, "text/plain", true);
    });

    server.on("/show", HTTP_GET, [](AsyncWebServerRequest *request){
        request->send(FILE_SYSTEM, SAVE_FILENAME, "text/plain");
    });

    server.on("/delete", HTTP_GET, [](AsyncWebServerRequest *request){
        if (DeleteFile()) {
            request->send_P(200, "text/html", download_html);
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
    server.on("/config", HTTP_GET, [](AsyncWebServerRequest *request){
        if (request->hasParam(READ_INTERVAL_QUERYPARAM)) {
            int newInterval = request->getParam(READ_INTERVAL_QUERYPARAM)->value().toInt();
            if (newInterval > 0) {
                readInterval = max(newInterval, MIN_READ_INTERVAL);
                EEPROM.writeLong(0, readInterval);
                EEPROM.commit();
                Serial.println(readInterval);
            }
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
