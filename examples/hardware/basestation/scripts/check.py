for i in range(30):
    print "Memory Utilization: " + str(node[args[0]].sys.util().mem)
    time.sleep(0.2)
    print "create new Airframe..." + str(node[args[0]].sys.create())
    time.sleep(0.2)
