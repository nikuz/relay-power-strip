#include <EspOta.h>

#include "def.h"
#include "AppWiFi.h"
#include "AppStorage.h"
#include "AppTime.h"
#include "Tools.h"
#include "Relay.h"
#include "AppBlynk.h"

static const char *TAG = "relay-power-strip";
AppTime timer;

// OTA settings
String otaHost = "selfproduct.com";
const int otaPort = 80;  // Non https. For HTTPS 443. As of today, HTTPS doesn't work.
#if PRODUCTION
String otaBin = "/esp32-updates/relay-power-strip.bin";
#else
String otaBin = "/esp32-updates/relay-power-strip-dev.bin";
#endif
#ifdef DEBUG
EspOta otaUpdate(otaHost, otaPort, otaBin, TAG, true);
#else
EspOta otaUpdate(otaHost, otaPort, otaBin, TAG);
#endif
const unsigned long otaCheckUpdateInterval = 60UL * 1000UL;  // check OTA update every minute

const unsigned long hourCheckInterval = 1UL * 1000UL;  // check current hour every second

// relays settings
// 1
int relay1DayStart = 6;  // 06:00
int relay1DayEnd = 20;   // 20:00
int relay1Enabled = 0;
// 2
int relay2DayStart = 6;  // 06:00
int relay2DayEnd = 20;   // 20:00
int relay2Enabled = 0;
// 3
int relay3DayStart = 6;  // 06:00
int relay3DayEnd = 20;   // 20:00
int relay3Enabled = 0;
// 4
int relay4DayStart = 6;  // 06:00
int relay4DayEnd = 20;   // 20:00
int relay4Enabled = 0;
// 5
int relay5DayStart = 6;  // 06:00
int relay5DayEnd = 20;   // 20:00
int relay5Enabled = 0;
// 6
int relay6DayStart = 6;  // 06:00
int relay6DayEnd = 20;   // 20:00
int relay6Enabled = 0;

const unsigned long checkWiFiConnectInterval = 5UL * 1000UL;  // check WiFi connection every 5 seconds

const unsigned long blynkSyncInterval = 2UL * 1000UL;  // sync blynk state every 2 seconds
const unsigned long blynkCheckConnectInterval = 30UL * 1000UL;  // check blynk connection every 30 seconds

const unsigned long uptimePrintInterval = 1UL * 1000UL;

void otaUpdateHandler() {
    if (Tools::millisOverflowIsClose()) {
        return;
    }
    if (otaUpdate._host != otaHost || otaUpdate._bin != otaBin) {
        otaUpdate.updateEntries(otaHost, otaPort, otaBin);
    }
    otaUpdate.begin(AppTime::getTimeString(AppTime::getCurrentTime()));
}

bool inLightDayDiapason(int hour, int start, int end) {
    if (start > end) {
        return hour >= start || hour < end;
    }
    return hour >= start && hour < end;
}

void hourCheck() {
    const int currentHour = AppTime::getCurrentHour();
    if (currentHour == -1) {
        return;
    }

    if (relay1Enabled != 0 && inLightDayDiapason(currentHour, relay1DayStart, relay1DayEnd)) {
        Relay::on("1");
    } else {
        Relay::off("1");
    }
    if (relay2Enabled != 0 && inLightDayDiapason(currentHour, relay2DayStart, relay2DayEnd)) {
        Relay::on("2");
    } else {
        Relay::off("2");
    }
    if (relay3Enabled != 0 && inLightDayDiapason(currentHour, relay3DayStart, relay3DayEnd)) {
        Relay::on("3");
    } else {
        Relay::off("3");
    }
    if (relay4Enabled != 0 && inLightDayDiapason(currentHour, relay4DayStart, relay4DayEnd)) {
        Relay::on("4");
    } else {
        Relay::off("4");
    }
    if (relay5Enabled != 0 && inLightDayDiapason(currentHour, relay5DayStart, relay5DayEnd)) {
        Relay::on("5");
    } else {
        Relay::off("5");
    }
    if (relay6Enabled != 0 && inLightDayDiapason(currentHour, relay6DayStart, relay6DayEnd)) {
        Relay::on("6");
    } else {
        Relay::off("6");
    }
}

