# Victron-VEDirect-reader
Yet another reader for the Victron VE.Direct serial protocol for Linux based systems (Ubuntu/RasPi)

## Introduction
I tried to read the VE.Direct protocol from my Victron devices with the use of cheap TTL serial to USB convertors (e.g the ones based on CH341). Most of the Victron devices work without any problems but on some of these devices, I got quite a bit of errors. Most errors are detected by the VE.Direct checksum protocol but once in a while, garbage appears in the output. The VE.Direct checksum protocol is very weak, e.g. it will not detect 2 MSB bit errors in a block. I'm pretty sure that the official Victron converters work much better but they are about 6x more expensive. Maybe I can fix it with a better (shielded) cables or reroute the cable (e.g. to avoid my noisy inverter) but I went for the "cheap" fix, I tried to fix this issue with an extra layer of validation, a format/grammar checker.

This implementation of the Victron VE.Direct reader takes care "standard features" like the configuration of the serial port, the removal of the hex protocol characters and the checksum verification of the received blocks. **The extra feature is that it checks the format/grammar of each line**. 

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
