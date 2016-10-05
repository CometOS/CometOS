import subprocess
import socket

s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
s.bind(('', 12056))
s.listen(1)

while True:
    conn, addr = s.accept()
    print 'Connected by', addr
    while 1:
        data = conn.recv(1)
        if (len(data) == 0):
            break
        print "Rcvd: " + str(data)
        if data == "0" or data == "1":
            subprocess.call(["gpio", "-g", "write", "17", data])
    conn.close()