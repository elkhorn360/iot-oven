#include "Arduino.h"
#include <MAX31855.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include "RunningMedian.h"
#include "brzo_i2c.h"
#define USING_BRZO 1
//#include "BME280I2C_BRZO.h"
#include "SSD1306Brzo.h"
#include <Encoder.h>

#define MISO 12 //data/"signal out" D6
#define SCK 13 // clock D7
#define heatCS 14 //chip select D5


//MAX31855_Class  heatProbe = MAX31855_Class(MISO, heatCS, SCK);
MAX31855_Class heatProbe;
int heatTemperature;

boolean ovenOn;
boolean heating;
int targetTemp;
String heaterMode;
int duration;
unsigned long startTime;
float currentTemp;

const char* ap_ssid     = "Era-Oven";

/* Put IP Address details */
IPAddress local_ip(192,168,1,1);
IPAddress gateway(192,168,1,1);
IPAddress subnet(255,255,255,0);

ESP8266WebServer server(80);

const uint32_t I2C_ACK_TIMEOUT = 2000;
SSD1306Brzo display(0x3c, D3, D4);

//Encoder myEnc(D2,D8); // CLK , DT
//BME280I2C_BRZO bme;

RunningMedian samplesHeatTemperature = RunningMedian(20); //this takes XX samples

long oldPosition  = -999;

void readMAX31855HeatProbe()
{
  int32_t ambientTemperature = heatProbe.readAmbient();                        // retrieve die ambient temperature //
  int32_t probeTemperature   = heatProbe.readProbe();                          // retrieve thermocouple probe temp //
  uint8_t faultCode          = heatProbe.fault();                              // retrieve any error codes         //

  float temp(NAN), hum(NAN), pres(NAN);

//   BME280::TempUnit tempUnit(BME280::TempUnit_Celsius);
//   BME280::PresUnit presUnit(BME280::PresUnit_Pa);
//  
//   bme.read(pres, temp, hum, tempUnit, presUnit);

   
  if(faultCode) {                                                             // Display error code when present  //
    Serial.print("Fault code ");                                              //                                  //
    Serial.print(faultCode);                                                  //                                  //
    Serial.println(" returned.");                                             //                                  //
  } else {                                                                    //                                  //
//    Serial.print("Ambient Temperature is ");                                  //                                  //
//    Serial.print((float)ambientTemperature/1000,3);                           //                                  //
//    Serial.println("\xC2\xB0""C");                                            //                                  //
//    Serial.print("Probe Temperature is   ");                                  //                                  //
   Serial.println((float)probeTemperature/1000,3); 
   if (ovenOn)  {
     display.clear();
     display.drawString(5, 0, "Temperature: ");
     currentTemp = (float)probeTemperature/1000;
     display.drawString(5, 10, "Now: " +String(currentTemp,2) +"°c Target: "+ targetTemp +"°c");
     display.drawString(5, 20, "Heating Mode: ");
     if (heating) {
      display.drawString(5, 30, heaterMode + " HEATING..");
     } else {
      display.drawString(5, 30, heaterMode); 
     }     
     display.drawString(5, 40, "Time Remaining: ");
     float remaingTime = startTime + (duration * 60 * 1000) - millis();
     display.drawString(5, 50, String(((float)remaingTime / (1000 * 60)),2));
     
  //   display.drawString(5, 30, String(hum) +"% RH");   
  //   display.drawString(5, 20, "Humidity: ");
  //   display.drawString(5, 30, String(hum) +"% RH");
  //   display.drawString(5, 40, "Perssure: ");
  //   display.drawString(5, 50, String(pres) +"pa");
     display.display();
   } else {
     display.clear();
     display.drawString(5, 0, "Oven is OFF"); 
     display.display(); 
   }
    
//    Serial.println("\xC2\xB0""C\n");                                          //                                  //
  } // of if-then-else an error occurred                                      //                                  //
}

void startAP() {
  display.clear();
  display.drawString(5, 0, "Starting Era-Oven");
  display.display();
  delay(100);
  WiFi.softAP(ap_ssid);
  WiFi.softAPConfig(local_ip, gateway, subnet);
  delay(100);
  display.clear();

  server.on("/", handle_OnConnect);
  server.on("/temp", handle_temp);
  server.on("/heat", handle_heat);
  server.on("/time", handle_duration);
  server.on("/status", handle_start);
//  server.onNotFound(handle_NotFound);
  server.begin();

  IPAddress IP = WiFi.softAPIP();
  display.drawString(5, 0, "AP IP address: ");
  display.drawString(5, 10, IP.toString());
  display.display();  
  delay(1000);
}

