/*

	MotorControl.h
	
	Tells motors to start and stop
	
*/
#include "MotorControl.h"
#include "SystemConfig.h"
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

const unsigned char CRC7_POLY = 0x91;
//Pololu CRC generating code
unsigned char getCRC(unsigned char message[], unsigned char length)
{
	unsigned char i, j, crc = 0;
	for (i = 0; i < length; i++)
	{
		crc ^= message[i];
		for (j = 0; j < 8; j++)
		{
			if (crc & 1)
				crc ^= CRC7_POLY;
			crc >>= 1;
		}
	}
	return crc;
}

void printTimeFromMs(unsigned long int ms)
{
	unsigned long int x, seconds, minutes, hours, days;
	x = ms / 1000;
	seconds = x % 60;
	x /= 60;
	minutes = x % 60;
	x /= 60;
	hours = x % 24;
	x /= 24;
	days = x;
	CTRL_SERIAL.printf(" %2d days, %2d hours, %02d minutes, %02d seconds", days, hours, minutes, seconds);
}

void MotorControl::sendSMCMessage(unsigned char* message, int length)
{
	int i;
	for(i=0;i<length;i++)
	{
		MC_CTRL_SERIAL.write(message[i]);
	}
	if (_crcEnabled)
	{
		MC_CTRL_SERIAL.write(getCRC(message,length));
	}
	MC_CTRL_SERIAL.flush();
}

MotorControl::MotorControl()
{
	_numControllers = 0;
	_crcEnabled = false;
}

void MotorControl::motorsInitialise(int baudrate)
{
	MC_CTRL_SERIAL.begin(baudrate);
	MC_CTRL_SERIAL.setTimeout(100);
}

void MotorControl::disableSafeStart(char devId)
{
	unsigned char message[3];
	message[0] = 0xAA;
	message[1] = devId;
	message[2] = 0x03;
	sendSMCMessage(message,3);
}

void MotorControl::estopMotor(char devId)
{
	// you'll need to disable safe start after doing this!
	unsigned char message[3];
	message[0] = 0xAA;
	message[1] = devId;
	message[2] = 0x60;
	brakeMotor(devId,37);
	sendSMCMessage(message,3);
}

void MotorControl::eStopAllMotors()
{
	// stop all motors, whether we know about them or not
	int index;
	for (index=0;index<254;index++)
	{
		estopMotor(index);
	}
	// also pull the ERR line low eventually.  
}

void MotorControl::safeStartAllMotors()
{
	MotorController *ptr;
	int index;
	for (index=0;index<_numControllers;index++)
	{
		ptr = &_motors[index];
		disableSafeStart(ptr->deviceId);
	}
}


unsigned int MotorControl::getVariable(char devId, char variableId)
{
	char returnedByte1,returnedByte2;
	unsigned char message[4];
	unsigned int returnedVariable;
	char returned[2];
	message[0] = 0xAA;
	message[1] = devId;
	message[2] = 0x21;
	message[3] = variableId;
	sendSMCMessage(message,4);
	if (MC_CTRL_SERIAL.readBytes(returned,2) == 0)
		return 0; // timed out
	returnedByte1 = returned[0];
	returnedByte2 = returned[1];
	returnedVariable = returnedByte1 | (returnedByte2 << 8);
	return returnedVariable;
}

void MotorControl::setMotor(char devId, bool direction, unsigned long percent)
{
	unsigned char message[5];
	char bytepercent = (char)(percent & 0xFF);
	// software limiting of motor speed ! (really should be done on the controllers, but just in case)
	if (bytepercent > MOTOR_MAX_SPEED)
		bytepercent = MOTOR_MAX_SPEED;
	message[0] = 0xAA;
	message[1] = devId;

	if (direction)
	{
		message[2] = 0x05; //forward`
	} else {
		message[2] = 0x06; //reverse
	}
	message[3] = 0x00;
	message[4] = bytepercent;
	sendSMCMessage(message,5);
}

void MotorControl::brakeMotor(char devId,char brakeAmount)
{
	unsigned char message[4];

	message[0] = 0xAA;
	message[1] = devId;
	message[2] = 0x12;
	message[3] = brakeAmount;
	
	sendSMCMessage(message,4);
}

