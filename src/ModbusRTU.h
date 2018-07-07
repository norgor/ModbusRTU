/*
Filename:	ModbusRTULib.h
Author:		Fusion
Contact:	admin@fusiondev.pw
Date:		27.03.2017
*/


#ifndef MODBUS_RTU_H
#define MODBUS_RTU_H

#define BIT_SET(variable, bit) ((variable) |= (1<<(bit)))
#define BIT_RESET(variable, bit) ((variable) &= ~(1<<(bit)))
#define BIT_CHECK(variable, bit) (variable & (1 << bit))

#define MODBUS_MAX_FRAME_LENGTH 255

namespace ModbusRTU
{
	extern uint16_t crc16(const uint8_t *nData, uint16_t wLength);
	extern uint16_t endianSwap16(uint16_t a);
	extern uint32_t endianSwap32(uint32_t a);

	struct ModbusRegister
	{
		enum RegisterType : uint8_t
		{
			None,
			Coil,
			DiscreteInput,
			InputRegister,
			HoldingRegister
		};

		ModbusRegister()
		{

		}

		RegisterType m_RegisterType;
		uint16_t m_RegisterNumber;
		uint8_t *m_pData;
	};

	template <uint16_t registerCount>
	class ModbusRTUSlave
	{

		enum FunctionCode : uint8_t
		{
			ReadCoils = 1,
			ReadDiscreteInputs = 2,
			ReadMultipleHoldingRegisters = 3,
			ReadInputRegisters = 4,
			WriteSingleCoil = 5,
			WriteSingleRegister = 6,
			WriteMultipleCoils = 15,
			WriteMultipleRegisters = 16,
			Exception = 128,
		};

		enum ExceptionCode : uint8_t
		{
			IllegalFunction = 1,
			IllegalDataAddress = 2,
			IllegalDataValue = 3,
		};

		HardwareSerial *m_pHardwareSerial;
		uint8_t m_SlaveID;
		ModbusRegister m_RegisterArray[registerCount];
		uint16_t m_AssignedRegisters;
		uint32_t m_BaudRate;
		uint8_t m_InputFrame[MODBUS_MAX_FRAME_LENGTH];
		uint8_t m_OutputFrame[MODBUS_MAX_FRAME_LENGTH];
		uint8_t m_InputFrameLength;

		//Checks if frame has been corrupted
		//
		//
		bool isFrameCorrupted(uint8_t *frame, uint16_t frameLength)
		{
			//Check if CRC16 is the same
			return (crc16(frame, frameLength - 2) != *(short*)&frame[frameLength - 2]);
		}

		//Sends exception to master
		//Uses last received frame as function code
		//
		void throwException(ExceptionCode exceptionCode)
		{
			m_OutputFrame[0] = m_SlaveID;
			m_OutputFrame[1] = m_InputFrame[1] + FunctionCode::Exception;
			m_OutputFrame[2] = exceptionCode;
			sendFrame(m_OutputFrame, 3);
		}

		//Attempts to find register in register array
		//Returns pointer to register if register found
		//Returns nullptr if register not found
		ModbusRegister *findRegister(uint16_t _register)
		{
			for (uint16_t i = 0; i < m_AssignedRegisters; i++)
			{
				if (m_RegisterArray[i].m_RegisterNumber == _register && m_RegisterArray[i].m_pData && m_RegisterArray[i].m_RegisterType != ModbusRegister::None)
					return &m_RegisterArray[i];
			}

			return nullptr;
		}

		//Receives frame from serial buffer
		//Function is non-blocking
		//Returns true when frame is successfully received
		bool receiveFrame()
		{
			if (m_pHardwareSerial->available())
			{
				m_InputFrameLength += m_pHardwareSerial->readBytes(&m_InputFrame[m_InputFrameLength], MODBUS_MAX_FRAME_LENGTH - m_InputFrameLength);

				// Check if frame function code has been received
				if (m_InputFrameLength >= 2)
				{
					//Function length check
					if (m_InputFrame[1] >= FunctionCode::ReadCoils && m_InputFrame[1] <= FunctionCode::WriteSingleRegister)
					{
						return m_InputFrameLength == 8;
					}
					else if (m_InputFrame[1] == FunctionCode::WriteMultipleCoils || m_InputFrame[1] == FunctionCode::WriteMultipleRegisters)
					{
						return m_InputFrameLength == m_InputFrame[6] + 9;
					}
					else
					{
						//Throw exception and wait for master to receive it.
						throwException(ExceptionCode::IllegalFunction);
						m_pHardwareSerial->flush();

						//Empty the serial input buffer.
						m_pHardwareSerial->readBytes(m_InputFrame, MODBUS_MAX_FRAME_LENGTH);
						clearInputFrame();
					}
				}
			}
			return false;
		}

