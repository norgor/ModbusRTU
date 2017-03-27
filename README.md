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
Whole libary is enclosed in a namespace called ModbusRTU.

## ModbusRTUSlave Class
The class is a template class with one parameter called registerCount. The parameter specifies the size of the register array or maximum register count.

### Members
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


```c++
   long addCoil(bool *coil, unsigned short _register)
```
   The function adds a coil to the register array.
   * Parameters
      * coil: Pointer to a read/write boolean value.
      * _register: The register number to be assigned to the coil.
   * Return value
      * Returns the register number upon success.
      * Returns -1 upon failure.


```c++
   long addDiscreteInput(const bool *discreteInput, unsigned short _register)
```
   The function adds a discrete input to the register array.
   * Parameters
      * discreteInput: Pointer to a read-only boolean value.
      * _register: The register number to be assigned to the discrete input.
   * Return value
      * Returns the register number upon success.
      * Returns -1 upon failure.


```c++
   long addInputRegister(const short *inputRegister, unsigned short _register)
```
   The function adds an input register to the register array.
   * Parameters
      * inputRegister: Pointer to a read-only 16-bit value.
      * _register: The register number to be assigned to the input register.
   * Return value
      * Returns the register number upon success.
      * Returns -1 upon failure.


```c++
   long addHoldingRegister(short *holdingRegister, unsigned short _register)
```
   The function adds an input register to the register array.
   * Parameters
      * holdingRegister: Pointer to a read/write 16-bit value.
      * _register: The register number to be assigned to the holding register.
   * Return value
      * Returns the register number upon success.
      * Returns -1 upon failure.
