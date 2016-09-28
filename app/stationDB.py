import sqlite3
import pandas as pd
import stationDB as st
from jinja2 import Template

from bokeh.plotting import *
from bokeh.resources import INLINE
# from bokeh.util.browser import view

from bokeh.models import HoverTool
from bokeh.embed import components
import time
import datetime
import os.path



class DB:
	def __init__(self,fileName, table):
		self.fileName=fileName
		self.table=table
		self.columnNames=[]
		if not os.path.isfile(self.fileName):
			self.createTable()
		self.getColumnNames()

	def createTable(self):
		con=sqlite3.connect(self.fileName)
		c=con.cursor()
		c.execute("CREATE TABLE %s (timestamp real, timeString text)" % self.table)

		con.close()

	def addColumn(self, name):
		con=sqlite3.connect(self.fileName)
		c=con.cursor()
		type="real"
		c.execute("ALTER TABLE %s ADD COLUMN %s %s" % (self.table, name, type))
		con.close()

	def getColumnNames(self):
		con=sqlite3.connect(self.fileName)
		command="SELECT * from %s" % self.table
		self.df=pd.read_sql_query(command, con)
		self.columnNames=self.df.columns
		con.close()

	def addData(self, packet):
		con=sqlite3.connect(self.fileName)
		c=con.cursor()

		# create column if not present
		for key in packet.keys():
			if key not in self.columnNames:
				self.addColumn(key)
				self.getColumnNames()


		columns=', '.join(packet.keys())
		placeholders = ', '.join('?' * len(packet))
		sql = 'INSERT INTO master ({}) VALUES ({})'.format(columns, placeholders)
		c.execute(sql, packet.values())

		# # command="create table if not exists %s (date real, dateString text, name text, value real)" % name
		# c.execute(command)
		# timeString=datetime.datetime.fromtimestamp(time).strftime('%Y/%m/%d %H:%M:%S')
		# dataTuple=(time, timeString, name, data,)
		# command="INSERT INTO %s VALUES (?, ?, ?, ?)" % name
		# c.execute(command, dataTuple)
		con.commit()
		con.close()

	def loadDB(self,name):
		con=sqlite3.connect(self.fileName)
		command="SELECT * from %s" % name
		self.df=pd.read_sql_query(command, con)
		con.close()

