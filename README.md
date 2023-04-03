# Victron-VE.Direct-reader
Yet another reader for the Victron VE.Direct serial protocol for Linux based systems (Ubuntu/RasPi)

## Introduction
I tried to read the VE.Direct protocol from my Victron devices with the use of cheap TTL serial to USB convertors (e.g the ones based on CH341). Most of the Victron devices work without any problems but on some of these devices, I got quite a bit of errors. Most errors are detected by the VE.Direct checksum protocol but once in a while, garbage appears on the output. The VE.Direct checksum protocol is very weak, e.g. it will not detect 2 MSB bit errors in a block. I'm pretty sure that the official Victron converters work much better but they are about 6x more expensive. Maybe I can fix it with a better (shielded) cables or reroute the cable (e.g. to avoid my noisy inverter) but I went for the "cheap" fix, I tried to fix this issue with an extra layer of validation, a format/grammar checker.

This implementation of the Victron VE.Direct reader takes care of the "standard features" like the configuration of the serial port, the removal of the hex protocol characters and the checksum verification of the received blocks. **The major extra feature is that it checks the format/grammar of each line**. The other feature is the ability to filter on field names (white list).

## Grammar
The VE.Direct grammer is based on observations of a limited number of VE.Direct devices.
1) SmartShunt 500A/50mV
2) SmartSolar MPPT 150/85 rev2
3) SmartSolar MPPT 150|70 rev2
4) SmartSolar MPPT 100|50
5) BMV-712 Smart - Battery monitor (BMV)

This grammar is by no means complete and I do expect some devices not to work with it

```
VE.Direct BNF Grammar:

<ve_direct_line> ::= <capitalized-name> <TAB> <number-value> <CRLF>
                  | <capitalized-name> <TAB> <hex-value> <CRLF>
                  | <capitalized-name> <TAB> "ON" <CRLF>
                  | <capitalized-name> <TAB> "OFF" <CRLF>
                  | <capitalized-name> <TAB> "---" <CRLF>
                  | "BMV" <TAB> <capitalized-string-value> <CRLF>
                  | "SER#" <TAB> <capitalized-string-value> <CRLF>
                  | "FWE" <TAB> <capitalized-string-value> <CRLF>
                  
<capitalized-name> ::= <uppercase-letter> <capitalized-name-rest>*
<capitalized-name-rest> ::= <uppercase-letter>
                         | <lowercase-letter>
                         | <digit>
                         
<capitalized-string-value> ::= <uppercase-letter> <capitalized-string-value-rest>*
                            | <digit> <capitalized-string-value-rest>*
<capitalized-string-value-rest> ::= <uppercase-letter>
                                 | <lowercase-letter>
                                 | <digit>
                                 | "/"
                                 | " "

<TAB> ::= "\t"
<CRLF> ::= "\r\n"  

<number-value> ::= <signed-integer>
<signed-integer> ::= <digit>+
                  | "-" <digit>+

<hex-value> ::= "0x" <hex-digit>+

<uppercase-letter> ::= "A" | "B" | "C" | ... | "Z"
<lowercase-letter> ::= "a" | "b" | "c" | ... | "z"
<digit> ::= "0" | "1" | "2" | ... | "9"
<hex-digit> ::= <digit> | "A" | "B" | "C" | "D" | "E" | "F"
```
## Build
From a terminal enter `./build`. This build take about 1 minute on a Raspberry Pi Zero W. On a Ubuntu PC, just a couple of seconds.
After the build you will find 2 executables in the current folder: `vicread` and `ttyusb2dev`

## vicread
This program reads data from a Victron device using the VE.Direct protocol. It removes any HEX-messages from the received data before processing it. Next it discards any blocks that fails the checksum or doesn't meet the expected format/grammar. Last, it filters based on a white list of field names provided on the command-line. The filtered VE.Direct lines are sent to **stdout**. All error and information messages are sent to **stderr**. 

The program uses Linux system calls to open the serial device in read-only mode and lock it for exclusive access. It also sets the serial port configuration according to the VE.Direct protocol. Regular experssions are used to check the format/grammer. The program runs in an infinite loop until terminated manually.

## ttyusb2dev
This program helps to find the full path of a serial USB device by either its name or physical address. If no argument is provided, it prints out a list of all available serial USB devices. The program uses the C++ standard library, Linux header files, and regular expressions. The full path device name is sent to **stdout***. All error and informational messages are send to **stderr**. 

The device names like /dev/ttyUSB0 are not "stable". Every reboot can result in a different mapping on the physical USB device


