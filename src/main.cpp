
#include "wifi_config/wifiTool.h"
#include <WiFiClientSecure.h>
#include <ESPmDNS.h>
#include <WebSocketsServer.h>
#include <ArduinoJson.h>
// #include "Tinker.h"
// #define ohled 4
// #define Switch 15
#define LED2 12
#define sensor1 33 // upper tank
#define sensor2 32 // lower tank

#include <EEPROM.h>
// #define manled 8
// #define autoled 7
#define relay 4
// #define underled 11
// #define pumpstop 34 // manual case ma start ra stop ko lagi
// #define buzzer 14
// #define start 12     // start button
// int AMselector = 13; // auto or mannual selector
// int OUselector = 19; // OH/underground selection
// int state1 = 1;      // for mode selection
// int state2 = 1;      // for tank selection
// int tank = 1;
// int mode = 1;
// int pumponled = 9;

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

    int LED1_status = doc["LED1"];
    int LED2_status = doc["LED2"];
    digitalWrite(relay, LED1_status);
    digitalWrite(LED2, LED2_status);
  }
}

/**********************************************************************************************************************************************/
//////////////////////////*mode selection*//////////////////////////////////
// void modeselection()
// {
//   switch (state1)
//   {
//   case 1: ////////////////////////////////////////MANNUAL MODE
//   {
//     EEPROM.write(4, 1);
//     state1 = 0;
//     mode = 1;
//     delay(100);
//     break;
//   }
//   case 0: ///////////////////////////////////////AUTO MODE
//   {
//     EEPROM.write(4, 2);
//     state1 = 1;
//     mode = 2;
//     delay(100);
//     break;
//   }
//   default:
//   {
//   }
//   }
// }

// //////////////////////////* tank selection*/////////////////////////////////
// void tankselection()
// {
//   switch (state2)
//   {
//   case 1: ///////////////////////////////////////// UPPERTANK
//   {
//     digitalWrite(underled, LOW);
//     digitalWrite(ohled, HIGH);
//     EEPROM.write(5, 1);
//     state2 = 0;
//     tank = 1;
//     delay(100);
//     break;
//   }
//   case 0: ///////////////////////////////////////////// LOWERTANK
//   {
//     EEPROM.write(5, 2);
//     digitalWrite(underled, HIGH);
//     digitalWrite(ohled, LOW);
//     state2 = 1;
//     tank = 2;
//     delay(100);
//     break;
//   }
//   default:
//   {
//   }
//   }
// }

