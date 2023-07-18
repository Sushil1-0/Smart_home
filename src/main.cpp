#include <EEPROM.h>
#include "wifi_config/wifiTool.h"
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
// #include "Tinker.h"
#define relay 4
#define Switch 11
#define LED2 12
#define sensor1 32 // upper tank
#define sensor2 33 // lower tank

#define EEPROM_SIZE 16

WifiTool wifiTool;

/////////////////////////////////////////////////
int Percentage;
void send_sensor();
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

    int LED1_status = doc["LED1"];
    int LED2_status = doc["LED2"];
    digitalWrite(relay, LED1_status);
    digitalWrite(LED2, LED2_status);
  }
}

void setup()
{
  // pinMode(LED_BUILTIN, OUTPUT);
  // digitalWrite(LED_BUILTIN, HIGH);
  Serial.begin(115200);
  pinMode(relay, OUTPUT);
  pinMode(LED2, OUTPUT);
  EEPROM.begin(EEPROM_SIZE);
  Serial.println("System started");

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
  send_sensor();
  websockets.loop();
}

void send_sensor()
{
  float a = analogRead(sensor1);
  Serial.println(a);
  if (a > 2900 && a <= 3100)
  {
    a = 1;
  }
  if (a > 3200 && a <= 3500)
  {
    a = 2;
  }
  if (a > 3600 && a <= 3900)
  {
    a = 3;
  }
  if (a > 4000 && a <= 4100)
  {
    a = 4;
  }
  if (a == 0)
  {
    EEPROM.write(2, 1);
  }

  //  if (a > 3400 && a <= 3500){
  //   a=1;
  // }
  // //  if (a<3100){
  //   a=1;
  // }
  //  if (a<3100){
  //   a=1;
  // }
  //  if (a<3100){
  //   a=1;
  // }
  //  if (a<3100){
  //   a=1;
  // }

  if (a == 1)
  {
    EEPROM.write(0, 1);
    EEPROM.write(1, 2);
    EEPROM.commit();
    Serial.println("level 1 memo");
  }
  if (a >= 4)
  {
    EEPROM.write(0, 2);
    EEPROM.write(1, 3);
    EEPROM.commit();
  }

  if (a != 4 && EEPROM.read(0) == 1 && EEPROM.read(1) == 2 && EEPROM.read(2)==1)
  {
    if (a == 1)
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

    if (a == 2)
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
    }
    if (a == 3)
    {
      Percentage = 30;
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
    digitalWrite(relay, LOW);
  }

  // if (a == 4)
  // {
  //   EEPROM.write(0, 2);
  //   Percentage = 90;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   digitalWrite(relay, LOW);
  //   Serial.println("motor stop");
  //   delay(1000);
  // }
  // else
  // {
  // }

  // if (a > 3000 && a <= 3100) //// 1 level COMON ANODE
  // {
  //   Percentage = 10;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   digitalWrite(relay, HIGH);
  //   Serial.println("motor start");
  //   delay(1000);
  // }

  // else if (a > 3400 && a <= 3500) ///////2 level COMON ANODE
  // {
  //   Percentage = 20;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   delay(1000);
  // }

  // else if (a > 3800 && a <= 3900) /////////3 level
  // {
  //   Percentage = 30;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   delay(1000);
  // }

  // else if (a > 4000 && a <= 4100) //////////////////////////4 level
  // {
  //   Percentage = 40;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   delay(1000);
  // }

  // else if (a > 730 && a <= 785) ////////////////////////////5 level
  // {
  //   Percentage = 50;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   delay(1000);
  // }

  // else if (a > 630 && a <= 690) ////////////////////////////////6 level
  // {
  //   Percentage = 60;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   delay(1000);
  // }

  // else if (a > 530 && a <= 590) /////////////////////////////7 level
  // {
  //   Percentage = 70;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   delay(1000);
  // }

  // else if (a > 400 && a <= 460) ///////////////////////////8 level COMMON CATHODE
  // {
  //   Percentage = 80;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   delay(1000);
  // }

  // else if (a < 300) // 9 level COMON CATHODE
  // {
  //   Percentage = 90;
  //   String JSON_Data = "{\"level\":";
  //   JSON_Data += Percentage;
  //   JSON_Data += "}";
  //   Serial.println(JSON_Data);
  //   websockets.broadcastTXT(JSON_Data);
  //   digitalWrite(relay, LOW);
  //   Serial.println("motor stop");
  //   delay(1000);
  // }
}