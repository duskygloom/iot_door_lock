/**
 * @todo
 * door_lock_button + rfid capabilities
 */

#include <SPI.h>
#include <Arduino.h>
#include <MFRC522.h>

#include "members.h"
#include "rfid_helper.h"

#define RELAY_PIN 26
#define BUTTON_PIN 25
#define RELAY_TIMEOUT 2000

#define RFID_RST_PIN 33

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

#define RELAY_ON (digitalRead(RELAY_PIN) == LOW)

int lastRelay = 0;

void activateRelay()
{
    if (RELAY_ON) return;
    digitalWrite(RELAY_PIN, LOW);
    lastRelay = millis();
}

static inline void deactivateRelay()
{
    digitalWrite(RELAY_PIN, HIGH);
}

#define RELAY_TIMED_OUT (millis() > lastRelay + RELAY_TIMEOUT)


/* BUTTON */

#define BUTTON_ON (digitalRead(BUTTON_PIN) == LOW)


/* RFID */

MFRC522 reader(RFID_RST_PIN);

void setupReader()
{
    SPI.begin();
    reader.PCD_Init();
    reader.PCD_DumpVersionToSerial();
}

#define CARD_READ (reader.PICC_IsNewCardPresent() && reader.PICC_ReadCardSerial())


/* SETUP */

void setup()
{
    // serial
    setupSerial();
    // pins
    pinMode(RELAY_PIN, OUTPUT);
    deactivateRelay();
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    // rfid
    setupReader();
}


/* LOOP */

void loop()
{
    // button
    if (!RELAY_ON && BUTTON_ON) {
        activateRelay();
        serialDebug("Door opened with button.");
    }
    // rfid
    if (!RELAY_ON && CARD_READ) {
        RFID rfid(reader.uid);
        bool matched = false;
        for (int i = 0; i < numMembers; ++i)
            if (rfid == members[i].rfid) {
                matched = true;
                activateRelay();
                serialDebug(members[i].name + " opened door with RFID.");
                break;
            }
        if (!matched)
            serialDebug(rfid.toString() + " tried to open the door with RFID.");
    }
    // close door
    if (RELAY_ON && RELAY_TIMED_OUT) {
        deactivateRelay();
        serialDebug("Door closed.");
    }
    // delay
    delay(100);
}
