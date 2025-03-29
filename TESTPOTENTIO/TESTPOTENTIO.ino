// Définition des pins pour l'encodeur
#define ENCODER_PIN_A 2    // Enc A (à gauche)
#define ENCODER_PIN_B 15   // Enc B (à droite)
#define ENCODER_PIN_PUSH 4   // arrière ()

volatile long encoderPos = 0;  // Compteur brut des transitions
volatile int lastEncoded = 0;  // Dernier état combiné

// Nombre de transitions par "détent" (clic) : ajustez si nécessaire
const int transitionsPerDetent = 2;

void IRAM_ATTR updateEncoder() {
  int MSB = digitalRead(ENCODER_PIN_A);  // Lecture de la pin A
  int LSB = digitalRead(ENCODER_PIN_B);  // Lecture de la pin B
  int encoded = (MSB << 1) | LSB;          // Combinaison en un code sur 2 bits
  int sum = (lastEncoded << 2) | encoded;  // Création d'un code sur 4 bits avec l'état précédent

  // Inversion de la logique pour que tourner à droite incrémente (+1)
  if(sum == 0b1110 || sum == 0b0111 || sum == 0b0001 || sum == 0b1000)
    encoderPos++;  // Rotation vers la droite
  if(sum == 0b1101 || sum == 0b0100 || sum == 0b0010 || sum == 0b1011)
    encoderPos--;  // Rotation vers la gauche

  lastEncoded = encoded;
}

void setup() {
  Serial.begin(115200);

  // Configuration des pins en entrée avec résistance pull-up
  pinMode(ENCODER_PIN_A, INPUT_PULLUP);
  pinMode(ENCODER_PIN_B, INPUT_PULLUP);
  pinMode(ENCODER_PIN_PUSH, INPUT_PULLUP);

  // Attacher les interruptions sur les deux pins de l'encodeur
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_A), updateEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(ENCODER_PIN_B), updateEncoder, CHANGE);
}

void loop() {
    // Si le bouton est pressé, la broche passera à LOW
  if (digitalRead(ENCODER_PIN_PUSH) == LOW) {
    Serial.println("Button pressed!");
    // Petite pause pour éviter plusieurs détections rapides
    delay(200);
  }

  static long lastDetent = 0;
  
  // Calcul de la position "détent" en divisant le compteur brut
  long detentPosition = encoderPos / transitionsPerDetent;
  
  // Limiter la position entre -100 et +100
  if(detentPosition > 100) {
    detentPosition = 100;
    encoderPos = 100 * transitionsPerDetent;
  }
  if(detentPosition < -100) {
    detentPosition = -100;
    encoderPos = -100 * transitionsPerDetent;
  }
  
  // Afficher la position seulement lorsqu'elle change
  if (detentPosition != lastDetent) {
    Serial.print("Encoder position: ");
    Serial.println(detentPosition);
    lastDetent = detentPosition;
  }
  
  delay(10); // Petit délai pour la stabilité
}
