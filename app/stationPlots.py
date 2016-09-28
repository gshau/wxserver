import stationDB as st
import numpy as np
import sqlite3
import pandas as pd
from bokeh.plotting import *
from bokeh.models import HoverTool

import time
import datetime
import pytz

db=st.DB('data.sdb','master')
db.loadDB('master')
UTCOffset = -5
dbv=st.DBview(db, db.df, UTCOffset)

attrList={}
attrList['Station Temperature']=['WeatherStationTemperature','WeatherStationHTUTemp','WeatherStationDewpoint']
attrList['Station IR']=['WeatherStationIRTemp','WeatherStationMLXTemp']
attrList['Brightness']=['WeatherStationBrightness']
attrList['Voltage']=['WeatherStationLoad']
attrList['Current']=['WeatherStationCurrent']
attrList['UV']=['WeatherStationUVIndex']

viewHistory=24
dbv.qphLive(attrList, 'DayView', viewHistory)
