FW = "../examples/mac/devboard/Device.hex"

#ex.otap(node.keys(), FW, 0)
for k in range(500):
    for n in nodes:
        print "----- Node " + hex(n.id) + " ----------------"
        print (n.sys.led(2))
        print(n.sys.ar())
        u = n.sys.util()
        if u != None:
            print(str(u.mem))
        print (str(n.sys.ping()))
        print(n.sys.ac())