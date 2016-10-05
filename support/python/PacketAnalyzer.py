import sys
from PyQt4 import QtGui, uic, QtCore 
from datetime import datetime
import threading
from Queue import Queue

# Load UI ---------------------------------------------------------------------
app = QtGui.QApplication(sys.argv)
gui = uic.loadUi("PacketAnalyzer.ui")

counter=0
channelCounter={}
channelIndentation={}
lastTimestamp=datetime.now()
marker=""
filt=""
pktQueue=Queue() 




# Define Processing Functions -------------------------------------------------


def buttonClearClicked(e):
	global counter, channelCounter,channelIndentation
	gui.text.clear()
	counter=0
	channelCounter={}
	channelIndentation={}
	gui.counter.setText(str(counter))

def editFilterFinished():
	global filt
	filt=gui.editFilter.displayText()
	

def editMarkerFinished():
	global marker
	marker=gui.editMarker.displayText()

class PacketObject():
    def __init__(self, ch, pkt):
        self.ch = ch
        self.pkt= pkt

#packetsignal = QtCore.pyqtSignal(PacketObject)
   

def _recvPacket():
	while (pktQueue.empty()==False):
		_processPacket(pktQueue.get())

def insertIndentation(ch):
	global channelIndentation
	for i in range(channelIndentation[ch]):
		gui.text.insertPlainText("        ")
		
	
def _processPacket(obj):
	global counter,channelCounter,channelIndentation,lastTimestamp, marker, filt
	pkt=obj.pkt
	ch=obj.ch
	gui.text.moveCursor(QtGui.QTextCursor.Start)


	indent=gui.checkBoxIndentation.isChecked() 


	# update global and channel counter
	counter=counter+1
	gui.counter.setText(str(counter))

	if (channelCounter.has_key(ch)==False):
		channelCounter[ch]=0
		channelIndentation[ch]=len(channelIndentation)

	channelCounter[ch]=channelCounter[ch]+1

	#write status line 
	curTimestamp=datetime.now();
	interval=curTimestamp-lastTimestamp;
	lastTimestamp=curTimestamp

	toFilter=False

	# apply filter
	try:
		if (len(filt) >0):
			toFilter=eval(str(filt))
			if (gui.checkBoxFilter.isChecked() and toFilter):
				return
	except Exception: 
		print "Error: Evaluating of filter failed"
		toFilter=False

	gui.text.setTextColor(QtCore.Qt.black)

	if (toFilter==True):
		gui.text.setTextColor(QtCore.Qt.gray)
	
	if (indent):
		insertIndentation(ch)
	gui.text.insertPlainText("ch "+ch+" | len "+str(len(pkt))+" | cnt "+str(channelCounter[ch])+" | "+str(curTimestamp)+" | "+str(interval)+"\n")

	if (indent):
		insertIndentation(ch)
	
	# print data as hex
	for i in range(0,len(pkt)):
		markText=False
		
		try:
			if (len(marker) >0):
				markText=eval(str(marker))
		except Exception: 
			print "Error: Evaluating of marker failed"
			markText=False
			

		if (markText):
			gui.text.setTextColor(QtCore.Qt.magenta)
		else:
			gui.text.setTextColor(QtCore.Qt.blue)
	
		if (toFilter==True):
			gui.text.setTextColor(QtCore.Qt.gray)
		
		gui.text.insertPlainText('{:02X} '.format(pkt[i]))

	gui.text.insertPlainText("\n");

	
	
	# print data as string
	gui.text.setTextColor(QtCore.Qt.darkGray)
	if (toFilter==True):
		gui.text.setTextColor(QtCore.Qt.gray)
		
	if (indent):
		insertIndentation(ch)
		
	for val in pkt:
		if (val>=33 and val<=127):
			gui.text.insertPlainText(chr(val)+"  ")
		else:
			gui.text.insertPlainText(".  ")
	
	gui.text.insertPlainText("\n");


	gui.text.insertPlainText("\n");

	
class PacketAnalyzer(QtCore.QObject):
    # Define a new signal called 'trigger' that has no arguments.
    trigger = QtCore.pyqtSignal()

ana=PacketAnalyzer()
ana.trigger.connect(_recvPacket)
        
def recvPacket(ch, pkt):
	
	pktQueue.put(PacketObject(ch,pkt))
	ana.trigger.emit()
	pass
	


# Connect Signals & Slots -----------------------------------------------------
gui.buttonClear.clicked.connect(buttonClearClicked)
gui.editFilter.editingFinished.connect(editFilterFinished)
gui.editMarker.editingFinished.connect(editMarkerFinished)
#packetsignal.connect(_recvPacket)

def run():
	gui.show()
	app.exec_()



