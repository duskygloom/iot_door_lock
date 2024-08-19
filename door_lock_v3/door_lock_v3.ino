/**
 * @brief
 * Door lock using fingerprint, IR and RFID.
 * IR can be used to enter many other commands.
 * - Open door
 * - Enroll fingerprint
 */

#include <SPI.h>
#include <MFRC522.h>
#include <IRremote.h>
#include <Adafruit_Fingerprint.h>

#include "ir_helper.h"
#include "lcd_helper.h"
#include "lcd_modified.h"
#include "members.h"
#include "rfid_helper.h"

#define DEBUG_MODE

#define IR_PIN 27
#define RELAY_PIN 26
#define TOUCH_PIN 33
#define BUTTON_PIN 25
#define RFID_RST_PIN 32

#define MAX_ADMINS 4
#define PIN_TIMEOUT 2000
#define CLEAR_TIMEOUT 4000
#define RELAY_TIMEOUT 2000
#define ENROLL_TIMEOUT 5000
#define SCAN_TIMEOUT 2000
#define GENERAL_TIMEOUT 100
#define SCAN_FAST_TIMEOUT 500
#define MAX_CONTROLS 2

MFRC522 mfrc(RFID_RST_PIN);
LiquidCrystal_I2C lcd(LCD_COLS, LCD_ROWS);

/**
 * @note
 * Contains the last time when each line was used.
 * It is used to clear lines after CLEAR_TIMEOUT.
 */
unsigned long lastLines[LCD_ROWS] = {0};

Adafruit_Fingerprint finger(&Serial2);

bool useFingerprint = false;

int numAdmins = 0;
int templateID = 0;

enum FingerStatus {
    FINGER_OK,
    FINGER_MATCHED,
    FINGER_NOT_MATCHED,
    FINGER_NOT_FOUND,
    FINGER_TIMEOUT,
    FINGER_ERROR
};


// misc functions

void serialDebug(const String &message)
{
    #ifdef DEBUG_MODE
    Serial.println(String("[DEBUG] ") + message);
    #endif
}

void displayFingerStatus()
{
    lcd.setCursor(LCD_COLS - 2, 0);
    lcd.write(useFingerprint ? FINGERPRINT_EMOJI : NO_FINGERPRINT_EMOJI);
}

/**
 * @note
 * Line will be cleared automatically after sometime.
 */
void printLine(const String &text, int line, LineAlignment align = LINE_LEFT, bool eraseLine = true)
{
    lcd.printLine(text, line, align, eraseLine);
    lastLines[line] = millis();
}


/* IR MISC FUNCTIONS */

IrCommand get_command(IRData data)
{
    switch (data.decodedRawData >> 16) {                            // last 4 hex are bf00 for all commands
        case 0xff00: return Key1;
        case 0xfe01: return Key2;
        case 0xfd02: return Key3;
        case 0xfb04: return Key4;
        case 0xfa05: return Key5;
        case 0xf906: return Key6;
        case 0xf708: return Key7;
        case 0xf609: return Key8;
        case 0xf50a: return Key9;
        case 0xf30c: return KeyStar;
        case 0xf20d: return Key0;
        case 0xf10e: return KeyHash;
        case 0xee11: return KeyUp;
        case 0xeb14: return KEY_LEFT;
        case 0xea15: return KEY_OK;
        case 0xe916: return KEY_RIGHT;
        case 0xe619: return KeyDown;
        default: return KEY_UNKNOWN;
    }
}


/* RELAY MISC FUNCTIONS */

unsigned int lastRelay = 0;

void relayOn()
{
    digitalWrite(RELAY_PIN, LOW);
    lastRelay = millis();
    lcd.setCursor(LCD_COLS - 1, 0);
    lcd.write(UNLOCKED_EMOJI);
}

void relayOff()
{
    digitalWrite(RELAY_PIN, HIGH);
    lastRelay = 0;
    lcd.setCursor(LCD_COLS - 1, 0);
    lcd.write(LOCKED_EMOJI);
}

void setupRelay()
{
    pinMode(RELAY_PIN, OUTPUT);
    relayOff();
}


