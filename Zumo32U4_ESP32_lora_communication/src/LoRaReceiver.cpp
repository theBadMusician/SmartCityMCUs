#ifdef RUN
//**********************************************************************//
//**********************************************************************//
#include <Arduino.h>
#include <SPI.h>
#include <LoRa.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCK 5
#define MISO 19
#define MOSI 27
#define SS 18
#define RST 14
#define DIO0 26

//433E6 for Asia
//866E6 for Europe
//915E6 for North America
#define BAND 866E6

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

struct dataPackage {
  uint8_t LR_value;
  uint8_t UD_value;
};
dataPackage data;


void setup() { 
  
  //reset OLED display via software
  pinMode(OLED_RST, OUTPUT);
  digitalWrite(OLED_RST, LOW);
  delay(20);
  digitalWrite(OLED_RST, HIGH);
  
  //initialize OLED
  Wire.begin(OLED_SDA, OLED_SCL);
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3c, false, false)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  display.clearDisplay();
  display.setRotation(3);
  display.setTextColor(WHITE);
  display.setTextSize(1);
  display.setCursor(0,0);
  display.print("LORA RECEIVER ");
  display.display();
  
  //initialize Serial Monitor
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, -1, 17);

  Serial.println("LoRa Receiver Test");
  
  //SPI LoRa pins
  SPI.begin(SCK, MISO, MOSI, SS);
  //setup LoRa transceiver module
  LoRa.setPins(SS, RST, DIO0);

  if (!LoRa.begin(BAND)) {
    Serial.println("Starting LoRa failed!");
    while (1);
  }
  Serial.println("LoRa Initializing OK!");
  display.setCursor(0,10);
  display.println("LoRa Initializing OK!");
  display.display();  
}

void loop() {
    int rssi;

    //try to parse packet
    int packetSize = LoRa.parsePacket();
    if (packetSize) {
        //received a packet

        //read packet
        while (LoRa.available()) LoRa.readBytes((uint8_t *)&data, packetSize);

        //print RSSI of packet
        rssi = LoRa.packetRssi();
    }
    Serial2.write((uint8_t*)&data, sizeof(dataPackage));
    
    char buffer[100];
    sprintf(buffer, "%d %d", data.LR_value, data.UD_value);
	if (millis() - check_time_OLED >= 100) {
        display.clearDisplay();
        display.setCursor(0, 0);
        display.println(buffer);
        
        display.setCursor(0, 9);
        display.print("RSSI: ");
        display.println(rssi);


        display.display();
        check_time_OLED = millis();
    }
}
#endif