class DBview:
	def __init__(self,db,df,UTCOffset):
		# self.timeRange=timeRange
		self.db = db
		self.df = df
		self.UTCOffset = UTCOffset


	def qp(self,attrList, name, timeRange):
		now=datetime.datetime.now()
		epoch=datetime.datetime.utcfromtimestamp(0)
		tstart=now-datetime.timedelta(days=timeRange)
		tcut=(tstart-epoch).total_seconds()
		dataFrame=self.df[self.df.timestamp>tcut]
		dataFrame['t']=dataFrame.timestamp*1000
		# y1=getattr(self.df,attr1)
		# y2=getattr(self.df,attr2)
		output_file(name+'.html')
		timeString=[datetime.datetime.fromtimestamp(dt).strftime('%Y/%m/%d %H:%M:%S') for dt in dataFrame.timestamp]
		source=ColumnDataSource(data=dataFrame.to_dict('list'))
		TOOLS="resize,hover,crosshair,pan,wheel_zoom,box_zoom,reset,tap,previewsave,box_select,poly_select,lasso_select"
		p=figure(x_axis_type="datetime",tools=TOOLS)
		for key in attrList:
			print key
			p.scatter('t',key, source=source)
		p.select(dict(type=HoverTool)).tooltips=[
					("Time", "@timeString"),
					("Value", "@y1"),
					("Value", "@y2"),
			]
		show(p)

	def qph(self,attrList, name, timeRange):
		now=datetime.datetime.now()
		epoch=datetime.datetime.utcfromtimestamp(0)
		tstart=now-datetime.timedelta(days=0,hours=timeRange)
		tcut=(tstart-epoch).total_seconds()
		dataFrame=self.df[self.df.timestamp>tcut]
		dataFrame['t']=dataFrame.timestamp*1000
		# y1=getattr(self.df,attr1)
		# y2=getattr(self.df,attr2)
		output_file(name+'.html')
		timeString=[datetime.datetime.fromtimestamp(dt).strftime('%Y/%m/%d %H:%M:%S') for dt in dataFrame.timestamp]
		source=ColumnDataSource(data=dataFrame.to_dict('list'))
		TOOLS="resize,hover,crosshair,pan,wheel_zoom,box_zoom,reset,tap,previewsave,box_select,poly_select,lasso_select"
		p=figure(x_axis_type="datetime",tools=TOOLS)
		for key in attrList:
			print key
			p.scatter('t',key, source=source)
		p.select(dict(type=HoverTool)).tooltips=[
					("Time", "@timeString"),
					("Value", "@y1"),
					("Value", "@y2"),
			]
		show(p)

		return source


	def qphLive(self,attrList, name, timeRange):
		# attrList=['stationLoadVolt']
		now=datetime.datetime.now()
		epoch=datetime.datetime.utcfromtimestamp(0)
		tstart=now-datetime.timedelta(days=0,hours=timeRange-self.UTCOffset)
		tcut=(tstart-epoch).total_seconds()
		dataFrame=self.df[self.df.timestamp>tcut]
		dataFrame['t']=dataFrame.timestamp*1000 - self.UTCOffset*3600*1000
		output_server(name,url='http://10.0.1.2:5006')
		colors=["red","blue","green","orange","purple","black","gray","magenta","cyan","brown","gold","darkkhaki","darksalmon"]
		timeString=[datetime.datetime.fromtimestamp(dt).strftime('%Y/%m/%d %H:%M:%S') for dt in dataFrame.timestamp]
		source=ColumnDataSource(data=dataFrame.to_dict('list'))
		TOOLS="resize,hover,crosshair,pan,wheel_zoom,box_zoom,reset,tap,previewsave,box_select,poly_select,lasso_select"
		keyList=attrList.keys()
		p={}
		ds={}
		for mainKey in keyList:
			if mainKey == keyList[0]:
				p[mainKey]=figure(x_axis_type="datetime",tools=TOOLS, width=600, height=400, title=mainKey)
			else:
				p[mainKey]=figure(x_axis_type="datetime",tools=TOOLS, width=600, height=400, title=mainKey, x_range=p[keyList[0]].x_range)
			ikey=0
			hover={}
			for key in attrList[mainKey]:
				print key
				# keySource=ColumnDataSource({'x': source.data['t'], 'y': series.values, 'series_name': name_for_display, 'Date': toy_df.index.format()})
				p[mainKey].scatter('t',key, source=source, name=key,fill_color=colors[ikey],line_color=colors[ikey], legend=key)

				hover = p[mainKey].select(dict(type=HoverTool))
				# hover[ikey].renderers=[source.data[key]]
				# hover[ikey].tooltips=tooltips+[("Series",key),("Time","@timeString"), ("Value", "@"+key)]
				hover.tooltips=[("Series",key),("Time","@timeString"), ("Value", "@"+key)]
				# hover.mode = "mouse"
				ikey+=1
			p[mainKey].legend.orientation="top_left"
			renderer = p[mainKey].select(dict(name=key))
			ds[mainKey]=renderer[0].data_source


		# allP = vplot(*p.values())
		# allP = gridplot([p.values()])
		group=lambda flat, size: [flat[i:i+size] for i in range(0,len(flat), size)]
		allP = gridplot(group(p.values(),1))
		show(allP)


		while True:
			print 'updating...'
			self.db=st.DB('data.sdb','master')
			self.db.loadDB('master')
			self.df = self.db.df
			now=datetime.datetime.now()
			tstart=now-datetime.timedelta(days=0,hours=timeRange-self.UTCOffset)
			tcut=(tstart-epoch).total_seconds()
			dataFrame=self.df[self.df.timestamp>tcut]
			dataFrame['t']=dataFrame.timestamp*1000 - self.UTCOffset*3600*1000
			for mainKey in keyList:
				ds[mainKey].data =  dataFrame.to_dict('list')
				# print ds.data['stationIRTemp']
				cursession().store_objects(ds[mainKey])
			time.sleep(30)
