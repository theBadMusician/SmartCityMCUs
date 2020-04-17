#include "include.h"
#include <Arduino.h>
#include <Zumo32U4.h>
#include <stdlib.h>
#include <Wire.h>
#include "TurnSensor.h"

using namespace std;


L3G gyro;
Zumo32U4LCD lcd;
Zumo32U4Motors motors;
Zumo32U4Encoders encoders;
Zumo32U4ButtonA buttonA;
Zumo32U4ButtonB buttonB;
Zumo32U4ButtonC buttonC;

// Motor speeds while turning
int turnSpeed = 140;

// Max motor speed
int motorSpeed = 400;

// Min motor speed as we get close
int minSpeed = 140;

// How fast to accelerate or deaccelerate
int acceleration = 2;

// How close do we need to be before slowing down
int slowDownDistance = 1400;

void setup()
{
    turnSensorSetup();
    delay(500);
    turnSensorReset();
    lcd.clear();
    lcd.print("Press C");
}

// Going Forward
void forward(long count)
{

    // Reset everything to zero
    turnSensorReset();
    encoders.getCountsAndResetLeft();
    encoders.getCountsAndResetRight();

    // Variables to keep track of stuff
    long countsLeft = 0;
    long countsRight = 0;
    int curSpeed = acceleration;

    // Loop until we get there
    while (countsLeft < count)
    {

        // --- Read sensor values
        // Wheel encoders for distance
        countsLeft += encoders.getCountsAndResetLeft();
        countsRight += encoders.getCountsAndResetRight();

        // Heading
        turnSensorUpdate();
        int angle = (((int32_t)turnAngle >> 16) * 360) >> 16;
        int distanceLeft = count - countsLeft;

        // --- Calculate our speed
        // Not up to full speed, but a long way to go
        // Accelerate!
        if (curSpeed < motorSpeed && distanceLeft >= slowDownDistance)
        {
            curSpeed += acceleration;
            if (curSpeed > motorSpeed)
                curSpeed = motorSpeed;
        }

        // Not up to min speed
        // Accelerate!
        else if (curSpeed < minSpeed)
        {
            curSpeed += acceleration;
            if (curSpeed > minSpeed)
                curSpeed = minSpeed;
        }

        // Getting close, and going fast
        // Slow down!
        else if (distanceLeft < slowDownDistance)
        {
            lcd.gotoXY(0, 0);
            int newSpeed = motorSpeed - (slowDownDistance - distanceLeft) / (float)slowDownDistance * (motorSpeed - minSpeed);
            if (newSpeed < curSpeed)
            {
                curSpeed = newSpeed;
            }
        }

        // Set the motor speed.
        // We adjust the speed according to our angle so we keep the same heading.
        motors.setSpeeds(curSpeed + (angle * 5), curSpeed - (angle * 5));

        // Display
        lcd.gotoXY(0, 0);
        lcd.print(angle);
        lcd.print(" ");
        lcd.print(curSpeed);
        lcd.print(" ");
        lcd.gotoXY(0, 1);
        lcd.print(distanceLeft);
        lcd.print(" ");
        delay(1);
    };

    // Full stop
    motors.setSpeeds(0, 0);
    delay(100);
}

// Turn left
void turnLeft(int degrees)
{

    // Reset our turn sensor
    turnSensorReset();
    int curAngle = 0;

    // Start turning
    motors.setSpeeds(-turnSpeed, turnSpeed);
    while (curAngle < degrees)
    {
        turnSensorUpdate();
        curAngle = (((int32_t)turnAngle >> 16) * 360) >> 16;
        lcd.gotoXY(0, 0);
        lcd.print(curAngle);
        lcd.print(" ");

        // If we get close, slow down our turn rate
        if (abs(degrees - curAngle) < 30)
        {
            motors.setSpeeds(-turnSpeed / 2, turnSpeed / 2);
        }
        delay(1);
    }

    // Stop
    motors.setSpeeds(0, 0);
    delay(100);
}

// Turn right
void turnRight(int degrees)
{

    // Reset our turn sensor
    turnSensorReset();
    int curAngle = 0;

    // Start turning
    motors.setSpeeds(turnSpeed, -turnSpeed);
    while (curAngle > -degrees)
    {
        turnSensorUpdate();
        curAngle = (((int32_t)turnAngle >> 16) * 360) >> 16;
        lcd.gotoXY(0, 0);
        lcd.print(curAngle);
        lcd.print(" ");

        // If we get close, slow down our turn rate
        if (abs(degrees - curAngle) < 25)
        {
            motors.setSpeeds(turnSpeed / 2, -turnSpeed / 2);
        }
        delay(1);
    }

    // Stop
    motors.setSpeeds(0, 0);
    delay(100);
}

void driveSquare() {
    for (int i = 0; i < 4; i++) {
        forward(5000);
        turnLeft(90);
        turnSensorReset();
    }
}

void driveLineBF() {
    for (int i = 0; i < 2; i++) {
        forward(5000);
        turnLeft(179);
        turnSensorReset();
    }
}

void driveCircle() {
    turnSensorReset();
    int curAngle = 0;
    motors.setSpeeds(100, 200);
    while (curAngle != -1) {
        turnSensorUpdate();
        curAngle = (((int32_t)turnAngle >> 16) * 360) >> 16;
        lcd.gotoXY(0, 0);
        lcd.print(curAngle);
        lcd.print(" ");
    }
    motors.setSpeeds(0, 0);
}

void loop()
{
    turnSensorUpdate();
    Serial.println(((turnAngle >> 16) * 360) >> 16);

    // If we press A, turn left.
    bool buttonPress = buttonA.getSingleDebouncedPress();
    if (buttonPress)
    {
        delay(500);
        driveCircle();
        //lcd.clear();
    }

    // If we press A, turn right.
    buttonPress = buttonB.getSingleDebouncedPress();
    if (buttonPress)
    {
        delay(500);
        driveLineBF();
        //lcd.clear();
    }

    buttonPress = buttonC.getSingleDebouncedPress();
    if (buttonPress)
    {
        delay(500);
        driveSquare();
        //lcd.clear();
    }
}