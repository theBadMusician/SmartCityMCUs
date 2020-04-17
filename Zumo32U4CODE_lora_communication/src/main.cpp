//******************************************************************************
// Zumo32U4 control using ESP32 and Blynk IoT services
// Version 1.0 22/02/2020
// 2020, Benjaminas Visockis
//
// To use this code
// Disconnect the LCD screen from Zumo (if connected)
// Establish UART communication from ESP32 to Zumo32U4 by connecting:
// ESP32			----->	Zumo32U4
// TXD2 (GPIO17)	----->	RXD1 (PD2)
// Vin (5V)			----->	5V
// GND				----->	GND
//
// IMPORTANT!
// If you want to establish two way communication between ESP32 and Zumo32U4,
// use a logic level converter, or at least, a voltage divider between ESP32 (RX), which uses 3.3V logic,
// and 32U4 (TX), 5V logic.
// (NOT required in this case, as this is only one way communication)
//******************************************************************************

#define DEBUGGING 0

#include <Arduino.h>
#include <Zumo32U4.h>
#include <stdlib.h>

using namespace std;

Zumo32U4Encoders    encoders;
Zumo32U4Motors      motors;
Zumo32U4ButtonA     buttonA;
Zumo32U4LCD         LCD;

int16_t             batteryCap;

uint32_t            check_time,
                    delta_time;

int16_t 		    value_x,
				    value_y,
				    LMS,
				    RMS,
				    max_speed = 400;

struct dataPackage {
	uint8_t 	    LR_value;
  	uint8_t 	    UD_value;
};
dataPackage 	    data;

void setup() {
	Serial1.begin(115200);
	delay(20);
	
	// Serial debugging with PC.
	#if DEBUGGING
		Serial.begin(9600);
		delay(500);
	#endif
	
}

void loop() {
	if (Serial1.available() > 0) {
		Serial1.readBytes((uint8_t*)&data, sizeof(dataPackage)) == sizeof(dataPackage);
	}
	if (data.UD_value < 111) {
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