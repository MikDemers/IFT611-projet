#pragma once

#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <commonDef.h>
#include "Screens.h"

// Merci: https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
#define SDA 33
#define SCL 32
#define PUSH_BUTTON 4
#define POTENTIO_RIGHT 2
#define POTENTIO_LEFT 15

extern short state;
extern AudioModule audioMod;

// Global instance for the screen
Adafruit_SSD1306 display(128, 64, &Wire, -1);

MainScreen* mainScreen = nullptr;
SettingsScreen* settingsScreen = nullptr;

unsigned long lastButtonTime = 0;

int cwCount = 0;
int ccwCount = 0;
uint8_t lastAB = 0;


void userSetup();
void userLoop();
void onEncoderChange();
void encoderLoop();
void buttonLoop();

void userTask(void*) {
  while(true) {
    userLoop();
  }
}

void userSetup() {
  // (pins 33(SDA), 32(SCL) généralement)
  Wire.begin(SDA, SCL);

  // Initialisation de l'écran OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erreur d'initialisation de l'écran OLED");
    while (1) { /* Blocage en cas d'erreur */ }
  }
  display.clearDisplay();
  display.display();

  // Initialisation des boutons + potentiomètre
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  pinMode(POTENTIO_RIGHT, INPUT);
  pinMode(POTENTIO_LEFT, INPUT);
  attachInterrupt(digitalPinToInterrupt(POTENTIO_RIGHT), onEncoderChange ,CHANGE);
  attachInterrupt(digitalPinToInterrupt(POTENTIO_LEFT), onEncoderChange ,CHANGE);

  mainScreen = new MainScreen{ display };
  settingsScreen = new SettingsScreen{ display }; 
}

void userLoop() {
  static TickType_t lastWakeTime = xTaskGetTickCount();
  encoderLoop();
  buttonLoop();

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

  vTaskDelayUntil(&lastWakeTime, pdMS_TO_TICKS(50));
}


void encoderLoop() {
  // Encoder logique
  int localCW = cwCount;
  int localCCW = ccwCount;
  cwCount = 0;
  ccwCount = 0;

  if (state == State::MODIF) {
    // Mettre à jour la valeur de la bande courante avec les impulsions du rotary encoder
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

  // Regarder si le bouton a été pressé ET si le temps écoulé depuis la dernière pression est supérieur à 300 ms
  // car sinon peut être interprété comme plusieurs pressions
  if (digitalRead(PUSH_BUTTON) == LOW && (currentTime - lastButtonTime > 300)) {
    lastButtonTime = currentTime;
    
    if (state != State::MODIF) {
      state = ++state % NB_STATES;
    } else {
      // En mode édition, passer à la prochaine bande
      if (settingsScreen->currentSelectedBand == NB_BANDS-1) {
        // Si on dépasse la dernière bande, on quitte le mode édition et on revient à l'écran principal
        state = ++state % NB_STATES;
      }
      settingsScreen->currentSelectedBand = ++settingsScreen->currentSelectedBand % NB_BANDS;
    }
  }
}

void onEncoderChange() {
  uint8_t currentAB = (digitalRead(POTENTIO_RIGHT) << 1) | digitalRead(POTENTIO_LEFT);
  if (currentAB == lastAB) return;
  // voir https://www.youtube.com/watch?v=9j-y6XlaE80&ab_channel=FriendlyWire

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
