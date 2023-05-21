// Key Sidecar
// Matt Perkins - Spawned out of the need to quickly type a lot of two factor autentication
// but still have some security while remaning mostly isolated from the host system.
// Not for public release at the moment.


char *mainver = "1.00";


#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h> // Serial 
#include <Preferences.h> // perstant storage

// Misc Fonts
#include "Fonts/Picopixel.h"
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeSans24pt7b.h"

#include <TOTP.h> // One time password time based library 
#include <PinButton.h> // Button Library 

#include <USB.h>
#include <USBHIDKeyboard.h>

#include <Arduino.h>
#ifdef ESP32
#include <WiFi.h>
#include <AsyncTCP.h>
#elif defined(ESP8266)
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#endif
#include <ESPAsyncWebSrv.h>
#include <NTPClient.h> // Time Deamon 



// Init HID so we look like a keyboard
USBHIDKeyboard Keyboard;

// Init our 5 keys
int bargraph_pos;
long time_x;
PinButton key1(5);
PinButton key2(6);
PinButton key3(9);
PinButton key4(10);
PinButton key5(11);

int keytest = 0;


  AsyncWebServer server(80);

  // Setup SSID
  const char* ssid     = "Key-Sidecar";

  // Paramaters
  const char* PARAM_INPUT_1 = "ssid";
  const char* PARAM_INPUT_2 = "password";
  const char* PARAM_INPUT_3 = "tz";


  // HTML web page to handle 3 input fields (input1, input2, input3)
  const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>Matt's 2FA Sidecar </title>
  
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
  <h2>2FA Sidecar configuration menu - Matt Perkins</h2>

  <form action="/get">
    SSID: <input type="text" name="ssid">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Password: <input type="text" name="password">
    <input type="submit" value="Submit">
  </form><br>
  <form action="/get">
    Timezone: <input type="text" name="tz">
    <input type="submit" value="Submit">
  </form>
</body></html>)rawliteral";








// Wifi Char
//char ssid[] = "";
//char password[] = "";


// Init Screen
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Init Preferences
Preferences preferences;


void setup() {
  Serial.begin(9600);
  Serial.printf("Key Sidecar %s - startup\n", mainver); // output version test serial.

  // turn on backlite
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);

  // turn on the TFT / I2C power supply
  pinMode(TFT_I2C_POWER, OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);

  // initialize TFT
  tft.init(135, 240); // Init ST7789 240x135
  tft.setRotation(3);
  tft.fillScreen(ST77XX_BLACK);
  tft.setFont(&FreeSans9pt7b);

  // Print Bootup / Welcome
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.printf("\nKey Sidecar Ver %s\nBy Matt Perkins\n", mainver);
  tft.printf("Press K1 to enter config/test\n");

  // Check for key and go to setup /test mode
  int lcount = 0 ;
  while (lcount < 140) {

    key1.update();

    if (key1.isClick()) {
      setup_test();
      ESP.restart();
    }

    tft.printf(".");
    delay(10);
    lcount++;
  }

  // Continue to run normaly.
  tft.setFont(&FreeSans9pt7b);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(3, 15);

  tft.printf("Main code here\n");
  delay(30000);


}

void loop() {
  // put your main code here, to run repeatedly:
  delay(1000);
  ESP.restart();

}
