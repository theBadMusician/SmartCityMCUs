//******************************************************************************
// Zumo32U4 line follower using proportional regulation in combination with cruise control.
// It works by reducing the drone speed the further away from the line it is,
// which allows for more time for the necessary adjustments and greater level of correction e.g. on sharp turns.
//
// This version has an implementation of derivative regulation as well as p-regulation,
// although it needs more testing/calibration of the p-term, d-term, and cruise control definition.
// D-regulation works fine with sample rate of 0ms, as there isn't a lot of noise/false readings from line sensors.
// Derivative was implemented by using simple average approximation between find the rate of change between them.
// For better sampling the program uses five points error measurements.
// This could increased/decreased for better definition/more noise reduction. 
//
// Version 2.3.1 02/03/2020
// 2020, Benjaminas Visockis
//******************************************************************************

#include <Arduino.h>
#include <Wire.h>
#include <Zumo32U4.h>
#include <stdlib.h>
#include <stdio.h>

#define DEBUG 0 // Sett til sant for debugging.

Zumo32U4LCD 				lcd;
Zumo32U4ButtonA 			buttonA;
Zumo32U4LineSensors 		lineSensors;
Zumo32U4ProximitySensors 	proxSensors;
Zumo32U4Motors 				motors;

#define		NUM_SENSORS 5
uint16_t	lineSensorValues[NUM_SENSORS];
bool 		useEmitters = true;
uint8_t 	selectedSensorIndex = 0;

// Disse verdiene kan eksperimenteres med og de kan kalibreres bedre.
#define  	proportional_term  0.2  // 0.5 funker bra, 0.3 enda bedre, 0.25 også, 0.1 starter å sakte ned.
#define		derivative_term  15.0	// Ble ikke testet så mye, men 9.0 ser ut til å funke godt.
#define		sample_rate 1			// Hvor ofte avvik måles, funker bra ved 1 us. Jo høyere tallet, jo bedre støyfiltrering, men også mer unøyaktighet.
#define		cc_definition 20000 	// 1500, 3000, 10000 funker bra, høyere verdier gjør at dronen sakter ned.
#define		cc_speed 250

// En array for avvik + en sjekk for hvor mange elementer i arrayen.
double 		error[5] = {0, 0, 0, 0, 0};
int16_t 	num_of_errors = sizeof(error)/sizeof(int);

// Kontrollvariabler.
int16_t 	left_speed,
			right_speed,
			turn_control,
			cruise_control;

// Tidsvariabler.
uint32_t	check_time_calibrate,
			check_time_errors[5],
			check_time_lcd;


// Kalibrerer sensorene i 4 sekunder.
void calibrate_sensors() {
	lcd.clear();

	delay(1000);
	check_time_calibrate = millis();
	while (millis() - check_time_calibrate < 4000) {;
		if (millis() - check_time_calibrate > 1500 && millis() - check_time_calibrate < 3500) {
			motors.setSpeeds(-100, 100);
		} else {
			motors.setSpeeds(100, -100);
		}
		lineSensors.calibrate();
	}
	motors.setSpeeds(0, 0);
}


void setup() {
	Serial.begin(9600);

	lcd.clear();
	lcd.print("Press A");
	lcd.gotoXY(0, 1);
	lcd.print("to calibrate");
	buttonA.waitForButton();
	
	lineSensors.initFiveSensors();
	calibrate_sensors();
	
	lcd.clear();
	delay(200);
	check_time_errors[5] = {millis()};
}


void loop() {
	
	#if DEBUG
		char buffer[100];
		sprintf(buffer, "%d   %d   %d   %d   %d", error[0], error[1], error[2], error[3], error[4]);
		Serial.println(buffer);
	#endif

	// IR-sensorene leses.
	int pos = lineSensors.readLine(lineSensorValues);

	// Førrige avviker oppdateres.
	for (uint8_t i = 4; i > 0; i--) {
		if(micros() - check_time_errors[i] > sample_rate) {
			error[i] = error[i - 1];
			check_time_errors[i] = {micros()};
		}
	}

	error[0] = map(pos, 0, 4000, 400, -400);

	// Fartskontrollsystemet. Jo lengere unna linja dronen peker, jo saktere kjør den.
	// Det gir bedre tid på å utføre vinkeljusteringer.
	uint16_t cruise_control = map(abs(error[0]), 0, 400, cc_definition, 0);

	// Gjøres om til eksponentiel fartskontroll.
	cruise_control = pow(cruise_control, 2)/pow(cc_definition, 2) * cc_speed;
	
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
	
	// Skrive avlesingen fra linje sensorene og fartene til LDC skjerm.
	if (millis() - check_time_lcd > 100) {
		lcd.clear();
		lcd.gotoXY(0,0);
		lcd.print(pos);
		lcd.gotoXY(0,1);
		lcd.print(left_speed);
		lcd.gotoXY(4,1);
		lcd.print(right_speed);
		check_time_lcd = millis();
	}
}
