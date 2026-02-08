// 2FA Sidecar
// Matt Perkins - Copyright (C) 2025
// Spawned out of the need to often type a lot of two factor authentication
// but still have some security while remaning mostly isolated from the host system.
// See github for 3D models and wiring diagram.
/*

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/

// Change for your country.
#define NTP_SERVER "au.pool.ntp.org" //Adjust to your local time perhaps. 
#define TZ "AEST" // Australian Estern time may be needed for clock display in the future. 



// Only other thing to select bellow is if your making the 5 key or 2 key version
// Default is 5 key.


char *mainver = "1.22";

#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <SPI.h>
#include <Preferences.h> // perstant storage
#include <esp_system.h> 

// Misc Fonts
#include "Fonts/FreeSans9pt7b.h"
#include "Fonts/FreeSans12pt7b.h"
#include "Fonts/FreeSans18pt7b.h"
#include "Fonts/FreeMono12pt7b.h"

#include <string>



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


#include <lwip/apps/sntp.h>
#include <TOTP-RC6236-generator.hpp>
#include <ESPmDNS.h>
#include <ArduinoOTA.h>



// Init HID so we look like a keyboard
USBHIDKeyboard Keyboard;

// Init our 5 keys
int bargraph_pos;
int updateotp;
long time_x;
String inputBuffer = "";
int notime;

PinButton key1(5);
PinButton key2(6);
PinButton key3(9);
PinButton key4(10);
PinButton key5(11);


int keytest = 0;

// Select either the 5 key or 2 key version
// maxkeys = 15 for 5 key
// maxkeys = 3 for 2 key

int maxkeys = 15;
int sline = 0;
int pinno = 0;
int wifitry = 10; // Number of times to try wifi before falling back to serial.
String  in_pin;

// Incorrect Pin Delay
int pindelay = 3000;

AsyncWebServer server(80);

// Setup SSID
String ssid     = "Key-Sidecar";
String password;
String  pin;

String tfa_name_1;
String tfa_seed_1;

String tfa_name_2;
String tfa_seed_2;

String tfa_name_3;
String tfa_seed_3;

String tfa_name_4;
String tfa_seed_4;

String tfa_name_5;
String tfa_seed_5;

// Paramaters wifi
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "password";
const char* PARAM_INPUT_3 = "pin";

// Paramaters 2FA
const char* TFA_INPUT_1 = "tfa_name_1";
const char* TFA_INPUT_2 = "tfa_seed_1";

const char* TFA_INPUT_3 = "tfa_name_2";
const char* TFA_INPUT_4 = "tfa_seed_2";

const char* TFA_INPUT_5 = "tfa_name_3";
const char* TFA_INPUT_6 = "tfa_seed_3";

const char* TFA_INPUT_7 = "tfa_name_4";
const char* TFA_INPUT_8 = "tfa_seed_4";

const char* TFA_INPUT_9 = "tfa_name_5";
const char* TFA_INPUT_10 = "tfa_seed_5";




// Init Screen
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);

// Init Preferences
Preferences preferences;


void setup() {

  
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

  // Print Bootup
  tft.setCursor(0, 0);
  tft.setTextColor(ST77XX_WHITE);
  tft.setTextWrap(true);
  tft.printf("\n2FA-Sidecar Ver %s\nBy Matt Perkins (C) 2024\n", mainver);
  tft.printf("Reset reason: %d\n", esp_reset_reason());
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

  tft.setFont(&FreeSans9pt7b);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  tft.setCursor(1, 15);
  tft.printf("2FA-Sidecar V%s - startup\n", mainver);

  preferences.begin("2FA_Sidecar", false);

  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");


  // load all pramaters
  tfa_name_1 = preferences.getString("tfa_name_1", "");
  tfa_seed_1 = preferences.getString("tfa_seed_1", "");

  tfa_name_2 = preferences.getString("tfa_name_2", "");
  tfa_seed_2 = preferences.getString("tfa_seed_2", "");

  tfa_name_3 = preferences.getString("tfa_name_3", "");
  tfa_seed_3 = preferences.getString("tfa_seed_3", "");

  tfa_name_4 = preferences.getString("tfa_name_4", "");
  tfa_seed_4 = preferences.getString("tfa_seed_4", "");

  tfa_name_5 = preferences.getString("tfa_name_5", "");
  tfa_seed_5 = preferences.getString("tfa_seed_5", "");

  WiFi.begin(ssid, password);

  sline = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tft.printf("Establishing WiFi %d\n", wifitry);
    sline = sline + 1;
    wifitry = wifitry - 1;
    if (sline > 4) {
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(1, 15);
      sline = 0 ;
    }
    if (wifitry == 0) {
      break;
    }
  }

  // If Wifi is found get time that way else try the serial port.
  if (wifitry != 0) {
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(1, 15);
    tft.print("IP: ");
    tft.println(WiFi.localIP());
    tft.print("Wifi: ");
    tft.print(WiFi.RSSI());
    // start the NTP client
    configTzTime(TZ, NTP_SERVER);
    tft.println();
    tft.printf("NTP started:%s", TZ);
    time_t t = time(NULL);
    tft.printf(":%d", t);
    tft.println();
    delay(1000);
  } else {
    // Wifi Not found get time via serial/usb
    notime = 0;
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_WHITE);
    tft.setCursor(1, 15);
    tft.print("WiFi not found. USB fallback");

    Serial.begin(115200);
    Serial.println("Enter a Unix timestamp:");
    while (notime == 0) {
      while (Serial.available()) {
        char c = Serial.read();

        if (c == '\n' || c == '\r') {
          if (inputBuffer.length() > 0) {
            time_t unixTime = inputBuffer.toInt();

            struct timeval tv = { unixTime, 0 };  // seconds, microseconds
            settimeofday(&tv, nullptr);          // Set system time

            struct tm timeinfo;
            if (getLocalTime(&timeinfo)) {
              Serial.print("Time set to: ");
              Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
              notime = 1;
            } else {
              Serial.println("Failed to get local time");
            }

            inputBuffer = "";  
            Serial.println("OK");
          }
        } else if (isDigit(c)) {
          inputBuffer += c;
        }
      }
      delay(10);
    }

  }


  // Check to seee if we have PIN set and ask if we do.
  pin = preferences.getString("pin", "");
  const char* npin = pin.c_str();
  if (strlen(npin) > 3) {
    tft.setCursor(25, 25);
    tft.setFont(&FreeSans12pt7b);
    tft.fillScreen(ST77XX_BLACK);
    tft.setTextColor(ST77XX_YELLOW);
    tft.print("PIN?");
    // read keys for PIN

    while (1) {
      static unsigned long lst = millis();
      if (millis() - lst < 1000) {
        // UPDATE KEYS
        key1.update();
        key2.update();
        key3.update();
        key4.update();
        key5.update();
      }

      lst = millis();

      if (key1.isClick()) {
        pinno = pinno + 1;
        in_pin = in_pin + "1";
        tft.print("*");
      }

      if (key2.isClick()) {
        pinno = pinno + 1;
        in_pin = in_pin + "2";
        tft.print("*");
      }

      if (key3.isClick()) {
        pinno = pinno + 1;
        in_pin = in_pin + "3";
        tft.print("*");
      }

      if (key4.isClick()) {
        pinno = pinno + 1;
        in_pin = in_pin + "4";
        tft.print("*");
      }

      if (key5.isClick()) {
        pinno = pinno + 1;
        in_pin = in_pin + "5";
        tft.print("*");
      }

      if ( pinno == 4) {
        if (in_pin == npin) {
          tft.println();
          tft.println("Correct.");
          break;
        } else {
          tft.println();
          tft.print("Incorrect!");
          delay(pindelay);
          ESP.restart();
        }
      }
    }



  } // end check PIN


  tft.setTextColor(ST77XX_WHITE);


  // Iniz Keyboard and begin display.
  tft.setFont(&FreeSans9pt7b);

  tft.println("Iniz USB keybaord\n");
  Keyboard.begin();
  USB.begin();
  delay(2000);
  tft.fillScreen(ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE);
  updateotp = 1;



}

void loop() {
  static unsigned long lst = millis();
  if (millis() - lst < 1000) {
    // UPDATE KEYS
    key1.update();
    key2.update();
    key3.update();
    key4.update();
    key5.update();
  }

  lst = millis();



  // Update Time.

  time_t t = time(NULL);

  if (t < 1000000) {
    delay(500);
    return;
  };


  // Print Bargraph
  bargraph_pos = (t % 60);
  if (bargraph_pos > 29) {
    bargraph_pos = bargraph_pos - 30;
  }

  bargraph_pos = bargraph_pos * 3;
  if (bargraph_pos < 70) {
    tft.fillCircle(bargraph_pos, 125, 3, ST77XX_GREEN);
  } else {
    tft.fillCircle(bargraph_pos, 125, 3, ST77XX_RED);
  }

  if (bargraph_pos == 0) {
    updateotp = 1;
  }


  // Display updated OTP per key

  if (updateotp == 1) {
    updateotp = 0;
    tft.setTextColor(ST77XX_YELLOW);
    tft.setFont(&FreeSans12pt7b);
    tft.fillScreen(ST77XX_BLACK);

    // Key 1
    if (String * otp1 = TOTP::currentOTP(tfa_seed_1)) {
      tft.setCursor(3, 17);
      tft.setTextColor(ST77XX_RED);
      tft.setFont(&FreeSans12pt7b);
      tft.print(tfa_name_1);
      tft.setCursor(140, 17);
      tft.setTextColor(ST77XX_YELLOW);
      tft.setFont(&FreeMono12pt7b);
      tft.println(*otp1);
    } else {
      tft.setCursor(3, 17);
      tft.setTextColor(ST77XX_RED);
      tft.print("NO VALID CONFIG");
      delay(10000);
      ESP.restart();

    };

    // Key 2
    if (String * otp2 = TOTP::currentOTP(tfa_seed_2)) {
      tft.setCursor(3, 40);
      tft.setTextColor(ST77XX_RED);
      tft.setFont(&FreeSans12pt7b);
      tft.print(tfa_name_2);
      tft.setCursor(140, 40);
      tft.setTextColor(ST77XX_YELLOW);
      tft.setFont(&FreeMono12pt7b);
      tft.println(*otp2);
    };

    // Key 3
    if (String * otp3 = TOTP::currentOTP(tfa_seed_3)) {
      tft.setCursor(3, 63);
      tft.setTextColor(ST77XX_RED);
      tft.setFont(&FreeSans12pt7b);
      tft.print(tfa_name_3);
      tft.setCursor(140, 63);
      tft.setTextColor(ST77XX_YELLOW);
      tft.setFont(&FreeMono12pt7b);
      tft.println(*otp3);
    };

    // Key 4
    if (String * otp4 = TOTP::currentOTP(tfa_seed_4)) {
      tft.setCursor(3, 86);
      tft.setTextColor(ST77XX_RED);
      tft.setFont(&FreeSans12pt7b);
      tft.print(tfa_name_4);
      tft.setCursor(140, 86);
      tft.setTextColor(ST77XX_YELLOW);
      tft.setFont(&FreeMono12pt7b);
      tft.println(*otp4);
    };

    // Key 5
    if (String * otp5 = TOTP::currentOTP(tfa_seed_5)) {
      tft.setCursor(3, 109);
      tft.setTextColor(ST77XX_RED);
      tft.setFont(&FreeSans12pt7b);
      tft.print(tfa_name_5);
      tft.setCursor(140, 109);
      tft.setTextColor(ST77XX_YELLOW);
      tft.setFont(&FreeMono12pt7b);
      tft.println(*otp5);
    };

    // Make up the rest of the second so we dont fliker the screen.
    delay(999);

  }


  // check keypress
  if (key1.isClick()) {
    String * otp1 = TOTP::currentOTP(tfa_seed_1);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Keyboard.println(*otp1);
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (key2.isClick()) {
    String * otp2 = TOTP::currentOTP(tfa_seed_2);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Keyboard.println(*otp2);
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (key3.isClick()) {
    String * otp3 = TOTP::currentOTP(tfa_seed_3);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Keyboard.println(*otp3);
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (key4.isClick()) {
    String * otp4 = TOTP::currentOTP(tfa_seed_4);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Keyboard.println(*otp4);
    digitalWrite(LED_BUILTIN, LOW);
  }

  if (key5.isClick()) {
    String * otp5 = TOTP::currentOTP(tfa_seed_5);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Keyboard.println(*otp5);
    digitalWrite(LED_BUILTIN, LOW);
  }
  // Hold down k5 to restart.
  if (key5.isLongClick()) {
    ESP.restart();
  }



}
