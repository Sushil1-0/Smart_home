
#include "wifi_config/wifiTool.h"
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
// #define LED2 12
#define sensor1 32 // upper tank
// #define sensor2 32 // lower tank
#define manled 12
#define buzzer 14
#define autoled 2
#define relay 4
const int pumpstop=25; // manual case ma start ra stop ko lagi
const int start=18;       // manual case ma start garna ko lagi
const int AMselector = 34; // auto or mannual selector
int state1 = 1;         // for mode selection
int mode = 1;

#define EEPROM_SIZE 16

WifiTool wifiTool;

/////////////////////////////////////////////////
int Percentage;
// void sensor();
// void send_sensor();
// Ticker timer;

char webpage[] PROGMEM = R"=====(
<!DOCTYPE html>
<html>
<script>
var connection = new WebSocket('ws://'+location.hostname+':81/');
var button_1_status = 0;
var button_2_status = 0;
var level_data = 0;
connection.onmessage = function(event){
  var full_data = event.data;
  console.log(full_data);
  var data = JSON.parse(full_data);
  level_data = data.level;
  document.getElementById("level_meter").value = level_data;
  document.getElementById("level_value").innerHTML = level_data;
}
function button_1_on()
{
   button_1_status = 1; 
  console.log("Motor is ON");
  send_data();
}
function button_1_off()
{
  button_1_status = 0;
  console.log("Motor is OFF");
  send_data();
}
function button_2_on()
{
   button_2_status = 1; 
  console.log("LED 2 is ON");
  send_data();
}
function button_2_off()
{
  button_2_status = 0;
console.log("LED 2 is OFF");
send_data();
}
function send_data()
{
  var full_data = '{"relay" :'+button_1_status+',"LED2":'+button_2_status+'}';
  connection.send(full_data);
}
</script>
<body>
<center>
<h1>My Home Automation</h1>
<h3> Motor </h3>
<button onclick= "button_1_on()" >On</button><button onclick="button_1_off()" >Off</button>
<h3> LED 2 </h3>
<button onclick="button_2_on()">On</button><button onclick="button_2_off()">Off</button>
</center>
<div style="text-align: center;">
<h3>WATER_LEVEL</h3><meter value="0" min="0" max="100" id="level_meter"> </meter><h3 id="level_value" style="display: inline-block;"> 0 </h3>
</body>
</html>
)=====";
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
    // deserialize the data
    DeserializationError error = deserializeJson(doc, message);
    // parse the parameters we expect to receive (TO-DO: error handling)
    // Test if parsing succeeds.
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
  case 1: ////////////////////////////////////////MANNUAL MODE
  {
    EEPROM.write(4, 1);
    state1 = 0;
    delay(100);
    break;
  }
  case 0: ///////////////////////////////////////AUTO MODE
  {
    EEPROM.write(4, 2);
    state1 = 1;
    delay(100);
    break;
  }
  default:
  {
  }
  }
}

