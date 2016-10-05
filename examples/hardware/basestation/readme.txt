This readme is outdated (2014-06-17)

PYTHON BUILD IS CURRENTLY IN TEST STATE

Only serialization interface is currently ported to Python!


prerequisites:
	python 2.x
	pySerial (for serial connection if using hardware)
	
	
use "python $(PYTHON_SETUP) build_ext -i -c mingw32" for windows and mingw



BUGS:
	Sometime the basestation crashes. A comm issue can be that an illegal 
	serialization, desrialization attempt is done. 
	Furthermore we have problem when receiving and writing data 
	at the same time (packet loss). We should implement
	the synchronous serial communication protocol for this reason. 
	
	A further performance problem is the polling of the SerialComm
	class for new packets. Since scheduler is thread-safe, a
	direct write would be better.
	
	
	Crash of basestation can also be caused by deadlock!!!!
	
	