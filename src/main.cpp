#include <Arduino.h>
#include <audioTask.h>
#include <displayTask.h>
#include <userTask.h>
#include <commonDef.h>

short state;

void setup() {
    Serial.begin(115200);

    state = State::VISUAL;

    audioSetup();
    displaySetup();
    userSetup();

    xTaskCreatePinnedToCore(audioTask, "Audio Task", 8192, NULL, 3, NULL, 0);
    xTaskCreatePinnedToCore(userTask, "User Interface Task", 2048, NULL, 2, NULL, 1);
    xTaskCreatePinnedToCore(displayTask, "Display Task", 2048, NULL, 2, NULL, 1);
}

void loop() {
}
