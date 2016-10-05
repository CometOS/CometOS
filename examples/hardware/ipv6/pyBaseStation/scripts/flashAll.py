assert len(args) == 2

image=args[0]
slot=args[1]

ex.otap(True, nodes, image, slot, 65, remote, otap, False, 15, "isv")