#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ArduinoOTA.h>
#include <ESPAsyncWebServer.h>

#include "config.h"

// Led Config
#define FASTLED_ESP8266_RAW_PIN_ORDER
#define FASTLED_ALLOW_INTERRUPTS 0

#include <FastLED.h>

#define DATA_PIN D1
#define COLOR_ORDER GRB
#define NUM_LEDS 8
#define CHIPSET WS2812B
// Define the array of leds
CRGB leds[NUM_LEDS];

#define LED_SCROLL_RIGHT 0
#define LED_SCROLL_LEFT 1
#define LED_SCROLL_SUPERCAR 2
#define LED_BLINK 3

// #include <DNSServer.h>
// #include <vector>

#include <string>
#include <iostream>

const char* PARAM_MESSAGE = "message";

AsyncWebServer server(TCP_PORT);

int alarm = 0;

void notFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not found");
}

void fadeall() {
  for (int i = 0; i < NUM_LEDS; i++) {
    leds[i].nscale8(160);
  }
}

void leds_effect (int effect, CHSV hue_color, int cycles) {

  int loops = 0;
  int i = 0;

  switch (effect)
  {
  case 0:
    Serial.println("Scrolling Leds Right to Left");
    for ( loops = 0; loops < cycles; loops++ ) {
      for ( i = 0; i < NUM_LEDS; i++ ) {
        // Set the i'th led to red
        leds[i] = CHSV(hue_color);
        // Show the leds
        FastLED.show();
        // now that we've shown the leds, reset the i'th led to black
        fadeall();
        // Wait a little bit before we loop around and do it again
        delay(75);
      }
    }
    break;
  case 1:
    Serial.println("Scrolling Leds Left to Right");
    for ( loops = 0; loops < cycles; loops++ ) {
      for (int i = (NUM_LEDS) - 1; i >= 0; i--) {
        // Set the i'th led to red
        leds[i] = CHSV(hue_color);
        // Show the leds
        FastLED.show();
        // now that we've shown the leds, reset the i'th led to black
        fadeall();
        // Wait a little bit before we loop around and do it again
        delay(75);
      }
    }
    break;
  case 2:
    //static uint8_t hue = 0;

    Serial.println("Supercar");

    for ( int hue = 0; hue < 255; hue++ ) {
      // First slide the led in one effect
      for (int i = 0; i < NUM_LEDS; i++) {
        // Set the i'th led to red
        leds[i] = CHSV(hue++, 255, 255);
        // Show the leds
        FastLED.show();
        // now that we've shown the leds, reset the i'th led to black
        // leds[i] = CRGB::Black;
        fadeall();
        // Wait a little bit before we loop around and do it again
        delay(75);
      }

    // Now go in the other effect.
      for (int i = (NUM_LEDS) - 1; i >= 0; i--) {
        // Set the i'th led to red
        leds[i] = CHSV(hue++, 255, 255);
        // Show the leds
        FastLED.show();
        // now that we've shown the leds, reset the i'th led to black
        // leds[i] = CRGB::Black;
        fadeall();
        // Wait a little bit before we loop around and do it again
        delay(75);
      }
    }
    break;
  case 3:
    Serial.println("Flash on/off");

    for ( loops = 0; loops < cycles; loops++ ) {
      // preload with full solid colour
      fill_solid( leds, NUM_LEDS, CHSV(hue_color));
      FastLED.show();

      // Decrease luminosity
      for ( i = 0; i < hue_color.val; i++ ) {
        
        fill_solid( leds, NUM_LEDS, CHSV(hue_color.hue, hue_color.sat, hue_color.val - i));
        FastLED.show();
        delay(5);
      }

      // Increase Luminosity
      for ( i = 0; i < hue_color.val; i++ ) {
        fill_solid( leds, NUM_LEDS, CHSV(hue_color.hue, hue_color.sat, i));
        FastLED.show();
        delay(5);
      }
    }
    break;
  default:
    break;
  }
}

