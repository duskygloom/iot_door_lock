/**
 * @todo
 * door_lock_ir + ir and lcd
 */

#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include <MFRC522.h>
#include <IRremote.h>

#include "members.h"
#include "rfid_helper.h"

#include "ir_helper.h"
#include "lcd_helper.h"
#include "lcd_modified.h"

#define RELAY_PIN 26
#define BUTTON_PIN 25
#define RELAY_TIMEOUT 2000

#define RFID_RST_PIN 33

#define IR_PIN 27

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


/* LCD */

LiquidCrystal_I2C lcd(LCD_COLS, LCD_ROWS);

void setupLCD()
{
    uint8_t address = get_i2c_address();
	Serial.println(String("LCD I2C address: ") + address);
	lcd.setAddress(address);
	lcd.init();
    lcd.clear();
	lcd.backlight();
    lcd.createChar(LOCKED_EMOJI, locked_emoji);
    lcd.createChar(UNLOCKED_EMOJI, unlocked_emoji);
    lcd.createChar(UNSELECTED_EMOJI, unselected_emoji);
    lcd.createChar(SELECTED_EMOJI, selected_emoji);
	lcd.printLine("IoT Club", 0, LINE_CENTER, false);
}


/* RELAY */

#define RELAY_ON (digitalRead(RELAY_PIN) == LOW)

int lastRelay = 0;

void activateRelay()
{
    if (RELAY_ON) return;
    digitalWrite(RELAY_PIN, LOW);
    lastRelay = millis();
    lcd.setCursor(LCD_COLS - 1, 0);
    lcd.write(LOCKED_EMOJI);
}

static inline void deactivateRelay()
{
    digitalWrite(RELAY_PIN, HIGH);
    lcd.setCursor(LCD_COLS - 1, 0);
    lcd.write(LOCKED_EMOJI);
}

#define RELAY_TIMED_OUT (millis() > lastRelay + RELAY_TIMEOUT)

void setupRelay()
{
    pinMode(RELAY_PIN, OUTPUT);
    deactivateRelay();
}


/* BUTTON */

#define BUTTON_ON (digitalRead(BUTTON_PIN) == LOW)

static inline void setupButton()
{
    pinMode(BUTTON_PIN, INPUT_PULLUP);
}


/* RFID */

MFRC522 reader(RFID_RST_PIN);

void setupReader()
{
    SPI.begin();
    reader.PCD_Init();
    #ifdef DEBUG_MODE
    reader.PCD_DumpVersionToSerial();
    #endif
}

#define CARD_READ (reader.PICC_IsNewCardPresent() && reader.PICC_ReadCardSerial())


/* IR REMOTE */

int controller = 0;

void setupIR()
{
    pinMode(IR_PIN, INPUT);
    IrReceiver.begin(IR_PIN, true);
}

void handleIR()
{
    if (IrReceiver.decode()) {
        IRRawDataType data = IrReceiver.decodedIRData.decodedRawData;
        switch (data) {
            case 0x0:
                break;
            case KEY_LEFT:
                serialDebug("Left button.");
                break;
            case KEY_RIGHT:
                serialDebug("Right button.");
                break;
            case KEY_OK:
                serialDebug("OK button.");
                break;
            default:
                String commandHex(data, HEX);
                serialDebug(String("Unknown command 0x") + commandHex);
                break;
        }
        IrReceiver.resume();
    }
}


/* SETUP */

void setup()
{
    setupSerial();
    setupLCD();
    setupRelay();
    setupButton();
    setupIR();
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
                serialDebug(String("Door opened with RFID by ") + members[i].name + ".");
                break;
            }
        if (!matched)
            serialDebug(rfid.toString() + " tried to open the door using RFID.");
    }
    // IR remote
    handleIR();
    // close door
    if (RELAY_ON && RELAY_TIMED_OUT) {
        deactivateRelay();
        serialDebug("Door closed.");
    }
    // delay
    delay(100);
}
