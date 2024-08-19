/**
 * @todo
 * door_lock_ir + fingerprint
 */

#include <SPI.h>
#include <Wire.h>
#include <Arduino.h>
#include <MFRC522.h>
#include <IRremote.h>
#include <Adafruit_Fingerprint.h>

#include "members.h"
#include "rfid_helper.h"

#include "ir_helper.h"
#include "lcd_helper.h"
#include "lcd_modified.h"

#define IR_PIN 27
#define RELAY_PIN 26
#define BUTTON_PIN 25
#define RFID_RST_PIN 33

#define RELAY_TIMEOUT 4000
#define LCD_LINE_TIMEOUT 2000

#define MAX_ADMINS 4

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

/**
 * @note
 * Used to store timestamps when a line is displayed in LCD.
 * Helps to erase the line after sometime.
 */
unsigned long lastLines[LCD_ROWS] = {0};

void setupLCD()
{
    uint8_t address = get_i2c_address();
	serialDebug(String("LCD I2C address: ") + address);
	lcd.setAddress(address);
	lcd.init();
    lcd.clear();
	lcd.backlight();
    lcd.createChar(FINGERPRINT_EMOJI, fingerprint_emoji);
    lcd.createChar(NO_FINGERPRINT_EMOJI, no_fingerprint_emoji);
    lcd.createChar(LOCKED_EMOJI, locked_emoji);
    lcd.createChar(UNLOCKED_EMOJI, unlocked_emoji);
    lcd.createChar(UNSELECTED_EMOJI, unselected_emoji);
    lcd.createChar(SELECTED_EMOJI, selected_emoji);
	lcd.printLine("IoT Club", 0);
}

void printLine(const String &text, int line, LineAlignment align = LINE_CENTER, bool eraseLine = true)
{
    lcd.printLine(text, line, align, eraseLine);
    lastLines[line] = millis();
}