/* SETUP MISC FUNCTIONS */

void setupSerial()
{
    Serial.begin(115200);
	while (!Serial) delay(10);
	Serial.println("\n\n----- New session -----\n\n");
}

void setupLCD()
{
    uint8_t address = get_i2c_address();
	Serial.println(String("LCD I2C address: ") + address);
	lcd.setAddress(address);
	lcd.init();
    lcd.clear();
	lcd.backlight();
    lcd.createChar(FINGERPRINT_EMOJI, fingerprint_emoji);
    lcd.createChar(NO_FINGERPRINT_EMOJI, no_fingerprint_emoji);
    lcd.createChar(LOCKED_EMOJI, locked_emoji);
    lcd.createChar(UNLOCKED_EMOJI, unlocked_emoji);
    lcd.createChar(ARROW_EMOJI, arrow_emoji);
	lcd.printLine("IoT Club", 0, LINE_CENTER, false);
    displayFingerStatus();
    relayOff();
}

void findAdmins()
{
    for (int i = 1; i <= MAX_ADMINS; ++i) {
        switch (finger.loadModel(i)) {
            case FINGERPRINT_OK:
                serialDebug(String("Found admin ") + i);
                ++numAdmins;
                break;
            case FINGERPRINT_BADLOCATION:
                serialDebug(String("Could not find admin ") + i);
                break;
            case FINGERPRINT_PACKETRECIEVEERR:
                serialDebug(String("Communication error during finding of admin ") + i);
                break;
            default:
                serialDebug(String("An unknown error occurred during finding of admin ") + i);
                break;
        }
    }
    Serial.println(String("Found ") + numAdmins + " admin(s).");
    if (numAdmins > 0) useFingerprint = true;
    else useFingerprint = false;
}

void setupFinger()
{
    // detect fingerprint sensor
    finger.begin(57600);
    if (finger.verifyPassword()) {
        Serial.println("Fingerprint sensor detected.");
        useFingerprint = true;
        displayFingerStatus();
    } else {
        Serial.println("Fingerprint sensor not detected.");
        useFingerprint = false;
        displayFingerStatus();
        Serial.println(String("Retrying after ") + SCAN_TIMEOUT/1000 + " seconds...");
        delay(SCAN_TIMEOUT);
        if (finger.verifyPassword()) {
            Serial.println("Fingerprint sensor detected.");
            useFingerprint = true;
            displayFingerStatus();
        } else {
            Serial.println("Fingerprint sensor not detected.");
            Serial.println("Fingerprint sensor will not be used.");
            useFingerprint = false;
            displayFingerStatus();
            return;
        }
    }
    // check fingerprint templates
    finger.getTemplateCount();
    Serial.println(String("Found ") + finger.templateCount + " fingerprint(s).");
    findAdmins();
}

void setupIR()
{
    IrReceiver.begin(IR_PIN, true);
}


/* SETUP */

void setup()
{
	setupSerial();
	setupLCD();
    setupFinger();
    setupIR();
    setupRelay();
    // touch
    pinMode(TOUCH_PIN, INPUT);
    // push button
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    // rfid
    SPI.begin();
    mfrc.PCD_Init();
}


/* FINGERPRINT ID MISC FUNCTIONS */

FingerStatus getFingerImage()
{
    switch (finger.getImage()) {
        case FINGERPRINT_OK:
            serialDebug("Image taken.");
            return FINGER_OK;
        case FINGERPRINT_NOFINGER:
            serialDebug("No finger detected.");
            return FINGER_NOT_FOUND;
        case FINGERPRINT_PACKETRECIEVEERR:
            serialDebug("Communication error.");
            return FINGER_ERROR;
        case FINGERPRINT_IMAGEFAIL:
            serialDebug("Imaging error.");
            return FINGER_ERROR;
        default:
            serialDebug("An unknown error occurred.");
            return FINGER_ERROR;
    }
}

/**
 * @param slot
 * Slot at which the template will be stored.
 * It can be either 1 or 2.
 * Usually slot 1 is used and slot 2 is used for verification.
 */