void setup()
{
  int a = 0;
  Serial.begin(115200);
  pinMode(relay, OUTPUT);
  pinMode(LED2, OUTPUT);
  EEPROM.begin(EEPROM_SIZE);
  EEPROM.write(1, 1);
  // pinMode(ohled, OUTPUT);
  // pinMode(underled, OUTPUT);
  // EEPROM.write (5,1);
  // EEPROM.write (4,2);
  Serial.println("System started");
  pinMode(sensor1, INPUT);
  pinMode(sensor2, INPUT);
  // digitalWrite(ohled, HIGH);
  // digitalWrite(relay, HIGH);
  // pinMode(pumponled, OUTPUT);
  // pinMode(AMselector, INPUT_PULLUP);
  // pinMode(OUselector, INPUT_PULLUP);

  // digitalWrite (start,     LOW);
  // digitalWrite(buzzer, HIGH);
  // digitalWrite(pumponled, HIGH);
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
  // delay(50);
  // EEPROM.read(4);
  // delay(50);
  // EEPROM.read(5);
  // int b = analogRead(A1);
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
  if (a < 1750 && a > 1650)
    a = 1;
  if (a < 2400 && a > 2200)
    a = 2;
  if (a < 2800 && a > 2600)
    a = 3;
  if (a < 3300 && a > 3100)
    a = 4;
  if (a < 3700 && a > 3600)
    a = 5;
  // if (a < 380 && a > 290)
  //   a = 6;
  // if (a < 290 && a > 150)
  //   a = 7;
  // if (a < 150 && a > 50)
  //   a = 8;

  //  Serial.println(a);

  // int buttonState1 = digitalRead(AMselector);
  // int buttonState2 = digitalRead(OUselector);

  // if (buttonState1 == 1)
  // {
  //   digitalWrite(buzzer, HIGH);
  //   delay(500);
  //   digitalWrite(buzzer, LOW);
  //   modeselection();
  // }

  // if (buttonState2 == 1)
  // {
  //   digitalWrite(buzzer, HIGH);
  //   delay(500);
  //   digitalWrite(buzzer, LOW);
  //   tankselection();
  // }

  // if (EEPROM.read(4) == 1) // mannual mode
  // {
  //   digitalWrite(manled, HIGH);
  //   digitalWrite(autoled, LOW);
  //   // int a = analogRead(A0);
  //   if (EEPROM.read(5) == 1)
  //   {
  //     // int a = analogRead(sensor1);
  //     if (digitalRead(pumpstop) != HIGH)
  //     {

  //       if (digitalRead(start) == HIGH && a != 9)
  //       {
  //         digitalWrite(buzzer, HIGH);
  //         delay(500);
  //         digitalWrite(buzzer, LOW);
  //         digitalWrite(relay, HIGH);
  //         digitalWrite(pumponled, HIGH);
  //       }

  //       if (a == 9)
  //       {
  //         digitalWrite(relay, LOW);
  //         digitalWrite(pumponled, LOW);
  //       }
  //     }
  //     else
  //     {
  //       digitalWrite(buzzer, HIGH);
  //       delay(500);
  //       digitalWrite(buzzer, LOW);
  //       digitalWrite(relay, LOW);
  //       digitalWrite(pumponled, LOW);
  //     }
  //     delay(50);
  //   }
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
  // }

  // if (EEPROM.read(4) == 2) // automode
  // {
  //   digitalWrite(manled, LOW);
  //   digitalWrite(autoled, HIGH);

  //   if (EEPROM.read(5) == 1)
  //   {
  //     // int a = analogRead(sensor1);

  //     if (digitalRead(pumpstop) != HIGH)
  //     {

  if (a == 0) //////////////////////////
  {
    EEPROM.write(0, 1);
    EEPROM.write(1, 2);
  }

  // if (a != 3 && EEPROM.read(0) == 1 && EEPROM.read(1) == 2)
  // {
  //   digitalWrite(relay, HIGH);
  //   // digitalWrite(pumponled, HIGH);
  // }

  // else
  // {

  //   // tone(buzzer,900,500);
  //   digitalWrite(relay, LOW);
  //   // digitalWrite(pumponled, LOW);
  // }

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
    else if(EEPROM.read(0) != 1 && EEPROM.read(1) != 2 && EEPROM.read(0)==2)
    {
       digitalWrite(relay, LOW);
    }
  }

  // }
  // else
  // {
  //   digitalWrite(buzzer, HIGH);
  //   delay(500);
  //   digitalWrite(buzzer, LOW);
  //   digitalWrite(relay, LOW);
  //   digitalWrite(pumponled, LOW);
  //   EEPROM.write(0, 2);
  // }
  // }

  /*if(EEPROM.read(5)==2)
 {
  int b = analogRead(sensor2);
  if (digitalRead(pumpstop)!=HIGH)
  {

  if(b>=853)/////////////////////////
  {
  EEPROM.write(0,1);
  EEPROM.write(1,2);
  }

  if (b>166&&EEPROM.read(0)==1&&EEPROM.read(1)==2)
  {
  digitalWrite(relay,HIGH);
  digitalWrite(pumponled,HIGH);
 // Serial.println(b);
// delay(100);
  }

  else
  {
  //tone(buzzer,900,500);
  digitalWrite(relay,LOW);
  digitalWrite(pumponled,LOW);
  }

  if (b<=166)
  {
  EEPROM.write(0,2);
  }
  }
  else
  {
  digitalWrite(relay,LOW);
  digitalWrite(pumponled,LOW);
  EEPROM.write(0,2);
  } }
//  */

  // if (EEPROM.read(5) == 1) // upper
  // {
  //   digitalWrite(underled, LOW);
  //   digitalWrite(ohled, HIGH);
  //   // Serial.println(EEPROM.read(5));
  //   // if(a>=0&&a<11)
  //   // Serial.println(a);
  //   {
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

  //       if (EEPROM.read(6) == 2)
  //       {
  //         EEPROM.write(6, 3);
  //         for (int i = 0; i < 3; i++)
  //         {
  //           digitalWrite(buzzer, HIGH);
  //           delay(600);
  //           digitalWrite(buzzer, LOW);
  //           delay(600);
  //         }
  //       }
  //     }
  //   }
  // }

  // if (EEPROM.read(5) == 2) ///////////////////////////////lower
  // {
  //   digitalWrite(underled, HIGH);
  //   digitalWrite(ohled, LOW);
  //   int b = analogRead(sensor2);
  //   // delay(50);        // delay in between reads for stability

  //   /*if (b>=945)/////////////////////////////////0 level
  //   {
  //   digitalWrite(segA, LOW) ;
  //   digitalWrite(segB, LOW);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, LOW);
  //   digitalWrite(segE, LOW);
  //   digitalWrite(segF, LOW);
  //   digitalWrite(segG, HIGH);
  //   digitalWrite(buzzer,HIGH);
  //   delay(600);
  //   digitalWrite(buzzer,LOW);
  //   }

  //   if(945>b&&b>=853)////////////////////////////// 1 level
  //   {
  //   digitalWrite(segA, HIGH);
  //   digitalWrite(segB, LOW);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, HIGH);
  //   digitalWrite(segE, HIGH);
  //   digitalWrite(segF, HIGH);
  //   digitalWrite(segG, HIGH);
  //   }

  //   if(853>b&&b>=764)/////////////////////////////2 level
  //   {
  //   digitalWrite(segA, LOW);
  //   digitalWrite(segB, LOW);
  //   digitalWrite(segC, HIGH);
  //   digitalWrite(segD, LOW);
  //   digitalWrite(segE, LOW);
  //   digitalWrite(segF, HIGH);
  //   digitalWrite(segG, LOW);
  //   }

  //   if(764>b&&b>=664)///////////////////////////////////3 level
  //   {
  //   digitalWrite(segA, LOW);
  //   digitalWrite(segB, LOW);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, LOW);
  //   digitalWrite(segE, HIGH);
  //   digitalWrite(segF, HIGH);
  //   digitalWrite(segG, LOW);
  //   }

  //   if(664>b&&b>=559)//////////////////////////////////4 level
  //   {
  //   digitalWrite(segA, HIGH);
  //   digitalWrite(segB, LOW);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, HIGH);
  //   digitalWrite(segE, HIGH);
  //   digitalWrite(segF, LOW);
  //   digitalWrite(segG, LOW);
  //   }

  //   if(559>b&&b>=456)///////////////////////////////////5 level
  //   {
  //   digitalWrite(segA, LOW);
  //   digitalWrite(segB, HIGH);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, LOW);
  //   digitalWrite(segE, HIGH);
  //   digitalWrite(segF, LOW);
  //   digitalWrite(segG, LOW);
  //   }

  //   if(456>b&&b>=350)// 6 level
  //   {
  //   digitalWrite(segA, LOW);
  //   digitalWrite(segB, HIGH);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, LOW);
  //   digitalWrite(segE, LOW);
  //   digitalWrite(segF, LOW);
  //   digitalWrite(segG, LOW);
  //   }

  //   if(360>b&&b>=264)///////////////////////////////////////7 level
  //   {
  //   digitalWrite(segA, LOW);
  //   digitalWrite(segB, LOW);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, HIGH);
  //   digitalWrite(segE, HIGH);
  //   digitalWrite(segF, HIGH);
  //   digitalWrite(segG, HIGH);

  //   EEPROM.write(6,2); }

  //   if(264>b&&b>=166)// 8 level
  //   {
  //   digitalWrite(segA, LOW);
  //   digitalWrite(segB, LOW);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, LOW);
  //   digitalWrite(segE, LOW);
  //   digitalWrite(segF, LOW);
  //   digitalWrite(segG, LOW);
  //   }

  //   if(b<166)
  //   {
  //   digitalWrite(segA, LOW);
  //   digitalWrite(segB, LOW);
  //   digitalWrite(segC, LOW);
  //   digitalWrite(segD, LOW);
  //   digitalWrite(segE, HIGH);
  //   digitalWrite(segF, LOW);
  //   digitalWrite(segG, LOW);
  //   if (EEPROM.read(6)==2)
  //   {
  //     EEPROM.write(6,3);
  //   for(int i=0;i<3;i++)
  //   {
  //   digitalWrite(buzzer,HIGH);
  //   delay(600);
  //   digitalWrite(buzzer,LOW);
  //  delay(600);
  //   }}
  //   }*/
  // }

  // send_sensor();
  websockets.loop();
}
// void send_sensor()
// {
//   int a = analogRead(sensor1);
//   Serial.println(a);
//   delay(1000);
// }
