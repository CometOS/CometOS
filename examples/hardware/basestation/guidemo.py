
from Tkinter import *
from threading import Thread
import time;



def runGui():
	root = Tk()
	root.title('Simple Plot - Version 3 - Smoothed')
	root.mainloop()


guiThr = Thread(target=runGui)
guiThr.start()


print "Do something"



def stop():
	guiThr.join()
	quit();