#define PRODUCTION 1    // production
//#define PRODUCTION 0    // development
//#define DEBUG 1

#define VERSION_ID "1"

#if PRODUCTION
#define VERSION_MARKER "P"
#else
#define VERSION_MARKER "D"
#endif

#define VERSION VERSION_MARKER VERSION_ID

#ifdef DEBUG
#define DEBUG_PRINT(x)    Serial.print(x)
#define DEBUG_PRINTLN(x)  Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// relays pins
#define RELAY_1 16
#define RELAY_2 17
#define RELAY_3 18
#define RELAY_4 19
#define RELAY_5 21
#define RELAY_6 22
