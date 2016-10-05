package cometos.src.communication.ieee802154.mac;


// similar to 802.15.4 mac layer
simple MacInterfTest extends MacAbstractionLayer
{
    parameters:
        @class( MacInterfTest);
        int max_pkt=default(10);
		int max_nodes=default(100);
		int payload=default(50);


}



