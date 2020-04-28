#include <Arduino.h>
#include <Servo.h>
#include <analogWrite.h>

#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>

#define DEBUG true

// ESP32 constants
#define LED 33
#define BUZZER 32

// Network settings
const char *ssid = "PrivatNettverk";
const char *password = "Plechavicius1652";

// Buzzer settings
int freq = 2000;
int channel = 0;
int resolution = 8;

// LED state
bool ledState = 0;

// Time check variables
uint32_t timeCheckAlarm = 0,
         timeCheckServer = 0;

// Alarm flag
bool isAlarmOn = 0;

// States
enum StateMachine
{
    stateAlarmON,
    stateAlarmOFF
} states;

// Function declarations
void AlarmON(uint8_t led, bool &led_state, uint16_t buzzer_channel, uint32_t &time_check, bool &check_flag);
void AlarmOFF(uint8_t led, bool &led_state, uint16_t buzzer_channel, uint32_t &time_check, bool &check_flag);

void checkServer();

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
    ledcSetup(channel, freq, resolution);
    ledcAttachPin(32, channel);

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
}

void loop()
{
    //    AlarmON(LED, ledState, channel, timeCheckAlarm, isAlarmOn);
    if (millis() - timeCheckServer > 5000)
        checkServer();

    switch (states)
    {
    case stateAlarmON:
        AlarmON(LED, ledState, channel, timeCheckAlarm, isAlarmOn);
        break;
 
    case stateAlarmOFF:
        AlarmOFF(LED, ledState, channel, timeCheckAlarm, isAlarmOn);
        break;
    }
}

void AlarmON(uint8_t led, bool &led_state, uint16_t buzzer_channel, uint32_t &time_check, bool &check_flag)
{
    uint16_t buzzerFreq = 1000;
    check_flag = 1;

    if (millis() - time_check > 500)
    {

        if (millis() - time_check > 1000)
        {
            led_state = 0;
            digitalWrite(led, led_state);

            ledcWriteTone(buzzer_channel, buzzerFreq * 0.5);

            time_check = millis();
            goto jumpOver;
        }

        led_state = 1;
        digitalWrite(led, led_state);

        ledcWriteTone(buzzer_channel, buzzerFreq);
    }
jumpOver:
#if DEBUG
    Serial.print("Alarm LED state: ");
    Serial.println(led_state);
#endif
}

void AlarmOFF(uint8_t led, bool &led_state, uint16_t buzzer_channel, uint32_t &time_check, bool &check_flag)
{
    if (check_flag)
    {
        check_flag = 0;
        led_state = 0;

        ledcWriteTone(buzzer_channel, led_state);
        digitalWrite(led, led_state);

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
        return;
    }

    HTTPClient http;

    http.begin("http://88.91.42.155/alarm-check"); //Specify destination for HTTP request
    int httpResponseCode = http.GET();             //Specify content-type header

    if (httpResponseCode > 0)
    {
        String response = http.getString(); //Get the response to the request

        Serial.println(httpResponseCode); //Print return code
        Serial.println(response);         //Print request answer

        if (response == "true") states = stateAlarmON;
        else states = stateAlarmOFF;
    }
    else
    {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
    }
    http.end(); //Free resources
    timeCheckServer = millis();
}