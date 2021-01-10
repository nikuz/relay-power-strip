#include <Arduino.h>
#include <EspOta.h>

#include "def.h"

#if DEBUG
#define BLYNK_DEBUG // Optional, this enables lots of prints
#define BLYNK_PRINT Serial
#endif
#define BLYNK_NO_BUILTIN   // Disable built-in analog & digital pin operations
#define BLYNK_NO_FLOAT     // Disable float operations
#define BLYNK_MSG_LIMIT 50

#include <BlynkSimpleEsp32.h>

#include "AppBlynkDef.h"
#include "AppBlynk.h"
#include "AppWiFi.h"
#include "AppStorage.h"
#include "Tools.h"
#include "AppTime.h"
#include "Relay.h"

// Blynk virtual pins
const int pinVersion = V5;
const int pinRtcBattery = V9;
const int pinOtaHost = V20;
const int pinOtaBin = V21;
const int pinOtaLastUpdateTime = V22;
const int pinUptime = V11;
const int pinRtcTemperature = V12;

// cache
int fishIntCache = -32000;
int versionCache = 0;
int rtcBatteryCache = 0;
int rtcTemperatureCache = 0;
String fishStringCache = "fish";
String otaHostCache = "";
String otaBinCache = "";
String otaLastUpdateTimeCache = "";
String uptimeCache = "";

const unsigned long blynkConnectAttemptTime = 5UL * 1000UL;  // try to connect to blynk server only 5 seconds
bool blynkConnectAttemptFirstTime = true;
WidgetTerminal blynkTerminal(V50);

static BlynkIntVariable intVariables[20];
static BlynkStringVariable stringVariables[10];
static BlynkSyncVariable syncVariables[] = {
    {"otaHost",           false},
    {"otaBin",            false},
    {"otaLastUpdateTime", false},
    {"uptime",            false},
    {"version",           false},
    {"rtcBattery",        false},
    {"rtcTemperature",    false},
};
const int syncValuesPerSecond = 5;

AppBlynk::AppBlynk() {};

AppBlynk::~AppBlynk() {};

// private

int AppBlynk::getPinById(const char *pinId) {
    if (strcmp(pinId, "version") == 0) return pinVersion;
    if (strcmp(pinId, "rtcBattery") == 0) return pinRtcBattery;
    if (strcmp(pinId, "otaHost") == 0) return pinOtaHost;
    if (strcmp(pinId, "otaBin") == 0) return pinOtaBin;
    if (strcmp(pinId, "otaLastUpdateTime") == 0) return pinOtaLastUpdateTime;
    if (strcmp(pinId, "uptime") == 0) return pinUptime;
    if (strcmp(pinId, "rtcTemperature") == 0) return pinRtcTemperature;

    return -1;
}

int &AppBlynk::getIntCacheValue(const char *pinId) {
    if (strcmp(pinId, "version") == 0) return versionCache;
    if (strcmp(pinId, "rtcBattery") == 0) return rtcBatteryCache;
    if (strcmp(pinId, "rtcTemperature") == 0) return rtcTemperatureCache;

    return fishIntCache;
}

String &AppBlynk::getStringCacheValue(const char *pinId) {
    if (strcmp(pinId, "otaHost") == 0) return otaHostCache;
    if (strcmp(pinId, "otaBin") == 0) return otaBinCache;
    if (strcmp(pinId, "otaLastUpdateTime") == 0) return otaLastUpdateTimeCache;
    if (strcmp(pinId, "uptime") == 0) return uptimeCache;

    return fishStringCache;
}

int &AppBlynk::getIntVariable(const char *pin) {
    const int intVarsLen = *(&intVariables + 1) - intVariables;
    for (int i = 0; i < intVarsLen; i++) {
        if (intVariables[i].pin == pin) {
            return *intVariables[i].var;
        }
    }

    return fishIntCache;
}

String &AppBlynk::getStringVariable(const char *pin) {
    const int stringVarsLen = *(&stringVariables + 1) - stringVariables;
    for (int i = 0; i < stringVarsLen; i++) {
        if (stringVariables[i].pin == pin) {
            return *stringVariables[i].var;
        }
    }

    return fishStringCache;
}

