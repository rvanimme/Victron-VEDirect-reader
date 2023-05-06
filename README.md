# Victron-VE.Direct-reader
Yet another reader for the Victron VE.Direct serial protocol for Linux-based systems (Ubuntu/RasPi). The major extra feature is that this reader also checks the format/grammar of each line. This will significantly reduce undetected checksum errors.

## Introduction
I tried to read the VE.Direct protocol from my Victron devices with the use of cheap TTL serial to USB converters (e.g. the ones based on CH341). Most of the Victron devices work without any problems, but on some of these devices, I got quite a few errors. Most errors are detected by the VE.Direct checksum protocol, but once in a while, garbage appears on the output. The VE.Direct checksum protocol is very weak, e.g., it will not detect 2 MSB bit errors in a block. I'm pretty sure that the official Victron converters work much better, but they are about 6x more expensive. Maybe I can fix it with better (shielded) cables or reroute the cable (e.g. to avoid my noisy inverter), but I went for the "cheap" fix. I tried to fix this issue with an extra layer of validation, a format/grammar checker.

This implementation of the Victron VE.Direct reader takes care of the "standard features" like the configuration of the serial port, the removal of the hex protocol characters, and the checksum verification of the received blocks. **The major extra feature is that this reader checks the format/grammar of each line. The other feature is the ability to filter on field names (white list)**.


Format errors really happen, not that frequent but enough to mess up the data collection. **This reader will significantly reduce undetected checksum errors**
```
[2023-04-16T14:02:34-0700] ERROR checksum, block discarded. Received bytes: 209331851, total blocks: 1351348, valid blocks: 1322040 (97.83%), checksum errors: 29223 (2.16%), format errors: 85 (0.01%),      
[2023-04-16T14:04:16-0700] ERROR checksum, block discarded. Received bytes: 209362624, total blocks: 1351552, valid blocks: 1322243 (97.83%), checksum errors: 29224 (2.16%), format errors: 85 (0.01%),      
[2023-04-16T14:04:26-0700] ERROR checksum, block discarded. Received bytes: 209365630, total blocks: 1351572, valid blocks: 1322262 (97.83%), checksum errors: 29225 (2.16%), format errors: 85 (0.01%),      
[2023-04-16T14:04:36-0700] ERROR checksum, block discarded. Received bytes: 209368657, total blocks: 1351592, valid blocks: 1322281 (97.83%), checksum errors: 29226 (2.16%), format errors: 85 (0.01%),      
[2023-04-16T14:05:15-0700] ERROR checksum, block discarded. Received bytes: 209380311, total blocks: 1351669, valid blocks: 1322357 (97.83%), checksum errors: 29227 (2.16%), format errors: 85 (0.01%),      
[2023-04-16T14:05:51-0700] ERROR checksum, block discarded. Received bytes: 209391210, total blocks: 1351741, valid blocks: 1322428 (97.83%), checksum errors: 29228 (2.16%), format errors: 85 (0.01%),      
[2023-04-16T14:06:13-0700] ERROR checksum, block discarded. Received bytes: 209398015, total blocks: 1351786, valid blocks: 1322472 (97.83%), checksum errors: 29229 (2.16%), format errors: 85 (0.01%),      
[2023-04-16T14:06:43-0700] ERROR format, line "BMV      712+5", block discarded. Received bytes: 209407066, total blocks: 1351846, valid blocks: 1322531 (97.83%), checksum errors: 29229 (2.16%), format errors: 86 (0.01%),                                    
[2023-04-16T14:08:17-0700] ERROR checksum, block discarded. Received bytes: 209435250, total blocks: 1352033, valid blocks: 1322717 (97.83%), checksum errors: 29230 (2.16%), format errors: 86 (0.01%),      
[2023-04-16T14:08:27-0700] ERROR checksum, block discarded. Received bytes: 209438251, total blocks: 1352053, valid blocks: 1322736 (97.83%), checksum errors: 29231 (2.16%), format errors: 86 (0.01%),      
[2023-04-16T14:08:48-0700] ERROR checksum, block discarded. Received bytes: 209444722, total blocks: 1352096, valid blocks: 1322778 (97.83%), checksum errors: 29232 (2.16%), format errors: 86 (0.01%),      
[2023-04-16T14:09:19-0700] ERROR checksum, block discarded. Received bytes: 209454109, total blocks: 1352158, valid blocks: 1322839 (97.83%), checksum errors: 29233 (2.16%), format errors: 86 (0.01%),
```

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
From a terminal enter `./build`. This build takes about 1 minute on a Raspberry Pi Zero W. On a Ubuntu PC, just a couple of seconds.
After the build you will find 2 executables in the current folder: `vicread` and `ttyusb2dev`.

