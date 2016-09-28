ESP8266 driven weather station with Flask webserver frontend.

There are two main components to this project.  The device source in esp8266/ and the webserver in app/.  Data on device can be accessed directly via: http://deviceIP

For debuggin purposes, the device can have firmware uploaded directly from the webpage.  It's recommended that after deployment, this feature be removed for security.

Recommend platformio for compiling and distributing the esp8266 source to devices.
![Alt text](/screenshots/WXDevice_ESP8266.png?raw=true "Device Webpage")

Flask and SocketIO frontend to weather station.  It can accept multiple stations simultaneously.  

To setup:
pip -r requirements

To run:
Run station receiver that pushes socketIO packets to flask webserver:
$python stationServer.py

Run flask webserver that listens for SocketIO packets:
$python app.py

Webpage will be located at and will update when packet is received from stations
http://localhost:5000/

![Alt text](/screenshots/FlaskWXServer.png?raw=true "Flask Server Webpage")