		//Send response frame to master
		//This function appends a CRC16 to the end of the frame
		//
		void sendFrame(uint8_t *pFrame, uint8_t frameLength)
		{
			*(uint16_t*)&pFrame[frameLength] = crc16(pFrame, frameLength);
			m_pHardwareSerial->write(pFrame, frameLength + 2);
		}

		//Set input frame length to zero
		//
		//
		void clearInputFrame()
		{
			m_InputFrameLength = 0;
		}

		//Parses input frame
		//
		//
		void parseFrame(uint8_t *frame, uint16_t frameLength)
		{
			if (frame[1] == ReadCoils)
			{
				uint16_t targetRegister = endianSwap16(*(uint16_t*)&frame[2]);
				uint16_t targetRegisterLength = endianSwap16(*(uint16_t*)&frame[4]);
				byte dataLength = (ceil(((float)targetRegisterLength) / 8.0f));

				//Write frame header
				m_OutputFrame[0] = m_SlaveID;
				m_OutputFrame[1] = frame[1];
				m_OutputFrame[2] = dataLength;

				//Loop through requested registers
				for (uint16_t i = 0; i < targetRegisterLength; i++)
				{
					//Find the register
					ModbusRegister *pRegister = findRegister(targetRegister + i);

					//Check if register is valid
					if (pRegister && pRegister->m_RegisterType == ModbusRegister::Coil)
					{
						//Write register value to response frame
						if (*pRegister->m_pData)
							BIT_SET(m_OutputFrame[3 + (i / 8)], i % 8); //Set corresponding register bit
						else
							BIT_RESET(m_OutputFrame[3 + (i / 8)], i % 8); //Reset corresponding register bit
					}
					else
					{
						//Throw exception if register is not valid
						throwException(ExceptionCode::IllegalDataAddress);
						return;
					}
				}

				//Send response frame to master
				sendFrame(m_OutputFrame, 3 + dataLength);
			}
			else if (frame[1] == ReadDiscreteInputs)
			{
				uint16_t targetRegister = endianSwap16(*(uint16_t*)&frame[2]);
				uint16_t targetRegisterLength = endianSwap16(*(uint16_t*)&frame[4]);
				byte dataLength = (ceil(((float)targetRegisterLength) / 8.0f));

				//Write frame header
				m_OutputFrame[0] = m_SlaveID;
				m_OutputFrame[1] = frame[1];
				m_OutputFrame[2] = dataLength;

				//Loop through requested registers
				for (uint16_t i = 0; i < targetRegisterLength; i++)
				{
					//Find the register
					ModbusRegister *pRegister = findRegister(targetRegister + i);

					//Check if register is valid
					if (pRegister && pRegister->m_RegisterType == ModbusRegister::DiscreteInput)
					{
						//Write register value to response frame
						if (*pRegister->m_pData)
							BIT_SET(m_OutputFrame[3 + (i / 8)], i % 8); //Set corresponding register bit
						else
							BIT_RESET(m_OutputFrame[3 + (i / 8)], i % 8); //Reset corresponding register bit
					}
					else
					{
						//Throw exception if register is not valid
						throwException(ExceptionCode::IllegalDataAddress);
						return;
					}
				}

				//Send response frame to master
				sendFrame(m_OutputFrame, dataLength + 3);
			}
			else if (frame[1] == ReadMultipleHoldingRegisters)
			{
				uint16_t targetRegister = endianSwap16(*(uint16_t*)&frame[2]);
				uint16_t targetRegisterLength = endianSwap16(*(uint16_t*)&frame[4]);
				uint8_t dataLength = targetRegisterLength * 2;

				//Write frame header
				m_OutputFrame[0] = m_SlaveID;
				m_OutputFrame[1] = frame[1];
				m_OutputFrame[2] = dataLength;

				//Loop through requested registers
				for (uint16_t i = 0; i < targetRegisterLength; i++)
				{
					//Find the register
					ModbusRegister *pRegister = findRegister(targetRegister + i);

					//Check if register is valid
					if (pRegister && pRegister->m_RegisterType == ModbusRegister::HoldingRegister)
					{
						*(uint16_t*)&m_OutputFrame[3 + (i * 2)] = endianSwap16(*(uint16_t*)pRegister->m_pData);
					}
					else
					{
						//Throw exception if register is not valid
						throwException(ExceptionCode::IllegalDataAddress);
						return;
					}
				}

				//Send response frame to master
				sendFrame(m_OutputFrame, dataLength + 3);
			}
			else if (frame[1] == ReadInputRegisters)
			{
				uint16_t targetRegister = endianSwap16(*(uint16_t*)&frame[2]);
				uint16_t targetRegisterLength = endianSwap16(*(uint16_t*)&frame[4]);
				byte dataLength = targetRegisterLength * 2;

				//Write frame header
				m_OutputFrame[0] = m_SlaveID;
				m_OutputFrame[1] = frame[1];
				m_OutputFrame[2] = dataLength;

				//Loop through requested registers
				for (uint16_t i = 0; i < targetRegisterLength; i++)
				{
					//Find the register
					ModbusRegister *pRegister = findRegister(targetRegister + i);

					//Check if register is valid
					if (pRegister && pRegister->m_RegisterType == ModbusRegister::InputRegister)
					{
						*(uint16_t*)&m_OutputFrame[3 + (i * 2)] = endianSwap16(*(short*)pRegister->m_pData);
					}
					else
					{
						//Throw exception if register is not valid
						throwException(ExceptionCode::IllegalDataAddress);
						return;
					}
				}

				//Send response frame to master
				sendFrame(m_OutputFrame, dataLength + 3);
			}
			else if (frame[1] == WriteSingleCoil)
			{
				ModbusRegister *pRegister = findRegister(endianSwap16(*(uint16_t*)&frame[2]));

				//Check if register exists and if it is of correct type
				if (pRegister && pRegister->m_RegisterType == ModbusRegister::Coil)
				{
					//Write the target value to coil
					*(bool*)pRegister->m_pData = ((*(uint16_t*)&frame[4]) == true);
				}
				else
				{
					//Throw exception if exister is non-existent or incorrect type
					throwException(ExceptionCode::IllegalDataAddress);
					return;
				}

				m_pHardwareSerial->write(frame, 8);
			}
			else if (frame[1] == WriteSingleRegister)
			{
				ModbusRegister *pRegister = findRegister(endianSwap16(*(uint16_t*)&frame[2]));

				//Check if register exists and if it is of correct type
				if (pRegister && pRegister->m_RegisterType == ModbusRegister::HoldingRegister)
				{
					//Write the target value to the register
					*(uint16_t*)pRegister->m_pData = endianSwap16(*(uint16_t*)&frame[4]);
				}
				else
				{
					//Throw exception if exister is non-existent or incorrect type
					throwException(ExceptionCode::IllegalDataAddress);
					return;
				}

				m_pHardwareSerial->write(frame, 8);
			}
			else if (frame[1] == WriteMultipleCoils)
			{
				uint16_t targetRegister = endianSwap16(*(uint16_t*)&frame[2]);
				uint16_t targetRegisterLength = endianSwap16(*(uint16_t*)&frame[4]);

				for (uint16_t i = 0; i < targetRegisterLength; i++)
				{
					ModbusRegister *pRegister = findRegister(targetRegister + i);

					//Check if register exists and if it is of correct type
					if (pRegister && pRegister->m_RegisterType == ModbusRegister::Coil)
					{
						//Write the target value to coil
						*(bool*)pRegister->m_pData = BIT_CHECK(frame[7 + (i / 8)], i % 8);
					}
					else
					{
						//Throw exception if register is non-existent or incorrect type
						throwException(ExceptionCode::IllegalDataAddress);
						return;
					}
				}

				uint16_t lastCrc = *(uint16_t*)&frame[6];

				*(uint16_t*)&frame[6] = crc16(frame, 6);
				m_pHardwareSerial->write(frame, 8);
				*(uint16_t*)&frame[6] = lastCrc;
			}
			else if (frame[1] == WriteMultipleRegisters)
			{
				uint16_t targetRegister = endianSwap16(*(uint16_t*)&frame[2]);
				uint16_t targetRegisterLength = endianSwap16(*(uint16_t*)&frame[4]);

				for (uint16_t i = 0; i < targetRegisterLength; i++)
				{
					ModbusRegister *pRegister = findRegister(targetRegister + i);

					//Check if register exists and if it is of correct type
					if (pRegister && pRegister->m_RegisterType == ModbusRegister::HoldingRegister)
					{
						//Write the target value to coil
						*(uint16_t*)pRegister->m_pData = endianSwap16(*(uint16_t*)&frame[7 + (i * 2)]);
					}
					else
					{
						//Throw exception if register is non-existent or incorrect type
						throwException(ExceptionCode::IllegalDataAddress);
						return;
					}
				}

				m_OutputFrame[0] = frame[0];
				m_OutputFrame[1] = frame[1];
				m_OutputFrame[2] = frame[2];
				m_OutputFrame[3] = frame[3];
				m_OutputFrame[4] = frame[4];
				m_OutputFrame[5] = frame[5];
				sendFrame(m_OutputFrame, 6);
			}
		}

