#if ARDUINO >= 100
 #include "Arduino.h"
#else
 #include "WProgram.h"
#endif

#ifdef __AVR_ATtiny85__
 #include "TinyWireM.h"
 #define Wire TinyWireM
#else
 #include <Wire.h>
#endif

#include "ESP8266WiFi.h"

#define NOTIFY_PIN 16
#define RESET_PIN 5

typedef struct  {
  String name;
  float value;
  String unit;
  float ul1, ul2, ll1, ll2;
  boolean invertLimits;
} Data;

typedef struct{
    String udpIP;
    int udpPort;
    float udpFRQ;
} Settings;

typedef struct  {
	int maxData=20;
	int nData;
	Data data[20];


} DataSet;



class ESP_HTTP {
  public:
	ESP_HTTP();
	boolean begin(void);
	boolean updatePage(DataSet dataset, String packet);
    Settings settings;
	String header;
	String stationName;
	String css;
	String body;
	String content;
	String dataContent;
	String buttons, jsonButton;
	String endHTML;
	String footer;
       String version;
       String getStatus(Data data);
	String page(void);
  float uptime;
};

class ESP_httplib {
  public:
	ESP_httplib();
	ESP_HTTP http;
	boolean begin(const char* ssid, const char* password);
	void triggerActivityLED(void);
	void triggerReset(void);
	String stationIP;
	String packet;
	void formPacket(DataSet dataset);
	void sendUDPPacket(void);
private:
  unsigned long int tStart;


};
