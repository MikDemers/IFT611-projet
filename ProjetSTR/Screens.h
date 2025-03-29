#ifndef SCREENS_H
#define SCREENS_H

#include <Adafruit_SSD1306.h>
#include <Arduino.h>

#define NUM_BANDS 5

template <typename State>
class Screen {
  private:
    int screen_width = 128;
    int screen_height = 64;
    int oled_reset = -1;
    int screen_address = 0x3C;
    int sda_pin = 34;
    int scl_pin = 35;
  public:
    void update() {
        static_cast<State*>(this)->update_state();
    };

    void draw() {
        static_cast<State*>(this)->draw_state();
    };

    protected:
    // Référence vers l'écran (partagée par tous les écrans)
    Adafruit_SSD1306 &display;

    Screen(Adafruit_SSD1306 &disp) : display(disp) {}
};

class MainScreen : public Screen<MainScreen> {
  public:

    MainScreen(Adafruit_SSD1306 &disp) : Screen<MainScreen>(disp) { }
    void update_state(){
        // Mettre à jour l'état de l'écran ici
    };

    void draw_state(){
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(20, 0);
        display.println("Portable EQ 1.0");
        display.display();
    }
};

class SettingsScreen : public Screen<SettingsScreen> {
    // variables pour le clignotement
  private:
    unsigned long lastBlinkTime = 0;
    unsigned long blinkInterval = 300; // ms
    bool blinkState = true;
  public:
    // 5 bandes de fréquences
    short bandLevels[NUM_BANDS] = {0, 0, 0, 0, 0};

    int currentSelectedBand = 0;
    bool isEditing = false;

    SettingsScreen(Adafruit_SSD1306 &disp) : Screen<SettingsScreen>(disp) { }

    void update_state(){
        // On gère ici le clignotement uniquement si l'on est en mode édition
        if(isEditing) {
            unsigned long currentTime = millis();
            if(currentTime - lastBlinkTime > blinkInterval) {
                blinkState = !blinkState;
                lastBlinkTime = currentTime;
            }
        }
    };

    void draw_state(){
        display.clearDisplay();

        // Titre
        display.setCursor(32, 0);
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.println("> Band mod.");

        // Coordonnées de base
        int midLineY = 32;     // Ligne centrale
        int barWidth = 10;      // Largeur de chaque barre
        int spacing = 20;      // Espace entre chaque barre
        int scale = 3;         // 1 niveau = 3 pixels 

        for (int i = 0; i < NUM_BANDS; i++) {
            int level = bandLevels[i];
            int barHeight = abs(level) * scale; // Hauteur en pixels
            int x = 10 + i * spacing;           // Décalage horizontal

            // Cliquer sur une bande en mode édition
            if(isEditing && i == currentSelectedBand && !blinkState) {
                // Ne pas dessiner la barre pour cet intervalle
                continue;
            }
            
            // Dessiner la barre vers le haut
            if (level > 0) {
                int yTop = midLineY - barHeight; 
                display.fillRect(x, yTop, barWidth, barHeight, SSD1306_WHITE);
            }
            // Dessiner la barre vers le bas
            else if (level < 0) {
                int yBottom = midLineY; 
                display.fillRect(x, yBottom, barWidth, barHeight, SSD1306_WHITE);
            }
            // level == 0, donc on dessine une ligne horizontale
            else {
                display.drawLine(x, midLineY, x + barWidth, midLineY, SSD1306_WHITE);
            }
        }

        // Affichage des valeurs des bandes de fréquences
        // Un autre for pour afficher les valeurs des bandes sans clignotement
        for (int i = 0; i < NUM_BANDS; i++) {
            int x = 10 + i * spacing;
            int yText = midLineY + 20;
            display.setCursor(x, yText);
            display.setTextSize(1);
            display.print(bandLevels[i]);
        }

        display.display();
    }
};

#endif