// Import required libraries
#include "ESP_Base64.h"
#include "ESP8266WiFi.h"
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUDP.h>
#include <ESP8266mDNS.h>
#include "weatherCalcs.h"
#include <Wire.h>
#include "ESP_httplib.h"
#include <ESP8266HTTPUpdateServer.h>
#include <ArduinoOTA.h>

#include "FS.h"

ESP8266HTTPUpdateServer httpUpdater;


#define QUOTE(name) #name
#define STR(macro) QUOTE(macro)

ESP_httplib esp = ESP_httplib();

#define VERSION "1.0"
// #define BUILD ""


////////////////////////////////////////////////////////////////////////////////////////////////
//////////// DEVICE SPECIFIC HEADERS AND DECLARATIONS //////////////////////////////////////////

#ifdef MCP
  #include "Adafruit_MCP9808.h"
  Adafruit_MCP9808 mcp = Adafruit_MCP9808();
#endif
#ifdef HTU
  #include "Adafruit_HTU21DF.h"
  Adafruit_HTU21DF htu = Adafruit_HTU21DF();
#endif

#ifdef MPL
  #include <Adafruit_MPL3115A2.h>
  Adafruit_MPL3115A2 mpl = Adafruit_MPL3115A2();
#endif

#ifdef INA
  #include <Adafruit_INA219.h>
  Adafruit_INA219 ina219(0x44);
#endif

#ifdef MLX
  #include <Adafruit_MLX90614.h>
  Adafruit_MLX90614 mlx = Adafruit_MLX90614();
#endif



#ifdef SI
  #include "Adafruit_SI1145.h"
  Adafruit_SI1145 si = Adafruit_SI1145();
#endif

#ifdef TMP
  #include "Adafruit_TMP007.h"
  Adafruit_TMP007 tmp(0x43);    // SCL to AD0
#endif

#ifdef TSL
  #include "Adafruit_TSL2591.h"
  Adafruit_TSL2591 tsl = Adafruit_TSL2591(2591);
  void configureSensor(tsl2591Gain_t gainSetting, tsl2591IntegrationTime_t timeSetting);
  float advancedLightSensorRead(void);
#endif

#ifdef SSD
    #include <Adafruit_GFX.h>
    #define OLED_RESET 15
    // #include <Adafruit_SSD1306.h>
    // Adafruit_SSD1306 display(OLED_RESET);
    #include <ESP_SSD1306.h>    // Modification of Adafruit_SSD1306 for ESP8266 compatibility
    ESP_SSD1306 display(OLED_RESET); // FOR I2C

#endif


// #include "Adafruit_LSM9DS0.h"

#ifdef BMP
  #include "Adafruit_BMP183.h"
  #define BMP183_CLK  12  // CLOCK
  #define BMP183_SDO  13  // AKA MISO
  #define BMP183_SDI  14  // AKA MOSI
  #define BMP183_CS   15  // CHIP SELECT
  Adafruit_BMP183 bmp = Adafruit_BMP183(BMP183_CLK, BMP183_SDO, BMP183_SDI, BMP183_CS);
#endif
#ifdef LED
    #ifndef SSD
        #include "Adafruit_LEDBackpack.h"
    #endif
    #include "Adafruit_GFX.h"
    void write4Digit(Adafruit_AlphaNum4 alpha, float num);
    Adafruit_AlphaNum4 alpha4 = Adafruit_AlphaNum4();
#endif
// Adafruit_LSM9DS0 lsm = Adafruit_LSM9DS0();
////////////////////////////////////////////////////////////////////////////////////////////////

float tLast = millis();

unsigned long int tBegin = millis();
float uptime = 0.0;

// WiFi parameters
const char* ssid = "WIFIAP";
const char* password = "PASSWORD";


ESP8266WebServer server(80);
boolean ledOn=true;

WiFiUDP Udp;
int udpPort = 9990;  // udp port
String udpIP = "XX.XX.XX.XX";  // app server ip address
float udpFRQ = 5.;  // update frequency

