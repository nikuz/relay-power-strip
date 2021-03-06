#include <Arduino.h>
#include <RtcDS3231.h>
#include <Wire.h>
#include <Time.h>

RtcDS3231 <TwoWire> Rtc(Wire);

#include "def.h"
#include "AppTime.h"
#include "AppWiFi.h"
#include "AppBlynk.h"
#include "Tools.h"

const char *ntpServer = "1.rs.pool.ntp.org";
const char *ntpServer2 = "0.europe.pool.ntp.org";
const char *ntpServer3 = "1.europe.pool.ntp.org";
const long gmtOffset_sec = 3600;
const int daylightOffset_sec = 3600;
static int blankVariable = -1;

struct tm NTPCurrentTime = {
        tm_sec: -1,
        tm_min: -1,
        tm_hour: -1,
        tm_mday: -1,
        tm_mon: -1,
        tm_year: -1,
        tm_wday: -1,
        tm_yday: -1,
        tm_isdst: -1
};
struct tm RTCCurrentTime = {
        tm_sec: -1,
        tm_min: -1,
        tm_hour: -1,
        tm_mday: -1,
        tm_mon: -1,
        tm_year: -1,
        tm_wday: -1,
        tm_yday: -1,
        tm_isdst: -1
};
bool rtcBatteryAlive = true;

static inline unsigned long elapsed() { return millis(); }
int overFlowCounter;

AppTime::AppTime() {
    unsigned long current_millis = elapsed();

    for (int i = 0; i < MAX_TIMERS; i++) {
        callbacks[i] = 0;                   // if the callback pointer is zero, the slot is free, i.e. doesn't "contain" any timer
        prev_millis[i] = current_millis;
        delays[i] = 0;
        names[i] = "";
    }

    numTimers = 0;
    overFlowCounter = 0;
}

AppTime::~AppTime() {}

void AppTime::RTCBegin() {
    Rtc.Begin();

    RtcDateTime compiled = RtcDateTime(__DATE__, __TIME__);
    if (!Rtc.IsDateTimeValid()) {
        Rtc.SetDateTime(compiled);
        Serial.println("RTC lost confidence in the DateTime!");
    }
    if (!Rtc.GetIsRunning()) {
        Rtc.SetIsRunning(true);
        Serial.println("RTC was not actively running, starting now");
    }
    RtcDateTime now = Rtc.GetDateTime();
    if (now < compiled) {
        Rtc.SetDateTime(compiled);
        Serial.println("RTC is older than compile time!  (Updating DateTime)");
    }

    // never assume the Rtc was last configured by you, so
    // just clear them to your needed state
    Rtc.Enable32kHzPin(false);
    Rtc.SetSquareWavePin(DS3231SquareWavePin_ModeNone);
}

void AppTime::obtainSNTP() {
    if (AppWiFi::isConnected()) {
        configTime(gmtOffset_sec, daylightOffset_sec, ntpServer, ntpServer2, ntpServer3);
    }
}

bool AppTime::localTime(struct tm *timeinfo) {
    return getLocalTime(timeinfo);
}

int AppTime::RTCGetTemperature() {
    RtcTemperature rtcTemp = Rtc.GetTemperature();
    return rtcTemp.AsFloatDegC();
}

bool AppTime::RTCBattery() {
    return rtcBatteryAlive;
}

void AppTime::RTCIsDateTimeValid() {
    const bool isValid = Rtc.IsDateTimeValid();

    if (!isValid) {
        rtcBatteryAlive = false;
    } else {
        rtcBatteryAlive = true;
    }
}

struct tm AppTime::RTCGetCurrentTime() {
    RtcDateTime rtcTime = Rtc.GetDateTime();
    AppTime::RTCIsDateTimeValid();

    struct tm dateTime = {
        tm_sec: rtcTime.Second(),
        tm_min: rtcTime.Minute(),
        tm_hour: rtcTime.Hour(),
        tm_mday: rtcTime.Day(),
        tm_mon: rtcTime.Month() - 1, // 0 based
        tm_year: rtcTime.Year() - 1900,
        tm_wday: 0,
        tm_yday: 0,
        tm_isdst: -1
    };

    return dateTime;
}

struct tm AppTime::getCurrentTime() {
    struct tm timeinfo = NTPCurrentTime;
    if (!AppWiFi::isConnected() || !AppTime::localTime(&timeinfo)) {
        if (AppTime::RTCBattery()) {
            timeinfo = AppTime::RTCGetCurrentTime();
        } else {
            // AppTime::RTCBegin();
            // AppTime::RTCUpdateByNtp();
        }
    }

    return timeinfo;
}

int AppTime::getCurrentHour() {
    struct tm currentTime = AppTime::getCurrentTime();
    return currentTime.tm_hour;
}

int AppTime::getCurrentMinute() {
    struct tm currentTime = AppTime::getCurrentTime();
    return currentTime.tm_min;
}

