#ifndef SOUND_UTILS_H
#define SOUND_UTILS_H

#include <Arduino.h>
#include <math.h>
#include <ESP_I2S.h>


inline int32_t pcm1808_to_s24(int32_t x) {
  return (x >> 8);
}

// Single-bin Goertzel power at target frequency
float goertzelPower(const float* x, int len, float targetFreq, float fs) {
  float k = 0.5f + ((len * targetFreq) / fs);
  float w = (2.0f * PI * floorf(k)) / len;
  float cosine = cosf(w);
  float coeff = 2.0f * cosine;

  float q0 = 0.0f, q1 = 0.0f, q2 = 0.0f;

  for (int i = 0; i < len; i++) {
    q0 = coeff * q1 - q2 + x[i];
    q2 = q1;
    q1 = q0;
  }

  float power = q1 * q1 + q2 * q2 - coeff * q1 * q2;
  return power;
}

// Sum a few nearby Goertzel bins around center frequency
float bandPower(const float* x, int len, float centerFreq, float fs) {
  // Step roughly one FFT-bin equivalent
  float df = fs / len;

  // 5-point sum around center
  float p = 0.0f;
  p += goertzelPower(x, len, centerFreq - 2 * df, fs);
  p += goertzelPower(x, len, centerFreq - 1 * df, fs);
  p += goertzelPower(x, len, centerFreq,          fs);
  p += goertzelPower(x, len, centerFreq + 1 * df, fs);
  p += goertzelPower(x, len, centerFreq + 2 * df, fs);

  return p;
}

float* getSamples(I2SClass* I2S, int32_t* frame, float* samples, int window, int* avg){
    *avg = 0;
    for (int i = 0; i < window; i++) {
        size_t n = I2S->readBytes((char*)frame, sizeof(frame));
        if (n != sizeof(frame)) {
        i--;
        continue;
        }

        int32_t left = pcm1808_to_s24(frame[0]);
        samples[i] = (float)left;
        // Serial.println(samples[i]);
        *avg += abs(samples[i])/window;
    }
    // Serial.println(avg);
    return samples;
}

float* removeDC(float* x, int len) {
  float mean = 0.0f;
  for (int i = 0; i < len; i++) mean += x[i];
  mean /= len;
  for (int i = 0; i < len; i++) x[i] -= mean;
  return x;
}

float* hannWindow(float* x, int len) {
  for (int i = 0; i < len; i++) {
    float w = 0.5f * (1.0f - cosf((2.0f * PI * i) / (len - 1)));
    x[i] *= w;
  }
    return x;
}

float* computeBands(float* levels, float* samples, int Nbands, int window, const float* bandcenters, int FS, const float* biasBand, float biasAll, float ampAll){
    for (int b = 0; b < Nbands; b++) {
        float p = bandPower(samples, window, bandcenters[b], FS);

        // log-compress for easier display / plotting
        float level = 10.0f * log10f(p + 1.0f) - biasBand[b];
        // Serial.print(level);
        // Serial.print(" ");
        levels[b] = (level-biasAll)*ampAll;
  }
    // Serial.println();
    return levels;
}

void beginPCM(I2SClass* I2S, uint8_t BCK, uint8_t WS, uint8_t DIN, uint8_t MCK, uint32_t FS) {
  I2S->setPins(BCK, WS, -1, DIN, MCK);

  if (!I2S->begin(I2S_MODE_STD, FS, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("PCM begin failed");
    while (1) delay(1000);
  }

  Serial.println("PCM1808 Initialized");
}

void beginMIC(I2SClass* I2S, uint8_t BCK, uint8_t WS, uint8_t DIN, uint32_t FS) {
  I2S->setPins(BCK, WS, -1, DIN, -1);

  if (!I2S->begin(I2S_MODE_STD, FS, I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_STEREO)) {
    Serial.println("MIC begin failed");
    while (1) delay(1000);
  }

  Serial.println("SPH0645 Initialized");
}

void update_channel(float* dlevels, float* slevels, float decay){
  for (int i=0; i<13; i+=2){
    if (slevels[i/2] >= dlevels[(i)]){
      dlevels[(i)] = slevels[i/2];
    }
    else{
      dlevels[(i)] -= decay;
    }
  }
  // Serial.println();
  for (int i=1; i<13; i+=2){
    dlevels[(i)] = (dlevels[i-1]+dlevels[(i+1)])/2;
  }
}

#endif