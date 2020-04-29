#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

#define OLED_SDA 4
#define OLED_SCL 15
#define OLED_RST 16
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RST);
//**********************************************************************//
//**********************************************************************//

// Define left/right and up/down joystick axes' pins.
uint32_t check_time_OLED = 0;

struct dataPackage
{
    uint8_t LR_value;
    uint8_t UD_value;
};
dataPackage data;

// Network settings
const char *ssid = "PrivatNettverk";
const char *password = "Plechavicius1652";
HTTPClient http;

// Time check variables
uint32_t timeCheck = 0;

// States
enum StateMachine
{
    stateDriveSquare,
    stateDriveCircle,
    stateDriveLine,
    stateDriveSnake,
    stateStandby,
    stateStop
} states;

// Is server checked flag
bool serverCheck = 0;
uint16_t checkInterval = 1000;

// Functions
void checkServer();

void setup()
{

    //reset OLED display via software
    pinMode(OLED_RST, OUTPUT);
    digitalWrite(OLED_RST, LOW);
    delay(20);
    digitalWrite(OLED_RST, HIGH);

    //initialize OLED
    Wire.begin(OLED_SDA, OLED_SCL);
    if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false))
    { // Address 0x3C for 128x32
        Serial.println(F("SSD1306 allocation failed"));
        for (;;)
            ; // Don't proceed, loop forever
    }

    display.clearDisplay();
    display.setRotation(1);
    display.setTextColor(WHITE);
    display.setTextSize(1);
    display.setCursor(0, 0);
    display.println("SMART CITY");
    display.setCursor(0, 9);
    display.println("by");
    display.setCursor(5, 18);
    display.println("bad");
    display.setCursor(8, 27);
    display.println("Musician");
    display.display();

    //initialize Serial Monitor
    Serial.begin(115200);
    Serial2.begin(115200, SERIAL_8N1, -1, 17);

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
    timeCheck = millis();
}

void loop()
{
    if (millis() - timeCheck > checkInterval) checkServer();
    Serial2.write((uint8_t*)&data, sizeof(dataPackage));


}


void checkServer()
{
    if (WiFi.status() != WL_CONNECTED)
    {
        Serial.println("Error in WiFi connection"); //Check WiFi connection status
        serverCheck = 0;
        return;
    }

    http.begin("http://88.91.42.155/zumo-control"); //Specify destination for HTTP request
    int httpResponseCode = http.GET();             //Specify request type

    if (httpResponseCode > 0)
    {
        String response = http.getString(); //Get the response to the request

        Serial.println(httpResponseCode); //Print return code
        Serial.println(response);         //Print request answer
        
        serverCheck = 1;

        if (response == "patternSquare")
            states = stateDriveSquare;
        else if (response == "patternSquare")
            states = stateDriveCircle;
        else if (response == "patternLine")
            states = stateDriveLine;
        else if (response == "patternSnake")
            states = stateDriveSnake;
        else if (response == "stop")
            states = stateStop;
        else states = stateStandby;
    }
    else
    {
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);
    }
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println(httpResponseCode);
    display.display();

    http.end(); //Free resources
    timeCheck = millis();
}