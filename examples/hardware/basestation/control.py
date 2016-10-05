from Tkinter import *
from threading import Thread

#currently under develop 

class LedControlGui:
    def __init__(self, master, cmds):
        frame = Frame(master)
        frame.pack()
        Button(frame, text="   red   ", bg="red", command=cmds['red']).pack(side=LEFT)
        Button(frame, text=" yellow ", bg="yellow", command=cmds['yellow']).pack(side=LEFT)
        Button(frame, text=" green ", bg="green", command=cmds['green']).pack(side=LEFT)



def say_hi():
    print "hi there, everyone!"
def say_ho():
    print "ho there, everyone!"

def runLedGui(id):
    root = Tk()
    root.wm_title("345: LedControl")
    app = LedControlGui(root,{'red':say_hi,'yellow':say_hi,'green':say_ho} )
    root.mainloop()
    



threadList=[]    
t=Thread(target=runLedGui, args=(3,))
t.start()
threadList.append(t)

def stop():
    for x in threadList:
        x.join()
        
    quit()