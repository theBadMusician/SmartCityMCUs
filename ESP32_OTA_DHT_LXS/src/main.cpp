#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <SPI.h>
#include "DHT.h"

#define DHTPIN 15     // Digital pin connected to the DHT sensor
#define DHTTYPE DHT22   // DHT 22  (AM2302), AM2321
DHT dht;


#define LIGHTPIN 33
int lightTotal, lightAvg;
int counter = 0;
int max_val = 551, min_val = 0;

const char* ssid = "PrivatNettverk";
const char* password = "Plechavicius1652";

uint32_t timeCheck0, timeCheck1;

StaticJsonDocument<JSON_OBJECT_SIZE(5)> doc;

void setup() {
    Serial.begin(115200);
    Serial.println("Booting");

    while (!Serial) continue;

    dht.setup(DHTPIN);    
  	
    // Setting up WIFI
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, password);
    while (WiFi.waitForConnectResult() != WL_CONNECTED) {
        Serial.println("Connection Failed! Rebooting...");
        delay(5000);
        ESP.restart();
    }

    // Port defaults to 3232
    // ArduinoOTA.setPort(3232);

    // Hostname defaults to esp3232-[MAC]
    // ArduinoOTA.setHostname("myesp32");

    // No authentication by default
    // ArduinoOTA.setPassword("admin");

    // Password can be set with it's md5 value as well
    // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
    // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

    ArduinoOTA
        .onStart([]() {
            String type;
            if (ArduinoOTA.getCommand() == U_FLASH)
                type = "sketch";
            else // U_SPIFFS
                type = "filesystem";

            // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
            Serial.println("Start updating " + type);
        })
        .onEnd([]() {
            Serial.println("\nEnd");
        })
        .onProgress([](unsigned int progress, unsigned int total) {
            Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
        })
        .onError([](ota_error_t error) {
            Serial.printf("Error[%u]: ", error);
            if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
            else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
            else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
            else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
            else if (error == OTA_END_ERROR) Serial.println("End Failed");
        });

    ArduinoOTA.begin();

    Serial.println("Ready");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());


    timeCheck0 = millis();
    timeCheck1 = millis();
}

void loop() {
    ArduinoOTA.handle();
    
	if (millis() - timeCheck0 > 200) {
	    int lightRead = analogRead(LIGHTPIN);
	    if (lightRead > max_val) max_val = lightRead;
		else if (lightRead < min_val) min_val = lightRead;
		lightRead = map(lightRead, min_val, max_val, 0, 100);

		lightTotal += lightRead;
		
		counter += 1;
		timeCheck0 = millis();
	}
    
    if (millis() - timeCheck1 > 1000) {
        if(WiFi.status() != WL_CONNECTED) {
        	Serial.println("Error in WiFi connection");						//Check WiFi connection status
        	return;        }
    	
    	String stringy;
    	
		JsonObject root = doc.to<JsonObject>();
		
		JsonObject docDHT22 = root.createNestedObject("DHT22");
		docDHT22["Temperature[°C]"] = dht.getTemperature();
        docDHT22["Humidity[%]"] = dht.getHumidity();

        JsonObject docLX = root.createNestedObject("Light_Sensor");
        docLX["Brightness[%]"] = (lightTotal / 1.0) / counter;        
        lightTotal = 0;
        counter = 0;
        
        serializeJson(root, stringy);

    	Serial.println(stringy);
		
        HTTPClient http;
 
        http.begin("http://88.91.42.155/post-test");    					//Specify destination for HTTP request
        http.addHeader("Content-Type", "application/json");                 //Specify content-type header

        int httpResponseCode = http.POST(stringy);     						//Send the actual POST request		Serial.println(stringy);
		
 		if(httpResponseCode>0){
            String response = http.getString();								//Get the response to the request
 
            Serial.println(httpResponseCode);								//Print return code
            Serial.println(response);										//Print request answer
        } else {
            Serial.print("Error on sending POST: ");
            Serial.println(httpResponseCode);
        }
	http.end();    //Free resources   
    timeCheck1 = millis();
	}
}
