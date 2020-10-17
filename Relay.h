#ifndef Relay_h
#define Relay_h

#include <Arduino.h>

struct RelayItem {
    int pin;
    const char *name;
    const bool highLevelTrigger;

    RelayItem(int _pin, const char *_name, const bool _hlt = true) : pin(_pin), name(_name), highLevelTrigger(_hlt) {}
};

class Relay {
public:
    Relay();

    ~Relay();

    static void initiate();

    static bool on(const char *name);

    static bool off(const char *name);

private:

    static RelayItem getRelayPin(const char *name);
};

#endif /* Relay_h */
