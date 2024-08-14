#ifndef RFID_HELPER_H
#define RFID_HELPER_H

#include "Arduino.h"
#include "MFRC522.h"

#define RFID_MAX_SIZE 10


class Member
{
public:
    String name;
    String rfid;

    Member(String name, String rfid)
    : name(name), rfid(rfid) {}
};


class RFID
{
private:
    int size;
    uint8_t bytes[RFID_MAX_SIZE];

public:
    RFID(const MFRC522::Uid &uid)
    : size(uid.size) {
        memcpy(bytes, uid.uidByte, uid.size * sizeof(uint8_t));
    }

    String toString() {
        if (size <= 0) return "";
        String rfid = (bytes[0] > 9 ? "" : "0") + String(bytes[0], 16);
        for (int i = 1; i < size; ++i)
            rfid += String("-") + String(bytes[i], 10);
        return rfid;
    }

    bool operator==(const RFID &rfid) {
        if (size != rfid.size) return false;
        int minsize = size < rfid.size ? size : rfid.size;
        for (int i = 0; i < minsize; ++i)
            if (this->bytes[i] != rfid.bytes[i]) return false;
        return true;
    }

    bool operator==(const String &rfidstr) {
        return rfidstr == toString();
    }
};

#endif // RFID_HELPER_H
