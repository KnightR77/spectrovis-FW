#ifndef WEB_UTILS_H
#define WEB_UTILS_H

#include <Arduino.h>
#include <WiFi.h>
#include <WebServer.h>
#include <SPIFFS.h>
#include <Update.h>

#include "wifi_utils.h"
#include "colors.h"
#include "disp_utils.h"

static const char* CONFIG_PATH = "/config.txt";
static const char* HTML_PATH   = "/index.html";

struct AppConfig {
  String ssid;
  String password;
  int timezone;     // -11..11 hours
  int dst;          // 0/1 hours
  int colorScheme;  // 1..5
  int ampLevelPCM;     // -2..2
  int biasLevelPCM;    // -2..2
  int ampLevelMIC;     // -2..2
  int biasLevelMIC;    // -2..2
  int decayPCM;       // -2..2
  int decayMIC;       // -2..2
  int brightness;   // 0..100
  int autoDim;      // 0/1
  int mode;  // 0=clock, 1=PCM, 2=MIC
};

extern Matrix13x5 disp;
extern float biasPCM;
extern float ampPCM;
extern float biasMIC;
extern float ampMIC;
extern float decayPCM;
extern float decayMIC;
extern float disp_level[13];
extern uint8_t mode;

extern const float biasPCM_TABLE[5];
extern const float ampPCM_TABLE[5];
extern const float biasMIC_TABLE[5];
extern const float ampMIC_TABLE[5];
extern const float decayPCM_TABLE[5];
extern const float decayMIC_TABLE[5];

extern int brightness;
extern int autoDim;

extern AppConfig appConfig;
WebServer server(80);

extern const char* CONFIG_PATH;
extern const char* HTML_PATH;

static uint32_t* getSchemePtrV(int scheme) {
  switch (scheme) {
    case 1: return vColors1;
    case 2: return vColors2;
    case 3: return vColors3;
    case 4: return vColors4;
    case 5: return vColors5;
    default: return vColors1;
  }
}

static uint32_t* getSchemePtrT(int scheme) {
  switch (scheme) {
    case 1: return tColors1;
    case 2: return tColors2;
    case 3: return tColors3;
    case 4: return tColors4;
    case 5: return tColors5;
    default: return tColors1;
  }

}

