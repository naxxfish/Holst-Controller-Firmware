# ![Holst Controller Firmware](http://naxx.fish/holst/logo-small.png "Holst Controller Firmware")
# Holst Controller Firmware
## Introduction
Holst is a set of tools which allow the simultaneous control of many Pololu Simple Motor Controllers, as well as sending serial commands, DMX and GPIO/Relay closures.

This repository is the firmware which runs on the [Teensy 3.2](https://www.pjrc.com/store/teensy32.html) which is installed onto the [Controller Board](https://github.com/naxxfish/Holst-Controller-Board).  

## Building
Holst Controller requires the following dependencies to be built:
 * [Teensyduino](https://www.pjrc.com/teensy/teensyduino.html)
 * [DmxSimple](https://www.pjrc.com/teensy/td_libs_DmxSimple.html) (included with Teensyduino)
 * [SdFat](https://github.com/greiman/SdFat)

First, install the Arduino IDE, then Teensyduino add-on, then open the .ino project in the IDE, set the board to Teensy 3.1/3.2 (Tools > Board > Teensy 3.1/3.2).

## Hardware Requirements
This code runs on a Teensy 3.1 (and presumaby 3.2, although this is untested).
The code assumes certain pins are connected to particular signals.  The below section of schematic shows the expected circuit.
![Teensy Schematic](http://naxx.fish/holst/teensy-pinout.png)

| Pin | Signal                   |
|-----|--------------------------|
| 2   | User Serial Rx           |
| 3   | User Serial Tx           |
| 4   | Motor Control (error)    |
| 5   | Motor Control (reset)    |
| 7   | OK LED (to ground)       |
| 8   | SDcard CS                |
| 9   | DMX Rx (serial)          |
| 10  | DMX Tx (serial)          |
| 11  | Motor Control Rx (serial)|
| 12  | Motor Control Tx (serial)|
| 13  | SPI MOSI / SDcard DI     |
| 14  | SPI MISO / SDcard DO     |
| 20  | SPI SCK  / SDcard CLK    |
| 21  | Motor Control (shutdown) |
| 22  | 1Wire                    |
| 23  | Emergency Stop (to ground)|
| 24  | Button                   |
| 25  | I2C SDA                  |
| 26  | I2C SCL                  |
| 29  | DMX Enable               |

### DMX
DMX is output as a serial signal on Pin 10, and needs to be converted into differential RS485 for connection to a DMX device.  The [Controller Board](https://github.com/naxxfish/Holst-Controller-Board) uses a [MAX3535E](https://www.maximintegrated.com/en/products/interface/transceivers/MAX3535E.html) for this purpose, providing isolation and differential driving in a single package - however a driver may be constructed from common components, for example [DMX Shield for Arduino with isolation](http://www.mathertel.de/arduino/dmxshield.aspx)

### Motor Control
The Motor Control port is a serial port, which is designed to be connected to one or more [Pololu Simple Motor Controller](https://www.pololu.com/category/94/pololu-simple-motor-controllers) boards. These are connected via TTL serial, and the Error, Reset and Shutdown signals. See the [Pololu Simple Motor Controller User's Guide](https://www.pololu.com/docs/0J44) for more information on these signals.  

### SDcard
Holst Controller loads and saves it's configuration and schedule onto SD card, and this is required for correct startup.  The SD card should be connected to the SPI pins (DI->13, DO->14, CLK->20, CS->8) - extended mode is not supported.  

### User serial
A serial port for interaction by a user terminal (e.g. with a laptop with a serial port) is also provided (pins Rx 2, Tx 3).  The CLI is provided on this port, as well as the Teensy's USB interface.

### I2C
A PCF8574 is expected on the the bus, to provide GPIO.  Other devices may be connected providing they do not conflict.

### 1Wire
1 wire can be used to gather sensor readings - any 1wire devices may be attached to this.

## Theory of Operation

There are three levels of hierarchy - Cue, Sequence and Schedule.

### cues
Cues can be any of the following:
 * Motor - set a motor speed value/braking
 * DMX - set a DMX channel to a value
 * Relay - open or close a relay output

### Sequence
A sequence is a list of cues to be executed. Each cue has an offset, which is the offset from the start of a sequence.

### Schedule
A schedule is a description of when a sequence should be executed.  Multiple schedules may exist for the same sequence. Schedules are defined using a cron like syntax, which allow intervals. 

## CLI Reference
Once booted, and connected to either the USB interface or the Control interface, you will be presented with a prompt.  The prompt describes which mode you are currently in.  To enter a command, type the command then press return.

One command works anywhere: `ESTOP`, which will stop all new commands and send an emergency stop signal to all conneced motors.  

### Root (#)
This is the mode which you are in when first booting, and is the top level.  The following commands are available in this mode.
#### `GETVER`
Get the current version of the Firmware running
#### `GETSTATUS`
Gets the current status of the system
#### `SETTIME`
Sets the current time in format:

    YYYYMMDD(dom)(dow)hhmmss

(dom - Day of Month (e.g. 01 for 1st of ))
(dow - Day of Week (i.e. MON,TUE,WED,THU,FRI,SAT,SUN))
#### `GETTIME`
Gets the current time.

#### `RUN`
Runs the current loaded schedule.

#### `STOP`
Stops the current running schedule.

#### `CONTROL`
Goes to CTRL mode.

#### `PROGRAM`
Goes to PROG mode.

### `CTRL` mode
In control mode, you can directly control some things.
#### `LISTI2C`
Lists any devices which have been found on the I2C bus.
#### `GETDMX`
Gets the current list of DMX channels which have been set.
#### `SETDMX`
Sets a given DMX channel to a value
#### `SETMOTOR`
Sets a motor with a given ID to a given speed
#### `GETMOTOR`
Gets the currently set motor speed of a given motor ID.
#### `RESTART`
Performs a safe restart after an emergency stop (ESTOP), either from the `ESTOP` command or by the ESTOP button input.
#### `EXIT`
Goes back to Root mode

### PROG mode
#### `LISTSEQ`
Lists all of the sequences which are currently defined.
#### `GETSEQ`
Lists cues in a given sequence
#### `EDITSEQ`
Adds a new sequence with a given ID
#### `CLEARSEQ`
Clears all sequences
#### `LISTSCHED`
Lists the current schedule items
#### `CLEARSCHED`
Clears schedule
#### `ADDSCHED`
Adds a new scheduled sequence
#### `SAVE`
Saves the current schedule/sequences to SD card.
#### `EXIT`
Exit back to Root mode.

### EDITSEQ mode
#### `LISTCUES`
Lists all cues
#### `ADDCUE`
Appends a cue to the end of the sequence.
#### `SAVE`
Saves the current sequence.
#### `EXIT`
Returns to PROG mode.
