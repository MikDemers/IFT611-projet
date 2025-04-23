#include <Arduino.h>
#include <audioTask.h>
#include <userTask.h>
#include <commonDef.h>

void setup() {
    Serial.begin(115200);

    state = State::VISUAL;

    audioSetup();
    userSetup();

    xTaskCreatePinnedToCore(audioTask, "Audio Task", 8192, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(userTask, "User Interface Task", 4096, NULL, 1, NULL, 1);
}

void loop() {
}
