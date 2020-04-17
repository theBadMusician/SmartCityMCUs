#ifdef RUN
#include <PointerFunctions.h>

double * getMetersCount () {
  double leftMetersCount = encoders.getCountsLeft() / 909.700 / 8.3000;
  double rightMetersCount = encoders.getCountsRight() / 909.700 / 8.3000;
  double metersAbs = (leftMetersCount + rightMetersCount) / 2.0;
  static double metersVec[3];
  metersVec[0] = leftMetersCount;
  metersVec[1] = rightMetersCount;
  return metersVec;
}

void driveMotors(double distance_in_meters) {
  double metersNow = *(pRevs + 0) / 8.30;
  Serial.print("DistanceNow:");
  Serial.print(metersNow);
  Serial.print("   ");
  if (buttonA.isPressed()) {
    metersStamp = *(pRevs + 0) / 8.30;
    Serial.print("metersStamp:");
    Serial.println(metersStamp);
    delay(1000);
  }
  else if (metersNow < distance_in_meters + metersStamp) {
    Serial.println(distance_in_meters + metersStamp);
    motors.setSpeeds(300, 300);
  } else {
    motors.setSpeeds(0, 0);
    Serial.println("MotorsStopped!");
  }
}
#endif