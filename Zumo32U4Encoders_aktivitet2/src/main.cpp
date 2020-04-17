#include <Arduino.h>
#include <Zumo32U4.h>
#include <stdlib.h>

using namespace std;

// Ved søppeltømmingssystemet med ruter:

// Utvidelse til programvarebatteri -> sjekke hvor mye batteri det er igjen 
// og hvis det er bare nok energi til å komme basen, kjøre tilbake til hjembasen ved bruk av samme ruten.

// Utvidelse til ladestasjon        -> jo mer man betaler per sekund/minutt, jo fortere/saktere skjer oppladninga.
//                                  -> Hvis man ikke har nok penger i konto, får man "låne" penger fra systemet med renter.

// Utvidelse til luftstasjon        -> Kritisk nivåvarsling - hvis temp, trykk, el. annet overstiger en terskelverdi, biler returnerer hjem,
// og stanser all virkning inntil verdiene blir "normale".

// Utvidelse til parkeringshus      -> Hvis en bil står lengere enn konto tilatter
//                                  -> Premiumparkeringsplasser

// Utvidelse til søppeltømming      -> QR-code gjennkjennelse til søppelbøtter

// Modul: kjøre på en bane mellom vegger: fortest som kjør uten treffe vegger vinner
//        -------||------- 

Zumo32U4Encoders  encoders;
Zumo32U4Motors    motors;
Zumo32U4ButtonA   buttonA;
Zumo32U4LCD       LCD;

int16_t motor_speed = 300,
        batteryCap;

uint32_t  check_time,
          delta_time;

struct {
  float revCounts[2];
  float meterCounts[2];
  float metersAbs;
  float batteryMeters;
} counters;

bool  flag = false,
      battery_flag = false;

void revCounters ();
void writeToLCD (String text);

void setup() {
  Serial.begin(115200);
  LCD.clear();
}

void loop() {
  if (buttonA.isPressed()) {
    motors.setSpeeds(0, 0);
    buttonA.waitForRelease();
    flag = !flag;
    delay(100);
  }
  if (!flag) motor_speed = 400;
  else motor_speed = -400;

  if (batteryCap - counters.batteryMeters > 0) battery_flag = true;
  else battery_flag = false;

  if (!battery_flag && motor_speed > 0) motors.setSpeeds(0, 0);
  else motors.setSpeeds(motor_speed, motor_speed);
   

  revCounters();

  Serial.print(counters.revCounts[0]);
  Serial.print("   ");
  Serial.print(counters.revCounts[1]);
  Serial.print("   ");
  Serial.print(counters.meterCounts[0]);
  Serial.print("   ");
  Serial.print(counters.meterCounts[1]);
  Serial.print("   ");
  Serial.println(counters.metersAbs);

  if (millis() - check_time > 100) {
    writeToLCD(String(counters.metersAbs));
    check_time = millis();
  }
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