FingerStatus getFingerTemplate(uint8_t slot = 1)
{
    switch (finger.image2Tz(slot)) {
        case FINGERPRINT_OK:
            serialDebug("Template created.");
            return FINGER_OK;
        case FINGERPRINT_IMAGEMESS:
            serialDebug("Image is too messy.");
            return FINGER_ERROR;
        case FINGERPRINT_PACKETRECIEVEERR:
            serialDebug("Communication error.");
            return FINGER_ERROR;
        case FINGERPRINT_IMAGEFAIL:
            serialDebug("Imaging error.");
            return FINGER_ERROR;
        default:
            serialDebug("An unknown error occurred.");
            return FINGER_ERROR;
    }
}

/**
 * @note
 * Finger template ID is stored in `finger.fingerID`
 * @note
 * Template match confidence is stored in `finger.confidence`
 */
FingerStatus searchFinger()
{
    switch (finger.fingerFastSearch()) {
        case FINGERPRINT_OK:
            Serial.print(String("Found fingerprint at ") + finger.fingerID);
            Serial.println(String(" with confidence of ") + finger.confidence);
            return FINGER_OK;
        case FINGERPRINT_NOTFOUND:
            serialDebug("No matching template found.");
            return FINGER_NOT_MATCHED;
        case FINGERPRINT_PACKETRECIEVEERR:
            serialDebug("Communication error.");
            return FINGER_ERROR;
        default:
            serialDebug("An unknown error occurred.");
            return FINGER_ERROR;
    }
}


/* FINGERPRINT ID */

/**
 * @returns
 * Returns -1 if any error is encountered,
 * 0 if fingerprint not matched
 * and fingerID if matched
 */
int getFingerprintID()
{
    // get image from sensor
    switch (getFingerImage()) {
        case FINGER_OK:
            break;
        default:
            return -1;
    }
    // create template from image
    switch (getFingerTemplate()) {
        case FINGER_OK:
            break;
        default:
            return -1;
    }
    // search for fingerprint
    switch (searchFinger()) {
        case FINGER_OK:
            return finger.fingerID;
        case FINGER_NOT_MATCHED:
            return 0;
        default:
            return -1;
    }
}


/* ENROLL FINGERPRINT MISC FUNCTIONS */

FingerStatus createFingerModel()
{
    switch (finger.createModel()) {
        case FINGERPRINT_OK:
            serialDebug("Model created.");
            return FINGER_OK;
        case FINGERPRINT_PACKETRECIEVEERR:
            serialDebug("Communication error.");
            return FINGER_ERROR;
        case FINGERPRINT_ENROLLMISMATCH:
            lcd.printLine("Retry...", 2, LINE_CENTER);
            serialDebug("Fingerprints did not match.");
            return FINGER_NOT_MATCHED;
        default:
            serialDebug("An unknown error occurred.");
            return FINGER_ERROR;
    }
}

FingerStatus storeFingerModel()
{
    finger.getTemplateCount();
    templateID = MAX_ADMINS + (finger.templateCount - numAdmins) + 1;
    switch (finger.storeModel(templateID)) {
        case FINGERPRINT_OK:
            serialDebug(String("Template stored at ") + templateID);
            return FINGER_OK;
        case FINGERPRINT_BADLOCATION:
            serialDebug(String("Invalid templateID ") + templateID);
            return FINGER_ERROR;
        case FINGERPRINT_FLASHERR:
            serialDebug("Failed store in flash.");
            return FINGER_ERROR;
        case FINGERPRINT_PACKETRECIEVEERR:
            serialDebug("Communication error.");
            return FINGER_ERROR;
        default:
            serialDebug("An unknown error occurred.");
            return FINGER_ERROR;
    }
}


/* ENROLL FINGERPRINT */

