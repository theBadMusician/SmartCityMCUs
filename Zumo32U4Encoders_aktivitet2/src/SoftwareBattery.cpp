#include "SoftwareBattery.h"


bool isBatteryEmpty () {
    if (batteryCap - counters.batteryMeters > 0) battery_flag = true;
    else battery_flag = false;
    return battery_flag;
}

void revCounters () {
  counters.revCounts[0] = encoders.getCountsLeft() / 909.70;
  counters.revCounts[1] = encoders.getCountsRight() / 909.70;

  counters.meterCounts[0] = counters.revCounts[0] / 8.300;
  counters.meterCounts[1] = counters.revCounts[1] / 8.300;

  if (counters.meterCounts[0] > 0.1 || counters.meterCounts[0] < -0.1 || counters.meterCounts[1] > 0.1 || counters.meterCounts[1] < -0.1) {
    counters.batteryMeters += (counters.meterCounts[0] + counters.meterCounts[1]) * 0.5;
    counters.batteryMeters = constrain(counters.batteryMeters, 0, batteryCap);
    counters.metersAbs += abs(counters.meterCounts[0] + counters.meterCounts[1]) * 0.5;

    counters.revCounts[0] = encoders.getCountsAndResetLeft() / 909.70;
    counters.revCounts[1] = encoders.getCountsAndResetRight() / 909.70;
  }
}

void writeToLCD (String text) {
  LCD.clear();
  LCD.gotoXY(0, 0);
  LCD.println(text);
}