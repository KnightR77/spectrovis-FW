#ifndef DISP_UTILS_H
#define DISP_UTILS_H

#include <Arduino.h>
#include <Adafruit_NeoPixel.h>
#include <font.h>

class Matrix13x5 {
public:
  Matrix13x5(Adafruit_NeoPixel& s) : leds(s) {}

  void begin() {
    leds.begin();
    leds.setBrightness(30);
    leds.clear();
    leds.show();
  }

  void clear() {
    leds.clear();
    // leds.show();
  }

  void setBrightness(uint8_t b) {
    leds.setBrightness(b);
    brightness = b;
  }

  void setAutoDim(bool autoDim) {
    this->autoDim = autoDim;
  }

  void setColorsV(uint32_t* colorsV, uint32_t* colorsT = nullptr) {
    for (int i = 0; i < W; i++) {
      colColorsV[i] = colorsV[i];
    }
  }

  void setColorsT(uint32_t* colorsT, uint32_t* colorsV = nullptr) {
    for (int i = 0; i < W; i++) {
      colColorsT[i] = colorsT[i];
    }
   }

  int xyToIndex(int x, int y) {
    if (x < 0 || x >= W || y < 0 || y >= H) return -1;
    return x * H + y;
  }

  void setPixelXY(int x, int y, uint32_t color) {
    int idx = xyToIndex(x, y);
    if (idx >= 0) leds.setPixelColor(idx, color);
  }

  uint32_t colorForColumnV(int x) {
    if (x < 0 || x >= W) return 0;
    return colColorsV[x];
  }
  
  uint32_t colorForColumnT(int x) {
    if (x < 0 || x >= W) return 0;
    return colColorsT[x];
  }

  // Draw one 3x5 digit at xcoord (leftmost column)
  void printNum(int num, int xcoord) {
    if (num < 0 || num > 9) return;

    for (int rowTop = 0; rowTop < 5; rowTop++) {
      uint8_t bits = DIGIT_3X5[num][rowTop];

      for (int dx = 0; dx < 3; dx++) {
        bool on = bits & (1 << (2 - dx));
        if (!on) continue;

        int x = xcoord + dx;
        int y = 4 - rowTop;  // convert top->bottom font row to bottom->top display row
        setPixelXY(x, y, colorForColumnT(x));
        // Serial.println("printed in color: " + String(colorForColumnT(x), HEX));
      }
    }
  }

  // center colon on one column
  void drawColon(int xcoord, bool on = true) {
    if (!on) return;
    // two dots on a 5-high display
    setPixelXY(xcoord, 1, colorForColumnT(xcoord));
    setPixelXY(xcoord, 3, colorForColumnT(xcoord));
  }

  // Layout uses exactly 13 columns:
  // Htens: 0..2
  // Hones: 3..5
  // colon: 6
  // Mtens: 7..9
  // Mones: 10..12
  void drawTimeHHMM(int hour, int minute, bool colonOn = true) {
    clear();

    int hTens = hour / 10;
    int hOnes = hour % 10;
    int mTens = minute / 10;
    int mOnes = minute % 10;

    if (autoDim) {
      if (hour >= 7 && hour < 22) {
        setBrightness(brightness);
      } else {
        if (brightness > 5) {
          setBrightness(5);
        } else {
           setBrightness(brightness);
        }
      }
    } else {
      setBrightness(brightness);
    }

    printNum(hTens, 0);
    printNum(hOnes, 3);
    drawColon(6, colonOn);
    printNum(mTens, 7);
    printNum(mOnes, 10);

    show();
  }

  void drawImage(const uint8_t h, const uint8_t w, uint8_t* image, int xOffset = 0, int yOffset = 0, uint32_t color = 0xFFFFFF) {
    for (int row = 0; row < h; row++) {
      uint8_t bits = image[row];
      for (int col = 0; col < w; col++) {
        bool on = bits & (1 << (w - 1 - col));
        if (!on) continue;

        int x = xOffset + col;
        int y = yOffset + (h - 1 - row);  // convert top->bottom image row to bottom->top display row
        setPixelXY(x, y, color);
      }
    }
  }

  void drawColBar(int col, int height) {
    if (height > H){
        height = H;
    }
    for (int y = 0; y < height; y++) {
      setPixelXY(col, y, colorForColumnV(col));
    }
    for (int y = height; y < H; y++) {
      setPixelXY(col, y, 0);
    }
  }

  void refreshBands(float* disp_levels){
    for (int col = 0; col < W; col++) {
      drawColBar(col, disp_levels[col]);
    }
  }

  void show() {
    leds.show();
  }


private:
  Adafruit_NeoPixel& leds;
  uint8_t H = 5;
  uint8_t W = 13;
  uint32_t colColorsV[13]= {
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
    0xFFFFFF,
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF
  };
  uint32_t colColorsT[13]= {
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
    0xFFFFFF,
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF,
    0xFFFFFF, 0xFFFFFF, 0xFFFFFF
  };
  uint8_t autoDim = 0;
  uint8_t brightness = 50;
};

#endif