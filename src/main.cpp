#include <Arduino.h>
#include <audioTask.h>
#include <userTask.h>
#include <commonDef.h>

short state;
enum State {
    VISUAL = 0,
    AUDIO = 1,
    MODIF = 2
};

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
