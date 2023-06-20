// 2FA Sidecar
// Matt Perkins - Copyright (C) 2023
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


void setup_test()
{


  int bargraph_pos;
  long time_x;
  PinButton key1(5);
  PinButton key2(6);
  PinButton key3(9);
  PinButton key4(10);
  PinButton key5(11);

  int keytest = 0;
  tft.fillScreen(ST77XX_RED);
  delay(500);
  tft.fillScreen(ST77XX_GREEN);
  delay(500);
  tft.fillScreen(ST77XX_BLUE);
  delay(500);


  tft.fillScreen(ST77XX_BLACK);

  tft.setFont(&FreeSans12pt7b);
  tft.setTextColor(ST77XX_WHITE);

  tft.setCursor(3, 18);
  tft.print("Key 1");

  tft.setCursor(3, 38);
  tft.print("Key 2");

  tft.setCursor(3, 58);
  tft.print("Key 3");

  tft.setCursor(3, 78);
  tft.print("Key 4");

  tft.setCursor(3, 98);
  tft.print("Key 5");

  while (1) {
    key1.update();
    key2.update();
    key3.update();
    key4.update();
    key5.update();


    if (key1.isClick()) {
      tft.setCursor(65, 18);
      tft.print("OK");
      keytest = keytest + 1;
    }

    if (key2.isClick()) {
      tft.setCursor(65, 38);
      tft.print("OK");
      keytest = keytest + 2;
    }
    if (key3.isClick()) {
      tft.setCursor(65, 58);
      tft.print("OK");
      keytest = keytest + 3;
    }
    if (key4.isClick()) {
      tft.setCursor(65, 78);
      tft.print("OK");
      keytest = keytest + 4;
    }
    if (key5.isClick()) {
      tft.setCursor(65, 98);
      tft.print("OK");
      keytest = keytest + 5;
    }

    if (keytest == 15) {
      tft.setCursor(105, 58);
      tft.print("Test Pass");
      delay(2000);
      tft.setCursor(0, 0);
      tft.fillScreen(ST77XX_RED);
      delay(500);
      tft.setTextColor(ST77XX_WHITE);
      break;


    }
  }
  wifi_setup();

}

