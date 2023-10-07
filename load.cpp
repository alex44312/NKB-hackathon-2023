#include "DHTesp.h"
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <WiFi.h>
#include <HTTPClient.h>

// DHT11 Configuration
DHTesp dht;
const int DHT_PIN = 4;

// BLE Configuration
bool foundMaster = false;
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
        if (advertisedDevice.getName() == "ESP32_MASTER") {
            foundMaster = true;
        }
    }
};

// WiFi Configuration
const char* ssid = "ESP32_MASTER";
const char* password = "masterpassword";

// Server Configuration
const char* serverURL = "http://your-server.com/data";

// ESP Data Structure and Array
struct ESPData {
    String espID;
    float temperature;
    float humidity;
};

const int maxESPs = 10;
ESPData espDataArray[maxESPs];

void updateData(String id, float temp, float hum) {
    for (int i = 0; i < maxESPs; i++) {
        if (espDataArray[i].espID == id || espDataArray[i].espID == "") {
            espDataArray[i].espID = id;
            espDataArray[i].temperature = temp;
            espDataArray[i].humidity = hum;
            break;
        }
    }
}

void setup() {
    Serial.begin(115200);

    // Initialize DHT11
    dht.setup(DHT_PIN, DHTesp::DHT11);

    // Start in BLE mode
    BLEDevice::init("ESP32_SLAVE");
    BLEScan* pBLEScan = BLEDevice::getScan();
    pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
    pBLEScan->setInterval(1349);
    pBLEScan->setWindow(449);
    pBLEScan->start(10, false);  // Scan for 10 seconds

    if (foundMaster) {
        // Found a master, connect to it
        WiFi.begin(ssid, password);
        while (WiFi.status() != WL_CONNECTED) {
            delay(1000);
            Serial.println("Connecting to Master ESP32...");
        }
        Serial.println("Connected to Master ESP32");
    } else {
        // Become the master
        WiFi.softAP(ssid, password);
        Serial.println("Started as Master ESP32");
    }

    // Initialize the ESPData array
    for (int i = 0; i < maxESPs; i++) {
        espDataArray[i].espID = "";
        espDataArray[i].temperature = 0.0;
        espDataArray[i].humidity = 0.0;
    }
}

void loop() {
    // Read data from DHT11
    float humidity = dht.getHumidity();
    float temperature = dht.getTemperature();
    Serial.println("Humidity: " + String(humidity) + "%");
    Serial.println("Temperature: " + String(temperature) + "°C");

    // Update own data
    updateData("ESP_SELF", temperature, humidity);

    // Send data to server if connected to WiFi
    if (WiFi.status() == WL_CONNECTED) {
        sendData(temperature, humidity);
    }

    delay(10000);  // Delay for 10 seconds before next loop
}

void sendData(float temperature, float humidity) {
    HTTPClient http;
    http.begin(serverURL);
    http.addHeader("Content-Type", "application/x-www-form-urlencoded");

    String postData = "temperature=" + String(temperature) + "&humidity=" + String(humidity);
    int httpResponseCode = http.POST(postData);

    if (httpResponseCode > 0) {
        String response = http.getString();
        Serial.println(response);
    } else {
        Serial.println("Error on sending data");
    }

    http.end();
} 
