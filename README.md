# OPC UA server for the Libera Digit 500

This is an OPC UA server running on the
[Libera Digit 500](https://www.i-tech.si/products/libera-digit-500/)
4-channel digitizer for remote control of the devices and data acquisition.

It is based on the [Open62541](https://github.com/open62541/open62541/)
open source implementation of OPC UA.

# Functionality

- Provides an OPC-UA server at TCP/IP port 10001.
- Access to device is handled via the internal MCI facility.
- The data stream created from the pulse processing is intercepted, the last received data block
is available through the server.

# Project status

The server compiles and runs stabily on the devices used for the tests.

The main functionality necessary to read and modify the device internal
MCI variables is established. The variable content imaged by the
OPC-UA server is defined in an XML file. A code generator is provided
to generate the necessary C/C++ code for the server application.
This does not cover the whole functionality provided by the devices, only the essentials.

The pulse data stream is intercepted and the data content made available as scalar values.

The number of pulses received per second is determined and reported.

# Build

## Tool chain
The server is built with a cross-compiler for the ARM target CPU of the device
running on a Linux system.

## Protocol stack
The OPC UA stack needs to be downloaded from [Open62541](https://github.com/open62541/open62541/) and built.
This can be done on the development system - there is no binary code produced at this stage.
The complete stack is obtained in two (amalgamated) files.
- open62541.h
- open62541.c
For easy access these 2 files are included with the repository but these need to be regenerated if
any configuration change of the server stack is desired.

## Libera libraries (for MCI)
To use the MCI facility to access the internal instrument data a few libraries proprietary to
[Instrumentation Technologies](http://www.i-tech.si/). These were obtained in two files
- `libera-base3.0-dev_3.0-426+r23640+helium_armelx.deb`
- `libera-mci3.0-dev_3.0-426+r23640+helium_armelx.deb`

The packages were manually extracted and the headers copied into local directories for MCI programming
- `LiberaDigit500-OPCUA/istd/*`
- `LiberaDigit500-OPCUA/mci/*`
- `LiberaDigit500-OPCUA/isig/*`

This is proprietory software by Instrumentation Technologies and can not be included here.
Any user of the server provided here needs to contact the manufacturer for these header files.

## Compile and link the server

Before the C/C++ code can be compiled the several code snippets (*.inc files)
need to be created with the code generator. This needs to be done
before activating the cross-target tool chain as this interferes
with the python environment settings on the development system.
- `python3 code_generator.py`

A makefile is not yet provided, just a few lines are required to build the server.
- `source ~/Libera/tools/environment`
- `$CC -c -std=c99 open62541.c`
- `$CXX -c -std=gnu++11 -I. -L$SDKTARGETSYSROOT/opt/libera/lib libera_mci.cpp`
- `$CC -c -std=c99 -I. libera_opcua.c`
- `$CC -c -std=c99 -I. OpcUaServer.c`
- `$CXX -o opcua_server OpcUaServer.o open62541.o libera_mci.o libera_opcua.o  -lpthread -L$SDKTARGETSYSROOT/opt/libera/lib -lliberamci -lliberaisig -lliberaistd -lliberainet -lomniORB4 -lomniDynamic4 -lomnithread`

## Testing

The server binary needs to copied to the instrument
- `scp opcua_server root@instrument:/tmp/`

On the instrument one needs to stop the libera-ioc service that will otherwise consume
the data stream and restart the digit500 application.
- `/opt/etc/init.d/S80libera-ioc stop`
- `/opt/etc/init.d/S50libera-digit500 restart`

Then the server can be started
- `ssh root@instrument`
- `cd /tmp/`
- `./opcua_server`

For a first test of the server access a universal OPC UA client like
[UaExpert](https://www.unified-automation.com/products/development-tools/uaexpert.html) is recommended.


