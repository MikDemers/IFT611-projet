#pragma once

class BarScreen {
private:
    int screen_width = 128;
    int screen_height = 64;
    int oled_reset = -1;
    int screen_address = 0x3C;
    int sda_pin = 34;
    int scl_pin = 35;
public:
    short bandLevels[NB_BANDS] = {0, 0, 0, 0, 0, 0};

    void virtual update();
    void virtual draw();

    void drawTitle(String title) {
        int cursorPos = screen_width / 5 - title.length() / 2;
        display.clearDisplay();
        display.setTextSize(1);
        display.setTextColor(SSD1306_WHITE);
        display.setCursor(cursorPos, 0);
        display.println(title);
    }

    void drawBar(short x, short barHeight) {
        if (barHeight == 0) {
            display.drawLine(x, midLineY, x + barWidth, midLineY, SSD1306_WHITE);
        } else {
            int yOrigin = barHeight > 0 ? midLineY - barHeight : midLineY;
            display.fillRect(x, yOrigin, barWidth, abs(barHeight), SSD1306_WHITE);
        }
    }

    void refreshDisplay() {
        display.display();
    }

protected:
    int midLineY = 32;     // Ligne centrale
    int barWidth = 10;     // Largeur de chaque barre
    int spacing = 16;      // Espace entre la bordure gauche de chaque barre
    int lineHeight = 3;    // 1 niveau = 3 pixels 
    
    // Référence vers l'écran (partagée entre tous les Screen)
    Adafruit_SSD1306 &display;

    BarScreen(Adafruit_SSD1306 &disp) : display(disp) {}
};


class MainScreen : public BarScreen {
private:
public:
    MainScreen(Adafruit_SSD1306 &disp) : BarScreen::BarScreen(disp) { }
    ~MainScreen() = default;
    void update() override {
        // Mettre à jour l'état de l'écran ici
    };
    void draw() override {
        drawTitle("Portable EQ 1.0");

        for (int i = 0; i < NB_BANDS; i++) {
            int barHeight = bandLevels[i] * lineHeight; // Hauteur en pixels
            int x = 10 + i * spacing;           // Décalage horizontal

            drawBar(x, barHeight);
        }
    }
};

class SettingsScreen : public BarScreen {
    // variables pour le clignotement
  private:
    unsigned long lastBlinkTime = 0;
    unsigned long blinkInterval = 300; // ms
    bool blinkState = true;
  public:
    int currentSelectedBand = 0;

    SettingsScreen(Adafruit_SSD1306 &disp) : BarScreen::BarScreen(disp) { }

    void update() override {
        unsigned long currentTime = millis();
        if(currentTime - lastBlinkTime > blinkInterval) {
            blinkState = !blinkState;
            lastBlinkTime = currentTime;
        }
    };

    void draw() override {
        drawTitle("> Band mod.");

        for (int i = 0; i < NB_BANDS; i++) {
            int barHeight = bandLevels[i] * lineHeight; // Hauteur en pixels
            int x = 10 + i * spacing;           // Décalage horizontal

            if(i == currentSelectedBand && !blinkState)
                continue;
            
            drawBar(x, barHeight);
        }

        // Affichage des valeurs des bandes de fréquences
        // Un autre for pour afficher les valeurs des bandes sans clignotement
        for (int i = 0; i < NB_BANDS; i++) {
            int x = 10 + i * spacing;
            int yText = midLineY + 20;
            display.setCursor(x, yText);
            display.setTextSize(1);
            display.print(bandLevels[i]);
        }
    }
};
