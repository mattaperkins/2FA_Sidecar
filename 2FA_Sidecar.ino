// 2FA Sidecar
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
#include "Fonts/FreeMono9pt7b.h"

#include <string>

#define NTP_SERVER "au.pool.ntp.org"


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
PinButton key1(5);
PinButton key2(6);
PinButton key3(9);
PinButton key4(10);
PinButton key5(11);

int keytest = 0;
int sline = 0;


AsyncWebServer server(80);

// Setup SSID
String ssid     = "Key-Sidecar";
String password;
String tz;

String tfa_name_1;
String tfa_seed_1;


// Paramaters wifi
const char* PARAM_INPUT_1 = "ssid";
const char* PARAM_INPUT_2 = "password";
const char* PARAM_INPUT_3 = "tz";

// Paramaters 2FA

const char* TFA_INPUT_1 = "tfa_name_1";
const char* TFA_INPUT_2 = "tfa_seed_1";




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
  tft.setCursor(1, 15);
  tft.printf("Key Sidecar %s - startup\n", mainver);

  preferences.begin("2FA_Sidecar", false);

  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");


  // load all pramaters
  tfa_name_1 = preferences.getString("tfa_name_1", "");
  tfa_seed_1 = preferences.getString("tfa_seed_1", "");


  WiFi.begin(ssid, password);

  sline = 0;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    tft.printf("Establishing WiFi\n");
    sline = sline + 1;
    if (sline > 4) {
      tft.fillScreen(ST77XX_BLACK);
      tft.setTextColor(ST77XX_WHITE);
      tft.setCursor(3, 5);
      sline = 0 ;
    }
  }
  tft.print("IP: ");
  tft.println(WiFi.localIP());
  tft.print("Wifi: ");
  tft.print(WiFi.RSSI());

  // start the NTP client
  tz = preferences.getString("tz", "");
  const char  *ntz = tz.c_str();
  configTzTime(ntz, NTP_SERVER);
  tft.println();
  tft.printf("NTP started:%s", ntz);
  time_t t = time(NULL);
  tft.printf(":%d", t);
  tft.println();

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
    Serial.println("Not having a stable time yet.. TOTP is not going to work.");
    return;
  };



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


  // Display updated OTP.
  if (updateotp == 1) {
    updateotp = 0;
    String * otp = TOTP::currentOTP(tfa_seed_1);
    tft.setTextColor(ST77XX_YELLOW);
    tft.setFont(&FreeSans12pt7b);
    tft.fillScreen(ST77XX_BLACK);

    // Key 1
    tft.setCursor(3, 17);
    tft.setTextColor(ST77XX_RED);
    tft.print(tfa_name_1);
    tft.setCursor(140, 17);
    tft.setTextColor(ST77XX_YELLOW);
    tft.println(*otp);

    // Key 2
    tft.setCursor(3, 40);
    tft.setTextColor(ST77XX_RED);
    tft.print(tfa_name_1);
    tft.setCursor(140, 40);
    tft.setTextColor(ST77XX_YELLOW);
    tft.println(*otp);

    // Key 3
    tft.setCursor(3, 63);
    tft.setTextColor(ST77XX_RED);
    tft.print(tfa_name_1);
    tft.setCursor(140, 63);
    tft.setTextColor(ST77XX_YELLOW);
    tft.println(*otp);

    // Key 4
    tft.setCursor(3, 86);
    tft.setTextColor(ST77XX_RED);
    tft.print(tfa_name_1);
    tft.setCursor(140, 86);
    tft.setTextColor(ST77XX_YELLOW);
    tft.println(*otp);

    // Key 5
    tft.setCursor(3, 109);
    tft.setTextColor(ST77XX_RED);
    tft.print(tfa_name_1);
    tft.setCursor(140, 109);
    tft.setTextColor(ST77XX_YELLOW);
    tft.println(*otp);

  }



  // check keypress
  if (key1.isClick()) {
    String * otp = TOTP::currentOTP(tfa_seed_1);
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, HIGH);
    Keyboard.println(*otp);
    //Keyboard.press(KEY_LEFT_CTRL); Keyboard.press('m'); Keyboard.releaseAll(); // CR=Ctrl-M
    digitalWrite(LED_BUILTIN, LOW);
  }

}
