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