void handle_OnConnect() {
//  LED1status = LOW;
//  LED2status = LOW;
//  Serial.println("GPIO7 Status: OFF | GPIO6 Status: OFF");
  server.send(200, "text/html", SendHTML()); 
}

void handle_temp() {
  targetTemp = server.arg("val").toInt();
  server.send(200, "text/html", SendHTML()); 
}

void handle_heat() {
  heaterMode = server.arg("val");
  server.send(200, "text/html", SendHTML()); 
}

void handle_duration() {
  duration = server.arg("val").toInt();
  server.send(200, "text/html", SendHTML()); 
}

void handle_start() {
  if( server.arg("val") == "start") {    
    ovenOn = true;
    startTime = millis();
  }else {
    ovenOn = false;
  }
  server.send(200, "text/html", SendHTML()); 
}


void setup() {
  pinMode(D0, OUTPUT);
  pinMode(D1, OUTPUT);
  // put your setup code here, to run once:  
  Serial.begin(115200);
  
  brzo_i2c_setup(D3, D4,I2C_ACK_TIMEOUT);
  
//  while(!bme.begin())
//   {
//      Serial.println("Could not find BME280 sensor!");
//      delay(1000);
//   }

//   switch(bme.chipModel())
//   {
//      case BME280::ChipModel_BME280:
//        Serial.println("Found BME280 sensor! Success.");
//        break;
//      case BME280::ChipModel_BMP280:
//        Serial.println("Found BMP280 sensor! No Humidity available.");
//        break;
//      default:
//        Serial.println("Found UNKNOWN sensor! Error!");
//   }

  display.init();
  display.setContrast(255);
//  display.drawString(5, 0, "Era Oven V1.0");
//  display.display();


  
//  Serial.println(F("Cooking heat temp MONITOR"));
  heatProbe.begin(heatCS,MISO,SCK);
  startAP();

}

void loop() {
  // put your main code here, to run repeatedly:
  server.handleClient();
  readMAX31855HeatProbe();
  controlOven();
//  long newPosition = myEnc.read();
//  if (newPosition != oldPosition) {
//    oldPosition = newPosition;
//    Serial.println(newPosition);
//    display.clear();
//    display.drawString(5, 0, String(newPosition));
//    display.display();
//  }
}

void controlOven() {
  if (ovenOn) {
    int endTime = startTime + (duration * 60 * 1000);
    if ( millis() < endTime ) {
      if ( float(targetTemp) > currentTemp) {
        heating = true;
        if (heaterMode == "top") {
          digitalWrite(D0, LOW);
          digitalWrite(D1, HIGH);
        } else if (heaterMode == "bottom") {
          digitalWrite(D0, HIGH);
          digitalWrite(D1, LOW);
        } else if (heaterMode == "both"){
          digitalWrite(D0, LOW);
          digitalWrite(D1, LOW);
        }
      } else {
        heating = false;
        digitalWrite(D0, HIGH);
        digitalWrite(D1, HIGH);
      }      
    } else {
      ovenOn = false;
      heating = false;
      digitalWrite(D0, HIGH);
      digitalWrite(D1, HIGH);
    }
  } else {
    heating = false;
    digitalWrite(D0, HIGH);
    digitalWrite(D1, HIGH);
  }
}