void wifi_setup()
{

  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid);

  tft.setCursor(3, 35);
  tft.fillScreen(ST77XX_RED);
  tft.printf("Connect via Wifi\nSSID:%s\nthen browse to \nhttp://192.168.4.1", ssid);



  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send_P(200, "text/html", "<HEAD><TITLE>Matt's 2FA-Sidecar </TITLE></HEAD><BODY><H2>2FA-Sidecar configuration menu - (C) 2023 Matt Perkins - GPL</H2>"
                    "You may submit only one option at a time - current settings are not displayed for security.<p>"
                    "<FORM ACTION=\"/get\">SSID: <input type=\"text\" name=\"ssid\"><input type=\"submit\" value=\"Submit\"></form><br>"
                    "<FORM ACTION=\"/get\">WiFi Password: <input type=\"text\" name=\"password\"><input type=\"submit\" value=\"Submit\"></form><br>"
                    "<FORM ACTION=\"/get\">Access PIN (4 digits.blank for none) : <input type=\"text\" name=\"pin\"><input type=\"submit\" value=\"Submit\"></form><br>"

                    "<FORM ACTION=\"/get\">2FA Key 1 Name : <input type=\"text\" name=\"tfa_name_1\"><input type=\"submit\" value=\"Submit\"></form><br>"
                    "<FORM ACTION=\"/get\">2FA Key 1 Seed : <input type=\"text\" name=\"tfa_seed_1\"><input type=\"submit\" value=\"Submit\"></form><br>"

                    "<FORM ACTION=\"/get\">2FA Key 2 Name : <input type=\"text\" name=\"tfa_name_2\"><input type=\"submit\" value=\"Submit\"></form><br>"
                    "<FORM ACTION=\"/get\">2FA Key 2 Seed : <input type=\"text\" name=\"tfa_seed_2\"><input type=\"submit\" value=\"Submit\"></form><br>"

                    "<FORM ACTION=\"/get\">2FA Key 3 Name : <input type=\"text\" name=\"tfa_name_3\"><input type=\"submit\" value=\"Submit\"></form><br>"
                    "<FORM ACTION=\"/get\">2FA Key 3 Seed : <input type=\"text\" name=\"tfa_seed_3\"><input type=\"submit\" value=\"Submit\"></form><br>"

                    "<FORM ACTION=\"/get\">2FA Key 4 Name : <input type=\"text\" name=\"tfa_name_4\"><input type=\"submit\" value=\"Submit\"></form><br>"
                    "<FORM ACTION=\"/get\">2FA Key 4 Seed : <input type=\"text\" name=\"tfa_seed_4\"><input type=\"submit\" value=\"Submit\"></form><br>"

                    "<FORM ACTION=\"/get\">2FA Key 5 Name : <input type=\"text\" name=\"tfa_name_5\"><input type=\"submit\" value=\"Submit\"></form><br>"
                    "<FORM ACTION=\"/get\">2FA Key 5 Seed : <input type=\"text\" name=\"tfa_seed_5\"><input type=\"submit\" value=\"Submit\"></form><br>"

                    "</BODY></HTML>");
  });


  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam;

    preferences.begin("2FA_Sidecar", false);


    // GET input1 value on <ESP_IP>/get?ssid=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      preferences.putString("ssid", inputMessage);

    }
    // GET input2 value on <ESP_IP>/get?password=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      preferences.putString("password", inputMessage);
    }
    // GET input3 value on <ESP_IP>/get?pin=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      preferences.putString("pin", inputMessage);
    }

    // Key 1
    // GET name 1
    else if (request->hasParam(TFA_INPUT_1)) {
      inputMessage = request->getParam(TFA_INPUT_1)->value();
      inputParam = TFA_INPUT_1;
      preferences.putString("tfa_name_1", inputMessage);
    }
    // GET name 2
    else if (request->hasParam(TFA_INPUT_2)) {
      inputMessage = request->getParam(TFA_INPUT_2)->value();
      inputParam = TFA_INPUT_2;
      preferences.putString("tfa_seed_1", inputMessage);
    }

    // Key 2
    // GET name 3
    else if (request->hasParam(TFA_INPUT_3)) {
      inputMessage = request->getParam(TFA_INPUT_3)->value();
      inputParam = TFA_INPUT_3;
      preferences.putString("tfa_name_2", inputMessage);
    }
    // GET name 4
    else if (request->hasParam(TFA_INPUT_4)) {
      inputMessage = request->getParam(TFA_INPUT_4)->value();
      inputParam = TFA_INPUT_4;
      preferences.putString("tfa_seed_2", inputMessage);
    }

    // Key 3
    // GET name 5
    else if (request->hasParam(TFA_INPUT_5)) {
      inputMessage = request->getParam(TFA_INPUT_5)->value();
      inputParam = TFA_INPUT_5;
      preferences.putString("tfa_name_3", inputMessage);
    }
    // GET name 6
    else if (request->hasParam(TFA_INPUT_6)) {
      inputMessage = request->getParam(TFA_INPUT_6)->value();
      inputParam = TFA_INPUT_6;
      preferences.putString("tfa_seed_3", inputMessage);
    }

    // Key 4
    // GET name 7
    else if (request->hasParam(TFA_INPUT_7)) {
      inputMessage = request->getParam(TFA_INPUT_7)->value();
      inputParam = TFA_INPUT_7;
      preferences.putString("tfa_name_4", inputMessage);
    }
    // GET name 8
    else if (request->hasParam(TFA_INPUT_8)) {
      inputMessage = request->getParam(TFA_INPUT_8)->value();
      inputParam = TFA_INPUT_8;
      preferences.putString("tfa_seed_4", inputMessage);
    }

    // Key 5
    // GET name 9
    else if (request->hasParam(TFA_INPUT_9)) {
      inputMessage = request->getParam(TFA_INPUT_9)->value();
      inputParam = TFA_INPUT_9;
      preferences.putString("tfa_name_5", inputMessage);
    }
    // GET name 10
    else if (request->hasParam(TFA_INPUT_10)) {
      inputMessage = request->getParam(TFA_INPUT_10)->value();
      inputParam = TFA_INPUT_10;
      preferences.putString("tfa_seed_5", inputMessage);
    }



    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }


    request->send(200, "text/html", "Setting preferences ("
                  + inputParam + ") with value: " + inputMessage +
                  "<br><a href=\"/\">Return to Home configuration</a>");

  });

  server.onNotFound(notFound);
  server.begin();

  delay(600000);
  ESP.restart();

}


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
