# About
The ModbusRTU library is a **simple** to use library that allows an arduino to act as a Modbus slave. It attempts to obscure all unnecessary parts of the protocol to provide a clean interface.

The library does not allocate memory dynamically after instantiation, which allows for simpler diagnostics and increased speed.


# Support
Slave ID's are supported, it is therefore possible to have multiple slaves on the same bus.

### Entities
* Discrete Inputs
* Coils
* Input Registers
* Holding Registers

### Function Codes
* 1   - Read Coils
* 2   - Read Discrete Inputs
* 3   - Read Multiple Holding Registers
* 4   - Read Input Registers
* 5   - Write Single Coil
* 6   - Write Single Holding Register
* 15  - Write Multiple Coils
* 16  - Write Multuple Holding Registers

### Exception Functionality
* Exception Response Codes
  * 1 - Illegal Function
  * 2 - Illegal Data Address
  * 3 - Illegal Data Value
  
# Documentation
Whole libary is enclosed in a namespace called "ModbusRTU".

### ModbusRTUSlave Class Members
```c++ 
   void begin(unsigned long baud, HardwareSerial *pHardwareSerial = &Serial, unsigned char slaveId = 1)
```
   The begin function sets up variables and configures the hardware serial class.
   * Parameters
      * baud: The baud rate the serial interface should run at.
      * pHardwareSerial: Pointer to a HardwareSerial instance. Defaults to &Serial.
      * slaveId: Slave ID to be used by this slave. Defaults to 1.
```c++
   void update()
```
   The function checks for incoming frames from master, attempts to parse the frame upon receiving, thereafter returns the result to the master. The parsing includes reading/writing to the modbus entites. If parsing a frame fails, the slave will attempt to send an exception.