int rssi;
String dpTemp="mcp";

boolean hasMCP=false,
        hasHTU=false,
        hasBMP=false,
        hasTMP=false,
        hasMPL=false,
        hasALPHNUM=false,
        hasSI=false,
        hasTSL=false,
        hasMLX=false,
        hasINA=false;

static boolean hasLSM=false;

float f,hum,htuT,dp,hi;
float bp, bmpT;
float mplbp,mplT,mplAlt;
float Vshunt, Vbus, Ima, Vload;
float uvI;
float objT,dieT;
float brightness;

Settings settings;

DataSet dataset;
void readSensors(void){

    #ifdef MCP
      f=mcp.readTempC() * 1.8 + 32.0;
    #endif

    #ifdef HTU
      hum=htu.readHumidity();
      htuT=htu.readTemperature() * 1.8 + 32.0;
    #endif

    #ifdef TMP
      objT=tmp.readObjTempC() * 1.8 + 32.0;
      dieT=tmp.readDieTempC() * 1.8 + 32.0;
    #endif

    #ifdef BMP
      bmpT=bmp.getTemperature() * 1.8 + 32.0;
      bp=bmp.getPressure() * 0.0002953;
      float alt = 290.;
      bp = meanSeaLevelPressure(bp, bmpT, alt);

    #endif


    #ifdef INA
      Vshunt = ina219.getShuntVoltage_mV();
      Vbus = ina219.getBusVoltage_V();
      Ima = ina219.getCurrent_mA();
      Vload = Vbus + (Vshunt / 1000.);
      hasINA=true;
    #endif

    #ifdef MPL
      mplbp = mpl.getPressure() * 0.0002953;
      mplT = mpl.getTemperature() * 1.8 + 32.;
      mplAlt = mpl.getAltitude();
      float alt = 292.0;
      mplbp = meanSeaLevelPressure(mplbp, mplT, alt);
    #endif

    #ifdef SI
      uvI = si.readUV();
      uvI /= 100.0;
    #endif

    #ifdef TSL
      brightness = advancedLightSensorRead();
    #endif

    #ifdef MLX
      dieT=mlx.readAmbientTempF();
      objT=mlx.readObjectTempF();

    #endif

      if (dpTemp=="mcp"){
        dp=dewPoint(f,hum);
        hi=heatIndex(f,hum);
      } else if (dpTemp=="htu"){
        dp=dewPoint(htuT,hum);
        hi=heatIndex(htuT,hum);
      }
      rssi = WiFi.RSSI();
      uptime = 0.001 * (millis() - tBegin);
      dataset.nData=0;
    // #ifdef MCP
      if (hasMCP){
          dataset.data[dataset.nData].value=f;
          dataset.data[dataset.nData].name="Temperature";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 75.;
          dataset.data[dataset.nData].ll1 = 70.;
          dataset.data[dataset.nData].ll2 = 65.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;
      }
    // #endif
    // #ifdef HTU
      if (hasHTU){
          dataset.data[dataset.nData].value=htuT;
          dataset.data[dataset.nData].name="HTU Temp";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 75.;
          dataset.data[dataset.nData].ll1 = 70.;
          dataset.data[dataset.nData].ll2 = 65.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

          dataset.data[dataset.nData].value=hum;
          dataset.data[dataset.nData].name="Humidity";
          dataset.data[dataset.nData].unit="%";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 60.;
          dataset.data[dataset.nData].ll1 = 40.;
          dataset.data[dataset.nData].ll2 = 20.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

          dataset.data[dataset.nData].value=dp;
          dataset.data[dataset.nData].name="Dewpoint";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 65.;
          dataset.data[dataset.nData].ul1 = 60.;
          dataset.data[dataset.nData].ll1 = 45.;
          dataset.data[dataset.nData].ll2 = 30.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

          dataset.data[dataset.nData].value=hi;
          dataset.data[dataset.nData].name="Heat Index";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 90.;
          dataset.data[dataset.nData].ul1 = 80.;
          dataset.data[dataset.nData].ll1 = 75.;
          dataset.data[dataset.nData].ll2 = -100.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

      }
    // #endif
    // #ifdef BMP
      if (hasBMP){
          dataset.data[dataset.nData].value=bp;
          dataset.data[dataset.nData].name="Pressure";
          dataset.data[dataset.nData].unit="in-Hg";
          dataset.data[dataset.nData].ul2 = 30.25;
          dataset.data[dataset.nData].ul1 = 30.;
          dataset.data[dataset.nData].ll1 = 29.75;
          dataset.data[dataset.nData].ll2 = 29.5;
          dataset.data[dataset.nData].invertLimits = true;
          dataset.nData++;

          dataset.data[dataset.nData].value=bmpT;
          dataset.data[dataset.nData].name="Pressure Temp";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 75.;
          dataset.data[dataset.nData].ll1 = 70.;
          dataset.data[dataset.nData].ll2 = 65.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;
      }
      if (hasMPL){
          dataset.data[dataset.nData].value=mplbp;
          dataset.data[dataset.nData].name="Pressure";
          dataset.data[dataset.nData].unit="in-Hg";
          dataset.data[dataset.nData].ul2 = 30.25;
          dataset.data[dataset.nData].ul1 = 30.;
          dataset.data[dataset.nData].ll1 = 29.75;
          dataset.data[dataset.nData].ll2 = 29.5;
          dataset.data[dataset.nData].invertLimits = true;
          dataset.nData++;

          dataset.data[dataset.nData].value=mplT;
          dataset.data[dataset.nData].name="Pressure Temp";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 75.;
          dataset.data[dataset.nData].ll1 = 70.;
          dataset.data[dataset.nData].ll2 = 65.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;
      }
      if (hasINA){
          dataset.data[dataset.nData].value=Vload;
          dataset.data[dataset.nData].name="Load";
          dataset.data[dataset.nData].unit="V";
          dataset.data[dataset.nData].ul2 = 5.5;
          dataset.data[dataset.nData].ul1 = 5.0;
          dataset.data[dataset.nData].ll1 = 3.4;
          dataset.data[dataset.nData].ll2 = 3.2;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;


          dataset.data[dataset.nData].value=Ima;
          dataset.data[dataset.nData].name="Current";
          dataset.data[dataset.nData].unit="mA";
          dataset.data[dataset.nData].ul2 = 200;
          dataset.data[dataset.nData].ul1 = 100;
          dataset.data[dataset.nData].ll1 = 50;
          dataset.data[dataset.nData].ll2 = 30;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

        }


    // #endif
    // #ifdef SI
      if (hasSI){
          dataset.data[dataset.nData].value=uvI;
          dataset.data[dataset.nData].name="UV Index";
          dataset.data[dataset.nData].unit="";
          dataset.data[dataset.nData].ul2 = 8.0;
          dataset.data[dataset.nData].ul1 = 6.5;
          dataset.data[dataset.nData].ll1 = 5.0;
          dataset.data[dataset.nData].ll2 = 3.0;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

      }
    // #endif
    // #ifdef TMP
      if (hasTMP){
          dataset.data[dataset.nData].value=objT;
          dataset.data[dataset.nData].name="IR Temp";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 75.;
          dataset.data[dataset.nData].ll1 = 70.;
          dataset.data[dataset.nData].ll2 = 65.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

          dataset.data[dataset.nData].value=dieT;
          dataset.data[dataset.nData].name="TMP Temp";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 75.;
          dataset.data[dataset.nData].ll1 = 70.;
          dataset.data[dataset.nData].ll2 = 65.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

          dataset.data[dataset.nData].value=dieT - objT;
          dataset.data[dataset.nData].name="IR dT";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 25.;
          dataset.data[dataset.nData].ul1 = 20.;
          dataset.data[dataset.nData].ll1 = 15.;
          dataset.data[dataset.nData].ll2 = 10.;
          dataset.data[dataset.nData].invertLimits = true;
          dataset.nData++;

      }

      if (hasMLX){
          dataset.data[dataset.nData].value=objT;
          dataset.data[dataset.nData].name="IR Temp";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 75.;
          dataset.data[dataset.nData].ll1 = 70.;
          dataset.data[dataset.nData].ll2 = 65.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

          dataset.data[dataset.nData].value=dieT;
          dataset.data[dataset.nData].name="MLX Temp";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 80.;
          dataset.data[dataset.nData].ul1 = 75.;
          dataset.data[dataset.nData].ll1 = 70.;
          dataset.data[dataset.nData].ll2 = 65.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

          dataset.data[dataset.nData].value=dieT - objT;
          dataset.data[dataset.nData].name="IR dT";
          dataset.data[dataset.nData].unit="&degF";
          dataset.data[dataset.nData].ul2 = 25.;
          dataset.data[dataset.nData].ul1 = 20.;
          dataset.data[dataset.nData].ll1 = 15.;
          dataset.data[dataset.nData].ll2 = 10.;
          dataset.data[dataset.nData].invertLimits = true;
          dataset.nData++;

      }
// #endif
    // #ifdef TSL
      if (hasTSL){
          dataset.data[dataset.nData].value=brightness;
          dataset.data[dataset.nData].name="Brightness";
          dataset.data[dataset.nData].unit="lux";
          dataset.data[dataset.nData].ul2 = 100000.;
          dataset.data[dataset.nData].ul1 = 10000.;
          dataset.data[dataset.nData].ll1 = 1000.;
          dataset.data[dataset.nData].ll2 = 100.;
          dataset.data[dataset.nData].invertLimits = false;
          dataset.nData++;

      }
    // #endif

      dataset.data[dataset.nData].value=uptime;
      dataset.data[dataset.nData].name="uptime";
      dataset.data[dataset.nData].unit="s";
      dataset.data[dataset.nData].ul2 = 80.;
      dataset.data[dataset.nData].ul1 = 75.;
      dataset.data[dataset.nData].ll1 = 70.;
      dataset.data[dataset.nData].ll2 = 65.;
      dataset.data[dataset.nData].invertLimits = false;
      dataset.nData++;

      dataset.data[dataset.nData].value=rssi;
      dataset.data[dataset.nData].name="RSSI";
      dataset.data[dataset.nData].unit="dBm";
      dataset.data[dataset.nData].ul2 = -20.;
      dataset.data[dataset.nData].ul1 = -40.;
      dataset.data[dataset.nData].ll1 = -60.;
      dataset.data[dataset.nData].ll2 = -70.;
      dataset.data[dataset.nData].invertLimits = true;
      dataset.nData++;




}



