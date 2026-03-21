#include <WiFi.h>
#include <Adafruit_NeoPixel.h>
#include <time.h>
#include <math.h>
#include <ESP_I2S.h>
#include <SPIFFS.h>
#include <WebServer.h>
// #include <LittleFS.h>

#include "disp_utils.h"
#include "sound_utils.h"
// #include "wifi_utils.h"
#include "web_utils.h"
#include "colors.h"
#include "images.h"

IPAddress local_ip(192,168,4,1);
IPAddress gateway(192,168,4,1);
IPAddress subnet(255,255,255,0);

uint8_t mode = 0; // 0=clock, 1=PCM, 2=MIC
uint8_t prevMode = 2;

static const int PIN_BCK  = 27;
static const int PIN_WS   = 25;
static const int PIN_DPCM  = 34;
static const int PIN_DMIC = 35;
static const int PIN_MCLK = 0; 
static const int PIN_LED = 33;
static const int PIN_BTN = 32;

static const int NUM_LEDS = 65; 

static const int NUM_BANDS = 7;
static const int SAMPLE_RATE = 48000;
static const int N = 1024;
// static const float DECAY1 = 0.2;
// static const float DECAY2 = 0.5;

long  GMT_OFFSET_SEC      =  8 * 3600;
int   DAYLIGHT_OFFSET_SEC =  0 * 3600;   // set 0 if you do not want DST

// TimeInfo timeinfo = {GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC};

char* SSID;
char* passwd;

static const char* ap_ssid = "SpectroVis";
static const char* ap_passwd = "1145141919810";

const float centers[NUM_BANDS] = {
 63.0f, 160.0f, 400.0f, 1000.0f, 2500.0f, 6250.0f, 16000.0f
};

const float bias[NUM_BANDS] = {
  115.0f, 115.0f, 105.0f, 95.0f, 90.0f, 85.0f, 85.0f
};

// const float biasPCM_TABLE[5] = {20.0f, 25.0f, 30.0f, 35.0f, 40.0f};
const float biasPCM_TABLE[5] = {40.0f, 35.0f, 30.0f, 25.0f, 20.0f};
const float ampPCM_TABLE[5] = {0.10f, 0.12f, 0.15f, 0.17f, 0.20f};
const float decayPCM_TABLE[5] = {0.1f, 0.15f, 0.2f, 0.25f, 0.3f};

const float biasMIC_TABLE[5] = {20.0f, 15.0f, 10.0f, 5.0f, 0.0f};
// const float biasMIC_TABLE[5] = {0.0f, 5.0f, 10.0f, 15.0f, 20.0f};
const float ampMIC_TABLE[5] = {0.15f, 0.20f, 0.25f, 0.30f, 0.35f};
const float decayMIC_TABLE[5] = {0.3f, 0.4f, 0.5f, 0.6f, 0.7f};

float biasPCM = 30;
float ampPCM = 0.15f;
float decayPCM = 0.2f;

float biasMIC = 5;
float ampMIC = 0.3f;
float decayMIC = 0.5f;


int colorScheme = 1;
int brightness = 50;
int autoDim = 0;

int32_t frame[2];
float samples[N];

float levels[7];
float disp_level[13] = {0,0,0,0,0,0,0,0,0,0,0,0,0};

I2SClass I2S;
Adafruit_NeoPixel strip(NUM_LEDS, PIN_LED, NEO_GRB + NEO_KHZ800);
Matrix13x5 disp(strip);
AppConfig appConfig;

void buttonISR() {
  mode = (mode + 1) % 3;
}

void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  delay(500);

  if (!SPIFFS.begin(true)) {
    Serial.println("SPIFFS mount failed");
  }

  WiFi.mode(WIFI_AP_STA);
  enableAP(ap_ssid, ap_passwd);

  disp.begin();

  loadConfigFromSpiffs();
  applyLoadedConfig();
  setupWebConfig();

  // beginPCM(&I2S, PIN_BCK, PIN_WS, PIN_DPCM, PIN_MCLK, SAMPLE_RATE);
  beginMIC(&I2S, PIN_BCK, PIN_WS, PIN_DMIC, SAMPLE_RATE);
  pinMode(PIN_BTN, INPUT);
  attachInterrupt(digitalPinToInterrupt(PIN_BTN), buttonISR, FALLING);

  // disp.setColors(vColors1);

  // connectWiFi(appConfig.ssid.c_str(), appConfig.password.c_str());
  // connectWiFi(SSID, passwd);

  getTime(GMT_OFFSET_SEC, DAYLIGHT_OFFSET_SEC);
  // xTaskCreate(getTime, "gettime", 4096, &timeinfo, 1, NULL);
}

void loop() {
  // put your main code here, to run repeatedly:
  // Serial.print("Curr Mode: ");
  // Serial.println(mode);
  handleWebServer();
  if (mode == 0){
    clock_tick();
    delay(1000);
  }
  else if (mode == 1){
    PCM_tick();
    delay(10);
  }
  else if (mode == 2){
    MIC_tick();
    delay(10);
  }
}

void PCM_tick(){
  if (prevMode != mode){
    prevMode = mode;
    Serial.println("Switching to PCM mode");
    // disp.setColors(vColors2);
    disp.clear();
    disp.show();
    beginPCM(&I2S, PIN_BCK, PIN_WS, PIN_DPCM, PIN_MCLK, SAMPLE_RATE);
  }
  getSamples(&I2S, frame, samples, N);
  removeDC(samples, N);
  hannWindow(samples, N);
  computeBands(levels, samples, NUM_BANDS, N, centers, SAMPLE_RATE, bias, biasPCM, ampPCM);
  update_channel(disp_level, levels, decayPCM);
  // for (int i=0; i<13; i++){
  //   Serial.print(disp_level[i]);
  //   Serial.print(" ");
  // }
  // Serial.println();
  disp.refreshBands(disp_level);
  disp.show();
  // Serial.println(biasPCM);
  // Serial.println(ampPCM);
  // Serial.println(decayPCM);
}

void MIC_tick(){
  if (prevMode != mode){
    prevMode = mode;
    Serial.println("Switching to MIC mode");
    // disp.setColors(vColors2);
    disp.clear();
    disp.show();
    beginMIC(&I2S, PIN_BCK, PIN_WS, PIN_DMIC, SAMPLE_RATE);
  }
  // Serial.println("MIC ticks");
  getSamples(&I2S, frame, samples, N);
  removeDC(samples, N);
  hannWindow(samples, N);
  computeBands(levels, samples, NUM_BANDS, N, centers, SAMPLE_RATE, bias, biasMIC, ampMIC);
  update_channel(disp_level, levels, decayMIC);
  disp.refreshBands(disp_level);
  disp.show();
  // Serial.println(biasMIC);
  // Serial.println(ampMIC);
}

void clock_tick(){
  if (prevMode != mode){
    prevMode = mode;
    Serial.println("Switching to clock mode");
    // disp.setColors(tColors2);
    disp.clear();
    disp.show();
  }
  // Serial.println("Clock ticks");
  time_t now = time(nullptr);
  Serial.println("Current time: " + String(ctime(&now)));
  struct tm* p_tm = localtime(&now);
  int hour = p_tm->tm_hour;
  int minute = p_tm->tm_min;

  disp.drawTimeHHMM(hour, minute,(p_tm->tm_sec % 2) == 0);
  disp.show();
}