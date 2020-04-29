#define DEBUGGING 1

#include <Arduino.h>
#include <Include.h>
#include <Zumo32U4.h>
#include <stdlib.h>
#include <Wire.h>
#include <TurnSensor.h>

using namespace std;

L3G gyro;
Zumo32U4Encoders encoders;
Zumo32U4Motors motors;
Zumo32U4ButtonA buttonA;

// Pattern driving variables
int turnSpeed = 140;
int motorSpeed = 400;
int minSpeed = 140;
int acceleration = 2;
int slowDownDistance = 1400;

// States
enum StateMachine
{
    stateStandby,
    stateStop,
    stateDriveSquare,
    stateDriveCircle,
    stateDriveLine,
    stateDriveSnake
} states;

struct dataPackage
{
    uint8_t STATE;
    uint8_t LR_value = 0;
    uint8_t UD_value = 0;
};
dataPackage data;

// Functions

// Turnsensor
void forward(long count);
void turnLeft(int degrees);
void turnRight(int degrees);

// Patterns
void driveSquare();
void driveLineBF();
void driveCircle();
void driveHalfCircle(bool reverseDirection);
void driveSnake();

void setup()
{
    Serial1.begin(115200);
    delay(20);

// Serial debugging with PC.
#if DEBUGGING
    Serial.begin(115200);
    delay(500);
#endif

    turnSensorSetup();
    delay(500);
    turnSensorReset();
}

void loop()
{
    if (Serial1.available() > 0)
    {
        Serial1.readBytes((uint8_t *)&data, sizeof(dataPackage)) == sizeof(dataPackage);
        states = (StateMachine)data.STATE;

        Serial.print("RX state: ");
        Serial.print(data.STATE);
        Serial.print(" StateMachine state: ");
        Serial.println(states);
    }

    switch (states)
    {
    case stateStandby:
        turnSensorReset();
        motors.setSpeeds(0, 0);
        break;

    case stateDriveSquare:
        turnSensorReset();
        driveSquare();
        break;

    case stateDriveCircle:
        turnSensorReset();
        driveCircle();
        break;

    case stateDriveLine:
        turnSensorReset();
        driveLineBF();
        break;
    
    case stateDriveSnake:
        turnSensorReset();
        driveSnake();
        break;
    }
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
            int newSpeed = motorSpeed - (slowDownDistance - distanceLeft) / (float)slowDownDistance * (motorSpeed - minSpeed);
            if (newSpeed < curSpeed)
            {
                curSpeed = newSpeed;
            }
        }

        // Set the motor speed.
        // We adjust the speed according to our angle so we keep the same heading.
        motors.setSpeeds(curSpeed + (angle * 5), curSpeed - (angle * 5));
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

void driveSquare()
{
    for (int i = 0; i < 4; i++)
    {
        forward(5000);
        turnLeft(90);
        turnSensorReset();
    }
}

void driveLineBF()
{
    for (int i = 0; i < 2; i++)
    {
        forward(5000);
        turnLeft(90);
        turnLeft(90);
        turnSensorReset();
    }
}

void driveCircle()
{
    turnSensorReset();
    int curAngle = 0;
    motors.setSpeeds(100, 200);
    while (curAngle != -2)
    {
        turnSensorUpdate();
        curAngle = (((int32_t)turnAngle >> 16) * 360) >> 16;
    }
    delay(100);
    motors.setSpeeds(0, 0);
    delay(100);
}

void driveHalfCircle(bool reverseDirection)
{
    turnSensorReset();
    int curAngle = 0;
    if (!reverseDirection)
        motors.setSpeeds(100, 200);
    else
        motors.setSpeeds(200, 100);
    while (curAngle != 179)
    {
        turnSensorUpdate();
        curAngle = (((int32_t)turnAngle >> 16) * 360) >> 16;
    }
    delay(50);
    motors.setSpeeds(0, 0);
    delay(100);
}

void driveSnake()
{
    driveHalfCircle(0);
    driveHalfCircle(1);
    driveCircle();
    driveHalfCircle(1);
    driveHalfCircle(0);

    motors.setSpeeds(100, 200);
    delay(100);
    motors.setSpeeds(0, 0);
    delay(100);
}
