#include <Arduino.h>
#include <Zumo32U4.h>
#include <stdlib.h>

using namespace std;

Zumo32U4Encoders    encoders;
Zumo32U4Motors      motors;
Zumo32U4ButtonA     buttonA;
Zumo32U4LCD         LCD;

int16_t             motor_speed,
                    batteryCap;

uint32_t            check_time,
                    delta_time;

struct {
    float           revCounts[2];
    float           meterCounts[2];
    float           metersAbs;
    float           batteryMeters;
} counters;

bool                flag,
                    battery_flag;


void                revCounters     ()              ;
void                writeToLCD      (String text)   ;
bool                isBatteryEmpty  ()              ; 
void                revCounters     ()              ;
void                writeToLCD      (String text)   ;