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

#include "ESP_httplib.h"
#include "ESP8266WiFi.h"

/**************************************************************************/
//	Instantiates new ESP class
/**************************************************************************/
ESP_HTTP::ESP_HTTP(){
}

/**************************************************************************/
//	Instantiates new ESP class
/**************************************************************************/
boolean ESP_HTTP::begin(void){
	header="<!DOCTYPE html><html><head><title>" + stationName +"</title>";
  // refresh HTML page
	header +="<meta http-equiv=\"refresh\" content=\"30\">";
	header +="</head>";

  css="";
  css+="<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
  css+="<link rel=\"stylesheet\" href=\"http://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/css/bootstrap.min.css\">";
  css+="<script src=\"https://ajax.googleapis.com/ajax/libs/jquery/1.12.0/jquery.min.js\"></script>";
  css+="<script src=\"http://maxcdn.bootstrapcdn.com/bootstrap/3.3.6/js/bootstrap.min.js\"></script>";
	css+="<style>";
	css+="footer {";
	css+="color: brown;";
	css+="font-style: oblique;";
	css+="}";
	css+="</style>";
	css+="<LINK href=\"https://bootswatch.com/slate/bootstrap.min.css\" rel=\"stylesheet\" type=\"text/css\">";
	body="<header>";
	endHTML="</header></body></html>";
	return true;
}

String ESP_HTTP::getStatus(Data data){

  //  Apply color coding status using data limits
    String dataclass;
    if (data.value > data.ul2){
        if (data.invertLimits){
          dataclass = "info";
        } else {
          dataclass = "danger";
        }
    } else     if (data.value > data.ul1){
        if (data.invertLimits){
          dataclass = "success";
        } else {
          dataclass = "warning";
        }
    } else     if (data.value < data.ll2){
        if (data.invertLimits){
          dataclass = "danger";
        } else {
          dataclass = "info";
        }
    } else     if (data.value < data.ll1){
        if (data.invertLimits){
          dataclass = "warning";
        } else {
          dataclass = "success";
        }
    } else     {
        dataclass = "success";
    }

    return dataclass;
}

boolean ESP_HTTP::updatePage(DataSet dataset, String packet){


  content = "<div class=\"container\">";
  content += "<div class=\"page-header\">";
  content += "<h1>" + stationName + "<h1>";
  content += "</div>";

	dataContent="<table class=\"table table-striped table-hover\"><thead><tr><th>Measurement</th><th>Value</th><th></th></tr></thead><tbody>";
  for (int i=0; i < dataset.nData; i++){
    if (dataset.data[i].name == "uptime"){
      uptime = dataset.data[i].value;
      continue;
    }
    String dataclass=getStatus(dataset.data[i]);

  	if (i==0) {
		dataContent += "<tr data-toggle='collapse' data-target='.collapseTest' class=\"" + dataclass + "\">" + " <td class=\"col-md-6\">" + dataset.data[i].name + "</td><td class=\"col-md-5\">" + String(dataset.data[i].value) + " " + dataset.data[i].unit + "</td><td class=\"col-md-1\"><button data-toggle='collapse' data-target='.collapseTest' class='btn btn-default btn-xs'><span class='glyphicon glyphicon-menu-up'></span></button></td></tr>";
		} else {
		dataContent += "<tr class=\"" + dataclass + " collapse in collapseTest\">" + " <td class=\"col-md-6\">" + dataset.data[i].name + "</td><td class=\"col-md-5\">" + String(dataset.data[i].value) + " " + dataset.data[i].unit + "</td><td class='col-md-1'></td></tr>";
		}

	}
  dataContent += "</tbody></table>";

  dataContent += "  <div class=\"dropdown\"> \
    <button class=\"btn btn-primary dropdown-toggle\" type=\"button\"  data-toggle=\"dropdown\">Dewpoint Calculation \
    <span class=\"caret\"></span></button> \
    <ul class=\"dropdown-menu\"> \
      <li><a href=\"post?dpTemp=mcp\">High Res Temperature</a></li> \
      <li><a href=\"post?dpTemp=htu\">Humidity Temperature</a></li> \
    </ul> \
  </div>";

  dataContent += "<form action=\"post\" method=\"post\" >";

  dataContent += "<div class=\"form-group row\">";
  dataContent += "<label  class=\"col-sm-4\" for=\"udpFRQ\">UDP Packet Frequency:  </label>";
  dataContent += "<div class=\"col-sm-4\"><input type=\"text\" name=\"udpFRQ\"; value=\""+String(settings.udpFRQ)+"\"class=\"btn-link\"></div>";
  dataContent += "</div>";

  dataContent += "<div class=\"form-group row\">";
  dataContent += "<label class=\"col-sm-4\" for=\"udpIP\">UDP IP:  </label>";
  dataContent += "<div class=\"col-sm-4\"><input type=\"text\" name=\"udpIP\"; value=\""+String(settings.udpIP)+"\"class=\"btn-link\"></div>";
  dataContent += "</div>";

  dataContent += "<div class=\"form-group row\">";
  dataContent += "<label  class=\"col-sm-4\" for=\"udpPort\">UDP Port:  </label>";
  dataContent += "<div class=\"col-sm-4\"><input type=\"text\" name=\"udpPort\"; value=\""+String(settings.udpPort)+"\"class=\"btn-link\"></div>";
  dataContent += "</div>";

  dataContent += "<input type=\"submit\" class=\"btn btn-info col-sm-4\" value=\"Submit\">";
  dataContent += "</form><br>";


  buttons = "<button type=\"button\" class=\"btn btn-info btn-block\" data-toggle=\"modal\" data-target=\"#myModal\">JSON Data</button><!-- Modal --><div class=\"modal fade\" id=\"myModal\" role=\"dialog\"><div class=\"modal-dialog\"><!-- Modal content--><div class=\"modal-content\"><div class=\"modal-header\"><button type=\"button\" class=\"close\" data-dismiss=\"modal\">&times;</button><h4 class=\"modal-title\">JSON Data</h4></div><div class=\"modal-body\"><p>"+ packet +"</p></div><div class=\"modal-footer\"><button type=\"button\" class=\"btn btn-default\" data-dismiss=\"modal\">Close</button></div></div></div></div>";


  buttons += "<form method='POST' action='/update' enctype='multipart/form-data'>\
                  <input type='file' name='update' class='btn btn-info col-xs-8'>\
                  <input type='submit' value='Update Firmware' class='btn btn-danger col-xs-4'> \
               </form>";

 buttons += "<br><form action='ledOff' method='GET'><button class='btn-danger col-xs-4' name=\"led\" value=\"off\">LED Off</button></form>";
 buttons += "<form action='ledOn' method='GET'><button class='btn-success col-xs-4' name=\"led\" value=\"off\">LED On</button></form>";

 buttons += "<button type=\"button\" class=\"btn btn-danger btn-block\"><a href=\"reset\">Reset Device</a></button>";

footer="<br<br><div class=\"container\"><div class=\"panel-footer\" class=\"container-fluid\"><div class=\"row\"></div>Hello from " + stationName + "<br>Station sensor: Version " + String(version) + " <br> Uptime: " + String(uptime) + " seconds </div></div></div>";


	return true;
}


