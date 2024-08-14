/**
 * @todo
 * Design a simple system with relay and button.
 * When the button is pressed, relay is turned on.
 * Relay is turned on for sometime and then it closes.
 */

#include <Arduino.h>

#define RELAY_PIN 26
#define BUTTON_PIN 25
#define RELAY_TIMEOUT 2000

#define DEBUG_MODE


/* SERIAL */

void setupSerial()
{
    Serial.begin(115200);
    while (!Serial);
    Serial.println("\n\n---------- New session ----------\n\n");
}

void serialDebug(const String &message)
{
    #ifdef DEBUG_MODE
    Serial.println(String("[DEBUG] ") + message);
    #endif
}


/* RELAY */

#define RELAY_ON (digitalRead(RELAY_PIN) == HIGH)

int lastRelay = 0;

void activateRelay()
{
    if (RELAY_ON) return;
    digitalWrite(RELAY_PIN, HIGH);
    lastRelay = millis();
}


/* BUTTON */

#define BUTTON_ON (digitalRead(BUTTON_PIN) == LOW)


/* SETUP */

void setup()
{
    setupSerial();
    pinMode(RELAY_PIN, OUTPUT);
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}


/* LOOP */

void loop()
{
    if (BUTTON_ON && !RELAY_ON) {
        activateRelay();
        serialDebug("Door opened with button.");
    }
    else if (RELAY_ON && millis() > lastRelay + RELAY_TIMEOUT) {
        digitalWrite(RELAY_PIN, LOW);
        serialDebug("Door closed.");
    }
    delay(100);
}
