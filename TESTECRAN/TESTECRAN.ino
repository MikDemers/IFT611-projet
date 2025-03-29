#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// Dimensions de l’écran
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_SCL_PIN 32
#define SCREEN_SDA_PIN 33
#define OLED_ADDRESS 0x3C
#define OLED_RESET   -1

// Coordonnées initiales du cercle (zone d'animation)
#define START_X_CIRCLE 64
#define START_Y_CIRCLE 40

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

int x = START_X_CIRCLE;
int y = START_Y_CIRCLE;
int dx = 1;

void setup() {
  Serial.begin(115200);
  // Initialisation de l'I2C avec vos broches définies (SDA=33, SCL=32)
  Wire.begin(SCREEN_SDA_PIN, SCREEN_SCL_PIN);

  if (!display.begin(SSD1306_SWITCHCAPVCC, OLED_ADDRESS)) {
    Serial.println(F("Échec de l'initialisation de l'écran OLED !"));
    while (true);
  }

  // Efface l’écran et dessine le texte statique
  display.clearDisplay();
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("PROJET STR!");
  display.println("Vincent & Mikael");
  // On affiche le texte une première fois
  display.display();
}

void loop() {
  // Pour ne pas effacer le texte, on efface seulement la zone du cercle.
  // Ici, on suppose que la zone d'animation est celle qui entoure le cercle.
  // Par exemple, on efface un rectangle couvrant la zone où se déplace le cercle.
  display.fillRect(0, 40 - 22, SCREEN_WIDTH, 44, BLACK);  // Efface la zone autour du cercle

  // Met à jour la position du cercle
  x += dx;
  if (x - 20 <= 0 || x + 20 >= SCREEN_WIDTH) {
    dx = -dx;  // Inverse la direction au bord de l'écran
  }

  // Dessine le cercle dans la nouvelle position
  display.fillCircle(x, y, 20, WHITE);

  // Affiche l'écran sans toucher à la zone de texte située en haut
  display.display();

  delay(10);
}
