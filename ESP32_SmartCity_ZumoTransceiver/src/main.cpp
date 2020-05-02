#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <LoRa.h>
#include <SocketIoClient.h>

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


// Network settings
const char *ssid = "PrivatNettverk";
const char *password = "Plechavicius1652";
HTTPClient http;

// Time check variables
uint32_t timeCheck = 0;

// RC control settings
uint32_t check_time_OLED = 0;

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 866E6

bool RCflag = 0;

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
    stateRCControl,
    stateDirectControl,
    nothing=99
} states;

struct dataPackage
{
    uint8_t STATE;
    uint8_t LR_value = 0;
    uint8_t UD_value = 0;
};
dataPackage data;

// Is server checked flag
bool serverCheck = 0;
uint32_t noConnectionTimer = 0;
uint16_t checkInterval = 1000;

/// Socket.IO Settings ///
char host[] = "88.91.42.155"; // Socket.IO Server Address
int port = 80; // Socket.IO Port Address
char path[] = "/socket.io/?transport=websocket"; // Socket.IO Base Path
bool useSSL = false; // Use SSL Authentication

bool directControlFlag = 0;

SocketIoClient webSocket;
WiFiClient client;

// Functions
void checkServer();
void updateStates();

void initializeLoRa();
void RCControl();

void socket_Connected(const char *payload, size_t length);
void socket_Disonnected(const char *payload, size_t length);
void socketToggleDirectControl(const char *payload, size_t length);
void socketDirectControlKeys(const char *payload, size_t length);

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

    webSocket.begin(host, port, path);

    // Websockets
    webSocket.on("connect", socket_Connected);
    webSocket.on("disconnect", socket_Disonnected);
    webSocket.on("zumoDirectControlCommand", socketDirectControlKeys);
    webSocket.on("directControlBtnUpdate", socketToggleDirectControl);

    
    // Time checks setup
    timeCheck = millis();
}

void loop()
{
    updateStates();

    if (RCflag)
    {
        initializeLoRa();
        while (RCflag)
        {
            RCControl();
            updateStates();
        }
    }

    else if (directControlFlag)
    {
        while (directControlFlag)
        {
            webSocket.loop();
            updateStates();

        }
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

    http.begin("http://88.91.42.155/zumo-control"); //Specify destination for HTTP request
    int httpResponseCode = http.GET();              //Specify request type

    display.clearDisplay();
    display.setTextSize(2);

    if (httpResponseCode > 0)
    {
        if (millis() - noConnectionTimer > 7000) RCflag = 0;

        String response = http.getString(); //Get the response to the request

        Serial.println(httpResponseCode); //Print return code
        Serial.println(response);         //Print request answer

        display.setCursor(0, 0);
        display.println(response);

        serverCheck = 1;

        if (response == "patternSquare")
        {
            states = stateDriveSquare;
            checkInterval = 10100;
        }
        else if (response == "patternCircle")
        {
            states = stateDriveCircle;
            checkInterval = 7100;
        }
        else if (response == "patternLine")
        {
            states = stateDriveLine;
            checkInterval = 5100;
        }
        else if (response == "patternSnake")
        {
            states = stateDriveSnake;
            checkInterval = 5100;
        }
        else if (response == "startLineFollower")
        {
            states = stateLineFollower;
            checkInterval = 2500;
        }
        else if (response == "startRCControl")
        {
            states = stateRCControl;
            checkInterval = 3100;
        }
        else if (response == "startDirectControl")
        {
            states = stateDirectControl;
            checkInterval = 3100;
        }
        else if (response == "stop")
        {
            display.setTextSize(2);
            RCflag = 0;
            directControlFlag = 0;
            states = stateStop;
            checkInterval = 500;
        }
        else
        {
            states = stateStandby;
            checkInterval = 500;
        }

        noConnectionTimer = millis();
    }
    else
    {
        display.setCursor(0, 0);
        display.println("ERROR");
        Serial.print("Error on sending POST: ");
        Serial.println(httpResponseCode);

        if (millis() - noConnectionTimer > 7000) {
            if (!RCflag) {
                states = stateRCControl;
                RCflag = 1;
            }
            display.setCursor(0, 32);
            display.println("DC RC EN");
            
        }
        else if (millis() - noConnectionTimer > 5000) states = stateStop;
        else states = stateStandby;
    }

    display.setCursor(0, 112);
    display.println(httpResponseCode);
    display.display();

    http.end(); //Free resources
    timeCheck = millis();
}

void updateStates()
{
    if (millis() - timeCheck > checkInterval)
    {
        checkServer();
    }

    if (serverCheck)
    {

        Serial.println(states);
        data.STATE = states;
        Serial2.write((uint8_t *)&data, sizeof(dataPackage));

        serverCheck = 0;

        if (states == stateRCControl) RCflag = 1;
        if (states == stateDirectControl) directControlFlag = 1;
        if (states == stateStop) {
            directControlFlag = 0;
            RCflag = 0;
        }

        states = stateStandby;
        data.STATE = states;
        Serial2.write((uint8_t *)&data, sizeof(dataPackage));
    }
}

void initializeLoRa()
{
    //SPI LoRa pins
    SPI.begin(SCK, MISO, MOSI, SS);
    //setup LoRa transceiver module
    LoRa.setPins(SS, RST, DIO0);

    if (!LoRa.begin(BAND))
    {
        Serial.println("Starting LoRa failed!");
        delay(5000);
        ESP.restart();
    }
    display.setTextSize(1);

}

void RCControl()
{
    int rssi;

    //try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize)
    {
        //received a packet

        //read packet
        while (LoRa.available())
            LoRa.readBytes((uint8_t *)&data, packetSize);

        //print RSSI of packet
        rssi = LoRa.packetRssi();
    }
    Serial2.write((uint8_t *)&data, sizeof(dataPackage));

    char buffer[100];
    sprintf(buffer, "%d\n%d", data.LR_value, data.UD_value);
    if (millis() - check_time_OLED >= 100)
    {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(buffer);

        display.setCursor(0, 50);
        display.print("RSSI: ");
        display.println(rssi);

        display.display();
        check_time_OLED = millis();
        Serial.print(buffer);
        Serial.print(" ");
        Serial.print(data.STATE);
        Serial.print(" ");
        Serial.println(states);
    }
}

void socket_Connected(const char *payload, size_t length)
{
    Serial.println("Socket.IO Connected!");
}

void socket_Disonnected(const char *payload, size_t length)
{
    Serial.println("Socket.IO Disconnected!");
}

void socketDirectControlKeys(const char *payload, size_t length)
{
    Serial.print("got message: ");
    if (data.LR_value != *(payload)-48)
        data.LR_value = *(payload)-48;
    Serial.println(data.LR_value);
    Serial2.write((uint8_t *)&data, sizeof(dataPackage));
}

void socketToggleDirectControl(const char *payload, size_t length)
{
    Serial.print("got message: ");
    Serial.println(payload);
    updateStates();
}