void controlLeds() {

  //if alarm is active LEDs go red
  if ( alarm ) {
    leds_effect(LED_SCROLL_RIGHT,rgb2hsv_approximate(CRGB::Red),1);
    //fill_solid(leds,NUM_LEDS,CRGB::Red);
    FastLED.show();
  } else {
    //leds_effect(LED_SCROLL_SUPERCAR,rgb2hsv_approximate(CRGB::Red),1);
    fill_solid(leds,NUM_LEDS,CRGB::Green);
    FastLED.show();
  }
}


std::string createWebPage() {
  /*------------------HTML Page Creation---------------------*/

  std::string reply;

  Serial.println("\nCreating Web Page...");

  reply.append("<!DOCTYPE HTML>\n");
  reply.append("<html>\n");
  reply.append("LED: ");

  reply.append( alarm ? "ALARM!!" : "HEALTHY");

  reply.append("\n<br><br>\n");
  reply.append("<a href='get?message=alarm'><button>ALARM</button></a>\n");
  reply.append("<a href='get?message=healthy'><button>HEALTHY</button></a><br />\n");
  reply.append("<a href='get?message=toggle'><button>TOGGLE</button></a><br />\n");
  reply.append("</html>\n");

  Serial.println("\nWeb Page Created:\n-------");
  Serial.println(reply.c_str());
  Serial.println("-------");

  return reply;

}

void setup() {
	
  Serial.begin(115200);
	delay(20);

  Serial.println("resetting");

  LEDS.addLeds<CHIPSET, DATA_PIN, COLOR_ORDER>(leds, NUM_LEDS);
  LEDS.setBrightness(84);

  Serial.print("\nConnecting to the Network");

  // connects to access point
	WiFi.mode(WIFI_STA);
	WiFi.begin(SSID, PASSWORD);

  int counter = 1;

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    counter++;
    if ( counter == 50 ) {
      Serial.print("\n");
      counter = 1;
    }
  }
  Serial.println("WiFi connected");
  Serial.println("Server started");

  Serial.print("IP Address of network: "); // Prints IP address on Serial Monitor
  Serial.println(WiFi.localIP());
  Serial.print("Copy and paste the following URL: http://");
  Serial.print(WiFi.localIP());
  Serial.println("/");

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
      request->send(200, "text/html", createWebPage().c_str());
      Serial.println("Posting Web Page!!");
  });

  // Send a GET request to <IP>/get?message=<message>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
      String message;
      if (request->hasParam(PARAM_MESSAGE)) {
          message = request->getParam(PARAM_MESSAGE)->value();

          if ( message == "alarm" )
          {
            alarm = 1;
          }
          if ( message == "healthy" )
          {
            alarm = 0;
          }

          if ( message == "toggle" )
          {
            Serial.print("TOGGLING ");
            Serial.print("alarm: ");
            Serial.println(alarm);
            
            if ( alarm ) {
              alarm = 0;
            } else {
              alarm = 1;
            }
          }

      } else {
          message = "No message sent";
      }
      
      request->send(200, "text/html", createWebPage().c_str());
  });

  // Send a POST request to <IP>/post with a form field message set to <message>
  server.on("/post", HTTP_POST, [](AsyncWebServerRequest *request){
      String message;
      if (request->hasParam(PARAM_MESSAGE, true)) {
          message = request->getParam(PARAM_MESSAGE, true)->value();

          if ( message == "alarm" )
          {
            alarm = 1;
          }
          if ( message == "healthy" )
          {
            alarm = 0;
          }

          if ( message == "toggle" )
          {
            Serial.print("TOGGLING ");
            Serial.print("alarm: ");
            Serial.println(alarm);
            
            if ( alarm ) {
              alarm = 0;
            } else {
              alarm = 1;
            }
          }
      } else {
          message = "No message sent";
      }
       request->send(200, "text/html", createWebPage().c_str());
  });

  server.onNotFound(notFound);

  server.begin();
}

void loop() {

  #ifdef ESP8266
  ArduinoOTA.handle();
  #endif

  controlLeds();

}