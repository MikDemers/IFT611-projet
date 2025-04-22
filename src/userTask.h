#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <commonDef.h>
#include "Screens.h"
#include <dsps_fft2r.h>
#include <utils.h>
#include <chrono>


// -=- Variables -=-

extern short state;
extern AudioModule audioMod;

// Screen and user interactions
Adafruit_SSD1306 display(128, 64, &Wire, -1);
MainScreen* mainScreen = nullptr;
SettingsScreen* settingsScreen = nullptr;
unsigned long lastButtonTime = 0;
int cwCount = 0;
int ccwCount = 0;
uint8_t lastAB = 0;

// FFT
bool dsps_initialized = false;
float fft_buffer[2 * BufferLen];
float bands_buffer[NB_BANDS];
std::pair<double, double> bandsRange[NB_BANDS]{
    {20, 100},      // 50 Hz (Bass)
    {100, 250},     // 150 Hz (Low-mids)
    {250, 650},     // 400 Hz (Mids)
    {650, 1800},    // 1000 Hz (High-mids)
    {1800, 5000},   // 3000 Hz (Presence)
    {5000, 15000}   // 8000 Hz (Brilliance)
};


// -=- Declarations -=-

void userSetup();
void userLoop();

void FFTLoop();
void buttonLoop();
void encoderLoop();
void displayLoop();

void onEncoderChange();


// -=- Definitions -=-

void userTask(void*) {
  while(true) {
    userLoop();
  }
}

void userSetup() {
  // Initialisation de l'écran OLED
  Wire.begin(SDA, SCL);
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erreur d'initialisation de l'écran OLED");
    while (1) { /* Blocage en cas d'erreur */ }
  }
  display.clearDisplay();
  display.display();
  mainScreen = new MainScreen{ display };
  settingsScreen = new SettingsScreen{ display }; 

  // Initialisation des boutons + potentiomètre
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  pinMode(POTENTIO_RIGHT, INPUT);
  pinMode(POTENTIO_LEFT, INPUT);
  attachInterrupt(digitalPinToInterrupt(POTENTIO_RIGHT), onEncoderChange ,CHANGE);
  attachInterrupt(digitalPinToInterrupt(POTENTIO_LEFT), onEncoderChange ,CHANGE);

  // Initialisation du module de FFT
  dsps_fft2r_init_fc32(NULL, BufferLen);
}

void userLoop() {
  static TickType_t lastWakeTime = xTaskGetTickCount();

  auto t0 = std::chrono::high_resolution_clock::now();
  FFTLoop();
  auto t1 = std::chrono::high_resolution_clock::now();
  auto t_fft = t1 - t0;
  encoderLoop();
  auto t2 = std::chrono::high_resolution_clock::now();
  auto t_encoder = t2 - t1;
  buttonLoop();
  auto t3 = std::chrono::high_resolution_clock::now();
  auto t_button = t3 - t2;
  displayLoop();
  auto t4 = std::chrono::high_resolution_clock::now();
  auto t_screen = t4-t3;

  /* Serial.print("FFT loop: ");
  Serial.print(std::chrono::duration_cast<std::chrono::microseconds>(t_fft).count());
  Serial.print(" | Encoder loop: ");
  Serial.print(std::chrono::duration_cast<std::chrono::microseconds>(t_encoder).count());
  Serial.print(" | Button loop: ");
  Serial.print(std::chrono::duration_cast<std::chrono::microseconds>(t_button).count());
  Serial.print(" | Screen loop: ");
  Serial.print(std::chrono::duration_cast<std::chrono::microseconds>(t_screen).count());
  Serial.println("\n"); */

  // 1 rafraichissement par 50 ms. => 20 hz
  vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(50));
}

void displayLoop() {
  switch (state)
  {
  case State::VISUAL:
    audioMod.masterGain = 8.0f;  
    mainScreen->update();
    mainScreen->draw();
    mainScreen->refreshDisplay();
    break;
  case State::MODIF:
    settingsScreen->update();
    settingsScreen->draw();
    settingsScreen->refreshDisplay();
    break;
  case State::AUDIO:
    audioMod.masterGain = 1.5f;
    break;
  default:
    break;
  }
}