void udpSendPacket(void);



////////////////////////////////////////////////////////////////////////
// Webserver handling
////////////////////////////////////////////////////////////////////////
void handle_root() {
    Settings settings;
    settings.udpFRQ=udpFRQ;
    settings.udpIP=udpIP;
    settings.udpPort=udpPort;
    esp.http.settings = settings;


  esp.http.updatePage(dataset, esp.packet);
  server.send(200, "text/html", esp.http.page());
}

void handle_post(){
    esp.formPacket(dataset);
 //    server.send(200, "application/json", esp.packet);            // send to someones browser when asked
    server.send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"1; URL='http://" + esp.stationIP + "'\" />");            // send to someones browser when asked
    Serial.println("Sent packet for temp");

    for (uint8_t i=0; i<server.args(); i++){
      if (server.argName(i)== "udpFRQ"){
        Serial.println("Updated " + server.argName(i) + " to: " + server.arg(server.argName(i)));
        udpFRQ=server.arg("udpFRQ").toFloat();
        esp.http.settings.udpFRQ = udpFRQ;
      }
      if (server.argName(i)== "dpTemp"){
        Serial.println("Updated " + server.argName(i) + " to: " + server.arg(server.argName(i)));
        dpTemp=server.arg("dpTemp");
        // settings.udpFRQ = udpFRQ;
      }
      if (server.argName(i)== "udpPort"){
        Serial.println("Updated " + server.argName(i) + " to: " + server.arg(server.argName(i)));
        udpPort=server.arg("udpPort").toInt();
        esp.http.settings.udpPort = udpPort;
      }
      if (server.argName(i)== "udpIP"){
        Serial.println("Updated " + server.argName(i) + " to: " + server.arg(server.argName(i)));
        udpIP=server.arg("udpIP");
        esp.http.settings.udpIP = udpIP;
      }
    }
}


