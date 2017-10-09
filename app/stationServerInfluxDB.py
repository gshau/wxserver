from datetime import datetime as dt
import sys
import os
from socketIO_client import SocketIO, LoggingNamespace
from subprocess import check_output
import socket
import time
import json
import stationDB
import datetime
from crypt import *
import numpy as np

from influxdb import InfluxDBClient




class Station:
    def __init__(self):
        self.name='Rpi'
        self.udpPort=8123
        self.BUF_SIZE=2048
        self.packets={}
        self.packetMean={}
        self.nMeasurements={}
        self.lastUpdate={}
#        self.avgFreq=30
        self.lastRelease=datetime.datetime.now()
	influxIP = '127.0.0.1'
	self.client = InfluxDBClient(influxIP, 8086, 'admin', 'admin', 'stationDB')




    def initSocket(self, host, port ):
        try:
                self.socketIO = SocketIO(host, port)
                self.socketConnected=True
        except:
                print "Unable to open socket: ", sys.exc_info()[0]
                self.socketConnected=False
                raise

    def startUDPListen(self,ip,port):
        self.sock = socket.socket(socket.AF_INET, # Internet
                             socket.SOCK_DGRAM) # UDP
        self.sock.bind((ip, port))

    def startUDPSend(self):
        self.sock = socket.socket(socket.AF_INET, # Internet
             socket.SOCK_DGRAM) # UDP


    def sendPacket(self, message, ip, port):
        self.sock.sendto(message, (ip, port))

    def recvPacket(self, verbose):
        data, addr = self.sock.recvfrom(self.BUF_SIZE) # buffer size is 1024 bytes
        # decoded = decodePacket(self.secret_key,data)
        try:
            packet=json.loads(data)
        except ValueError, e:
            print 'bad json data'
            print data
            return {}

        if verbose:
            print "Message from ", addr, " :", packet
        return packet


#
#db=stationDB.DB('/root/sio/data.sdb','master')
s=Station()
s.udpPort=9990
s.startUDPListen('0.0.0.0',s.udpPort)

sio=SocketIO('localhost', 5000)
verbose=True #False
readPackets=True
while readPackets:
    rawPacket=s.recvPacket(verbose)
    print 'raw Packet: ',rawPacket
    print 'length packet: ', len(rawPacket)
    try:
        sio.emit('dataPacket', rawPacket)
        print 'sent packet'
    except:
        print 'socket not connected'

    # rawPacket['name']

    for dataName in rawPacket['data'].keys():
	packet = rawPacket['name']
        key = rawPacket['name'].replace(' ','') + dataName.replace(' ','')
        value = rawPacket['data'][dataName]['value']
        print 'Updating db: ',key,' = ',value
	now = datetime.datetime.utcnow().strftime("%Y-%m-%dT%H:%M:%S")
	json_body = [{"measurement": dataName, "tags": {"host": packet},"time": now,"fields": {"value": value}}]
	print json_body
	s.client.write_points(json_body)


s.sock.close()
