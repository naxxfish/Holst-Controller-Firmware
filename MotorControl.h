/*

	MotorControl.h
	
	Tells motors to start and stop
	
*/

#ifndef MOTORCONTROL_H
#define MOTORCONTROL_H
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define MOTORS_MAX_DEVICES 12


#define MOTORCONTROLLER_VAR_SPEED 21
#define MOTORCONTROLLER_VAR_BRAKEAMT 22
#define MOTORCONTROLLER_VAR_VIN 23
#define MOTORCONTROLLER_VAR_TEMP 24
#define MOTORCONTROLLER_VAR_BAUDRATE 27
#define MOTORCONTROLLER_VAR_UPTIME1 28
#define MOTORCONTROLLER_VAR_UPTIME2 29
#define MOTORCONTROLLER_VAR_ERROR_STATUS 0
#define MOTORCONTROLLER_VAR_ERROR_COUNTS 1
#define MOTORCONTROLLER_VAR_ERROR_SERIAL 2
#define MOTORCONTROLLER_VAR_LIMIT_STATUS 3
#define MOTORCONTROLLER_VAR_RESET_FLAGS 127

#define MOTORCONTROLLER_ERRORSTATUS_SAFESTART 		(0b1)
#define MOTORCONTROLLER_ERRORSTATUS_REQCHANINVALID 	(0b1 << 1)
#define MOTORCONTROLLER_ERRORSTATUS_SERIALERR 		(0b1 << 2)
#define MOTORCONTROLLER_ERRORSTATUS_COMMAND_TIMEOUT (0b1 << 3)
#define MOTORCONTROLLER_ERRORSTATUS_KILLSWITCH 		(0b1 << 4)
#define MOTORCONTROLLER_ERRORSTATUS_LOWVIN			(0b1 << 5)
#define MOTORCONTROLLER_ERRORSTATUS_HIGHVIN		(0b1 << 6)
#define MOTORCONTROLLER_ERRORSTATUS_OVERTEMP		(0b1 << 7)
#define MOTORCONTROLLER_ERRORSTATUS_DRIVERERROR		(0b1 << 8)
#define MOTORCONTROLLER_ERRORSTATUS_ERRLINE			(0b1 << 9)

#define MOTORCONTROLLER_SERIALERRORS_FRAME			(0b1 << 1)
#define MOTORCONTROLLER_SERIALERRORS_NOISE			(0b1 << 2)
#define MOTORCONTROLLER_SERIALERRORS_RXOVERRUN		(0b1 << 3)
#define MOTORCONTROLLER_SERIALERRORS_FORMAT			(0b1 << 4)
#define MOTORCONTROLLER_SERIALERRORS_CRC			(0b1 << 5)

#define MOTORCONTROLLER_LIMITSTATUS_SAFESTART				(0b1)
#define MOTORCONTROLLER_LIMITSTATUS_TEMPERATURE				(0b1 << 1)
#define MOTORCONTROLLER_LIMITSTATUS_SPEEDLIMIT				(0b1 << 2)
#define MOTORCONTROLLER_LIMITSTATUS_STARTSPEEDLIMIT			(0b1 << 3)
#define MOTORCONTROLLER_LIMITSTATUS_ACCELERATIONLIMIT		(0b1 << 4)
#define MOTORCONTROLLER_LIMITSTATUS_RC1KILLENGAGED			(0b1 << 5)
#define MOTORCONTROLLER_LIMITSTATUS_RC2KILLENGAGED			(0b1 << 6)
#define MOTORCONTROLLER_LIMITSTATUS_AN1KILLENGAGED			(0b1 << 7)
#define MOTORCONTROLLER_LIMITSTATUS_AN2KILLENGAGED			(0b1 << 8)
#define MOTORCONTROLLER_LIMITSTATUS_USBKILLENGAGED			(0b1 << 9)