void setup() {
    // Begin debug Serial
    Serial.begin(115200);
    while (!Serial) {
        ;
    }

    Relay::initiate();

    // restore preferences
    AppStorage::setVariable(&otaHost, "otaHost");
    AppStorage::setVariable(&otaBin, "otaBin");
    // 1
    AppStorage::setVariable(&relay1DayStart, "relay1DayStart");
    AppStorage::setVariable(&relay1DayEnd, "relay1DayEnd");
    AppStorage::setVariable(&relay1Enabled, "relay1Enabled");
    // 2
    AppStorage::setVariable(&relay2DayStart, "relay2DayStart");
    AppStorage::setVariable(&relay2DayEnd, "relay2DayEnd");
    AppStorage::setVariable(&relay2Enabled, "relay2Enabled");
    // 3
    AppStorage::setVariable(&relay3DayStart, "relay3DayStart");
    AppStorage::setVariable(&relay3DayEnd, "relay3DayEnd");
    AppStorage::setVariable(&relay3Enabled, "relay3Enabled");
    // 4
    AppStorage::setVariable(&relay4DayStart, "relay4DayStart");
    AppStorage::setVariable(&relay4DayEnd, "relay4DayEnd");
    AppStorage::setVariable(&relay4Enabled, "relay4Enabled");
    // 5
    AppStorage::setVariable(&relay5DayStart, "relay5DayStart");
    AppStorage::setVariable(&relay5DayEnd, "relay5DayEnd");
    AppStorage::setVariable(&relay5Enabled, "relay5Enabled");
    // 6
    AppStorage::setVariable(&relay6DayStart, "relay6DayStart");
    AppStorage::setVariable(&relay6DayEnd, "relay6DayEnd");
    AppStorage::setVariable(&relay6Enabled, "relay6Enabled");
    AppStorage::restore();

    // setup wifi ip address etc.
    AppWiFi::connect();

    //get internet time
    AppTime::obtainSNTP();

    // register Blynk variables
    AppBlynk::setVariable(&otaHost, "otaHost");
    AppBlynk::setVariable(&otaBin, "otaBin");
    // 1
    AppBlynk::setVariable(&relay1DayStart, "relay1DayStart");
    AppBlynk::setVariable(&relay1DayEnd, "relay1DayEnd");
    AppBlynk::setVariable(&relay1Enabled, "relay1Enabled");
    // 2
    AppBlynk::setVariable(&relay2DayStart, "relay2DayStart");
    AppBlynk::setVariable(&relay2DayEnd, "relay2DayEnd");
    AppBlynk::setVariable(&relay2Enabled, "relay2Enabled");
    // 3
    AppBlynk::setVariable(&relay3DayStart, "relay3DayStart");
    AppBlynk::setVariable(&relay3DayEnd, "relay3DayEnd");
    AppBlynk::setVariable(&relay3Enabled, "relay3Enabled");
    // 4
    AppBlynk::setVariable(&relay4DayStart, "relay4DayStart");
    AppBlynk::setVariable(&relay4DayEnd, "relay4DayEnd");
    AppBlynk::setVariable(&relay4Enabled, "relay4Enabled");
    // 5
    AppBlynk::setVariable(&relay5DayStart, "relay5DayStart");
    AppBlynk::setVariable(&relay5DayEnd, "relay5DayEnd");
    AppBlynk::setVariable(&relay5Enabled, "relay5Enabled");
    // 6
    AppBlynk::setVariable(&relay6DayStart, "relay6DayStart");
    AppBlynk::setVariable(&relay6DayEnd, "relay6DayEnd");
    AppBlynk::setVariable(&relay6Enabled, "relay6Enabled");

    // start Blynk connection
    AppBlynk::initiate();

    timer.setInterval("hourCheck", hourCheckInterval, hourCheck);
    timer.setInterval("ota", otaCheckUpdateInterval, otaUpdateHandler);
    timer.setInterval("checkWiFiConnect", checkWiFiConnectInterval, AppWiFi::reconnect);
    timer.setInterval("blynkCheckConnect", blynkCheckConnectInterval, AppBlynk::checkConnect);
    timer.setInterval("blynkSync", blynkSyncInterval, AppBlynk::sync);
}

void loop() {
    AppBlynk::run();

    timer.run();
}
