#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <time.h>

// struct TimeInfo{
//   long TZ;
//   int DLS;
// };

extern IPAddress local_ip;
extern IPAddress gateway;
extern IPAddress subnet;

void scanWiFi() {
  Serial.println("Scanning for WiFi networks...");
  int n = WiFi.scanNetworks();
  if (n == 0) {
    Serial.println("No networks found");
  } else {
    Serial.println("Networks found:");
    for (int i = 0; i < n; i++) {
      Serial.print(i + 1);
      Serial.print(": ");
      Serial.print(WiFi.SSID(i));
      Serial.print(" (");
      Serial.print(WiFi.RSSI(i));
      Serial.println(" dBm)");
    }
  }
}

void connectWiFi(const char* ssid, const char* password) {
    Serial.print("Connecting to WiFi network: ");
    Serial.println(ssid);

    WiFi.begin(ssid, password);

    int maxRetries = 20;
    int retryCount = 0;
    while (WiFi.status() != WL_CONNECTED && retryCount < maxRetries) {
        delay(500);
        Serial.print(".");
        retryCount++;
    }

    if (WiFi.status() == WL_CONNECTED) {
        Serial.println("\nWiFi connected successfully!");
        Serial.print("IP address: ");
        Serial.println(WiFi.localIP());
    } else {
        Serial.println("\nFailed to connect to WiFi.");
    }
}

void enableAP(const char* ssid, const char* password) {
    Serial.print("Enabling Access Point: ");
    Serial.println(ssid);

    WiFi.softAPConfig(local_ip, gateway, subnet);
    WiFi.softAP(ssid, password);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());

}

// void getTime(void *pvParameters) {
void getTime(const long TZ, const int DLS) {
    // TimeInfo *p = (TimeInfo *)pvParameters;
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Not connected to WiFi. Cannot get time.");
        return;
    }
    // configTime(p->TZ, p->DLS, "pool.ntp.org", "time.nist.gov");
    configTime(TZ, DLS, "pool.ntp.org", "time.nist.gov");
    // Serial.println("Waiting for time synchronization...");
    // while (time(nullptr) < 8 * 3600 * 2) {
    //     vTaskDelay(pdMS_TO_TICKS(500));
    //     Serial.print(".");
    // }
    // Serial.println("\nTime synchronized!");
}

#endif