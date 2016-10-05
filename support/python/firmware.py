__docformat__ = "javadoc"


def loadFirmwareFromHex(filename):
    """Loads firmware image from hex file.
    @param    filename    valid firmware in intel hex format
    @return    tuple (start address, list of segments, crc for firmware)
    """
    import array
    import crc16
    from intelhex import IntelHex

    SEGMENT_SIZE=256
    CRC_START=0xFFFF
    PADDING_BYTE=0xFF

    ih = IntelHex(filename)

    startaddr=ih.minaddr()

    bin=ih.tobinarray()

    segcount=(len(bin)-1)/SEGMENT_SIZE+1
    segs=[]

    # create packets
    for i in xrange(segcount):
        # fill array with default value if necessary
        b=bin[(i*SEGMENT_SIZE):((i+1)*SEGMENT_SIZE)]
        segs.append(b)

    # use padding for last segment (a generator expression is used)
    b.extend((PADDING_BYTE for x in xrange(len(b),SEGMENT_SIZE)))
    #crcfile = open("crcs.dat", 'w')
    # create CRC16 for 
    crc=CRC_START
    #k = 0
    for s in segs:
        crc = crc16.crc16xmodem(s.tostring(),crc)
        #crcfile.write(str(k) + " " + str(crc) + "\n")
        #k += 1
        
    return (startaddr,segs,crc)





