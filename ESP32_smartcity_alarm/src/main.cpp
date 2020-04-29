#include <Arduino.h>
#include <analogWrite.h>
#include <Servo.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>

#define DEBUG true

// ESP32 constants
#define LED 33
#define BUZZER 32
#define SERVO 25

// Declare classes
Servo servo1;
HTTPClient http;

// Network settings
const char *ssid = "PrivatNettverk";
const char *password = "Plechavicius1652";

// Buzzer settings
int buzzerFreq = 2000;
int buzzerChannel = 5;
int buzzerResolution = 8;

// LED state
bool ledState = 0;

// Time check variables
uint32_t timeCheckAlarm = 0,
         timeCheckServer = 0,
         timeCheckServo = 0;

// Alarm flag
bool isAlarmOn = 0;

// Servo settings
uint16_t servoDegs = 0;
uint8_t stopPos = 0;

// Is server checked flag
bool serverCheck = 0;
uint16_t checkInterval = 1000;

// States
enum StateMachine
{
    stateAlarmON,
    stateAlarmOFF,
    stateTestServo
} states;

// Function declarations
void AlarmON(uint8_t led, bool &led_state, uint16_t buzzer_channel, uint32_t &time_check, bool &check_flag);
void AlarmOFF(uint8_t led, bool &led_state, uint16_t buzzer_channel, uint32_t &time_check, bool &check_flag);

void checkServer();
void sweepServo();

void setup()
{
    // Serial initialization
#if DEBUG
    Serial.begin(115200);
    while (!Serial)
        ;
    Serial.println("Serial initialized...");
#endif

    // Digital pin setup
    pinMode(LED, OUTPUT);

    // Buzzer/PWM setup
    ledcSetup(buzzerChannel, buzzerFreq, buzzerResolution);
    ledcAttachPin(32, buzzerChannel);

    // Servo setup
    servo1.attach(SERVO);

    // WIFI setup
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED)
    {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    // Time checks setup
    timeCheckAlarm = millis();
    timeCheckServer = millis();
    timeCheckServo = millis();
}

void loop()
{
    //    AlarmON(LED, ledState, buzzerChannel, timeCheckAlarm, isAlarmOn);
    if (millis() - timeCheckServer > checkInterval) checkServer();

    switch (states)
    {
    case stateAlarmON:
        if (serverCheck) {
            AlarmON(LED, ledState, buzzerChannel, timeCheckAlarm, isAlarmOn);
            sweepServo();
            checkInterval = 10000;
        }
        break;

    case stateAlarmOFF:
        AlarmOFF(LED, ledState, buzzerChannel, timeCheckAlarm, isAlarmOn);
        checkInterval = 1000;
        break;

    case stateTestServo:
        if (serverCheck) {
            isAlarmOn = 1;
            sweepServo();
            checkInterval = 5000;
        }
        break;

    default:
        AlarmOFF(LED, ledState, buzzerChannel, timeCheckAlarm, isAlarmOn);
        checkInterval = 1000;
        break;
    }
}

void AlarmON(uint8_t led, bool &led_state, uint16_t buzzer_channel, uint32_t &time_check, bool &check_flag)
{
    uint16_t buzzerFreq = 1000;

    if (millis() - time_check > 1000 && led_state == 1)
    {
        led_state = 0;
        digitalWrite(led, led_state);

        ledcWriteTone(buzzer_channel, buzzerFreq * 0.5);

        time_check = millis();
    }

    else if (millis() - time_check > 500 && led_state == 0)
    {
        led_state = 1;
        digitalWrite(led, led_state);

        ledcWriteTone(buzzer_channel, buzzerFreq);
    }

#if DEBUG
    if (!check_flag)
        Serial.println("Alarm ON!\n");
#endif
    check_flag = 1;
}

void AlarmOFF(uint8_t led, bool &led_state, uint16_t buzzer_channel, uint32_t &time_check, bool &check_flag)
{
    if (check_flag)
    {
        check_flag = 0;
        led_state = 0;

        ledcWriteTone(buzzer_channel, led_state);
        digitalWrite(led, led_state);

        stopPos = !stopPos;
        if (stopPos) servoDegs = 0;
        else servoDegs = 180;

        servo1.write(servoDegs);

#if DEBUG
        Serial.print("Alarm OFF");
#endif
    }
}

void checkServer()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Error in WiFi connection"); //Check WiFi connection status
        serverCheck = 0;
        return;
    }

    http.begin("http://88.91.42.155/alarm-check"); //Specify destination for HTTP request
    int httpResponseCode = http.GET();             //Specify request type
    if (httpResponseCode > 0)
    {
        String response = http.getString(); //Get the response to the request

        Serial.println(httpResponseCode); //Print return code
        Serial.println(response);         //Print request answer
        
        serverCheck = 1;
        
        if (response == "true")
            states = stateAlarmON;
        else if (response == "test")
            states = stateTestServo;
        else
            states = stateAlarmOFF;
    }
    else
    {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
    }
    http.end(); //Free resources
    timeCheckServer = millis();
}

void sweepServo()
{
    servo1.write(servoDegs);

    if (millis() - timeCheckServo > 2000)
    {
        servoDegs = 0;
        timeCheckServo = millis();
    }
    else if (millis() - timeCheckServo > 1000) {
        servoDegs = 180;
    }
}