void handle_data(){
    esp.formPacket(dataset);
    server.send(200, "application/json", esp.packet);            // send to someones browser when asked
    Serial.println("Sent packet for temp");
}


void handle_reset(){
    server.send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"3; URL='http://" + esp.stationIP + "'\" />");            // send to someones browser when asked
    Serial.println("Reset requested");
    esp.triggerReset();
}

void handle_ledOff(){
    ledOn=false;
    server.send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL='http://" + esp.stationIP + "'\" />");
}
void handle_ledOn(){
    ledOn=true;
    server.send(200, "text/html", "<meta http-equiv=\"refresh\" content=\"0; URL='http://" + esp.stationIP + "'\" />");
}


void handle_spiffs(){
    SPIFFS.begin();

    File f = SPIFFS.open("/test.html", "r");
    if (!f) {
        Serial.println("file open failed");
    } else{

      // Serial.println(f.readString());
      server.send(200, "text/html", f.readString());
    }
}


////////////////////////////////////////////////////////////////////////
// Setup
////////////////////////////////////////////////////////////////////////
void setup(void)
{
  Wire.begin();
  Wire.pins(4, 0);
  // Start Serial
  Serial.begin(115200);

  esp.begin(ssid, password);
  //  esp.http.begin();
  esp.http.stationName = STR(NAME);//"Development Server";
  esp.http.version = VERSION;
  esp.http.begin();
  esp.http.updatePage(dataset, esp.packet);

#ifdef SSD
  display.begin(SSD1306_SWITCHCAPVCC, 0x3D);  // initialize with the I2C addr 0x3D (for the 128x64)
  display.setTextSize(0);
  display.setTextColor(WHITE);
  int line = 0;
// delay(2000);


#endif


  server.on("/", handle_root);

  server.on("/data", handle_data);

  server.on("/post", handle_post);

  server.on("/reset", handle_reset);

  server.on("/ledOff", handle_ledOff);
  server.on("/ledOn", handle_ledOn);

  server.on("/spiffs", handle_spiffs);

  httpUpdater.setup(&server);
  server.begin();
  Serial.println("HTTP server started");

  // MDNS.addService("http", "tcp", 80);
  // if (!MDNS.begin("snazzy")) {
  //   Serial.println("Error setting up MDNS responder!");
  //   while(1) {
  //     delay(1000);
  //   }
  // }
  // Serial.println("mDNS responder started");


#ifdef MCP
  if (mcp.begin()) {
    Serial.println("Found MCP sensor");
    hasMCP=true;
  }
#endif

#ifdef HTU
  if (htu.begin()) {
    Serial.println("Found HTU sensor");
    hasHTU=true;
  }
#endif

#ifdef BMP
  if (bmp.begin()) {
    Serial.println("Found BMP sensor");
    hasBMP=true;
  }
#endif

#ifdef TMP
  if (tmp.begin()) {
    Serial.println("Found TMP sensor");
    hasTMP=true;
  }
#endif

#ifdef MLX
  Serial.println("MLX sensor requested");
  mlx.begin();
  hasMLX=true;
#endif

#ifdef SI
 Serial.println("SI requested");
  if (si.begin()) {
    Serial.println("Found SI sensor");
    hasSI=true;
  }
  hasSI=true;
#endif

#ifdef TSL
  if (tsl.begin()) {
    Serial.println("Found TSL sensor");
    configureSensor(TSL2591_GAIN_LOW, TSL2591_INTEGRATIONTIME_100MS);
    hasTSL=true;
  }
#endif

#ifdef INA
  Serial.println("INA sensor requested");
  ina219.begin();
  hasINA=true;
#endif

#ifdef MPL
  if (mpl.begin()) {
    Serial.println("Found MPL sensor");
    hasMPL=true;
  }
#endif

#ifdef LED
    alpha4.begin(0x70);  // pass in the address
#endif

  Serial.println("Finished uploading sensors");
  //

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("snazz-face");

  ArduinoOTA.setPassword((const char *)"insecure");

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
  Serial.println("Ready");
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  // String str = "";
  // Dir dir = SPIFFS.openDir("/");
  // while (dir.next()) {
  //     str += dir.fileName();
  //     str += " / ";
  //     str += dir.fileSize();
  //     str += "\r\n";
  // }
  // Serial.print(str);
}