FingerStatus checkAdminFinger()
{
    lcd.printLine("Waiting for admin...", 2, LINE_CENTER);
    unsigned long startTime = millis();
    int status = getFingerprintID();
    while (true) {
        if (millis() > startTime + ENROLL_TIMEOUT)
            return FINGER_TIMEOUT;
        if (status < 0) {
            
        }
        delay(SCAN_FAST_TIMEOUT);

    }
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


/* LOOP MISC FUNCTIONS */

void clearLines()
{
    // clear content
    for (int i = 0; i < LCD_ROWS; ++i)
        if (lastLines[i] > 0 && millis() > lastLines[i] + CLEAR_TIMEOUT) {
            lcd.clearLine(i);
            lastLines[i] = 0;
        }
}

void closeRelay()
{
    if (lastRelay > 0 && millis() > lastRelay + RELAY_TIMEOUT)
        relayOff();
}


/* IR */

int controller = 0;

void displayController()
{
    for (int i = 0; i < MAX_CONTROLS; ++i) {
        lcd.setCursor(1, LCD_COLS - MAX_CONTROLS + i);
        if (i == controller) lcd.write(ARROW_EMOJI);
        else lcd.write(' ');
    }
}

void controllerFunction()
{
    switch (controller) {
        case 0:
            enrollFingerprint();
            break;
        case 1:
            relayOn();
            printLine("Opened using IR.", 2, LINE_CENTER);
            break;
    }
}

void handleIR()
{
    // received ir signal
    if (IrReceiver.decode()) {
        IrCommand command = get_command(IrReceiver.decodedIRData);
        switch (command) {
            case KEY_LEFT:
                --controller;
                if (controller < 0) controller = MAX_CONTROLS - 1;
                displayController();
                break;
            case KEY_RIGHT:
                ++controller;
                if (controller >= MAX_CONTROLS) controller = 0;
                displayController();
                break;
            case KEY_OK:
                controllerFunction();
                break;
            default:
                serialDebug("Unknown command.");
                break;
        }
        IrReceiver.resume();
    }
}


/* FINGERPRINT */

void controlFinger()
{
    uint16_t fingerID = getFingerprintID();
    // error
    if (fingerID < 0) return;
    // not matched
    if (fingerID == 0) {
        lcd.printLine("Not matched!", 3, LINE_CENTER);
        lastLines[3] = millis();
    }
    // user matched
    else if (fingerID >= 1 && fingerID <= 127) {
        lcd.printLine("Matched!", 3, LINE_CENTER);
        relayOn();
        lastLines[3] = millis();
    }
}

#define TOUCH_ON (digitalRead(TOUCH_PIN) == HIGH)
#define BUTTON_ON (digitalRead(BUTTON_PIN) == LOW)


/* RFID */

void handleRFID()
{
    if (!mfrc.PICC_IsNewCardPresent()) {
        serialDebug("No card detected.");
        return;
    }
    if (!mfrc.PICC_ReadCardSerial()) {
        serialDebug("Could not read card.");
        return;
    }
    RFID rfid(mfrc.uid);
    serialDebug(String("Scanned RFID: ") + rfid.toString());
    for (int i = 0; i < numMembers; ++i)
        if (rfid == members[i].rfid) {
            serialDebug("Door opened with RFID.");
            lcd.printLine(String("Welcome ") + members[i].name + (members[i].name.length() % 2 == 0 ? "" : "!"), 2, LINE_CENTER);
            lastLines[2] = millis();
            relayOn();
            return;
        }
    serialDebug("RFID did not match.");
    lcd.printLine("Not matched!", 3, LINE_CENTER);
    lastLines[3] = millis();
}


/* LOOP */

void loop()
{
    if (millis() % GENERAL_TIMEOUT == 0) {
        handleIR();
        clearLines();
        closeRelay();
        // button
        if (BUTTON_ON) {
            relayOn();
            serialDebug("Door opened with button.");
        }
    }
    /**
     * @todo
     * Scan for fingerprints only when touch is active.
     * Currently fingerprints are scanned every second.
     */
    if (millis() % SCAN_TIMEOUT == 0) {
        handleRFID();
        if (useFingerprint) {
            serialDebug(String("Touch: ") + (TOUCH_ON ? "ON" : "OFF"));
            controlFinger();
        }
    }
    // 1 millisecond delay
	delay(1);
}