void MotorControl::updateFirmwareVersion(MotorController* controller)
{
	char prodId1,prodId2;
	unsigned char message[3];
	char returned[4];
	message[0] = 0xAA;
	message[1] = controller->deviceId;
	message[2] = 0x42;
	sendSMCMessage(message,3);
	if (MC_CTRL_SERIAL.readBytes(returned,4) == 0)
		return; // timed out
	prodId1 = returned[0];
	prodId2 = returned[1];
	controller->firmwareRevision.productId = (prodId1) | (prodId2 << 8);
	controller->firmwareRevision.minorFwVersion = returned[3];
	controller->firmwareRevision.majorFwVersion = returned[4];
}

bool MotorControl::addMotor(char devId)
{
	MotorController *ptr = &_motors[_numControllers++];
	ptr->deviceId = devId;
	refreshMotor(ptr);
	disableSafeStart(devId);
	//brakeMotor(devId,37);	
	return true;
}

MotorController* MotorControl::getMotor(char devId)
{
	MotorController *ptr = NULL;
	int index;
	for (index=0;index<_numControllers;index++)
	{
		if (_motors[index].deviceId == devId)
		{
			ptr = &_motors[index];
		}
	}
	return ptr;
}

bool MotorControl::pollMotor(char motorId)
{
	//a very basic check to see if a motor is present with a given ID
	unsigned char message[3];
	unsigned char returned[4];
	message[0] = 0xAA;
	message[1] = motorId;
	message[2] = 0x42;
	MC_CTRL_SERIAL.setTimeout(100);
	sendSMCMessage(message,3);
	if (MC_CTRL_SERIAL.readBytes(returned,4) == 0)
		return false; // timed out
	return true; // something came back!
	
}

void MotorControl::refreshMotor(MotorController* controller)
{
	MotorController *ptr = controller;
	unsigned int uptime1,uptime2,errorStatus,limitStatus,serialErrors;
	// first see if the motor responds at all, otherwise bail.
	if (!pollMotor(ptr->deviceId))
		return;
	updateFirmwareVersion(ptr);
	ptr->speed = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_SPEED); 
	ptr->brakeAmount = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_BRAKEAMT);
	ptr->inputVoltage = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_VIN);
	ptr->temperature = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_TEMP);
	ptr->baudRate = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_BAUDRATE);
	uptime1 = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_UPTIME1);
	uptime2 = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_UPTIME2);
	ptr->systemTime = uptime1 | (uptime2 << 16);
	
	errorStatus = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_ERROR_STATUS);
	ptr->statusFlags.safeStartViolation = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_SAFESTART) > 0);
	ptr->statusFlags.requiredChannelInvalid = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_REQCHANINVALID) > 0);
	ptr->statusFlags.serialError = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_SERIALERR) > 0);
	ptr->statusFlags.commandTimeout = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_COMMAND_TIMEOUT) > 0);
	ptr->statusFlags.killSwitch = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_KILLSWITCH) > 0);
	ptr->statusFlags.lowVin = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_LOWVIN) > 0);
	ptr->statusFlags.highVin = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_HIGHVIN) > 0);
	ptr->statusFlags.overTemperature = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_OVERTEMP) > 0);
	ptr->statusFlags.motorDriverError = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_DRIVERERROR) > 0);
	ptr->statusFlags.errLineHigh = ((errorStatus & MOTORCONTROLLER_ERRORSTATUS_ERRLINE) > 0);
	
	serialErrors = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_ERROR_SERIAL);
	ptr->statusFlags.serialFrameError = ((serialErrors & MOTORCONTROLLER_SERIALERRORS_FRAME) > 0);
	ptr->statusFlags.serialNoise = ((serialErrors & MOTORCONTROLLER_SERIALERRORS_NOISE) > 0);
	ptr->statusFlags.serialRxOverrun = ((serialErrors & MOTORCONTROLLER_SERIALERRORS_RXOVERRUN) > 0);
	ptr->statusFlags.serialFormatError = ((serialErrors & MOTORCONTROLLER_SERIALERRORS_FORMAT) > 0);
	ptr->statusFlags.serialCRCError = ((serialErrors & MOTORCONTROLLER_SERIALERRORS_CRC) > 0);
	
	ptr->statusFlags.errorsOccurred = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_ERROR_COUNTS);
	limitStatus = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_LIMIT_STATUS);
	ptr->statusFlags.safeStartEnabled = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_SAFESTART) > 0);
	
	ptr->statusFlags.overTemperatureLimiting = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_TEMPERATURE) > 0); 
	ptr->statusFlags.speedLimitLimiting = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_SPEEDLIMIT) > 0); 
	ptr->statusFlags.startingSpeedLimitLimiting = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_STARTSPEEDLIMIT) > 0); 
	ptr->statusFlags.accelerationLimiting = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_ACCELERATIONLIMIT) > 0); 
	ptr->statusFlags.rc1Kill = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_RC1KILLENGAGED) > 0); 
	ptr->statusFlags.rc2Kill = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_RC2KILLENGAGED) > 0);
	ptr->statusFlags.an1Kill = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_AN1KILLENGAGED) > 0);
	ptr->statusFlags.an2Kill = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_AN2KILLENGAGED) > 0);
	ptr->statusFlags.usbKill = ((limitStatus & MOTORCONTROLLER_LIMITSTATUS_USBKILLENGAGED) > 0);
	
	ptr->statusFlags.resetFlags = getVariable(ptr->deviceId,MOTORCONTROLLER_VAR_RESETFLAGS);
}

