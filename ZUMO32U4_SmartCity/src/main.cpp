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
Zumo32U4LineSensors lineSensors;
Zumo32U4ProximitySensors proxSensors;

// Line Follower settings and variables
#define NUM_SENSORS 5
uint16_t lineSensorValues[NUM_SENSORS];
bool useEmitters = true;
uint8_t selectedSensorIndex = 0;

// Disse verdiene kan eksperimenteres med og de kan kalibreres bedre.
#define proportional_term 0.2 // 0.5 funker bra, 0.3 enda bedre, 0.25 også, 0.1 starter å sakte ned.
#define derivative_term 10.0  // Ble ikke testet så mye, men 9.0 ser ut til å funke godt.
#define sample_rate 1         // Hvor ofte avvik måles, funker bra ved 1 us. Jo høyere tallet, jo bedre støyfiltrering, men også mer unøyaktighet.
#define cc_definition 20000   // 1500, 3000, 10000 funker bra, høyere verdier gjør at dronen sakter ned.
#define cc_speed 250

// En array for avvik + en sjekk for hvor mange elementer i arrayen.
double error[5] = {0, 0, 0, 0, 0};
int16_t num_of_errors = sizeof(error) / sizeof(int);

// Kontrollvariabler.
int16_t left_speed,
    right_speed,
    turn_control,
    cruise_control;

// Tidsvariabler.
uint32_t check_time_calibrate,
    check_time_errors[5];

// Pattern driving variables
#define turnSpeed 140
#define motorSpeed 400
#define minSpeed 140
#define acceleration 2
#define slowDownDistance 1400

// RC control variables
uint32_t            check_time,
                    delta_time;

int16_t 		    value_x,
				    value_y,
				    LMS,
				    RMS,
				    max_speed = 400;

// States
enum StateMachine
{
    stateStandby,
    stateStop,
    stateDriveSquare,
    stateDriveCircle,
    stateDriveLine,
    stateDriveSnake,
    stateLineFollower,
    stateRCControl
} states;

struct dataPackage
{
    uint8_t STATE;
    // Uses for direct control
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

// Line follower
void calibrateLineSensors();
void lineFollowerSetup();
void lineFollowerMain();

//RC control
void RCControl();

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

    case stateLineFollower:
        lineFollowerSetup();
        while (states != stateStop)
        {
            lineFollowerMain();
            if (Serial1.available() > 0)
            {
                Serial1.readBytes((uint8_t *)&data, sizeof(dataPackage)) == sizeof(dataPackage);
                states = (StateMachine)data.STATE;
            }
        }
    case stateRCControl:
        while (states != stateStop) {
            RCControl();
            if (Serial1.available() > 0)
            {
                Serial1.readBytes((uint8_t *)&data, sizeof(dataPackage)) == sizeof(dataPackage);
                states = (StateMachine)data.STATE;
            }
        }
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

// Line follower functions
void calibrateLineSensors()
{
    delay(1000);
    check_time_calibrate = millis();
    while (millis() - check_time_calibrate < 4000)
    {
        ;
        if (millis() - check_time_calibrate > 1500 && millis() - check_time_calibrate < 3500)
        {
            motors.setSpeeds(-100, 100);
        }
        else
        {
            motors.setSpeeds(100, -100);
        }
        lineSensors.calibrate();
    }
    motors.setSpeeds(0, 0);
}

void lineFollowerSetup()
{
    lineSensors.initFiveSensors();
    calibrateLineSensors();

    delay(200);
    check_time_errors[5] = {millis()};
}

void lineFollowerMain()
{

    // IR-sensorene leses.
    int pos = lineSensors.readLine(lineSensorValues);

    // Førrige avviker oppdateres.
    for (uint8_t i = 4; i > 0; i--)
    {
        if (micros() - check_time_errors[i] > sample_rate)
        {
            error[i] = error[i - 1];
            check_time_errors[i] = {micros()};
        }
    }

    error[0] = map(pos, 0, 4000, 400, -400);

#if DEBUGGING
    Serial.print(error[0]);
    Serial.print("   ");
    Serial.print(error[1]);
    Serial.print("   ");
    Serial.print(error[2]);
    Serial.print("   ");
    Serial.print(error[3]);
    Serial.print("   ");
    Serial.print(error[4]);
    Serial.println("   ");
#endif

    // Fartskontrollsystemet. Jo lengere unna linja dronen peker, jo saktere kjør den.
    // Det gir bedre tid på å utføre vinkeljusteringer.
    uint16_t cruise_control = map(abs(error[0]), 0, 400, cc_definition, 0);

    // Gjøres om til eksponentiel fartskontroll.
    cruise_control = pow(cruise_control, 2) / pow(cc_definition, 2) * cc_speed;

    // Kalkulasjoner for PD-regulator. Avviket ganges med en konstant verdi i første parantes.
    // Snittendringsrate mellom nåværende avvik og et førrige avvik finnes i den andre parantes. Så adderes dem.
    int16_t turn_control = round((error[0] * proportional_term) + (derivative_term * ((error[0] - error[5]) / num_of_errors)));

    // Alle regulatorene summeres.
    left_speed = 100 + cruise_control + turn_control;
    right_speed = 100 + cruise_control - turn_control;

    // Farten begrenses.
    left_speed = constrain(left_speed, -400, 400);
    right_speed = constrain(right_speed, -400, 400);

    motors.setSpeeds(right_speed, left_speed);
}

void RCControl() {
	if (Serial1.available() > 0) {
		Serial1.readBytes((uint8_t*)&data, sizeof(dataPackage)) == sizeof(dataPackage);
	}
	if (data.UD_value < 114) {
		LMS = map(data.UD_value, 111, 0, 0, -max_speed);
		RMS = map(data.UD_value, 111, 0, 0, -max_speed);
	} else if (data.UD_value > 119) {
		LMS = map(data.UD_value, 119, 255, 0, max_speed);
		RMS = map(data.UD_value, 119, 255, 0, max_speed);
	} else {
		LMS = 0;
		RMS = 0;
	}
	if (data.LR_value < 111) {
		int map_x = map(data.LR_value, 111, 0, 0, max_speed);
		
		LMS -= map_x;
		RMS += map_x;

		if (LMS < -max_speed) {
			LMS = -max_speed;
		}
		if (RMS > max_speed) {
			RMS = max_speed;
		}
	}
	if (data.LR_value > 119) {
		int map_x = map(data.LR_value, 119, 255, 0, max_speed);
		
		LMS += map_x;
		RMS -= map_x;

		if (LMS > max_speed) {
			LMS = max_speed;
		}
		if (RMS < -max_speed) {
			RMS = -max_speed;
		}
	}
	motors.setSpeeds(LMS, RMS);

	#if DEBUGGING
		Serial.println(String(String(LMS) + String("   ") + String(RMS)));
	#endif
}