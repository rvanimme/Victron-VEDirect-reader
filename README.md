# Victron-VEDirect-reader
Yet another reader for the Victron VE.Direct serial protocol for Linux based systems (Ubuntu/RasPi)

## Introduction
I tried to read the VE.Direct protocol from my Victron devices with the use of cheap TTL serial to USB convertors (e.g the ones based on CH230G). I noticed a substantial amount of bit errors in the serial communication. I tried some of the availabe VE.Direct reader programs but I noticed that the VE.Direct checksum protocol is very weak and that plenty of errors were not detected (e.g. it will not detect 2 MSB bit errors in a block). I'm pretty sure that the official Victron converters work much better but they are about 6x more expensive. 

This implementation of the Victron VE.Direct reader takes care "standard features" like the configuration of the serial port, the removal of the hex protocol characters and the checksum verification of the received blocks. **The extra feature is that it checks the syntax of each line**. I added a whole bunch of checks to discover any corruption in the message and when it does, it will discard that packet.


