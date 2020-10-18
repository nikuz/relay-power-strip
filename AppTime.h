#ifndef AppTime_h
#define AppTime_h

#include <Arduino.h>
#include <Time.h>

typedef void (*timer_callback)(void);

class AppTime {
public:
    AppTime();

    ~AppTime();

    static void obtainSNTP();

    static bool localTime(struct tm *timeinfo);

    static void RTCBegin();

    static void RTCUpdateByNtp();

    static int RTCGetTemperature();

    static bool RTCBattery();

    static struct tm RTCGetCurrentTime();

    static struct tm getCurrentTime();

    static int getCurrentHour();

    static int getCurrentMinute();

    static char *getTimeString(struct tm timeStruct, char format[] = "%02u/%02u/%04u %02u:%02u:%02u");

    static struct tm getTmFromString(const char *value);

    static void print();

    static int getOverFlowCounter();

    // maximum number of timers
    const static int MAX_TIMERS = 16;

    // setTimer() constants
    const static int RUN_FOREVER = 0;
    const static int RUN_ONCE = 1;

    // this function must be called inside loop()
    void run();

    // call function f every d milliseconds
    int setInterval(char * name, long d, timer_callback f);

private:
    // find the first available slot
    int findFirstFreeSlot();

    char *names[MAX_TIMERS];
    // value returned by the millis() function
    // in the previous run() call
    unsigned long prev_millis[MAX_TIMERS];

    // pointers to the callback functions
    timer_callback callbacks[MAX_TIMERS];

    // delay values
    long delays[MAX_TIMERS];

    // actual number of timers in use
    int numTimers;

    static void RTCIsDateTimeValid();
};

#endif /* AppTime_h */