void clearLines()
{
    for (int i = 0; i < LCD_ROWS; ++i)
        if (lastLines[i] > 0 && millis() > lastLines[i] + LCD_LINE_TIMEOUT) {
            lcd.clearLine(i);
            lastLines[i] = 0;
        }
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
    lcd.write(UNLOCKED_EMOJI);
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

void enrollControl()
{

}

void unlockControl()
{
    activateRelay();
    serialDebug("Door unlocked with remote.");
}

/**
 * @note
 * Contains functions for each control state.
 */
void (*controlFunctions[])() = {
    enrollControl,
    unlockControl
};

const int numControls = sizeof(controlFunctions) / sizeof(void(*)());

void displayController()
{
    for (int i = 0; i < numControls; ++i) {
        lcd.setCursor(LCD_COLS - numControls + i, 1);
        if (i == controller) lcd.write(SELECTED_EMOJI);
        else lcd.write(UNSELECTED_EMOJI);
    }
}

void leftControl()
{
    --controller;
    if (controller < 0) controller = numControls - 1;
    displayController();
}

void rightControl()
{
    ++controller;
    if (controller >= numControls) controller = 0;
    displayController();
}

void centerControl()
{
    controlFunctions[controller]();
}

/**
 * @note
 * Contains functions for each key of remote.
 */
void (*keyFunctions[])() = {
    leftControl,
    rightControl,
    centerControl
};

void setupIR()
{
    pinMode(IR_PIN, INPUT);
    IrReceiver.begin(IR_PIN, true);
    displayController();
}

void handleIR()
{
    if (IrReceiver.decode()) {
        IRRawDataType data = IrReceiver.decodedIRData.decodedRawData;
        switch (data) {
            case 0x0:
                break;
            case KEY_LEFT:
                keyFunctions[0]();
                break;
            case KEY_RIGHT:
                keyFunctions[1]();
                break;
            case KEY_OK:
                keyFunctions[2]();
                break;
            default:
                serialDebug(String("Unknown command 0x") + String(data, HEX));
                break;
        }
        IrReceiver.resume();
    }
}


/* FINGERPRINT */

Adafruit_Fingerprint finger(&Serial2);

uint16_t numAdmins = 0;

bool checkFinger()
{
    lcd.setCursor(LCD_COLS - 2, 0);
    if (finger.verifyPassword()) {
        lcd.write(FINGERPRINT_EMOJI);
        return true;
    } else {
        serialDebug("Fingerprint sensor not found.");
        lcd.write(NO_FINGERPRINT_EMOJI);
        return false;
    }
}

void setupFinger()
{
    finger.begin(57600);
    checkFinger();
    // fingerprints
    finger.getTemplateCount();
    serialDebug(String("Found ") + finger.templateCount + " fingerprint(s).");
    // admin fingerprints
    for (int i = 1; i <= MAX_ADMINS; ++i)
        if (finger.loadModel(i) == FINGERPRINT_OK)
            ++numAdmins;
    serialDebug(String("Found ") + numAdmins + " admin(s).");
}

/**
 * Dangerous copy paste zone.
 */

bool getFingerImage()
{
    return finger.getImage() == FINGERPRINT_OK;
}

/**
 * @param slot
 * Slot at which the template will be stored.
 * It can be either 1 or 2.
 * Usually slot 1 is used and slot 2 is used for verification.
 */
bool getFingerTemplate(uint8_t slot = 1)
{
    return finger.image2Tz(slot) == FINGERPRINT_OK;
}

/**
 * @note
 * Finger template ID is stored in `finger.fingerID`
 * Template match confidence is stored in `finger.confidence`
 */
bool searchFinger()
{
    return finger.fingerFastSearch() == FINGERPRINT_OK;
}

bool createFingerModel()
{
    return finger.createModel() == FINGERPRINT_OK;
}

bool storeFingerModel()
{
    finger.getTemplateCount();
    uint16_t id = MAX_ADMINS + (finger.templateCount - numAdmins) + 1;
    return finger.storeModel(id);
}

void enrollFingerprint()
{
    unsigned long startTime = millis(), currTime;
    // wait for admin approval
    // release finger
    lcd.printLine("Release...", 2, LINE_CENTER);
    while ((currTime = millis()) < startTime + ENROLL_TIMEOUT && getFingerImage() != FINGER_NOT_FOUND)
        delay(SCAN_FAST_TIMEOUT);
    // first scan
    lcd.printLine("Press to scan...", 2, LINE_CENTER);
    while ((currTime = millis()) < startTime + ENROLL_TIMEOUT && getFingerImage() != FINGER_OK)
        delay(SCAN_FAST_TIMEOUT);
    // if timed out
    if (currTime > startTime + ENROLL_TIMEOUT) {
        lcd.printLine("Timed out!", 2, LINE_CENTER);
        serialDebug("Enrollment timeout.");
        return;
    }
    // if template not created
    if (getFingerTemplate(1) != FINGER_OK) {
        lcd.printLine("Retry...", 2, LINE_CENTER);
        return;
    }
    // release finger
    lcd.printLine("Release...", 2, LINE_CENTER);
    while ((currTime = millis()) < startTime + ENROLL_TIMEOUT && getFingerImage() != FINGER_NOT_FOUND)
        delay(SCAN_FAST_TIMEOUT);
    // second scan
    lcd.printLine("Press to verify...", 2, LINE_CENTER);
    startTime = millis();
    while ((currTime = millis()) < startTime + ENROLL_TIMEOUT && getFingerImage() != FINGER_OK)
        delay(SCAN_FAST_TIMEOUT);
    // if timed out
    if (currTime > startTime + ENROLL_TIMEOUT) {
        lcd.printLine("Timed out!", 2, LINE_CENTER);
        serialDebug("Verification timeout.");
        return;
    }
    // if template not created
    if (getFingerTemplate(2) != FINGER_OK) {
        lcd.printLine("Retry...", 2, LINE_CENTER);
        return;
    }
    // check if template already exists
    if (getFingerTemplate(1) == FINGER_OK && searchFinger() == FINGER_OK) {
        lcd.printLine("Template exists.", 2, LINE_CENTER);
        return;
    }
    // create finger model
    switch (createFingerModel()) {
        case FINGER_OK:
            break;
        case FINGER_NOT_MATCHED:
            lcd.printLine("Mismatch, retry...", 2, LINE_CENTER);
            return;
        default:
            lcd.printLine("Retry...", 2, LINE_CENTER);
            return;
    }
    // store finger model
    String message;
    switch (storeFingerModel()) {
        case FINGER_OK:
            message = "Enrolled at ";
            message += String((templateID < 10) ? "0" : "") + templateID;
            lcd.printLine(message, 2, LINE_CENTER);
            return;
        default:
            lcd.printLine("Retry...", 2, LINE_CENTER);
            return;
    }
}

/**
 * Dangerous copy paste zone ends
 */


/**
 * @returns
 * Returns fingerprint ID if found.
 * If not found returns 0.
 * If any error occurred returns -1.
 */
int getFingerprintID()
{
    // obtain fingerprint
    if (finger.getImage() != FINGERPRINT_OK) {
        serialDebug("Failed to obtain fingerprint.");
        return -1;
    }
    // create fingerprint template
    if (finger.image2Tz() != FINGERPRINT_OK) {
        serialDebug("Failed to create fingerprint template.");
        return -1;
    }
    // search fingerprint
    uint8_t status = finger.fingerFastSearch();
    if (status == FINGERPRINT_OK) {
        serialDebug("Fingerprint found.");
        return finger.fingerID;
    } else if (status == FINGERPRINT_NOTFOUND) {
        serialDebug("Fingerprint not found.");
        return 0;
    }
    return finger.fingerID;
}



void handleFinger()
{
    int fingerID = getFingerprintID();
    // not matched
    if (fingerID == 0)
        printLine("Not matched!", 2);
    // user matched
    else if (fingerID >= 1 && fingerID <= 127) {
        printLine("Matched!", 2);
        activateRelay();
        serialDebug("Door opened with fingerprint.");
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
    setupFinger();
}


/* LOOP */

void loop()
{
    clearLines();
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
                serialDebug(members[i].name + " opened the door with RFID.");
                printLine("Welcome " + members[i].name + "!", 2);
                break;
            }
        if (!matched) {
            serialDebug(rfid.toString() + " tried to open the door with RFID.");
            printLine("Unrecognized RFID.", 2);
        }
    }
    // fingerprint
    if (!RELAY_ON && checkFinger())
        handleFinger();
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