void AppBlynk::sync() { // every second
    DEBUG_PRINTLN("Check connect:");
    DEBUG_PRINT("Wifi connected: ");
    DEBUG_PRINTLN(AppWiFi::isConnected());
    DEBUG_PRINT("Blynk connected: ");
    DEBUG_PRINTLN(Blynk.connected());
    DEBUG_PRINT("Millis: ");
    DEBUG_PRINTLN(millis());
    DEBUG_PRINT("Overflow is close: ");
    DEBUG_PRINTLN(Tools::millisOverflowIsClose());
    if (!AppWiFi::isConnected() || !Blynk.connected() || Tools::millisOverflowIsClose()) {
        return;
    }

    int syncCounter = 0;
    const int varsLen = *(&syncVariables + 1) - syncVariables;
    DEBUG_PRINT("Vars to sync: ");
    DEBUG_PRINTLN(varsLen);
    for (int i = 0; i < varsLen; i++) {
        if (syncCounter < syncValuesPerSecond) {
            if (syncVariables[i].synced) {
                continue;
            }

            const char *pin = syncVariables[i].pin;
            DEBUG_PRINT("Sync pin: ");
            DEBUG_PRINT(pin);
            DEBUG_PRINT(": ");
            if (strcmp(pin, "otaHost") == 0) {
                String &otaHostVariable = AppBlynk::getStringVariable(pin);
                AppBlynk::postData(pin, otaHostVariable);
            };
            if (strcmp(pin, "otaBin") == 0) {
                String &otaBinVariable = AppBlynk::getStringVariable(pin);
                AppBlynk::postData(pin, otaBinVariable);
            };
            if (strcmp(pin, "otaLastUpdateTime") == 0) {
                AppBlynk::postData(pin, EspOta::getUpdateTime());
            };
            if (strcmp(pin, "uptime") == 0) {
                AppBlynk::postData(pin, String(Tools::getUptime()));
            };
            if (strcmp(pin, "version") == 0) {
                AppBlynk::postData(pin, VERSION);
            };
            if (strcmp(pin, "rtcBattery") == 0) {
                AppBlynk::postData(pin, AppTime::RTCBattery() ? 255 : 0);
            };
            if (strcmp(pin, "rtcTemperature") == 0) {
                AppBlynk::postData(pin, AppTime::RTCGetTemperature());
            };
            syncVariables[i].synced = true;
            syncCounter++;
        }
    }

    if (syncCounter < syncValuesPerSecond) {
        for (int i = 0; i < varsLen; i++) {
            syncVariables[i].synced = false;
        }
    }
}

void writeHandler(const char *pin, int value, bool store) {
    int &variable = AppBlynk::getIntVariable(pin);
    AppBlynk::getData(variable, pin, value, store);
}

void writeHandler(const char *pin, String value, bool store) {
    String &variable = AppBlynk::getStringVariable(pin);
    AppBlynk::getData(variable, pin, value, store);
}

void checkRelay(const char *pin, const char *relayName, int value) {
    int &isCurrentlyEnabled = AppBlynk::getIntVariable(pin);
    if (value == 0 && isCurrentlyEnabled == 1) {
        Relay::off(relayName);
    }
}

void saveRelayTime(const char *pinStart, const char *pinEnd, TimeInputParam t) {
    if (t.hasStartTime() && t.hasStopTime()) {
        writeHandler(pinStart, t.getStartHour(), true);
        writeHandler(pinEnd, t.getStopHour(), true);
    }
}

