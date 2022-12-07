extern bool relayState;
extern int relayPin;
extern bool shouldReboot;

extern uint redLightValue;
extern uint greenLightValue;
extern uint blueLightValue;

extern uint redLightPin;
extern uint greenLightPin;
extern uint blueLightPin;

void toggleRelayStateChange();
void writeColors();