void udpSendPacket(void){
  if (millis() - tLast > int(1000*udpFRQ)){
    Udp.beginPacket(udpIP.c_str(),udpPort);
    esp.formPacket(dataset);
    Udp.print(esp.packet);
    // Serial.println(esp.packet);
    tLast=millis();
    Udp.endPacket();
    if (ledOn) {
      esp.triggerActivityLED();
    }
#ifdef SSD
    display.clearDisplay();
    display.setCursor(0,0);
    display.print(esp.http.stationName);
    display.setCursor(0,8);
    display.print("Version: ");
    display.print(VERSION);
    display.setCursor(0,16);
    display.print("RSSI: ");
    display.print(rssi);
    display.setCursor(0,24);
    display.print("IP: ");
    display.print(esp.stationIP);

    for (int i=0; i < dataset.nData; i++){
        display.setCursor(0,32+8*i);
        display.print(dataset.data[i].name + " = ");
        display.print(dataset.data[i].value);
        // display.print(dataset.data[i].unit);

    }

    display.display();
    // display.setTextSize(1);
    // display.setTextColor(WHITE);
    // display.setCursor(0,0);
    // display.println("Hello, world!");
    // display.setTextColor(BLACK, WHITE); // 'inverted' text
    // display.println(3.141592);
    // display.setTextSize(2);
    // display.setTextColor(WHITE);
    // display.print("0x"); display.println(0xDEADBEEF, HEX);
    // display.display();
    // delay(2000);
#endif

  }
}



