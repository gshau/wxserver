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


class Station:
    def __init__(self):
        self.name='Rpi'
        self.udpPort=8123
        self.BUF_SIZE=1024
        self.packets={}
        self.packetMean={}
        self.nMeasurements={}
        self.lastUpdate={}
        self.avgFreq=30
        self.lastRelease=datetime.datetime.now()
        # self.secret_key = '1234567890123456'




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

    def checkForPacketRelease(self):
        releasePacket=False
        now=datetime.datetime.now()
        releaseTime=0
        if (now-self.lastRelease).seconds > self.avgFreq:
            self.packetMean['timestamp']=time.time()

            releasePacket=True
            releaseTime=float(now.strftime("%s"))
            self.lastRelease=now
            timeString=now.strftime('%Y/%m/%d %H:%M:%S')
            self.packetMean['timeString']=timeString
            self.nMeasurements['timeString']=1
            # reset packet list
            for key in self.packets.keys():
                self.packets[key]=[]


        return (releasePacket,releaseTime)


    def updateMeasurement(self, key, value):

        if key not in self.lastUpdate.keys():
            self.lastUpdate[key]=datetime.datetime.now()

        self.lastUpdate[key]=datetime.datetime.now()
        if key not in self.packets.keys():
            self.packets[key]=value
        self.packets[key]=np.append(self.packets[key],value)
        self.packetMean[key] = np.mean(self.packets[key])
        self.nMeasurements[key] = len(self.packets[key])


#
db=stationDB.DB('data.sdb','master')
s=Station()
s.udpPort=9990
s.startUDPListen('0.0.0.0',s.udpPort)

sio=SocketIO('localhost', 5000)
verbose=False
readPackets=True
while readPackets:
    rawPacket=s.recvPacket(verbose)
    print rawPacket
    try:
        sio.emit('dataPacket', rawPacket)
        print 'sent packet'
    except:
        print 'socket not connected'

    # rawPacket['name']

    for dataName in rawPacket['data'].keys():
        key = rawPacket['name'].replace(' ','') + dataName.replace(' ','')
        value = rawPacket['data'][dataName]['value']
        releasePacket=s.updateMeasurement(key,value)
    releasePacket,releaseTime=s.checkForPacketRelease()
    if releasePacket:
        for (key,value) in s.packetMean.items():
            print 'Updating db: ',key,' = ',value
        db.addData(s.packetMean)
        s.packetMean={}


s.sock.close()