void MotorControl::refreshAllMotors()
{
	int i;
	MotorController *ptr;
	for (i=0;i<_numControllers;i++)
	{
		ptr = &_motors[i];
		refreshMotor(ptr);
		//printMotor(ptr->deviceId);
	}
}

void MotorControl::refreshMotor(char devId)
{
	MotorController *ptr = getMotor(devId);
	if (ptr == NULL)
		return;
	refreshMotor(ptr);
}

void MotorControl::printMotor(char devId)
{
	MotorController* ptr = getMotor(devId);
	CTRL_SERIAL.printf("Motor %d\n",devId);
	if (ptr == NULL)
	{
		CTRL_SERIAL.println("Motor not been initialised!");
		return;
	}
	CTRL_SERIAL.printf("ProductID: \t\t%d\r\n", ptr->firmwareRevision.productId);
	CTRL_SERIAL.printf("FW Version: \t%d.%02d\r\n", ptr->firmwareRevision.majorFwVersion, ptr->firmwareRevision.minorFwVersion);
	CTRL_SERIAL.printf("Speed: \t\t%d\r\n", ptr->speed);
	CTRL_SERIAL.printf("Voltage: \t\t%.2f V\r\n", ((float) ptr->inputVoltage)/1000.0);
	CTRL_SERIAL.printf("Temperature: \t\t%.2f deg C\r\n", ((float) ptr->temperature)/10.0);
	CTRL_SERIAL.printf("Baud rate:\t\t %d\r\n",ptr->baudRate);
	CTRL_SERIAL.print("System Uptime: \t\tJ99");
	printTimeFromMs(ptr->systemTime);
	CTRL_SERIAL.println();
	
	CTRL_SERIAL.println("Status flags active:");
	if( ptr->statusFlags.safeStartViolation) 
		CTRL_SERIAL.println("Safe safe start violation");

	if (ptr->statusFlags.requiredChannelInvalid) CTRL_SERIAL.println("Required Channel Invalid");		//• Bit 1: Required Channel Invalid
	if (ptr->statusFlags.serialError) CTRL_SERIAL.println("Required Channel Invalid"); 					// Bit 2: Serial Error
	if (ptr->statusFlags.commandTimeout) CTRL_SERIAL.println("Command Timeout"); 				//• Bit 3: Command Timeout
	if (ptr->statusFlags.killSwitch) CTRL_SERIAL.println("Kill Switch Enabled");					//• Bit 4: Limit/Kill Switch
	if (ptr->statusFlags.lowVin) CTRL_SERIAL.println("Vin Low"); 						//• Bit 5: Low VIN
	if (ptr->statusFlags.highVin) CTRL_SERIAL.println("Vin High"); 						//• Bit 6: High VIN
	if (ptr->statusFlags.overTemperature) CTRL_SERIAL.println("Over temperature");				//• Bit 7: Over Temperature
	if (ptr->statusFlags.motorDriverError) CTRL_SERIAL.println("Motor Driver Error"); 				//• Bit 8: Motor Driver Error
	if (ptr->statusFlags.errLineHigh) CTRL_SERIAL.println("Error Line high");					//• Bit 9: ERR Line High	

	if (ptr->statusFlags.serialFrameError) CTRL_SERIAL.println("Serial Frame Error"); 				//• Bit 1: Frame
	if (ptr->statusFlags.serialNoise) CTRL_SERIAL.println("Serial noise errors"); 					// • Bit 2: Noise
	if (ptr->statusFlags.serialRxOverrun) CTRL_SERIAL.println("Serial RX Overrun"); 				//• Bit 3: RX Overrun
	if (ptr->statusFlags.serialFormatError) CTRL_SERIAL.println("Serial Format Error"); 			//• Bit 4: Format
	if (ptr->statusFlags.serialCRCError) CTRL_SERIAL.println("Serial CRC error"); 				//• Bit 5: CRC
	if (ptr->statusFlags.safeStartEnabled) CTRL_SERIAL.println("Motor cannot run - safe start enabled"); 				// • Bit 0: Motor is not allowed to run due to an error or safe-start violation.
	if (ptr->statusFlags.overTemperatureLimiting) CTRL_SERIAL.println("Over temperature limiting speed"); 		//• Bit 1: Temperature is active reducing target speed.
	if (ptr->statusFlags.speedLimitLimiting) CTRL_SERIAL.println("Speed Limiting engaged"); 			//• Bit 2: Max speed limit is actively reducing target speed (target speed > max speed).
	if (ptr->statusFlags.startingSpeedLimitLimiting) CTRL_SERIAL.println("Starting speed limit engaged"); 	//• Bit 3: Starting speed limit is actively reducing target speed to zero (target speed < starting speed).
	if (ptr->statusFlags.accelerationLimiting) CTRL_SERIAL.println("Acceleration limiting engaged"); 			//• Bit 4: Motor speed is not equal to target speed because of acceleration, deceleration, or
	//brake duration limits.
	if (ptr->statusFlags.rc1Kill) CTRL_SERIAL.println("RC1 Killswitch Enabled"); //• Bit 5: RC1 is configured as a limit/kill switch and the switch is active (scaled value ≥1600).
	if (ptr->statusFlags.rc2Kill) CTRL_SERIAL.println("RC2 Killswitch Enabled"); //• Bit 6: RC2 limit/kill switch is active (scaled value ≥ 1600).
	if (ptr->statusFlags.an1Kill) CTRL_SERIAL.println("AN1 Killswitch Enabled"); //• Bit 7: AN1 limit/kill switch is active (scaled value ≥ 1600).
	if (ptr->statusFlags.an2Kill) CTRL_SERIAL.println("AN2 Killswitch Enabled"); //• Bit 8: AN2 limit/kill switch is active (scaled value ≥ 1600).
	if (ptr->statusFlags.usbKill) CTRL_SERIAL.println("USB Killswitch Enabled"); //• Bit 9: USB kill switch is active.*/
	CTRL_SERIAL.print("Last reset reason: ");
	switch(ptr->statusFlags.resetFlags)
	{
		case MOTORCONTROLLER_RESETFLAGS_RST:
			CTRL_SERIAL.println("RST line pulled low");
			break;
		case MOTORCONTROLLER_RESETFLAGS_POWER:
			CTRL_SERIAL.println("Power on reset");
			break;
		case MOTORCONTROLLER_RESETFLAGS_SOFTWARE:
			CTRL_SERIAL.println("Software controlled reset");
			break;
		case MOTORCONTROLLER_RESETFLAGS_WATCHDOG:
			CTRL_SERIAL.println("Watchdog timer reset");
			break;
		default:
			CTRL_SERIAL.printf(" unknown: %0x\n",ptr->statusFlags.resetFlags);
	}
}