static int clampInt(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static String getKV(const String& text, const char* key, const String& def = "") {
  String prefix = String(key) + "=";
  int start = text.indexOf(prefix);
  if (start < 0) return def;
  start += prefix.length();
  int end = text.indexOf('\n', start);
  if (end < 0) end = text.length();
  String val = text.substring(start, end);
  val.trim();
  return val;
}

bool saveConfigToSpiffs() {
  File f = SPIFFS.open(CONFIG_PATH, FILE_WRITE);
  if (!f) return false;
  f.printf("ssid=%s\n", appConfig.ssid.c_str());
  f.printf("password=%s\n", appConfig.password.c_str());
  f.printf("timezone=%d\n", appConfig.timezone);
  f.printf("dst=%d\n", appConfig.dst);
  f.printf("colorScheme=%d\n", appConfig.colorScheme);
  f.printf("ampLevelPCM=%d\n", appConfig.ampLevelPCM);
  f.printf("biasLevelPCM=%d\n", appConfig.biasLevelPCM);
  f.printf("ampLevelMIC=%d\n", appConfig.ampLevelMIC);
  f.printf("biasLevelMIC=%d\n", appConfig.biasLevelMIC);
  f.printf("decayPCM=%d\n", appConfig.decayPCM);
  f.printf("decayMIC=%d\n", appConfig.decayMIC);
  f.printf("brightness=%d\n", appConfig.brightness);
  f.printf("autoDim=%d\n", appConfig.autoDim);
  f.printf("mode=%d\n", appConfig.mode);
  f.close();
  return true;
}

void ensureDefaultFiles() {
  if (!SPIFFS.exists(CONFIG_PATH)) {
    appConfig.ssid = "";
    appConfig.password = "";
    appConfig.timezone = 0;
    appConfig.dst = 0;
    appConfig.colorScheme = 1;
    appConfig.ampLevelPCM = 0;
    appConfig.biasLevelPCM = 0;
    appConfig.ampLevelMIC = 0;
    appConfig.biasLevelMIC = 0;
    appConfig.brightness = 100;
    appConfig.autoDim = 0;
    appConfig.decayPCM = 0;
    appConfig.decayMIC = 0;
    appConfig.mode = 0;
    saveConfigToSpiffs();
  }
}

bool loadConfigFromSpiffs() {
  ensureDefaultFiles();
  File f = SPIFFS.open(CONFIG_PATH, FILE_READ);
  if (!f) return false;
  String text = f.readString();
  f.close();

  appConfig.ssid        = getKV(text, "ssid", "");
  appConfig.password    = getKV(text, "password", "");
  appConfig.timezone    = clampInt(getKV(text, "timezone", "0").toInt(), -11, 11);
  appConfig.dst         = clampInt(getKV(text, "dst", "0").toInt(), 0, 1);
  appConfig.colorScheme = clampInt(getKV(text, "colorScheme", "1").toInt(), 1, 5);
  appConfig.ampLevelPCM    = clampInt(getKV(text, "ampLevelPCM", "0").toInt(), -2, 2);
  appConfig.biasLevelPCM   = clampInt(getKV(text, "biasLevelPCM", "0").toInt(), -2, 2);
  appConfig.ampLevelMIC    = clampInt(getKV(text, "ampLevelMIC", "0").toInt(), -2, 2);
  appConfig.biasLevelMIC   = clampInt(getKV(text, "biasLevelMIC", "0").toInt(), -2, 2);
  appConfig.decayPCM       = clampInt(getKV(text, "decayPCM", "0").toInt(), -2, 2);
  appConfig.decayMIC       = clampInt(getKV(text, "decayMIC", "0").toInt(), -2, 2);
  appConfig.brightness     = clampInt(getKV(text, "brightness", "100").toInt(), 0, 100);
  appConfig.autoDim        = clampInt(getKV(text, "autoDim", "0").toInt(), 0, 1);
  appConfig.mode          = clampInt(getKV(text, "mode", "0").toInt(), 0, 2);
  Serial.println("Config loaded:");
  Serial.println("SSID: " + appConfig.ssid);
  Serial.println("Password: " + appConfig.password);
  Serial.println("Timezone: " + String(appConfig.timezone));
  Serial.println("DST: " + String(appConfig.dst));
  Serial.println("Color Scheme: " + String(appConfig.colorScheme));
  Serial.println("Decay PCM: " + String(appConfig.decayPCM));
  Serial.println("Decay MIC: " + String(appConfig.decayMIC));
  Serial.println("Amp Level PCM: " + String(appConfig.ampLevelPCM));
  Serial.println("Bias Level PCM: " + String(appConfig.biasLevelPCM));
  Serial.println("Amp Level MIC: " + String(appConfig.ampLevelMIC));
  Serial.println("Bias Level MIC: " + String(appConfig.biasLevelMIC));
  Serial.println("Brightness: " + String(appConfig.brightness));
  Serial.println("Auto Dim: " + String(appConfig.autoDim));
  Serial.println("Mode: " + String(appConfig.mode));
  return true;
}

void applyWifiConfig() {
  if (appConfig.ssid.length() == 0) return;
  WiFi.begin(appConfig.ssid.c_str(), appConfig.password.c_str());
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("\nWiFi connected!");
}

void applyMode() {
  mode = appConfig.mode;
}

void applyTimeConfig() {
  long tzSec = appConfig.timezone * 3600L;
  int dstSec = appConfig.dst ? 3600 : 0;
  getTime(tzSec, dstSec);
}

void applyDispConfig() {
  // disp.setColors(getSchemePtrV(appConfig.colorScheme));
  disp.setColorsV(getSchemePtrV(appConfig.colorScheme));
  disp.setColorsT(getSchemePtrT(appConfig.colorScheme));
  // disp.brightness = appConfig.brightness;
  // disp.autoDim = appConfig.autoDim;
    disp.setBrightness(appConfig.brightness);
  disp.setAutoDim(appConfig.autoDim);
  decayPCM = decayPCM_TABLE[appConfig.decayPCM + 2];
  decayMIC = decayMIC_TABLE[appConfig.decayMIC + 2];
  disp.show();
}

void applyGainConfig() {
  // int ampIdx = appConfig.ampLevel + 2;
  // int biasIdx = appConfig.biasLevel + 2;
  int ampIdxPCM = appConfig.ampLevelPCM + 2;
  int biasIdxPCM = appConfig.biasLevelPCM + 2;
  int ampIdxMIC = appConfig.ampLevelMIC + 2;
  int biasIdxMIC = appConfig.biasLevelMIC + 2;
  ampPCM = ampPCM_TABLE[ampIdxPCM];
  ampMIC = ampMIC_TABLE[ampIdxMIC];
  biasPCM = biasPCM_TABLE[biasIdxPCM];
  biasMIC = biasMIC_TABLE[biasIdxMIC];

  for (int i = 0; i < 13; i++) disp_level[i] = 0;
}

void applyLoadedConfig() {
  applyDispConfig();
  applyGainConfig();
  applyWifiConfig();
  applyTimeConfig();
  applyMode();
}


static void sendConfigJson() {
  String json = "{";
  json += "\"ssid\":\"" + appConfig.ssid + "\",";
  json += "\"timezone\":" + String(appConfig.timezone) + ",";
  json += "\"dst\":" + String(appConfig.dst) + ",";
  json += "\"colorScheme\":" + String(appConfig.colorScheme) + ",";
  json += "\"brightness\":" + String(appConfig.brightness) + ",";
  json += "\"autoDim\":" + String(appConfig.autoDim) + ",";
  json += "\"ampLevelPCM\":" + String(appConfig.ampLevelPCM) + ",";
  json += "\"biasLevelPCM\":" + String(appConfig.biasLevelPCM) + ",";
  json += "\"ampLevelMIC\":" + String(appConfig.ampLevelMIC) + ",";
  json += "\"biasLevelMIC\":" + String(appConfig.biasLevelMIC) + ",";
  json += "\"decayPCM\":" + String(appConfig.decayPCM) + ",";
  json += "\"decayMIC\":" + String(appConfig.decayMIC) + ",";
  json += "\"mode\":" + String(appConfig.mode);
  json += "}";
  server.send(200, "application/json", json);
}

static void handleScan() {
  int n = WiFi.scanNetworks();
  String json = "[";
  for (int i = 0; i < n; i++) {
    if (i) json += ",";
    String s = WiFi.SSID(i);
    s.replace("\\", "\\\\");
    s.replace("\"", "\\\"");
    json += "{\"ssid\":\"" + s + "\",\"rssi\":" + String(WiFi.RSSI(i)) + "}";
  }
  json += "]";
  server.send(200, "application/json", json);
}

static void handleSaveMode() {
  appConfig.mode = clampInt(server.arg("mode").toInt(), 0, 2);
  saveConfigToSpiffs();
  applyMode();
  Serial.println("Mode set to: " + server.arg("mode"));
  server.send(200, "text/plain", "OK");
}


static void handleSaveWifi() {
  appConfig.ssid = server.arg("ssid");
  appConfig.password = server.arg("password");
  saveConfigToSpiffs();
  applyWifiConfig();
  server.send(200, "text/plain", "OK");
}

static void handleSaveTime() {
  appConfig.timezone = clampInt(server.arg("timezone").toInt(), -11, 11);
  appConfig.dst = clampInt(server.arg("dst").toInt(), 0, 1);
  saveConfigToSpiffs();
  applyTimeConfig();
  server.send(200, "text/plain", "OK");
}

static void handleSaveDisp() {
  appConfig.colorScheme = clampInt(server.arg("scheme").toInt(), 1, 5);
  appConfig.brightness = clampInt(server.arg("brightness").toInt(), 0, 100);
  appConfig.autoDim = clampInt(server.arg("autoDim").toInt(), 0, 1);
  appConfig.decayPCM = clampInt(server.arg("decayPCM").toInt(), -2, 2);
  appConfig.decayMIC = clampInt(server.arg("decayMIC").toInt(), -2, 2);
  saveConfigToSpiffs();
  applyDispConfig();
  server.send(200, "text/plain", "OK");
}

static void handleSaveGain() {
  appConfig.ampLevelPCM = clampInt(server.arg("ampLevelPCM").toInt(), -2, 2);
  appConfig.biasLevelPCM = clampInt(server.arg("biasLevelPCM").toInt(), -2, 2);
  appConfig.ampLevelMIC = clampInt(server.arg("ampLevelMIC").toInt(), -2, 2);
  appConfig.biasLevelMIC = clampInt(server.arg("biasLevelMIC").toInt(), -2, 2);
  saveConfigToSpiffs();
  applyGainConfig();
  server.send(200, "text/plain", "OK");
}

static void handleFirmwareUpload() {
  HTTPUpload& upload = server.upload();
  if (upload.status == UPLOAD_FILE_START) {
    Serial.println("Firmware upload started");
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Serial.println("Failed to start update");
    }
  } else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println("Failed to write firmware chunk");
    }
  } else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.println("Firmware update successful, rebooting...");
      ESP.restart();
    } else {
      Serial.println("Firmware update failed");
    }
  }
}

void setupWebConfig() {
  server.on("/", HTTP_GET, []() {
    File f = SPIFFS.open(HTML_PATH, FILE_READ);
    if (!f) {
      server.send(404, "text/plain", "index.html missing");
      return;
    }
    server.streamFile(f, "text/html");
    f.close();
  });

  server.on("/api/config", HTTP_GET, sendConfigJson);
  server.on("/api/scan", HTTP_GET, handleScan);
  server.on("/save/mode", HTTP_POST, handleSaveMode);
  server.on("/save/wifi", HTTP_POST, handleSaveWifi);
  server.on("/save/time", HTTP_POST, handleSaveTime);
  server.on("/save/disp", HTTP_POST, handleSaveDisp);
  server.on("/save/gain", HTTP_POST, handleSaveGain);
  server.begin();
  server.on("/api/update", HTTP_POST, []() {
    server.send(200, "text/plain", "OK");
  }, handleFirmwareUpload);

}

void handleWebServer() {
  server.handleClient();
}

#endif