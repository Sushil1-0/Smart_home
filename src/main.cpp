
#include "wifi_config/wifiTool.h"
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
#include <SPIFFS.h>
// #define LED2 12
#define sensor1 32 // upper tank
// #define sensor2 32 // lower tank
#define manled 12
#define buzzer 14
#define autoled 2
#define relay 4
const int pumpstop = 25;   // manual case ma start ra stop ko lagi
const int start = 18;      // manual case ma start garna ko lagi
const int AMselector = 34; // auto or mannual selector

int state1 = 1;
int state2 = 0;
int state3 = 0;
#include <ezButton.h> // Include ezButton library

ezButton button1(AMselector); // Initialize ezButton library on AMselector pin
ezButton button2(pumpstop);
ezButton button3(start);

#define EEPROM_SIZE 16

WifiTool wifiTool;

/////////////////////////////////////////////////
int Percentage;
//////////////////////////////////////////////////////
#include <ESPAsyncWebServer.h>
AsyncWebServer server(80); // server port 80
WebSocketsServer websockets(81);

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Page Not found");
}

void webSocketEvent(uint8_t num, WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[%u] Disconnected!\n", num);
    break;
  case WStype_CONNECTED:
  {
    IPAddress ip = websockets.remoteIP(num);
    Serial.printf("[%u] Connected from %d.%d.%d.%d url: %s\n", num, ip[0], ip[1], ip[2], ip[3], payload);
  }
  break;
  case WStype_TEXT:
    Serial.printf("[%u] get Text: %s\n", num, payload);
    String message = String((char *)(payload));
    Serial.println(message);

    DynamicJsonDocument doc(200);
    DeserializationError error = deserializeJson(doc, message);
    if (error)
    {
      Serial.print("deserializeJson() failed: ");
      Serial.println(error.c_str());
      return;
    }

    int LED1_status = doc["relay"];
    int LED2_status = doc["LED2"];
    digitalWrite(relay, LED1_status);
    // digitalWrite(LED2, LED2_status);
  }
}

/**********************************************************************************************************************************************/
////////////////////////*mode selection*//////////////////////////////////
void modeselection()
{
  switch (state1)
  {
  case 1: // Auto Mode
    EEPROM.write(4, 2);
    delay(100);
    break;
  case 2: // Manual Mode
    EEPROM.write(4, 1);
    delay(100);
    break;
  default:
    break;
  }
}

void on()
{
  switch (state3)
  {
  case 1:
    EEPROM.write(5, 1);
    break;

  default:
    break;
  }
}
void off()
{
  switch (state2)
  {
  case 1:
    EEPROM.write(5, 0);
    break;

  default:
    break;
  }
}
void setup()
{
  int a = 0;
  Serial.begin(115200);
  pinMode(relay, OUTPUT);
  pinMode(autoled, OUTPUT);
  pinMode(manled, OUTPUT);
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(1, 1);
  pinMode(buzzer, OUTPUT);
  EEPROM.write(4, 2);
  Serial.println("System started");
  pinMode(sensor1, INPUT);
  pinMode(AMselector, INPUT);
  pinMode(start, INPUT);
  pinMode(pumpstop, INPUT);
  SPIFFS.begin();

  wifiTool.begin(false);
  if (!wifiTool.wifiAutoConnect())
  {
    Serial.println("fail to connect to wifi!!!!");
    wifiTool.runApPortal();
  }
  if (MDNS.begin("ESP"))
  { // esp.local/
    Serial.println("MDNS responder started");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/main.html", "text/html"); });
  server.onNotFound(notFound);

  server.begin();
  websockets.begin();
  websockets.onEvent(webSocketEvent);
  // timer.attach(2, send_sensor);
}

void loop()
{
  delay(50);
  EEPROM.read(4);
  int a = analogRead(sensor1);
  Serial.println(a);
  delay(1000);
  if (a <= 50)
  {
    a = 11;
  }
  if (a > 4000)
    a = 9;
  if (a < 1000 && a > 800)
    a = 0;
  if (a < 1750 && a > 1550)
    a = 1;
  if (a < 2400 && a > 2100)
    a = 2;
  if (a < 3000 && a > 2500)
    a = 3;
  if (a < 3300 && a > 3100)
    a = 4;
  if (a < 3700 && a > 3600)
    a = 5;
  if (a < 4000 && a > 3800)
    a = 6;
  if (a < 290 && a > 150)
    a = 7;
  if (a < 150 && a > 50)
    a = 8;
  // }
  button1.loop();
  button2.loop();
  button3.loop();
  if (button1.isPressed())
  {
    state1++;
    if (state1 > 2)
    {
      state1 = 1;
    }
    modeselection();
    Serial.println(state1);
  }
  if (button3.isPressed())
  {
    state3++;
    if (state3 > 1)
    {
      state3 = 0;
      state1 = 0;
    }
    on();
    Serial.println(state3); // print the updated state value
  }

  if (button2.isPressed())
  {
    state2++;
    if (state2 > 1)
    {
      int state1 = 1;
      int state2 = 0;
      int state3 = 0;
    }
    off();
    Serial.println(state3);
  }

  if (EEPROM.read(4) == 1 && EEPROM.read(5) == 1)
  {
    Serial.println("manual mode start");
    digitalWrite(manled, HIGH);
    digitalWrite(autoled, LOW);
    digitalWrite(relay, HIGH);
    Serial.println("motor start");
    Serial.println("inside button3 press");

    if (a == 0) //////////////////////////// 0 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);

      delay(1000);
    }

    if (a == 1) ////////////////////1 level
    {
      Percentage = 20;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
    }

    if (a == 2) /////////////////////////2 level
    {
      Percentage = 30;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 3) ////////////////////////////3 level
    {
      Percentage = 40;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      digitalWrite(relay, LOW);
      Serial.println("motor stop");
      delay(1000);
    }

    if (a == 4) //////////////////////////////4 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 5) ////////////////////////////////5 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 6) /////////////////////////////6 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 7) ///////////////////////////7 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 8) //////////////////////////8 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 9) // 9
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }
  }
  if (EEPROM.read(4) == 1 && EEPROM.read(5) == 0)
  {
    Serial.println("inside off mode");
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    digitalWrite(relay, LOW);
    EEPROM.write(4, 2);
  }

  if (EEPROM.read(4) == 2)
  {
    Serial.println("auto mode start");
    digitalWrite(manled, LOW);
    digitalWrite(autoled, HIGH);
    if (a == 0) //////////////////////////// 0 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      digitalWrite(relay, HIGH);
      Serial.println("motor start");
      delay(1000);
    }

    if (a == 1) ////////////////////1 level
    {
      Percentage = 20;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 2) /////////////////////////2 level
    {
      Percentage = 30;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 3) ////////////////////////////3 level
    {
      Percentage = 40;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      digitalWrite(relay, LOW);
      Serial.println("motor stop");
      delay(1000);
    }

    if (a == 4) //////////////////////////////4 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 5) ////////////////////////////////5 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 6) /////////////////////////////6 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 7) ///////////////////////////7 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 8) //////////////////////////8 level
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }

    if (a == 9) // 9
    {
      Percentage = 10;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      delay(1000);
    }
  }

  websockets.loop();
}
