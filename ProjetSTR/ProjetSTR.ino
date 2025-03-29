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

int counter = 0;
int currentStateCLK;
int lastStateCLK;
String currentDir ="";

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
  pinMode(POTENTIO_RIGHT, INPUT_PULLUP);
  pinMode(POTENTIO_LEFT, INPUT_PULLUP);

  lastStateCLK = digitalRead(POTENTIO_RIGHT);

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

  // Read the current state of CLK
	currentStateCLK = digitalRead(POTENTIO_RIGHT);

  // Potentiomètre test
  // If last and current state of CLK are different, then pulse occurred
	// React to only 1 state change to avoid double count
	if (currentStateCLK != lastStateCLK  && currentStateCLK == 1){

		// If the DT state is different than the CLK state then
		// the encoder is rotating CCW so decrement
		if (digitalRead(POTENTIO_LEFT) != currentStateCLK) {
			counter --;
			currentDir ="CCW";
		} else {
			// Encoder is rotating CW so increment
			counter ++;
			currentDir ="CW";
		}

		Serial.print("Direction: ");
		Serial.print(currentDir);
		Serial.print(" | Counter: ");
		Serial.println(counter);
	}

	// Remember last CLK state
	lastStateCLK = currentStateCLK;

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
