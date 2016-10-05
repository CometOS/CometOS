Port of the TinyOS rfxlink layers and the ATmega128RFA1 radio driver to 
the cometos mac_interface. Due to the port, the flexibility of the 
rfxlink layers is gone, because we directly hacked the nesc implementation
into .cc files using global functions. 

NesC wiring was hardcoded into the layers (by renaming functions),
so that removal or addition of new layers is only possible by changing code. 
Also, we worked around tasklet fanout by renaming the corresponding 
tasklet_run functions of each user. 

Same is true for the RadioAlarm, which was realized by providing the caller's
ID at the call of the wait function and renaming the fired() functions. 

There are two different backoff layers, TosRandomCollisionLayer, which uses
the original TinyOS backoff procedure -- and 802154CsmaLayer, which implements 
the 802.15.4 unslotted backoff procedure. 

TosRandomCollision additionally uses some mechanism, which checks if a frame 
is received and uses the STATE_BARRIER to denote this -- the overall idea 
behind it, however, is not completely clear.