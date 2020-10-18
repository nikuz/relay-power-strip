#include <Arduino.h>

#include "def.h"
#include "Relay.h"

// RelayItem({pin}, {name}, {highLevelTrigger}) - highLevelTrigger is true by default
static RelayItem relays[] = {
        RelayItem(RELAY_1, "1", true),
        RelayItem(RELAY_2, "2", true),
        RelayItem(RELAY_3, "3", true),
        RelayItem(RELAY_4, "4", true),
        RelayItem(RELAY_5, "5", true),
        RelayItem(RELAY_6, "6", true),
};

Relay::Relay() {}

Relay::~Relay() {}

// public

void Relay::initiate() {
    int relaysCounts = *(&relays + 1) - relays;
    for (int i = 0; i < relaysCounts; i++) {
        pinMode(relays[i].pin, OUTPUT);
        // turn off relays by default
        // not all relays is HIGH level triggered
        if (relays[i].highLevelTrigger) {
            Serial.print("HIGH level trigger: ");
            Serial.println(relays[i].name);
            digitalWrite(relays[i].pin, LOW);
        } else {
            Serial.print("LOW level trigger: ");
            Serial.println(relays[i].name);
            digitalWrite(relays[i].pin, HIGH);
        }
    }
}

// private

bool Relay::on(const char *name) {
    RelayItem relayItem = Relay::getRelayPin(name);
    if (relayItem.pin != -1) {
        DEBUG_PRINT("Relay ON: ");
        DEBUG_PRINTLN(relayItem.name);
        if (relayItem.highLevelTrigger) {
            digitalWrite(relayItem.pin, HIGH);
        } else {
            digitalWrite(relayItem.pin, LOW);
        }
        return true;
    }

    DEBUG_PRINT("Relay doesn't exist: ");
    DEBUG_PRINTLN(name);
    return false;
}

bool Relay::off(const char *name) {
    RelayItem relayItem = Relay::getRelayPin(name);
    if (relayItem.pin != -1) {
        DEBUG_PRINT("Relay OFF: ");
        DEBUG_PRINTLN(relayItem.name);
        if (relayItem.highLevelTrigger) {
            digitalWrite(relayItem.pin, LOW);
        } else {
            digitalWrite(relayItem.pin, HIGH);
        }

        return true;
    }

    DEBUG_PRINT("Relay doesn't exist: ");
    DEBUG_PRINTLN(name);
    return false;
}

RelayItem Relay::getRelayPin(const char *name) {
    int relaysCounts = *(&relays + 1) - relays;
    for (int i = 0; i < relaysCounts; i++) {
        if (strcmp(relays[i].name, name) == 0) {
            return relays[i];
        }
    }

    return RelayItem(-1, "");
}
