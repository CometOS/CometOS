set breakpoint pending on
enable pretty printing
set python print-stack none
set print object on
set print sevenbit-strings on
set charset ISO-8859-1
set mem inaccessible-by-default off
set auto-solib-add on
target remote localhost:2331
monitor speed 1000
monitor clrbp
monitor reset
monitor halt
monitor regs
flushreg
monitor speed auto
monitor flash breakpoints 1
monitor semihosting enable
monitor semihosting IOClient 1
symbol-file build/hw/Device.hex
load build/hw/Device.hex
monitor clrbp
monitor reset
monitor regs
monitor halt
flushreg
monitor go
quit