float toggleTime=millis();
float lastTime = toggleTime;
int count=0, toggle=0;
void loop()
{
  ArduinoOTA.handle();
  server.handleClient();
  readSensors();
  udpSendPacket();


#ifdef LED
    count++;
    if(millis() - toggleTime > 2*1000){
      toggleTime=millis();
      toggle++;
    }
    if(millis() - lastTime > 250){
      lastTime = millis();
      if(toggle % 2 == 0){
        alpha4.setBrightness(15);
        write4Digit(alpha4,dataset.data[0].value);
      } else{
         alpha4.setBrightness(3);
         write4Digit(alpha4,dataset.data[3].value);
      }
    }
#endif

}

#ifdef LED

    static const uint16_t alphafonttable[] PROGMEM =  {

    0b0000000000000001,
    0b0000000000000010,
    0b0000000000000100,
    0b0000000000001000,
    0b0000000000010000,
    0b0000000000100000,
    0b0000000001000000,
    0b0000000010000000,
    0b0000000100000000,
    0b0000001000000000,
    0b0000010000000000,
    0b0000100000000000,
    0b0001000000000000,
    0b0010000000000000,
    0b0100000000000000,
    0b1000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0000000000000000,
    0b0001001011001001,
    0b0001010111000000,
    0b0001001011111001,
    0b0000000011100011,
    0b0000010100110000,
    0b0001001011001000,
    0b0011101000000000,
    0b0001011100000000,
    0b0000000000000000, //
    0b0000000000000110, // !
    0b0000001000100000, // "
    0b0001001011001110, // #
    0b0001001011101101, // $
    0b0000110000100100, // %
    0b0010001101011101, // &
    0b0000010000000000, // '
    0b0010010000000000, // (
    0b0000100100000000, // )
    0b0011111111000000, // *
    0b0001001011000000, // +
    0b0000100000000000, // ,
    0b0000000011000000, // -
    0b0000000000000000, // .
    0b0000110000000000, // /
    0b0000110000111111, // 0
    0b0000000000000110, // 1
    0b0000000011011011, // 2
    0b0000000010001111, // 3
    0b0000000011100110, // 4
    0b0010000001101001, // 5
    0b0000000011111101, // 6
    0b0000000000000111, // 7
    0b0000000011111111, // 8
    0b0000000011101111, // 9
    0b0001001000000000, // :
    0b0000101000000000, // ;
    0b0010010000000000, // <
    0b0000000011001000, // =
    0b0000100100000000, // >
    0b0001000010000011, // ?
    0b0000001010111011, // @
    0b0000000011110111, // A
    0b0001001010001111, // B
    0b0000000000111001, // C
    0b0001001000001111, // D
    0b0000000011111001, // E
    0b0000000001110001, // F
    0b0000000010111101, // G
    0b0000000011110110, // H
    0b0001001000000000, // I
    0b0000000000011110, // J
    0b0010010001110000, // K
    0b0000000000111000, // L
    0b0000010100110110, // M
    0b0010000100110110, // N
    0b0000000000111111, // O
    0b0000000011110011, // P
    0b0010000000111111, // Q
    0b0010000011110011, // R
    0b0000000011101101, // S
    0b0001001000000001, // T
    0b0000000000111110, // U
    0b0000110000110000, // V
    0b0010100000110110, // W
    0b0010110100000000, // X
    0b0001010100000000, // Y
    0b0000110000001001, // Z
    0b0000000000111001, // [
    0b0010000100000000, //
    0b0000000000001111, // ]
    0b0000110000000011, // ^
    0b0000000000001000, // _
    0b0000000100000000, // `
    0b0001000001011000, // a
    0b0010000001111000, // b
    0b0000000011011000, // c
    0b0000100010001110, // d
    0b0000100001011000, // e
    0b0000000001110001, // f
    0b0000010010001110, // g
    0b0001000001110000, // h
    0b0001000000000000, // i
    0b0000000000001110, // j
    0b0011011000000000, // k
    0b0000000000110000, // l
    0b0001000011010100, // m
    0b0001000001010000, // n
    0b0000000011011100, // o
    0b0000000101110000, // p
    0b0000010010000110, // q
    0b0000000001010000, // r
    0b0010000010001000, // s
    0b0000000001111000, // t
    0b0000000000011100, // u
    0b0010000000000100, // v
    0b0010100000010100, // w
    0b0010100011000000, // x
    0b0010000000001100, // y
    0b0000100001001000, // z
    0b0000100101001001, // {
    0b0001001000000000, // |
    0b0010010010001001, // }
    0b0000010100100000, // ~
    0b0011111111111111,

    };
    void write4Digit(Adafruit_AlphaNum4 alpha, float num){
      int digit,indx;
      int lognumIndx = int(log10(num));



      for (indx=3; indx>=0; indx--){
        int digit = int(num * pow(10.0,-lognumIndx+indx)) % 10;
        char buff[1];
        itoa(digit,buff,10);
    //    Serial.print(indx);
    //   Serial.print( buff[0]);
    //   Serial.println();
           alpha.writeDigitAscii(indx,buff[0]);
        if (indx == lognumIndx) {
           alpha.writeDigitRaw(indx,0x4000+pgm_read_word(alphafonttable+buff[0]));
        }
      }
    //  Serial.println();
      alpha4.clear();
      alpha.writeDisplay();


    }
