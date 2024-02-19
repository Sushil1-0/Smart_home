
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
#define ohled 19
#define underled 21
#define pumpstop 25  // manual case ma start ra stop ko lagi
#define start 18     // start button
int AMselector = 34; // auto or mannual selector
// int OUselector = 13; // OH/underground selection
int state1 = 1; // for mode selection
int state2 = 1; // for tank selection
int tank = 1;
int mode = 1;
int pumponled = 9;
// int state1 = 1;
// int state2 = 0;
// int state3 = 0;
#include <ezButton.h> // Include ezButton library

ezButton button1(AMselector); // Initialize ezButton library on AMselector pin
ezButton button2(start);
ezButton button3(pumpstop);
// ezButton button4(OUselector);

#define EEPROM_SIZE 16

WifiTool wifiTool;

// /////////////////////////////////////////////////
int Percentage;
// //////////////////////////////////////////////////////
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
    uint8_t mac[6];
    esp_read_mac(mac, ESP_MAC_WIFI_STA);

    // Print the MAC address as a device ID
    char deviceId[7];
    sprintf(deviceId, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    String chipID = String(ESP.getEfuseMac(), HEX);
    String extendedDeviceID = chipID + deviceId;
    Serial.println("Sending device ID: " + extendedDeviceID);
    websockets.sendTXT(num, "{\"deviceID\": \"" + extendedDeviceID + "\"}");
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

//////////////////////////*mode selection*//////////////////////////////////
void modeselection()
{
  switch (state1)
  {
  case 1: ////////////////////////////////////////MANNUAL MODE
  {
    Serial.println("manual mode start");
    EEPROM.write(4, 1);
    Serial.println(EEPROM.read(4));
    state1 = 0;
    mode = 1;
    delay(100);
    EEPROM.commit();
    break;
  }
  case 0: ///////////////////////////////////////AUTO MODE
  {
    Serial.println("auto mode start");
    EEPROM.write(4, 2);
    state1 = 1;
    mode = 2;
    delay(100);
    EEPROM.commit();
    break;
  }
  default:
  {
  }
  }
}

//////////////////////////* tank selection*/////////////////////////////////
void tankselection()
{
  switch (state2)
  {
  case 1: ///////////////////////////////////////// UPPERTANK
  {
    digitalWrite(underled, LOW);
    digitalWrite(ohled, HIGH);
    EEPROM.write(5, 1);
    state2 = 0;
    tank = 1;
    delay(100);
    EEPROM.commit();
    break;
  }
  case 0: ///////////////////////////////////////////// LOWERTANK
  {
    EEPROM.write(5, 2);
    digitalWrite(underled, HIGH);
    digitalWrite(ohled, LOW);
    state2 = 1;
    tank = 2;
    delay(100);
    EEPROM.commit();
    break;
  }
  default:
  {
  }
  }
}

// String getDeviceId() {
//   uint8_t mac[6];
//   WiFi.macAddress(mac);

//   String deviceId = "";
//   for (int i = 0; i < 6; ++i) {
//     if (mac[i] < 0x10) {
//       deviceId += "0";
//     }
//     deviceId += String(mac[i], HEX);
//     if (i < 5) {
//       deviceId += ":";
//     }
//   }

//   return deviceId;
// }

void setup()
{

  int a = 0;
  Serial.begin(115200);
  //   pinMode(relay, OUTPUT);
  //   pinMode(autoled, OUTPUT);
  //   pinMode(manled, OUTPUT);
  EEPROM.begin(EEPROM_SIZE);
  //   EEPROM.write(1, 1);
  //   pinMode(buzzer, OUTPUT);
  //   EEPROM.write(4, 2);
  //   Serial.println("System started");
  //   pinMode(sensor1, INPUT);
  //   pinMode(AMselector, INPUT);
  //   pinMode(start, INPUT);
  //   pinMode(pumpstop, INPUT);
  //   SPIFFS.begin();

  // Serial.begin (9600);
  EEPROM.write(1, 1);
  // EEPROM.write (5,1);
  // EEPROM.write (4,2);
  pinMode(sensor1, INPUT);
  pinMode(relay, OUTPUT);
  digitalWrite(relay, LOW);
  pinMode(manled, OUTPUT);
  pinMode(buzzer, OUTPUT);
  pinMode(autoled, OUTPUT);
  pinMode(ohled, OUTPUT);
  pinMode(underled, OUTPUT);
  pinMode(pumponled, OUTPUT);
  digitalWrite(pumponled, LOW);
  pinMode(pumpstop, INPUT);
  pinMode(AMselector, INPUT);
  pinMode(start, INPUT);
  // pinMode(OUselector, INPUT);
  SPIFFS.begin();

  /********************************* Get the device ID********************************************/
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);

  // Print the MAC address as a device ID
  char deviceId[13];
  sprintf(deviceId, "%02X%02X%02X%02X%02X%02X", mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  String chipID = String(ESP.getEfuseMac(), HEX);
  String extendedDeviceID = chipID + deviceId;

  Serial.print("Extended Device ID: ");
  Serial.println(extendedDeviceID);

  /**************************************************************************************************/
  wifiTool.begin(false);
  if (!wifiTool.wifiAutoConnect())
  {
    Serial.println("fail to connect to wifi!!!!");
    wifiTool.runApPortal();
  }
  if (MDNS.begin("esp"))
  { // esp.local/
    Serial.println("MDNS responder started");
  }

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send(SPIFFS, "/main.html", "text/html"); });
  server.onNotFound(notFound);

  server.begin();
  websockets.begin();
  websockets.onEvent(webSocketEvent);
}