		//Add register to register array
		//Length of data type depends on register type
		//Input registers and holding registers are two-byte
		//Discrete inputs and coils are one-byte
		int32_t addRegister(byte *pData, uint16_t _register, ModbusRegister::RegisterType registerType)
		{
			if (m_AssignedRegisters >= registerCount || findRegister(_register))
				return -1;

			m_RegisterArray[m_AssignedRegisters].m_RegisterType = registerType;
			m_RegisterArray[m_AssignedRegisters].m_RegisterNumber = _register;
			m_RegisterArray[m_AssignedRegisters].m_pData = pData;

			m_AssignedRegisters++;
		}

	public:

		ModbusRTUSlave() {}

		//Adds coil to register list and returns register number
		//Returns -1 when no more registers available
		//
		int32_t addCoil(bool *coil, uint16_t _register)
		{
			return addRegister((byte*)coil, _register, ModbusRegister::Coil);
		}

		//Adds discrete input to register list and returns register number
		//Returns -1 when no more registers available
		//
		int32_t addDiscreteInput(const bool *discreteInput, uint16_t _register)
		{
			return addRegister((byte*)discreteInput, _register, ModbusRegister::DiscreteInput);
		}

		//Adds input register to register list and returns register number'
		//Returns -1 when no more registers available
		//
		int32_t addInputRegister(const uint16_t *inputRegister, uint16_t _register)
		{
			return addRegister((byte*)inputRegister, _register, ModbusRegister::InputRegister);
		}