void FFTLoop() {
  while (outProcessingBufferMutex.test_and_set(std::memory_order_acquire)) { ; }

  int samplesRead = nbSamplesRead;

  // Le standard pour ESP-DSP est [real_1, imag_1, real_2, imag_2, ...]
  // On ne fait que la partie réelle avant de libérer la ressource.
  for (int i = 0; i != samplesRead; ++i) {
      fft_buffer[i * 2] = ((float)outProcessingBuffer[i] / MAX_INT16F);
  }
  
  outProcessingBufferMutex.clear(std::memory_order_release);

  // Terminer la partie imaginaire.
  for (int i = 0; i != samplesRead; ++i) {
      fft_buffer[i * 2 + 1] = 0.0f;
  }
  
  // La FFT
  dsps_fft2r_fc32(fft_buffer, BufferLen);
  // "Inversion de bit". Je sais pas vraiment c'est quoi mais c'est recommandé.
  dsps_bit_rev_fc32(fft_buffer, BufferLen);
  
  bandMagnitudes(fft_buffer, samplesRead, bands_buffer, bandsRange);

  for (int i = 0; i != NB_BANDS; ++i) {
      mainScreen->bandLevels[i] = (short) bands_buffer[i];
  }
}

void encoderLoop() {
  int localCW = cwCount;
  int localCCW = ccwCount;
  cwCount = 0;
  ccwCount = 0;

  // Mettre à jour la valeur de la bande courante avec les impulsions du rotary encoder
  if (state == State::MODIF) {
    int bandIndex = settingsScreen->currentSelectedBand;
    if (bandIndex >= 0 && bandIndex < NB_BANDS) {
      for (int i = 0; i < localCW; i++) {
        settingsScreen->bandLevels[bandIndex] = constrain(settingsScreen->bandLevels[bandIndex] + 1, -5, 5);
        audioMod.setBand(bandIndex, (settingsScreen->bandLevels[bandIndex] + 5));
      }
      for (int i = 0; i < localCCW; i++) {
        settingsScreen->bandLevels[bandIndex] = constrain(settingsScreen->bandLevels[bandIndex] - 1, -5, 5);
        audioMod.setBand(bandIndex, (settingsScreen->bandLevels[bandIndex] + 5));
      }
    }
  }
  if (state == State::AUDIO) {
    for (int i = 0; i < localCW; i++) {
      audioMod.masterGain += 0.1;
    }
    for (int i = 0; i < localCCW; i++) {
      audioMod.masterGain -= 0.1;
    }
  }
}


void buttonLoop() {
  unsigned long currentTime = millis();

  // Temps minimum entre pression sur bouton, pour éviter les "double-clic" non-intentionnels.
  if (digitalRead(PUSH_BUTTON) == LOW && (currentTime - lastButtonTime > 300)) {
    lastButtonTime = currentTime;
    if (state != State::MODIF) {
      state = ++state % NB_STATES;
    } else {
      // En mode édition, passer à la prochaine bande si on est pas à la dernière.
      if (settingsScreen->currentSelectedBand == NB_BANDS-1) {
        state = ++state % NB_STATES;
      }
      settingsScreen->currentSelectedBand = ++settingsScreen->currentSelectedBand % NB_BANDS;
    }
  }
}

// Source: https://www.youtube.com/watch?v=9j-y6XlaE80&ab_channel=FriendlyWire
void onEncoderChange() {
  uint8_t currentAB = (digitalRead(POTENTIO_RIGHT) << 1) | digitalRead(POTENTIO_LEFT);
  if (currentAB == lastAB) return;

  // Sens de rotation horaire 00 → 01 → 11 → 10 → 00 ...
  // Rotation CW
  if ((lastAB == 0b00 && currentAB == 0b01) ||
      (lastAB == 0b01 && currentAB == 0b11) ||
      (lastAB == 0b11 && currentAB == 0b10) ||
      (lastAB == 0b10 && currentAB == 0b00)) {
    if (currentAB == 0b00) {
      cwCount++;
    }
  }

  // sens de rotation anti-horaire 00 → 10 → 11 → 01 → 00 ...
  // Rotation CCW
  else if ((lastAB == 0b00 && currentAB == 0b10) ||
           (lastAB == 0b10 && currentAB == 0b11) ||
           (lastAB == 0b11 && currentAB == 0b01) ||
           (lastAB == 0b01 && currentAB == 0b00)) {
    if (currentAB == 0b00) {
      ccwCount++;
    }
  }

  // on update la valeur de lastAB pour savoir le sens de rotation pour la prochaine interruption
  lastAB = currentAB;
}
