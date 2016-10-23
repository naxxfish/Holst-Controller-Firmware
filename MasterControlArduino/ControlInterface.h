/*

	ControlInterface.h
	
	The "user" interface for the system, this class pokes the various components and lets you know what's going down.

*/

#ifndef CONTROL_INT_H
#define CONTROL_INT_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SystemConfig.h"
#include "Scheduler.h"
#include "MotorControl.h"
#include "ClockManager.h"

#define CONTROL_INT_VER "0.1"
#define SERIALCOMMANDBUFFER 254
#define MAXDELIMETER 5

enum ControlState {
				MAIN_MENU,
				CONTROL,
				PROGRAM,
				PROG_ADDSEQ
			};

class ControlInterface{
		public:
			ControlInterface();
			void start(int baudrate);
			void interactive();
			void readSerial();
			void clearBuffer();
			void issuePrompt();
			char* next();
			
			// application methods
			void runSchedule();
			void stopSchedule();
			void getVersion();
			void getStatus();
			void getTime();
			void intSetTime();
			void intSetDate();
			void addSeq();
			void listSeq();
			void clearSeq();
			void listCues();
			void addCue();
			void addSched();
			void clearSched();
			void saveSched();
			void loadSched();
			void listSched();
			void listRunningSeq();
			void printDmx();
			void printI2CDevices();
			void setDmx();
			void debugEnable(bool offon);
			// helpers
			void printLog(String logline);
			void print2digits(int number);
			void printSeq(long seqId);
			void setSched(Scheduler *sched);
			void relayToggle();
			void setSystemController(SystemControl* controller);
			void setClockManager(ClockManager* clockManager);
			void controlLogging(bool offon);
			void setMotor();
			void setMotorControl(MotorControl* motors);
			void printMotor();
		private:
			char inChar;          // A character read from the serial stream 
			char buffer[SERIALCOMMANDBUFFER];   // Buffer of stored characters while waiting for terminator character
			int  bufPos;                        // Current position in the buffer
			char delim[MAXDELIMETER];           // null-terminated list of character to be used as delimeters for tokenizing (default " ")
			char term;                          // Character that signals end of command (default '\r')
			char *token;                        // Returned token from the command buffer as returned by strtok_r
			char *last;                         // State variable used by strtok_r during processing
			long currentSeqId;
			int numCommand;
			ControlState _state;
			Scheduler* _sched;
			void printTimestamp();
			void printDigits(int digits);
			bool started;
			SystemControl *_systemController;
			MotorControl *_motors;
			ClockManager *_clockManager;
};

#endif