#define MOTORCONTROLLER_RESETFLAGS_RST				0x04
#define MOTORCONTROLLER_RESETFLAGS_POWER			0x0c
#define MOTORCONTROLLER_RESETFLAGS_SOFTWARE			0x14
#define MOTORCONTROLLER_RESETFLAGS_WATCHDOG			0x24

#define MOTORCONTROLLER_VAR_RESETFLAGS				127

typedef struct _motorFirmware {
	int productId;
	char minorFwVersion;
	char majorFwVersion;
} MotorControllerFirmware;

typedef struct _motorStatusFlags {
	bool safeStartViolation;			//• Bit 0: Safe Start Violation
	bool requiredChannelInvalid;		//• Bit 1: Required Channel Invalid
	bool serialError; 					// Bit 2: Serial Error
	bool commandTimeout; 				//• Bit 3: Command Timeout
	bool killSwitch; 					//• Bit 4: Limit/Kill Switch
	bool lowVin; 						//• Bit 5: Low VIN
	bool highVin; 						//• Bit 6: High VIN
	bool overTemperature; 				//• Bit 7: Over Temperature
	bool motorDriverError; 				//• Bit 8: Motor Driver Error
	bool errLineHigh; 					//• Bit 9: ERR Line High	
	int errorsOccurred;
	bool serialFrameError; 				//• Bit 1: Frame
	bool serialNoise; 					// • Bit 2: Noise
	bool serialRxOverrun; 				//• Bit 3: RX Overrun
	bool serialFormatError; 			//• Bit 4: Format
	bool serialCRCError; 				//• Bit 5: CRC
	bool safeStartEnabled; 				// • Bit 0: Motor is not allowed to run due to an error or safe-start violation.
	bool overTemperatureLimiting; 		//• Bit 1: Temperature is active reducing target speed.
	bool speedLimitLimiting; 			//• Bit 2: Max speed limit is actively reducing target speed (target speed > max speed).
	bool startingSpeedLimitLimiting; 	//• Bit 3: Starting speed limit is actively reducing target speed to zero (target speed < starting speed).
	bool accelerationLimiting; 			//• Bit 4: Motor speed is not equal to target speed because of acceleration, deceleration, or
	//brake duration limits.
	bool rc1Kill; //• Bit 5: RC1 is configured as a limit/kill switch and the switch is active (scaled value ≥1600).
	bool rc2Kill; //• Bit 6: RC2 limit/kill switch is active (scaled value ≥ 1600).
	bool an1Kill; //• Bit 7: AN1 limit/kill switch is active (scaled value ≥ 1600).
	bool an2Kill; //• Bit 8: AN2 limit/kill switch is active (scaled value ≥ 1600).
	bool usbKill; //• Bit 9: USB kill switch is active.*/
	unsigned int resetFlags;
} MotorControllerStatusFlags;

typedef struct _motor {
	MotorControllerFirmware firmwareRevision;
	char deviceId;
	MotorControllerStatusFlags statusFlags;
	unsigned int speed;
	unsigned int brakeAmount;
	unsigned int inputVoltage;
	unsigned int temperature;
	unsigned int baudRate;
	unsigned long systemTime;
} MotorController;


class MotorControl{
	public:
		MotorControl();
		void setMotor(char devId, bool direction, unsigned long percent);
		void motorsInitialise(int baudrate);
		MotorController* getMotor(char motorId);
		void disableSafeStart(char devId);
		void estopMotor(char devId);
		void brakeMotor(char devId,char brakeAmount);
		unsigned int getVariable(char devId, char variableId);
		void updateFirmwareVersion(MotorController* controller);
		bool addMotor(char devId);
		void refreshMotor(char devId);
		void refreshMotor(MotorController* controller);
		void printMotor(char devId);
		void refreshAllMotors();
		void eStopAllMotors();
		void safeStartAllMotors();
		bool pollMotor(char motorId);
	private:
		MotorController _motors[MOTORS_MAX_DEVICES];
		int _numControllers;
		bool _crcEnabled;
		void sendSMCMessage(unsigned char* message, int length);
};



#endif