String SendHTML(){
  String ptr ="<!DOCTYPE html>";
  ptr +="<html>";
  ptr +="  <head>";
  ptr +="    <meta";
  ptr +="      name=\"viewport\"";
  ptr +="      content=\"width=device-width, initial-scale=1.0, user-scalable=no\"";
  ptr +="    />";
  ptr +="    <title>Oven Control</title>";
  ptr +="    <style>";
  ptr +="      html{font-family:Helvetica;display:inline-block;margin:0 auto;text-align:center}body{margin-top:50px}h1{color:#444;margin:50px auto 30px}h3{color:#444;margin-bottom:50px}.button{display:inline-block;width:40px;background-color:#1abc9c;border:none;color:#fff;padding:13px 20px;text-decoration:none;font-size:25px;margin:0 auto 20px;cursor:pointer;border-radius:4px}.button-inline{background-color:#1abc9c}.button-on{background-color:#1abc9c}.button-on:active{background-color:#16a085}.button-off{background-color:#34495e}.button-off:active{background-color:#2c3e50}p{font-size:14px;color:#888;margin-bottom:10px}";
  ptr +="    </style>";
  ptr +="  </head>";
  ptr +="  <body>";
  ptr +="    <h1>Era Oven 1.0</h1>";
  
  if(ovenOn) { ptr +="<h3>ON</h3>"; } else {ptr +="    <h3>OFF</h3>";}
  
  ptr +="    <p>Select Temp:</p>";
  ptr +="    <span>";
  if (targetTemp == 120) {
      ptr +="      <a class=\"button button-on\" href=\"/temp?val=120\">120</a>";  
  } else {
      ptr +="      <a class=\"button button-off\" href=\"/temp?val=120\">120</a>";  
  }
  if (targetTemp == 150) {
      ptr +="      <a class=\"button button-on\" href=\"/temp?val=150\">150</a>";  
  } else {
      ptr +="      <a class=\"button button-off\" href=\"/temp?val=150\">150</a>";  
  }
  if (targetTemp == 180) {
      ptr +="      <a class=\"button button-on\" href=\"/temp?val=180\">180</a>";  
  } else {
      ptr +="      <a class=\"button button-off\" href=\"/temp?val=180\">180</a>";  
  }
  if (targetTemp == 200) {
      ptr +="      <a class=\"button button-on\" href=\"/temp?val=200\">200</a>";  
  } else {
      ptr +="      <a class=\"button button-off\" href=\"/temp?val=200\">200</a>";  
  }
  if (targetTemp == 240) {
      ptr +="      <a class=\"button button-on\" href=\"/temp?val=240\">240</a>";  
  } else {
      ptr +="      <a class=\"button button-off\" href=\"/temp?val=240\">240</a>";  
  }
  ptr +="    </span>";

  ptr +="    <p>Select Heater: </p>";
  ptr +="    <span>";

  if (heaterMode == "top") {
    ptr +="<a class=\"button button-on\" href=\"/heat?val=top\">&#8256;</a>"; 
  } else {
    ptr +="<a class=\"button button-off\" href=\"/heat?val=top\">&#8256;</a>"; 
  }
  if (heaterMode == "bottom") {
    ptr +="<a class=\"button button-on\" href=\"/heat?val=bottom\">&#8255;</a>"; 
  } else {
    ptr +="<a class=\"button button-off\" href=\"/heat?val=bottom\">&#8255;</a>"; 
  }
  if (heaterMode == "both") {
    ptr +="<a class=\"button button-on\" href=\"/heat?val=both\">=</a>"; 
  } else {
    ptr +="<a class=\"button button-off\" href=\"/heat?val=both\">=</a>"; 
  }
  ptr +="    </span>";
  
  ptr +="    <p>Select Time: </p>";
  ptr +="    <span>";
  if (duration == 10) {
    ptr +="<a class=\"button button-on\" href=\"/time?val=10\">10</a>"; 
  } else {
    ptr +="<a class=\"button button-off\" href=\"/time?val=10\">10</a>"; 
  }
  if (duration == 15) {
    ptr +="<a class=\"button button-on\" href=\"/time?val=15\">15</a>"; 
  } else {
    ptr +="<a class=\"button button-off\" href=\"/time?val=15\">15</a>"; 
  }
  if (duration == 20) {
    ptr +="<a class=\"button button-on\" href=\"/time?val=20\">20</a>"; 
  } else {
    ptr +="<a class=\"button button-off\" href=\"/time?val=20\">20</a>"; 
  }
  if (duration == 30) {
    ptr +="<a class=\"button button-on\" href=\"/time?val=30\">30</a>"; 
  } else {
    ptr +="<a class=\"button button-off\" href=\"/time?val=30\">30</a>"; 
  }
  if (duration == 45) {
    ptr +="<a class=\"button button-on\" href=\"/time?val=45\">45</a>"; 
  } else {
    ptr +="<a class=\"button button-off\" href=\"/time?val=45\">45</a>"; 
  }
  ptr +="    </span>";
  ptr +="    <p></p>";
  if (!ovenOn && targetTemp && heaterMode && duration) {
    ptr +="<a class=\"button button-on\" href=\"/status?val=start\">Start</a>"; 
  } else if (ovenOn) {
    ptr +="<a class=\"button button-off\" href=\"/status?val=stop\">Stop</a>"; 
  }
  ptr +="  </body>";
  ptr +="</html>";  
  return ptr;
}
