#include <Arduino.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "Screens.h"
// Merci: https://lastminuteengineers.com/rotary-encoder-arduino-tutorial/
#define PUSH_BUTTON 4
#define POTENTIO_RIGHT 2
#define POTENTIO_LEFT 15

// Global instance for the screen
Adafruit_SSD1306 display(128, 64, &Wire, -1);

SettingsScreen* settingsScreen = nullptr;
MainScreen* mainScreen = nullptr;

bool inEditingMode = false;

 int cwCount = 0;
 int ccwCount = 0;
 uint8_t lastAB = 0;



void setup() {
  Serial.begin(115200);
  
  // (pins 33(SDA), 32(SCL) généralement)
  Wire.begin(33, 32);

  // Initialisation de l'écran OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("Erreur d'initialisation de l'écran OLED");
    while (1) { /* Blocage en cas d'erreur */ }
  }

  // Initialisation des boutons + potentiomètre
  pinMode(PUSH_BUTTON, INPUT_PULLUP);
  pinMode(POTENTIO_RIGHT, INPUT);
  pinMode(POTENTIO_LEFT, INPUT);
  attachInterrupt(digitalPinToInterrupt(POTENTIO_RIGHT), handleEncoder ,CHANGE);
  attachInterrupt(digitalPinToInterrupt(POTENTIO_LEFT), handleEncoder ,CHANGE);
  // Efface l'écran pour partir sur une base propre
  display.clearDisplay();
  display.display();

  settingsScreen = new SettingsScreen{ display };
  mainScreen = new MainScreen{ display };

  // valeur de test
  settingsScreen->bandLevels[0] = 5; 
  settingsScreen->bandLevels[1] = -5; 

  mainScreen->draw();
}

void loop() {
  static unsigned long lastButtonTime = 0;
  unsigned long currentTime = millis();

  // Encoder logique
  int localCW = cwCount;
  int localCCW = ccwCount;
  cwCount = 0;
  ccwCount = 0;

  if (inEditingMode) {
    // Mettre à jour la valeur de la bande courante avec les impulsions du rotary encoder
    int bandIndex = settingsScreen->currentSelectedBand;
    if (bandIndex >= 0 && bandIndex < NUM_BANDS) {
      for (int i = 0; i < localCW; i++) {
        settingsScreen->bandLevels[bandIndex] = constrain(settingsScreen->bandLevels[bandIndex] + 1, -5, 5);
        // JUSTE POUR DEBUG
        Serial.print("Band ");
        Serial.print(bandIndex);
        Serial.print(" increased to ");
        Serial.println(settingsScreen->bandLevels[bandIndex]);
      }
      for (int i = 0; i < localCCW; i++) {
        settingsScreen->bandLevels[bandIndex] = constrain(settingsScreen->bandLevels[bandIndex] - 1, -5, 5);
        // JUSTE POUR DEBUG
        Serial.print("Band ");
        Serial.print(bandIndex);
        Serial.print(" decreased to ");
        Serial.println(settingsScreen->bandLevels[bandIndex]);
      }
    }
  }
  // juste pour tester
  // else {
  //   // Si on n'est pas en mode édition, vous pouvez laisser afficher les messages CW/CCW
  //   while (localCW-- > 0) {
  //     Serial.println("CW");
  //   }
  //   while (localCCW-- > 0) {
  //     Serial.println("CCW");
  //   }
  // }

  // Regarder si le bouton a été pressé ET si le temps écoulé depuis la dernière pression est supérieur à 300 ms
  // car sinon peut être interprété comme plusieurs pressions
 if (digitalRead(PUSH_BUTTON) == LOW && (currentTime - lastButtonTime > 300)) {
    lastButtonTime = currentTime;
    Serial.println("Bouton pressé");
    
    if (!inEditingMode) {
      // Passer en mode édition
      inEditingMode = true;
      settingsScreen->isEditing = true;
      settingsScreen->currentSelectedBand = 0;
    } else {
      // En mode édition, passer à la prochaine bande
      settingsScreen->currentSelectedBand++;
      if (settingsScreen->currentSelectedBand >= NUM_BANDS) {
        // Si on dépasse la dernière bande, on quitte le mode édition et on revient à l'écran principal
        inEditingMode = false;
        settingsScreen->isEditing = false;
        mainScreen->draw();
      }
    }
  }
   // Mettre à jour et dessiner l'écran actif
  if (inEditingMode) {
    settingsScreen->update();
    settingsScreen->draw();
  } else {
    mainScreen->update();
    mainScreen->draw();
  }

  delay(50);
}

void handleEncoder() {
  uint8_t currentAB = (digitalRead(POTENTIO_RIGHT) << 1) | digitalRead(POTENTIO_LEFT);
  if (currentAB == lastAB) return;
  // voir https://www.youtube.com/watch?v=9j-y6XlaE80&ab_channel=FriendlyWire
  // Rotation CW
  if ((lastAB == 0b00 && currentAB == 0b01) ||
      (lastAB == 0b01 && currentAB == 0b11) ||
      (lastAB == 0b11 && currentAB == 0b10) ||
      (lastAB == 0b10 && currentAB == 0b00)) {
    if (currentAB == 0b00) {
      cwCount++;
    }
  }
  // Rotation CCW
  else if ((lastAB == 0b00 && currentAB == 0b10) ||
           (lastAB == 0b10 && currentAB == 0b11) ||
           (lastAB == 0b11 && currentAB == 0b01) ||
           (lastAB == 0b01 && currentAB == 0b00)) {
    if (currentAB == 0b00) {
      ccwCount++;
    }
  }

  lastAB = currentAB;
}