		//Adds holding register to register list and returns register 
		//Returns -1 when no more registers available
		//
		int32_t addHoldingRegister(uint16_t *holdingRegister, uint16_t _register)
		{
			return addRegister((byte*)holdingRegister, _register, ModbusRegister::HoldingRegister);
		}

		//Initializes Modbus and serial data transmission
		//
		//
		void begin(uint32_t baud, HardwareSerial *pHardwareSerial = &Serial, uint8_t slaveId = 1)
		{
			//Clear all registers
			for (uint16_t i = 0; i < registerCount; i++)
			{
				m_RegisterArray[i].m_RegisterType = ModbusRegister::None;
				m_RegisterArray[i].m_pData = nullptr;
			}


			pinMode(13, OUTPUT);

			//Initialize variables
			m_BaudRate = baud;
			m_SlaveID = slaveId;

			clearInputFrame();

			//Initialize serial
			m_pHardwareSerial = pHardwareSerial;
			pHardwareSerial->begin(m_BaudRate);
			pHardwareSerial->setTimeout(0);

			//Empty the serial buffer
			m_pHardwareSerial->readBytes(m_InputFrame, MODBUS_MAX_FRAME_LENGTH);
		}

		//Checks for incoming frames from master
		//If frame has been received then it is parsed
		//
		void update()
		{
			//Check if a new frame is available
			if (receiveFrame())
			{
				//Check slave ID
				if (m_InputFrame[0] != m_SlaveID)
				{
					clearInputFrame();
					return;
				}

				//Check if frame is corrupted
				if (isFrameCorrupted(m_InputFrame, m_InputFrameLength))
				{
					clearInputFrame();
					return;
				}

				//Parses the incoming frame
				parseFrame(m_InputFrame, m_InputFrameLength);

				clearInputFrame();
			}
		}
	};
}
#endif