char *AppTime::getTimeString(struct tm timeStruct, char format[]) {
    static char timeString[20];
    snprintf_P(
        timeString,
        sizeof timeString,
        PSTR(format),
        timeStruct.tm_mon + 1,
        timeStruct.tm_mday,
        timeStruct.tm_year + 1900,
        timeStruct.tm_hour,
        timeStruct.tm_min,
        timeStruct.tm_sec
    );
    return timeString;
}

struct tm AppTime::getTmFromString(const char *value) {
    // 10/10/2018 00:09:21
    int month = -1;
    int day = -1;
    int year = -1;
    int hours = -1;
    int minutes = -1;
    int seconds = -1;

    char dateString[20];
    strncpy(dateString, value, 20);

    char *pch;
    const char *delimiter = " /:";
    pch = strtok(dateString, delimiter);
    int i = 0;
    while (pch != NULL) {
        switch (i) {
            case 0:
                month = atoi(pch);
                break;
            case 1:
                day = atoi(pch);
                break;
            case 2:
                year = atoi(pch);
                break;
            case 3:
                hours = atoi(pch);
                break;
            case 4:
                minutes = atoi(pch);
                break;
            case 5:
                seconds = atoi(pch);
                break;
        }
        pch = strtok(NULL, delimiter);
        i++;
    }

    struct tm dateTime = {
            tm_sec: seconds,
            tm_min: minutes,
            tm_hour: hours,
            tm_mday: day,
            tm_mon: month - 1, // 0 based
            tm_year: year - 1900,
            tm_wday: 0,
            tm_yday: 0,
            tm_isdst: -1
    };

    return dateTime;
}

void AppTime::print() {
    struct tm timeinfo = {0};
    AppTime::localTime(&timeinfo);

    char *ntpTime[] = {"ntpTime: ", AppTime::getTimeString(timeinfo)};
    char *ntpTimeStr = Tools::getCharArray(ntpTime, 2);
    AppBlynk::println(ntpTimeStr);

    char *rtcTime[] = {"rtcTime: ", AppTime::getTimeString(AppTime::RTCGetCurrentTime())};
    char *rtcTimeStr = Tools::getCharArray(rtcTime, 2);
    AppBlynk::println(rtcTimeStr);
}

void AppTime::run() {
    int i;
    unsigned long current_millis;
    bool overflowIncreased = false;

    // get current time
    current_millis = elapsed();

    for (i = 0; i < MAX_TIMERS; i++) {
        // no callback == no timer, i.e. jump over empty slots
        if (callbacks[i]) {
            // is it time to process this timer ?
            // see http://arduino.cc/forum/index.php/topic,124048.msg932592.html#msg932592
            if (current_millis < prev_millis[i]) { // overflow
                if (!overflowIncreased) {
                    overFlowCounter++;
                    overflowIncreased = true;
                }
                prev_millis[i] = current_millis;
            }

            if (current_millis - prev_millis[i] >= delays[i]) {
                // update time
                prev_millis[i] = current_millis;
                DEBUG_PRINT("Run timer: ");
                DEBUG_PRINTLN(names[i]);
                (*callbacks[i])();
            }
        }
    }
}

// find the first available slot
// return -1 if none found
int AppTime::findFirstFreeSlot() {
    int i;

    // all slots are used
    if (numTimers >= MAX_TIMERS) {
        return -1;
    }

    // return the first slot with no callback (i.e. free)
    for (i = 0; i < MAX_TIMERS; i++) {
        if (callbacks[i] == 0) {
            return i;
        }
    }

    // no free slots found
    return -1;
}

int AppTime::setInterval(char *name, long d, timer_callback f) {
    int freeTimer;

    freeTimer = findFirstFreeSlot();
    if (freeTimer < 0) {
        return -1;
    }

    if (f == NULL) {
        return -1;
    }

    names[freeTimer] = name;
    delays[freeTimer] = d;
    callbacks[freeTimer] = f;
    prev_millis[freeTimer] = elapsed();

    numTimers++;

    return freeTimer;
}

int AppTime::getOverFlowCounter() {
    return overFlowCounter;
};

void AppTime::RTCDateTimeUpdate() {
    const char months[][4] = {
        "Jan", "Feb", "Mar", "Apr", "May", "Jun",
        "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    };
    struct tm ntpTime = {0};

    if (getLocalTime(&ntpTime)) {
        char date[15];
        char time[10];

        sprintf(
            date,
            "%.3s%3d %4d",
            months[ntpTime.tm_mon],
            ntpTime.tm_mday,
            ntpTime.tm_year + 1900
        );

        sprintf(
            time,
            "%02d:%02d:%02d",
            ntpTime.tm_hour,
            ntpTime.tm_min,
            ntpTime.tm_sec
        );

        // 10/10/2018 00:09:21
        RtcDateTime ntpDateTime = RtcDateTime(date, time);
        Rtc.SetDateTime(ntpDateTime);
    }
}