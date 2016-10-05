import sys
from PyQt4 import QtGui, uic, QtCore 
from datetime import datetime
import threading
import os
from Queue import Queue


# Load UI ---------------------------------------------------------------------
app = QtGui.QApplication(sys.argv)

gui = uic.loadUi(os.path.dirname(__file__)+"/PacketAnalyzer2.ui")

MAX_ITEMS=50
ITEM_BASE_HEIGHT=25
ITEM_ADD_HEIGHT=15

counter = 0
channelCounter = {}
channelColor = {}
lastTimestamp = datetime.now()
marker = ""
filt = ""
pktQueue = Queue() 

bgColor = ["#ddffff", "#ffddff", "#ffffdd", "#ffeeee", "#eeeeff"]

pktList = []


class PacketView (QtGui.QTextBrowser):
	def __init__(self, header, ch, pkt):
		QtGui.QTextBrowser.__init__(self)
	    
		self.ch = ch
		self.header = header
		self.pkt = pkt
		self.setFont(QtGui.QFont('Monospace', 9))
		self.setColor(bgColor[channelColor[ch]])
		self.updateMarker()
		self.updateText()

		
	def setColor(self, color):
		self.setStyleSheet("QWidget { background-color: %s }" % color)
	
	def updateVisibility(self):
		global filt 
		pkt = self.pkt
		ch = self.ch
		hide=False
		try:
			if (len(filt) > 0):
				hide = eval(str(filt))
			
		except Exception: 
			print "Error: Evaluating of filter failed"
			hide = False
		self.setVisible(not hide)
		
		
	def updateMarker(self):
		global marker
		pkt = self.pkt
		ch = self.ch
		self.markText = [False] * len(pkt)
		for i in range(0, len(pkt)):
			try:
				if (len(marker) > 0):
					self.markText[i] = eval(str(marker))
			except Exception: 
				print "Error: Evaluating of marker failed"
				self.markText[i] = False
					
	
	def updateText(self):
		global ITEM_BASE_HEIGHT,ITEM_ADD_HEIGHT
		self.clear()
		pkt = self.pkt
		ch = self.ch
		
		length = ITEM_BASE_HEIGHT
		self.setTextColor(QtCore.Qt.blue)
		self.insertPlainText(self.header)
		
		printHex = gui.checkHex.isChecked()
		printDec = gui.checkDec.isChecked()
		printAscii = gui.checkAscii.isChecked()
		
					
		if (gui.checkHex.isChecked()):
			length = length + ITEM_ADD_HEIGHT
			self.insertPlainText("\n");
			
			for i in range(0, len(pkt)):
				if (self.markText[i]):
					self.setTextColor(QtCore.Qt.magenta)
				else:
					self.setTextColor(QtCore.Qt.black)

				self.insertPlainText('x{:02X} '.format(pkt[i]))

		if (gui.checkDec.isChecked()):
			length = length + ITEM_ADD_HEIGHT
			self.insertPlainText("\n");
			
			for i in range(0, len(pkt)):
				if (self.markText[i]):
					self.setTextColor(QtCore.Qt.magenta)
				else:
					self.setTextColor(QtCore.Qt.black)

				self.insertPlainText('{:03d} '.format(pkt[i]))

		if (gui.checkAscii.isChecked()):
			length = length + ITEM_ADD_HEIGHT
			self.insertPlainText("\n");
			
			for i in range(0, len(pkt)):
				if (self.markText[i]):
					self.setTextColor(QtCore.Qt.magenta)
				else:
					self.setTextColor(QtCore.Qt.black)

				if (pkt[i] >= 33 and pkt[i] <= 127):
					self.insertPlainText(" "+chr(pkt[i]) + "  ")
				else:
					self.insertPlainText(" .  ")
		
  


		self.setMaximumHeight(length)
		
		
# Define Processing Functions -------------------------------------------------

def checkValueChanged(value):
	global pktList
	for p in pktList:
		p.updateText()


def buttonClearClicked(e):
	global counter, channelCounter, channelIndentation, pktList
	
	while len(pktList)>0:
		pktList.pop(0).setVisible(False)
		gui.framePktsContents.layout().takeAt(0)	
	

	counter=0
	channelCounter={}
	gui.counter.setText(str(counter))

def editFilterFinished():
	global filt
	filt = gui.editFilter.displayText()
	for p in pktList:
		p.updateVisibility()
	

def editMarkerFinished():
	global marker
	marker = gui.editMarker.displayText()
	for p in pktList:
		p.updateMarker()
		p.updateText()

class PacketObject():
    def __init__(self, ch, pkt):
        self.ch = ch
        self.pkt = pkt

#packetsignal = QtCore.pyqtSignal(PacketObject)
   

def _recvPacket():
	while (pktQueue.empty() == False):
		_processPacket(pktQueue.get())

def insertIndentation(ch):
	global channelIndentation
	for i in range(channelIndentation[ch]):
		gui.text.insertPlainText("        ")
		
	
def _processPacket(obj):
	global counter, channelCounter,lastTimestamp, marker, filt

	pkt = obj.pkt
	ch = obj.ch

	if (channelCounter.has_key(ch) == False):
		channelCounter[ch] = 0
	channelCounter[ch] = channelCounter[ch] + 1

	if (channelColor.has_key(ch) == False):
		channelColor[ch] = len(channelColor) % len(bgColor)

	curTimestamp = datetime.now();
	interval = curTimestamp - lastTimestamp;
	lastTimestamp = curTimestamp
	header="ch " + ch + " | len " + str(len(pkt)) + " | cnt " + str(channelCounter[ch]) + " | " + str(curTimestamp) + " | " + str(interval)

	counter = counter + 1
	gui.counter.setText(str(counter))

		
	p = PacketView(header, ch, pkt)	
	pktList.insert(0, p);
	gui.framePktsContents.layout().insertWidget(0, p)
	if (len(pktList)==MAX_ITEMS+1):
		pktList.pop(MAX_ITEMS)
		gui.framePktsContents.layout().takeAt(MAX_ITEMS)
	p.updateVisibility()		


	
class PacketAnalyzer(QtCore.QObject):
    # Define a new signal called 'trigger' that has no arguments.
    trigger = QtCore.pyqtSignal()

ana = PacketAnalyzer()
ana.trigger.connect(_recvPacket)
        
def recvPacket(ch, pkt):
	
	pktQueue.put(PacketObject(ch, pkt))
	ana.trigger.emit()
	pass
	


# Connect Signals & Slots -----------------------------------------------------
gui.buttonClear.clicked.connect(buttonClearClicked)

gui.checkHex.stateChanged.connect(checkValueChanged)
gui.checkDec.stateChanged.connect(checkValueChanged)
gui.checkAscii.stateChanged.connect(checkValueChanged)

gui.editFilter.editingFinished.connect(editFilterFinished)
gui.editMarker.editingFinished.connect(editMarkerFinished)

#packetsignal.connect(_recvPacket)


def run():
	gui.show()
	app.exec_()