#endif


#ifdef TSL
void configureSensor(tsl2591Gain_t gainSetting, tsl2591IntegrationTime_t timeSetting)
{
  // You can change the gain on the fly, to adapt to brighter/dimmer light situations
  tsl.setGain(gainSetting);
  //tsl.setGain(TSL2591_GAIN_LOW);    // 1x gain (bright light)
  //tsl.setGain(TSL2591_GAIN_MED);      // 25x gain
  //tsl.setGain(TSL2591_GAIN_HIGH);   // 428x gain

  // Changing the integration time gives you a longer time over which to sense light
  // longer timelines are slower, but are good in very low light situtations!
  tsl.setTiming(timeSetting);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_100MS);  // shortest integration time (bright light)
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_200MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_300MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_400MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_500MS);
  //tsl.setTiming(TSL2591_INTEGRATIONTIME_600MS);  // longest integration time (dim light)

  /* Display the gain and integration time for reference sake */
  // XBee.println("------------------------------------");
  // XBee.print  ("Gain:         ");
  tsl2591Gain_t gain = tsl.getGain();
  // switch(gain)
  // {
  // case TSL2591_GAIN_LOW:
  //   XBee.println("1x (Low)");
  //   break;
  // case TSL2591_GAIN_MED:
  //   XBee.println("25x (Medium)");
  //   break;
  // case TSL2591_GAIN_HIGH:
  //   XBee.println("428x (High)");
  //   break;
  // case TSL2591_GAIN_MAX:
  //   XBee.println("9876x (Max)");
  //   break;
  // }
  // XBee.print  ("Timing:       ");
  // XBee.print((tsl.getTiming() + 1) * 100, DEC);
  // XBee.println(" ms");
  // XBee.println("------------------------------------");
  // XBee.println("");
}