## vicread
This program reads data from a Victron device using the VE.Direct protocol. It removes any hex-messages from the received data before processing it. 
Next it discards any blocks that fail the checksum or doesn't meet the expected format/grammar. Last, it filters based on a white list of field names provided on the command-line. The filtered VE.Direct lines are sent to **stdout**. All error and information messages are sent to **stderr**. 

The program uses Linux system calls to open the serial device in read-only mode and lock it for exclusive access. It also sets the serial port configuration according to the VE.Direct protocol. Regular expressions are used to check the format/grammar. The program runs in an infinite loop until terminated manually.

### Help
```
rvanimm@raspi:~/Projects/GitHub/Victron-VEDirect-reader $ ./vicread
This application reads the VE.Direct protocol from a serial device and prints the data to stdout.
The VE.Direct protocol is used by Victron Energy devices to communicate with a host computer.

Usage: ./vicread <serial_device> [<white_list_filter>]

The serial device is typically a USB to serial adapter (e.g. /dev/ttyUSB0) connected to the VE.Direct port on the device. 
The white list filter is list of field names (e.g. "P,SOC" or "P SOC") that will be printed on stdout.
You can use commas and/or spaces to separate the names. Quotes are optional.
The names in the filter are case sensitive. If no filter is specified, all fields are printed.

Any block with a checksum error will be discarded.
As the checksum coverage isn't very strong an additional layer of checking has been added based on the grammar
Any lines that don't meet the expected format/grammer will result in a discard of the current block
```

### Example 1
vicread with USB device
```
$ ./vicread /dev/ttyUSB3
```
### Example 2
vicread with physical USB and white list filter for P and SOC including error redirection to file.
```
./vicread $(./ttyusb2dev 1-1.1.3) P,SOC 2>errlog-usb1-1.1.3.txt
```
From another terminal you can monitor the error log with:
```
tail -f errlog-usb1-1.1.3.txt
```

## ttyusb2dev
This program helps to find the full path of a serial USB device by either it's name or physical address. When no argument is provided, it prints out a list of all available serial USB devices. The program uses the C++ standard library, Linux header files, and regular expressions. The full path device name is sent to **stdout***. All error and informational messages are sent to **stderr**. 

The device names like /dev/ttyUSB0 are not "stable". Every reboot can result in a different mapping on the physical USB device. Some USB hubs seems to be very stable but other USB hubs shuffle the devices names with every reboot. **The only stable USB device name is the physical name**.

### Show all available serial USB devices including physical address
```
rvanimme@raspi:~/Projects/GitHub/Victron-VEDirect-reader $ ./ttyusb2dev 
This application returns the full device path based on the device name or physical address of a serial USB device.

Usage: ./ttyusb2dev [ <device_name | physical_address> ]

device_name is the name of the serial USB device (e.g. ttyUSB0 or ttyUSB5).
physical_address is the topology based address of the serial USB device (e.g. 3-2.3 or 1-1.1.3)

In case the device exists, the full path to the device is sent to stdout
In case the device does not exists or no argument is provided, a list of all tty USB devices is printed

Available ttyUSB device

Physical        Device
1-1.1.2         ttyUSB5
1-1.1.4         ttyUSB3
1-1.1.1         ttyUSB1
1-1.4           ttyUSB6
1-1.3           ttyUSB4
1-1.1.3         ttyUSB2
1-1.2           ttyUSB0
```

### Converting physical USB names to USB device names
```
rvanimme@raspi:~/Projects/GitHub/Victron-VEDirect-reader $ ./ttyusb2dev 1-1.1.4
/dev/ttyUSB3
```

### Using physical USB names with vicread command
```
rvanimme@raspi:~/Projects/GitHub/Victron-VEDirect-reader $ ./vicread $(./ttyusb2dev 1-1.1.3)
No white list filter used
Using serial device: "/dev/ttyUSB2"                                                                    
PID     0xA381
V       27393
T       24
I       111
P       3
CE      -551
...
```