BLYNK_WRITE(V20) { // otaHost
    writeHandler("otaHost", param.asStr(), true);
};
BLYNK_WRITE(V21) { // otaBin
    writeHandler("otaBin", param.asStr(), true);
};
// relay 1
BLYNK_WRITE(V30) {
    int value = param.asInt();
    checkRelay("relay1Enabled", "1", value);
    writeHandler("relay1Enabled", value, true);
}
BLYNK_WRITE(V31) {
    TimeInputParam t(param);
    saveRelayTime("relay1DayStart", "relay1DayEnd", t);
}
// relay 2
BLYNK_WRITE(V32) { // relay2Enabled
    int value = param.asInt();
    checkRelay("relay2Enabled", "2", value);
    writeHandler("relay2Enabled", value, true);
};
BLYNK_WRITE(V33) { // relay2DayStart
    TimeInputParam t(param);
    saveRelayTime("relay2DayStart", "relay2DayEnd", t);
};
// relay 3
BLYNK_WRITE(V34) { // relay3Enabled
    int value = param.asInt();
    checkRelay("relay3Enabled", "3", value);
    writeHandler("relay3Enabled", value, true);
};
BLYNK_WRITE(V35) { // relay3DayStart
    TimeInputParam t(param);
    saveRelayTime("relay3DayStart", "relay3DayEnd", t);
};
// relay 4
BLYNK_WRITE(V36) { // relay4Enabled
    int value = param.asInt();
    checkRelay("relay4Enabled", "4", value);
    writeHandler("relay4Enabled", value, true);
};
BLYNK_WRITE(V37) { // relay4DayStart
    TimeInputParam t(param);
    saveRelayTime("relay4DayStart", "relay4DayEnd", t);
};
// relay 5
BLYNK_WRITE(V38) { // relay5Enabled
    int value = param.asInt();
    checkRelay("relay5Enabled", "5", value);
    writeHandler("relay5Enabled", value, true);
};
BLYNK_WRITE(V39) { // relay5DayStart
    TimeInputParam t(param);
    saveRelayTime("relay5DayStart", "relay5DayEnd", t);
};
// relay 6
BLYNK_WRITE(V40) { // relay6Enabled
    int value = param.asInt();
    checkRelay("relay6Enabled", "6", value);
    writeHandler("relay6Enabled", value, true);
};
BLYNK_WRITE(V41) { // relay6DayStart
    TimeInputParam t(param);
    saveRelayTime("relay6DayStart", "relay6DayEnd", t);
};
//
BLYNK_WRITE(V10) { // ping
    if (param.asInt() == 1) {
        Blynk.notify("PONG");
        Blynk.virtualWrite(V10, 0);
    }
};
BLYNK_WRITE(V13) { // get time
    if (param.asInt() == 1) {
        AppTime::print();
        Blynk.virtualWrite(V13, 0);
    }
};
BLYNK_WRITE(V23) { // restart
    if (param.asInt() == 1) {
        Blynk.virtualWrite(V23, 0);
        delay(2000);
        ESP.restart();
    }
};

BLYNK_CONNECTED() {
    Blynk.syncAll();
};

// public

void AppBlynk::setVariable(int *var, const char *pin) {
    int varsCount = *(&intVariables + 1) - intVariables;
    for (int i = 0; i < varsCount; i++) {
        if (!intVariables[i].pin) {
            intVariables[i] = BlynkIntVariable(var, pin);
            break;
        }
    }
}

void AppBlynk::setVariable(String *var, const char *pin) {
    int varsCount = *(&stringVariables + 1) - stringVariables;
    for (int i = 0; i < varsCount; i++) {
        if (!stringVariables[i].pin) {
            stringVariables[i] = BlynkStringVariable(var, pin);
            break;
        }
    }
}

void AppBlynk::checkConnect() {
    DEBUG_PRINTLN("Check connect:");
    DEBUG_PRINT("Wifi connected: ");
    DEBUG_PRINTLN(AppWiFi::isConnected());
    DEBUG_PRINT("Blynk connected: ");
    DEBUG_PRINTLN(Blynk.connected());
    DEBUG_PRINT("Millis: ");
    DEBUG_PRINTLN(millis());
    DEBUG_PRINT("Overflow is close: ");
    DEBUG_PRINTLN(Tools::millisOverflowIsClose());
    if (!blynkConnectAttemptFirstTime && Tools::millisOverflowIsClose()) {
        return;
    }
    if (AppWiFi::isConnected() && !Blynk.connected()) {
        unsigned long startConnecting = millis();
        while (!Blynk.connected()) {
            Blynk.connect();
            if (millis() > startConnecting + blynkConnectAttemptTime) {
                Serial.println("Unable to connect to Blynk server.\n");
                break;
            }
        }
        if (Blynk.connected() && blynkConnectAttemptFirstTime) {
            blynkTerminal.clear();
        }
        blynkConnectAttemptFirstTime = false;
    }
}