void setup()
{
  int a = 0;
  Serial.begin(115200);
  pinMode(relay, OUTPUT);
  pinMode(autoled,OUTPUT);
  pinMode(manled,OUTPUT);
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(1, 1);
  pinMode(buzzer,OUTPUT);
  EEPROM.write (4,2);
  Serial.println("System started");
  pinMode(sensor1, INPUT);
  pinMode(AMselector, INPUT_PULLUP);
  pinMode(start, INPUT_PULLUP);
  pinMode(pumpstop, INPUT_PULLUP);

  // digitalWrite (start,     LOW);
  // digitalWrite(buzzer, HIGH);
  // digitalWrite(pumponled, HIGH);

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
  server.on("/", [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", webpage); });

  server.on("/led1/on", HTTP_GET, [](AsyncWebServerRequest *request)
            { 
    digitalWrite(relay,HIGH);
  request->send_P(200, "text/html", webpage); });

  server.onNotFound(notFound);

  server.begin(); // it will start webserver
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
  if (a < 2800 && a > 2500)
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

  if (a == 0) //////////////////////////
  {
    EEPROM.write(0, 1);
    EEPROM.write(1, 2);
  }

  if (a == 3)
  {
    EEPROM.write(0, 2);
  }
  if (a == 11)
  {
    if (a != 3 && EEPROM.read(0) == 1 && EEPROM.read(1) == 2)
    {
      digitalWrite(relay, HIGH);
    }
    else if (EEPROM.read(0) != 1 && EEPROM.read(1) != 2 && EEPROM.read(0) == 2)
    {
      digitalWrite(relay, LOW);
    }
  }
  //  Serial.println(a);

  int buttonState1 = digitalRead(AMselector);

  if (buttonState1 == 1)
  {
    digitalWrite(buzzer, HIGH);
    delay(500);
    digitalWrite(buzzer, LOW);
    modeselection();
  }

  if (EEPROM.read(4) == 1)
  {
    Serial.println("manual mode start");
    digitalWrite(manled,HIGH);
    digitalWrite(autoled,LOW);
    if (digitalRead(pumpstop) != HIGH)
    {
      if (digitalRead(start) == HIGH && a != 9)
      {
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

          // digitalWrite(buzzer, HIGH);
          // delay(600);
          // digitalWrite(buzzer, LOW);
        }

        if (a == 1) ////////////////////1 level
        {
          Percentage = 20;
          String JSON_Data = "{\"level\":";
          JSON_Data += Percentage;
          JSON_Data += "}";
          Serial.println(JSON_Data);
          websockets.broadcastTXT(JSON_Data);
          // digitalWrite(relay, HIGH);
          // Serial.println("motor start");
          delay(1000);

          // Serial.println(a);
        }

        if (a == 2) /////////////////////////2 level
        {
          Percentage = 30;
          String JSON_Data = "{\"level\":";
          JSON_Data += Percentage;
          JSON_Data += "}";
          Serial.println(JSON_Data);
          websockets.broadcastTXT(JSON_Data);
          // digitalWrite(relay, HIGH);
          // Serial.println("motor start");
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
          // digitalWrite(relay, HIGH);
          // Serial.println("motor start");
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
          // digitalWrite(relay, HIGH);
          // Serial.println("motor start");
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
          // digitalWrite(relay, HIGH);
          // Serial.println("motor start");
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
          // digitalWrite(relay, HIGH);
          // Serial.println("motor start");
          delay(1000);

          // EEPROM.write(6, 2);
        }

        if (a == 8) //////////////////////////8 level
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

          // Serial.println(a);
        }

        if (a == 9) // 9
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
          //  Serial.println(a);
        }
      }
    }
    else
    {
      digitalWrite(buzzer, HIGH);
      delay(500);
      digitalWrite(buzzer, LOW);
      digitalWrite(relay, LOW);
    }
  }
  if (EEPROM.read(4) == 2)
  {
    Serial.println("auto mode start");
    digitalWrite(manled,LOW);
    digitalWrite(autoled,HIGH);
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

      // digitalWrite(buzzer, HIGH);
      // delay(600);
      // digitalWrite(buzzer, LOW);
    }

    if (a == 1) ////////////////////1 level
    {
      Percentage = 20;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      // digitalWrite(relay, HIGH);
      // Serial.println("motor start");
      delay(1000);

      // Serial.println(a);
    }

    if (a == 2) /////////////////////////2 level
    {
      Percentage = 30;
      String JSON_Data = "{\"level\":";
      JSON_Data += Percentage;
      JSON_Data += "}";
      Serial.println(JSON_Data);
      websockets.broadcastTXT(JSON_Data);
      // digitalWrite(relay, HIGH);
      // Serial.println("motor start");
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
      // digitalWrite(relay, HIGH);
      // Serial.println("motor start");
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
      // digitalWrite(relay, HIGH);
      // Serial.println("motor start");
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
      // digitalWrite(relay, HIGH);
      // Serial.println("motor start");
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
      // digitalWrite(relay, HIGH);
      // Serial.println("motor start");
      delay(1000);

      // EEPROM.write(6, 2);
    }

    if (a == 8) //////////////////////////8 level
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

      // Serial.println(a);
    }

    if (a == 9) // 9
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
      //  Serial.println(a);
    }
  }
 
  websockets.loop();
}
