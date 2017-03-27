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

## Class ModbusRTUSlave
### Members
> void begin(unsigned long baud, HardwareSerial *pHardwareSerial = &Serial, unsigned char slaveId = 1)
