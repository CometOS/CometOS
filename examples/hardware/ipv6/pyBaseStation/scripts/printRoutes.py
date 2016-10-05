for n in nodes:
    print "----- Node " + hex(n.id) + " ----------------"
    num = n.sr.gnr()
    if num != None:
        print "Found " + str(num) + " routing entries:"
        for i in range(num):
            r = n.sr.gr(i)
            if r != None:
                printRouteInfo(r)
        