void loop()
{
  delay(50);
  EEPROM.read(4);
  delay(50);
  EEPROM.read(5);
  // int b = analogRead(A1);
  int a = analogRead(sensor1);
  Serial.println(a);
  delay(500);

  if (a <= 50)
  {
    a = 11;
  }
  if (a > 4000)
    a = 10;
  if (a < 3900 && a > 3800)
    a = 1; //330ohm
  if (a < 3400 && a > 3300)
    a = 2;//270ohm
  if (a < 3100 && a > 3000)
    a = 3;//220ohm
  if (a < 2950 && a > 2800)
    a = 4;//200ohm
  if (a < 2550 && a > 2400)
    a = 5;//150ohm
  if (a < 2000 && a > 1900)
    a = 6;//100ohm
  if (a < 1200 && a > 1100)
    a = 7;//47ohm
  if (a < 700 && a > 600)
    a = 8;//22ohm
  if (a < 450 && a > 350)
    a = 9;//10ohm

  int buttonState1 = digitalRead(AMselector);
  // int buttonState2 = digitalRead(OUselector);

  if (buttonState1 == 1)
  {
    Serial.println("inside buttonstate = 1");
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    modeselection();
  }

  // if (buttonState2 == 1)
  // {
  //   digitalWrite(buzzer, HIGH);
  //   delay(500);
  //   digitalWrite(buzzer, LOW);
  //   tankselection();
  // }

  if (EEPROM.read(4) == 1) // mannual mode
  {
    digitalWrite(manled, HIGH);
    digitalWrite(autoled, LOW);
    // int a = analogRead(A0);

    // int a = analogRead(sensor1);
    if (digitalRead(pumpstop) != HIGH)
    {

      if (a == 1) //////////////////////////// 0 level
      {
        Percentage = 10;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;

        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        // digitalWrite(relay, HIGH);
        // Serial.println("motor start");
        delay(1000);
      }

      if (a == 2) ////////////////////1 level
      {
        Percentage = 20;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 3) /////////////////////////2 level
      {
        Percentage = 30;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 4) ////////////////////////////3 level
      {
        Percentage = 40;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        // digitalWrite(relay, LOW);
        // Serial.println("motor stop");
        delay(1000);
      }

      if (a == 5) //////////////////////////////4 level
      {
        Percentage = 50;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 6) ////////////////////////////////5 level
      {
        Percentage = 60;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 7) /////////////////////////////6 level
      {
        Percentage = 70;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 8) ///////////////////////////7 level
      {
        Percentage = 80;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 9) //////////////////////////8 level
      {
        Percentage = 90;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }
      if (digitalRead(start) == HIGH && a != 9)
      {
        digitalWrite(buzzer, HIGH);
        delay(500);
        digitalWrite(buzzer, LOW);
        digitalWrite(relay, HIGH);
        digitalWrite(pumponled, HIGH);
      }

      // if (a == 9)
      // {
      //   Percentage = 100;
      //   String JSON_Data = "{\"level\":";
      //   JSON_Data += Percentage;
      //   JSON_Data += "}";
      //   Serial.println(JSON_Data);
      //   websockets.broadcastTXT(JSON_Data);
      //   delay(1000);
      //   digitalWrite(relay, LOW);
      //   digitalWrite(pumponled, LOW);
      // }
    }
    else
    {
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      digitalWrite(relay, LOW);
      digitalWrite(pumponled, LOW);
    }
    delay(50);

    /*if(EEPROM.read(5)==2)
  {
   int  b= analogRead(sensor2);
    if (digitalRead(pumpstop)!=HIGH)
    {

    if(digitalRead(start)==HIGH && b>166 )
    {
   digitalWrite(buzzer,HIGH);
    delay(500);
    digitalWrite(buzzer,LOW);
    digitalWrite(relay,HIGH);
    digitalWrite(pumponled,HIGH);
    }

    if(b<=166 )
    {
   digitalWrite(pumponled,LOW);
      digitalWrite(relay,LOW);
    }
    }
    else
    {
      digitalWrite(buzzer,HIGH);
    delay(500);
    digitalWrite(buzzer,LOW);

    digitalWrite(relay,LOW);
    digitalWrite(pumponled,LOW);
    }
    delay(50);
    }*/
  }

  if (EEPROM.read(4) == 2)
  {
    // Serial.println("auto mode start");
    digitalWrite(manled, LOW);
    digitalWrite(autoled, HIGH);
    if (digitalRead(pumpstop) != HIGH)
    {
      if (a == 1) //////////////////////////// 0 level
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

      if (a == 2) ////////////////////1 level
      {
        Percentage = 20;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 3) /////////////////////////2 level
      {
        Percentage = 30;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 4) ////////////////////////////3 level
      {
        Percentage = 40;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 5) //////////////////////////////4 level
      {
        Percentage = 50;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 6) ////////////////////////////////5 level
      {
        Percentage = 60;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 7) /////////////////////////////6 level
      {
        Percentage = 70;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 8) ///////////////////////////7 level
      {
        Percentage = 80;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        delay(1000);
      }

      if (a == 9) //////////////////////////8 level
      {
        Percentage = 90;
        String JSON_Data = "{\"level\":";
        JSON_Data += Percentage;
        JSON_Data += "}";
        Serial.println(JSON_Data);
        websockets.broadcastTXT(JSON_Data);
        digitalWrite(relay, LOW);
        Serial.println("motor stop");
        delay(1000);
      }
    }
    else
    {
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      digitalWrite(relay, LOW);
      digitalWrite(pumponled, LOW);
    }
  }
  websockets.loop();
}
