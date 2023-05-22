
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
    request->send_P(200, "text/html", "<HEAD><TITLE>Matt's 2FA Sidecar </TITLE></HEAD><BODY><H2>2FA Sidecar configuration menu - Matt Perkins</H2>" 
    "<FORM ACTION=\"/get\">SSID: <input type=\"text\" name=\"ssid\"><input type=\"submit\" value=\"Submit\"></form><br>" 
    "<FORM ACTION=\"/get\">WiFi Password: <input type=\"text\" name=\"password\"><input type=\"submit\" value=\"Submit\"></form><br>" 
    "<FORM ACTION=\"/get\">Timezone (seconds+/-) : <input type=\"text\" name=\"tz\"><input type=\"submit\" value=\"Submit\"></form><br>" 

    "</BODY></HTML>");
  });


  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest * request) {
    String inputMessage;
    String inputParam;
    
   preferences.begin("2FA_Sidecar",false);

    
    // GET input1 value on <ESP_IP>/get?ssid=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      inputParam = PARAM_INPUT_1;
      preferences.putString("ssid",inputMessage);

    }
    // GET input2 value on <ESP_IP>/get?password=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_2)->value();
      inputParam = PARAM_INPUT_2;
      preferences.putString("password",inputMessage);
    }
    // GET input3 value on <ESP_IP>/get?tz=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_3)->value();
      inputParam = PARAM_INPUT_3;
      preferences.putString("tz",inputMessage);
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }
    
    Serial.println(inputMessage);
    request->send(200, "text/html", "Configuring preferences ("
                  + inputParam + ") with value: " + inputMessage +
                  "<br><a href=\"/\">Return to Home Page</a>");
                  
  });
  
  server.onNotFound(notFound);
  server.begin();

  delay(600000);
  ESP.restart();

}


void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}