float advancedLightSensorRead(void)
{
  // More advanced data read example. Read 32 bits with top 16 bits IR, bottom 16 bits full spectrum
  // That way you can do whatever math and comparisons you want!
  uint32_t lum = tsl.getFullLuminosity();
  uint16_t ir, full;
  float lux;
  lum = tsl.getFullLuminosity(); // first reading will be incorrect of Gain or Time was changed
  ir = lum >> 16;
  full = lum & 0xFFFF;
  lux = tsl.calculateLux(full, ir);
//  XBee.println();
//  XBee.print(F("Lux "));
//  XBee.println(lux);

  if (full < 100){
    switch (tsl.getGain())
    {
    case TSL2591_GAIN_LOW :
      configureSensor(TSL2591_GAIN_MED, TSL2591_INTEGRATIONTIME_200MS);
      // XBee.print(F("Gain raised to MED"));
      break;
    case TSL2591_GAIN_MED :
      configureSensor(TSL2591_GAIN_HIGH, TSL2591_INTEGRATIONTIME_200MS);
      // XBee.print(F("Gain raised to HIGH"));
      break;
      /*        case TSL2591_GAIN_HIGH :
       configureSensor(TSL2591_GAIN_MAX, TSL2591_INTEGRATIONTIME_200MS);
       XBee.print("Gain raised to MAX");
       break;
       case TSL2591_GAIN_MAX :
       XBee.print("Gain already at MAX");
       break;
       */
    default:
      configureSensor(TSL2591_GAIN_MED, TSL2591_INTEGRATIONTIME_200MS);
      break;
    }

  }

  if (full > 30000){
    switch (tsl.getGain())
    {
    case TSL2591_GAIN_LOW :
      // XBee.print(F("Gain already at LOW"));
      break;
    case TSL2591_GAIN_MED :
      configureSensor(TSL2591_GAIN_LOW, TSL2591_INTEGRATIONTIME_200MS);
      // XBee.print(F("Gain lowered to LOW"));
      break;
      /*        case TSL2591_GAIN_HIGH :
       configureSensor(TSL2591_GAIN_MED, TSL2591_INTEGRATIONTIME_200MS);
       XBee.print("Gain lowered to MED");
       break;
       case TSL2591_GAIN_MAX :
       configureSensor(TSL2591_GAIN_HIGH, TSL2591_INTEGRATIONTIME_200MS);
       XBee.print("Gain lowered to MED");
       break;
       */
    default:
      configureSensor(TSL2591_GAIN_MED, TSL2591_INTEGRATIONTIME_200MS);
      break;
    }

  }
  return lux;

}
#endif
