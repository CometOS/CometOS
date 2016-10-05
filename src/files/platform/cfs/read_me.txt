In this folder, following changes are done

Added following files

Makefile.default
cfs-coffee-arch.h
cfs.h
eeprom.h
cfs-coffee.c
cfs-eeprom.c
cfs-coffee.h
cfs-coffee-arch.c



Makefile.default is  added to port the coffee files into the CometOS. 
cfs-coffee.c and cfs-coffee.h are the file for the flash Coffee file system 
cfs-eeprom.c and eeprom.h are for the EEPROM Coffee file system
EEPROM file system is not enable and included in the CometOS.To enable the EEPROM file system, additional interface layer has to develop.