String ESP_HTTP::page(void){
	String htmlpage = header + css + body + content + dataContent + buttons + jsonButton + endHTML + footer;
	return htmlpage;
}



/**************************************************************************/
//	Instantiates new ESP class
/**************************************************************************/
ESP_httplib::ESP_httplib(){
}

/**************************************************************************/
//	Instantiates new ESP class
/**************************************************************************/
boolean ESP_httplib::begin(const char* ssid, const char* password) {

  pinMode(RESET_PIN, OUTPUT);
  digitalWrite(RESET_PIN, HIGH);
  pinMode(NOTIFY_PIN, OUTPUT);
  digitalWrite(NOTIFY_PIN, HIGH);

  http.begin();
  //  // Connect to WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
	Serial.begin(115200);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");

  // Print the IP address
  stationIP=WiFi.localIP().toString();
  Serial.println(stationIP);

  // tStart = millis();


  return true;
}

// float uptime(void){
//   return 0.001 * ( millis() - tStart);
// }

void ESP_httplib::triggerActivityLED(void){
  digitalWrite(NOTIFY_PIN, LOW);    // turn the LED off by making the voltage LOW
  delay(10);
  digitalWrite(NOTIFY_PIN, HIGH);    // turn the LED off by making the voltage LOW

}

void ESP_httplib::triggerReset(){
    digitalWrite(RESET_PIN, LOW);    // turn the LED off by making the voltage LOW
}



void ESP_httplib::formPacket(DataSet dataset){

  String dataPacket = "";
  for (int i=0; i < dataset.nData; i++){
    dataPacket += "\"" + dataset.data[i].name + "\": { \"value\": " + String(dataset.data[i].value,3) + ", \"class\": \"" + http.getStatus(dataset.data[i]) + "\", \"unit\": \"" + dataset.data[i].unit + "\"}";
    if (i != dataset.nData - 1){
      dataPacket += ", ";
    }
  }

  String name = "\"name\": \"" + http.stationName + "\"";
  String address = "\"ip\": \"" + stationIP + "\"";

  String rssiPacket = "\"rssi\": " + String(WiFi.RSSI());
  String network = "\"network\": { " + address + ", " + rssiPacket + "}";

  packet = "{ " + name + ", " + network + ", \"data\"  : {" + dataPacket + "} }";
}