void AppBlynk::initiate() {
    Blynk.config(blynkAuth, blynkDomain, blynkPort);
    AppBlynk::checkConnect();
}

void AppBlynk::run() {
    if (Blynk.connected()) {
        Blynk.run();
    }
}

void AppBlynk::getData(int &localVariable, const char *pinId, int pinData, const bool storePreferences) {
    if (localVariable == -1) {
        return;
    }
    if (pinData != localVariable) {
        localVariable = pinData;
        if (storePreferences) {
            AppStorage::putUInt(pinId, pinData);
        }
    }
}

void AppBlynk::getData(String &localVariable, const char *pinId, String pinData, const bool storePreferences) {
    if (localVariable == "fish") {
        return;
    }
    if (pinData != localVariable) {
        localVariable = pinData;
        if (storePreferences) {
            AppStorage::putString(pinId, pinData);
        }
    }
}

void AppBlynk::postData(const char *pinId, int value) {
    int blynkPin = AppBlynk::getPinById(pinId);
    if (blynkPin == -1) {
        return;
    }
    int &cacheValue = AppBlynk::getIntCacheValue(pinId);
    if (cacheValue != -32000 || cacheValue != value) { // post data also if cache not applied for pin
        if (Blynk.connected()) {
            Blynk.virtualWrite(blynkPin, value);
        }
        if (cacheValue != -32000) {
            cacheValue = value;
        }
    }
}

void AppBlynk::postData(const char *pinId, String value) {
    int blynkPin = AppBlynk::getPinById(pinId);
    if (blynkPin == -1) {
        return;
    }
    String &cacheValue = AppBlynk::getStringCacheValue(pinId);
    if (cacheValue != "fish" || cacheValue != value) { // post data also if cache not applied for pin
        if (Blynk.connected()) {
            Blynk.virtualWrite(blynkPin, value);
        }
        if (cacheValue != "fish") {
            cacheValue = value;
        }
    }
}

void AppBlynk::postDataNoCache(const char *pinId, int value) {
    int blynkPin = AppBlynk::getPinById(pinId);
    if (blynkPin == -1) {
        return;
    }
    if (Blynk.connected()) {
        Blynk.virtualWrite(blynkPin, value);
    }
}

void AppBlynk::postDataNoCache(const char *pinId, String value) {
    int blynkPin = AppBlynk::getPinById(pinId);
    if (blynkPin == -1) {
        return;
    }
    if (Blynk.connected()) {
        Blynk.virtualWrite(blynkPin, value);
    }
}

void AppBlynk::print(String value) {
    Serial.print(value);
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        blynkTerminal.print(value);
    }
}

void AppBlynk::print(char *value) {
    Serial.print(value);
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        blynkTerminal.print(value);
    }
}

void AppBlynk::print(int value) {
    Serial.print(value);
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        blynkTerminal.print(value);
    }
}

void AppBlynk::print(double value) {
    Serial.print(value);
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        blynkTerminal.print(value);
    }
}

void AppBlynk::println(String value) {
    Serial.println(value);
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        blynkTerminal.print(value);
        blynkTerminal.flush();
    }
}

void AppBlynk::println(char *value) {
    Serial.println(value);
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        blynkTerminal.println(value);
        blynkTerminal.flush();
    }
}

void AppBlynk::println(int value) {
    Serial.println(value);
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        blynkTerminal.println(value);
        blynkTerminal.flush();
    }
}

void AppBlynk::println(double value) {
    Serial.println(value);
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        blynkTerminal.println(value);
        blynkTerminal.flush();
    }
}

void AppBlynk::notify(String value) {
    if (AppWiFi::isConnected() && Blynk.connected() && !Tools::millisOverflowIsClose()) {
        Blynk.notify(value);
    }
}
