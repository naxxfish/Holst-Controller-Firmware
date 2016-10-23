/*

	ControlInterface.cpp
	
	The "user" interface for the system, this class pokes the various components and lets you know what's going down.

*/
#include "SystemConfig.h"
#include "ControlInterface.h"
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include <Time.h>
#include "Scheduler.h"
#include "MotorControl.h"
#include "ClockManager.h"

ControlInterface::ControlInterface()
{
	strncpy(delim," ",MAXDELIMETER);  // strtok_r needs a null-terminated string
	term='\r';   // return character, default terminator for commands
	numCommand=0;    // Number of callback handlers installed
	clearBuffer(); 
	started=false;
	// do whatever in here to make stuff great.
}

void ControlInterface::start(int baudrate)
{
	CTRL_SERIAL.begin(baudrate);
}

void ControlInterface::interactive()
{
	started=true;
	CTRL_SERIAL.println(F("==========="));
	CTRL_SERIAL.println(F(" H-O-L-S-T"));
	CTRL_SERIAL.println(F("==========="));
	CTRL_SERIAL.println(F("    from naxxfish industries"));
	CTRL_SERIAL.println();
	CTRL_SERIAL.print(F("Firmware Version: "));
	CTRL_SERIAL.println(HOLST_VERSION);
	CTRL_SERIAL.print(F("Control Interface Version: "));
	CTRL_SERIAL.println(CONTROL_INT_VER);
	issuePrompt();
}

void ControlInterface::issuePrompt()
{
	const __FlashStringHelper* prompt = F("?");

	if (_state == MAIN_MENU)
	{
		prompt = F("# ");
	} else if (_state == PROGRAM)
	{
		prompt = F("PROG> ");
	} else if (_state == CONTROL)
	{
		prompt = F("CTRL> ");
	} 
	else if (_state == PROG_ADDSEQ)
	{
		prompt = F("PROGSEQ> ");
	}
	CTRL_SERIAL.printf(F("\r\n"));
	if (_systemController->isStopped())
	{
		CTRL_SERIAL.print(F("*ESTOP* "));
	}
	CTRL_SERIAL.printf(F("%s"),prompt);
}

void sendLoggingCommand(const char* logSource, char* logLine)
{
	CTRL_SERIAL.printf(F("(%s)> %s\n"),logSource, logLine);
}

boolean isIt(char* test, const char* cmd)
{
	return strncmp(test,cmd,SERIALCOMMANDBUFFER) == 0;
}

void ControlInterface::readSerial()
{
	if (!started) return;
		// If we're using the Hardware port, check it.   Otherwise check the user-created SoftwareSerial Port
	while (CTRL_SERIAL.available() > 0) 
	{
		boolean matched; 
		inChar=CTRL_SERIAL.read();   // Read single available character, there may be more waiting
		#ifdef SERIALCOMMANDDEBUG
		CTRL_SERIAL.print(inChar);   // Echo back to serial stream
		#endif
		if (inChar==term) {     // Check for the terminator (default '\r') meaning end of command
			#ifdef SERIALCOMMANDDEBUG
			CTRL_SERIAL.print(F("Received: ")); 
			CTRL_SERIAL.println(buffer);
		    #endif
			bufPos=0;           // Reset to start of buffer
			token = strtok_r(buffer,delim,&last);   // Search for command at start of buffer
			if (token == NULL) 
			{
					issuePrompt();
					return;
			}
			matched=false; 
			CTRL_SERIAL.println();
			/* Global Commands */
			
			if (isIt(token,"ESTOP"))
			{
				CTRL_SERIAL.println(F("!!! STOP !!!"));
				_sched->stop();
				_systemController->eStop();
				matched=true;
			}
			
			if (isIt(token,"RESTART"))
			{
				CTRL_SERIAL.println(F("Resetting from ESTOP"));
				_systemController->restart();
				CTRL_SERIAL.println(F("You may now restart the schedule with the RUN command when you're ready"));
				matched=true;				
			}
			
			if (_state == MAIN_MENU)
			{
				/* Main Menu commands */
				if (isIt(token,"EXECUTE"))
				{
					_sched->execute();
					matched=true;
				}
				if (isIt(token,"TICKON"))
				{
					_clockManager->setTicking(true);
					matched=true;
				}
				if (isIt(token,"TICKOFF"))
				{
					_clockManager->setTicking(false);
					matched=true;
				}
				if (isIt(token,"SCHEDLOGON"))
				{
						debugEnable(true);
						matched=true;
				}
				if (isIt(token,"SCHEDLOGOFF"))
				{
						debugEnable(false);
						matched=true;
				}
				if (isIt(token,"CTRLLOGON"))
				{
					controlLogging(true);
					matched=true;
				}
				if (isIt(token,"CTRLLOGOFF"))
				{
					controlLogging(false);
					matched=true;
				}
				if (isIt(token,"GETSTATUS"))
				{
					getStatus();
					matched=true;
				}
				if (isIt(token,"GETVER"))
				{
					getVersion();
					matched=true;
				}
				if (isIt(token,"RUN"))
				{
					runSchedule();
					matched=true;
				}
				if (isIt(token,"STOP"))
				{
					stopSchedule();
					matched=true;
				}
				if (isIt(token,"GETTIME"))
				{
					getTime();
					matched=true;
				}
				if (isIt(token,"SETTIME"))
				{
					intSetTime();
					matched=true;
				}	
				if (isIt(token,"LISTRUNNING"))
				{
					listRunningSeq();
					matched=true;
				}
				if (isIt(token,"SETDATE"))
				{
					intSetDate();
					matched=true;
				}				
				/* State transitions */
				if (isIt(token,"CONTROL"))
				{
					matched=true;
					_state = CONTROL;

				}
				if (isIt(token,"PROGRAM"))
				{
					matched=true;
					_state = PROGRAM;

				}
				
			} else if (_state == CONTROL)
			{
				if(isIt(token,"RELAY"))
				{
					matched=true;
					relayToggle();
				}
				if(isIt(token,"SETMOTOR"))
				{
					matched=true;
					setMotor();
				}
				if(isIt(token,"GETMOTOR"))
				{
					matched=true;
					printMotor();
				}
				if (isIt(token,"GETDMX"))
				{
					matched=true;
					printDmx();
				}
				if (isIt(token,"SETDMX"))
				{
					matched=true;
					setDmx();
				}
				if (isIt(token,"LISTI2C"))
				{
					matched=true;
					printI2CDevices();
				}
				/* State transitions */
				if (isIt(token,"EXIT"))
				{
					matched=true;
					_state = MAIN_MENU;

				}
				
			} else if (_state == PROGRAM)
			{
				if (isIt(token,"LISTSEQ"))
				{
					matched=true;
					listSeq();
				}
				if (isIt(token,"CLEARSEQ"))
				{
					matched=true;
					clearSeq();
				}
				if (isIt(token,"ADDSCHED"))
				{
					matched=true;
					addSched();
				}
				if (isIt(token,"CLEARSCHED"))
				{
					matched=true;
					clearSched();
				}		
				if (isIt(token,"LISTSCHED"))
				{
					matched=true;
					listSched();
				}					
				if (isIt(token,"SAVE"))
				{
					matched=true;
					saveSched();
				}
				if (isIt(token,"LOAD"))
				{
					matched=true;
					loadSched();
				}						
				/* State transitions */
				if (isIt(token,"EDITSEQ"))
				{
					matched=true;
					addSeq();
					_state = PROG_ADDSEQ;
				}
				if (isIt(token,"EXIT"))
				{
					matched=true;
					_state = MAIN_MENU;
				}
			} else if (_state == PROG_ADDSEQ)
			{
				if (isIt(token,"LISTCUES"))
				{
					listCues();
					matched=true;
				}
				if (isIt(token,"ADDCUE"))
				{
					matched=true;
					addCue();
				}

				/* State Transitions */
				if (isIt(token,"EXIT"))
				{
					matched=true;
					_state = PROGRAM;
				}				
			}
			
			if (matched==false) {
				CTRL_SERIAL.println("$ INVALID COMMAND");
				clearBuffer(); 
				issuePrompt();
			} else {
				clearBuffer();
				issuePrompt();
			}

		}
		if (inChar == 127 || inChar == 8)
		{
			if (bufPos > 0)
			{
				CTRL_SERIAL.write(inChar);
				buffer[bufPos--] = '\0';
			}
		}
		if (isprint(inChar))   // Only printable characters into the buffer
		{
			CTRL_SERIAL.write(inChar);
			buffer[bufPos++]=inChar;   // Put character into buffer
			buffer[bufPos]='\0';  // Null terminate
			if (bufPos > SERIALCOMMANDBUFFER-1) bufPos=0; // wrap buffer around if full  
		}
	}
}

//
// Initialize the command buffer being processed to all null characters
//
void ControlInterface::clearBuffer()
{
	for (int i=0; i<SERIALCOMMANDBUFFER; i++) 
	{
		buffer[i]='\0';
	}
	bufPos=0; 
}

// Retrieve the next token ("word" or "argument") from the Command buffer.  
// returns a NULL if no more tokens exist.   
char *ControlInterface::next() 
{
	char *nextToken;
	nextToken = strtok_r(NULL, delim, &last); 
	return nextToken; 
}

void ControlInterface::setSched(Scheduler* sched)
{
	_sched = sched;
}

void ControlInterface::setSystemController(SystemControl* controller)
{
	_systemController = controller;
}
/* Application methods below */

void ControlInterface::getVersion()
{
	CTRL_SERIAL.print(F("Firmware version: "));
	CTRL_SERIAL.println(HOLST_VERSION);
}

void ControlInterface::getStatus()
{
	// read the system board information out
	CTRL_SERIAL.println(F("System Status"));
	CTRL_SERIAL.printf(F("Firmware Revision: %s ; HW Revision: %d\r\n"),HOLST_VERSION,HW_REV);
	CTRL_SERIAL.printf(F("Temperature: %.2f\r\n"),_systemController->getTemperatureC());
	CTRL_SERIAL.printf(F("CPU Temperature: %.2f\r\n"),_systemController->getInternalTemperatureC());
	if (_systemController->isStopped())
	{
		CTRL_SERIAL.println(F("ESTOP is engaged"));
	}
}

void ControlInterface::runSchedule()
{
	CTRL_SERIAL.println("Telling the schedule to run");
	if (_sched != NULL)
	{
		_sched->start();
	}
}

void ControlInterface::stopSchedule()
{
	CTRL_SERIAL.println("Telling the schedule to run");
	if (_sched != NULL)
	{
		_sched->stop();
	}
}

void ControlInterface::relayToggle()
{
	String relayNumberStr = String(next());
	String stateStr = String(next());
	unsigned long relayNumber = relayNumberStr.toInt();
	unsigned long state = 0;
	if (stateStr.equals("1"))
	{
		state = 100;
	}
	_systemController->setRelay(relayNumber,state,0);

}

void ControlInterface::debugEnable(bool offon)
{
	if (_sched != NULL)
	{
		_sched->_debugging = offon;
	}
}

void ControlInterface::controlLogging(bool offon)
{
	if (_systemController != NULL)
	{
		_systemController->loggingEnable(offon);
	}
}

void printDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    CTRL_SERIAL.print('0');
  CTRL_SERIAL.print(digits);
}

void printTime()
{
	time_t t = now();
	printDigits(hour(t));
	CTRL_SERIAL.print(":");
	printDigits(minute(t));
	CTRL_SERIAL.print(":");
	printDigits(second(t));
	CTRL_SERIAL.print(" ");
	CTRL_SERIAL.print(day(t));
	CTRL_SERIAL.print("/");
	CTRL_SERIAL.print(month(t));
	CTRL_SERIAL.print("/");
	CTRL_SERIAL.print(year(t)); 
}

void ControlInterface::getTime()
{
	printTime();
	CTRL_SERIAL.println();

	if (timeStatus() == timeNotSet)
	{
		CTRL_SERIAL.println("Time is NOT set from RTC!");
	} else if (timeStatus() == timeSet)
	{
		CTRL_SERIAL.println("Time has been set from RTC");
	} else if (timeStatus() ==timeNeedsSync)
	{
		CTRL_SERIAL.println("Time has been set, but sync has failed, so it may not be accurate");
	}
}

void ControlInterface::print2digits(int number) {
	  if (number >= 0 && number < 10) {
		CTRL_SERIAL.write('0');
	  }
	  CTRL_SERIAL.print(number);
}

void ControlInterface::intSetTime()
{
	int Hour, Min, Sec;
	tmElements_t tm;
	time_t oldTime = now();
	//time_t newTime;
	breakTime(oldTime,tm);
	char* inStr = next();
	
	if (sscanf(inStr,"%d:%d:%d:",&Hour,&Min,&Sec) !=3)
	{
		CTRL_SERIAL.println("Couldn't read your time, did you use the right format? HH:MM:SS");
		return;
	}
	//tm.Hour = Hour;
	//tm.Minute = Min;
	//tm.Second = Sec;
	//newTime = makeTime(tm);
	setTime( Hour, Min, Sec, tm.Day, tm.Month, tm.Year );				// set the System Time. 
	Teensy3Clock.set( now() ); 	// set the RTC
	if( timeStatus() == timeSet)
	{
		CTRL_SERIAL.println("Set RTC successfully!");
		getTime();
	}
}

void ControlInterface::intSetDate()
{
	tmElements_t tm;
	time_t oldTime = now();
	//time_t newTime;
	breakTime(oldTime,tm);
	int Month, Day, Year;
	char* inStr = next();

	if (sscanf(inStr, "%d/%d/%d",  &Day, &Month, &Year) != 3) 
	{
	  CTRL_SERIAL.println("Couldn't read your date, did you use the right format? DD/MM/YYYY");
	  return;
	};

	tm.Day = Day;
	tm.Month = Month;
	tm.Year = CalendarYrToTm(Year);
	setTime( hour(), minute(), second(), Day, Month, Year );				// set the System Time. 
	Teensy3Clock.set( now() );
	
	if( timeStatus() == timeSet)
	{
		CTRL_SERIAL.println("Set RTC successfully!");
		getTime();
	}
}



void ControlInterface::addSeq()
{
	char* seqId = next();
	currentSeqId = strtol(seqId,NULL,10);
	CTRL_SERIAL.printf("Adding sequence %d\n",currentSeqId);
}

void ControlInterface::clearSeq()
{
	// all of them!
	CTRL_SERIAL.println("Removing ALL sequences!");
	_sched->sequenceClearAll();
}

void ControlInterface::listSeq()
{
	// do a list of all available sequences
	Sequence *seq;
	int i;
	CTRL_SERIAL.println("Sequences");
	for(i=0;i<_sched->_numSequences;i++)
	{
		seq = &_sched->_sequences[i];
		CTRL_SERIAL.printf("%i [%d] (num cues: %d)\n",seq->sequenceId,i,seq->numCues);
	}
	CTRL_SERIAL.println("----------------------");
}

void ControlInterface::addCue()
{
	char* cue = next();
	String cueString = String(cue);
	if (!cueString.substring(0,1).equals("$"))
	{
		CTRL_SERIAL.println("Invalid cue specification, must start with $");
	}
	_sched->sequenceAppendCue(currentSeqId,cue);
}

void ControlInterface::listCues()
{
	int i;
	Sequence *seq = _sched->sequenceGet(currentSeqId);
	if (seq == NULL)
	{
		CTRL_SERIAL.printf("Sequence %i not yet created, add a cue first please!",currentSeqId);
		return;
	}
	CTRL_SERIAL.printf("Sequence: %i (total cues: %d)\n",currentSeqId,seq->numCues);
	for (i=0;i<seq->numCues;i++)
	{
		CTRL_SERIAL.printf("%s\n",seq->cues[i]);
	}
	CTRL_SERIAL.print("------------------------------------\n");
}

void ControlInterface::printSeq(long seqIdToPrint)
{
	CTRL_SERIAL.printf("I'm printing sequence %d\n",seqIdToPrint);
	// print a sequence
}

void ControlInterface::addSched()
{

	char* strSeqId = next();
	char* schedDef = next();
	long seqId = strtol(strSeqId,NULL,10);
	CTRL_SERIAL.printf("Adding a schedule for sequence %d - %s\n",seqId,schedDef);
	_sched->scheduleAdd(seqId,schedDef);
}

void ControlInterface::clearSched()
{
	CTRL_SERIAL.println("Clearing the schedule!");
	_sched->scheduleClear();
	CTRL_SERIAL.println("Cleared");
}

void ControlInterface::saveSched()
{
	CTRL_SERIAL.println("Saving schedule to SD card");
	if (_sched->saveToSD())
	{
		CTRL_SERIAL.println("Saved.");
	} else {
		CTRL_SERIAL.println("Did NOT save successfully!");
	}
}

void ControlInterface::loadSched()
{
	CTRL_SERIAL.println("Loading schedule and sequences from SD card");
	if(!_sched->loadFromSD())
	{
		CTRL_SERIAL.println("Did NOT load schedule successfully!");
	} else {
		CTRL_SERIAL.println("Loaded.");
	}
}

void ControlInterface::listSched()
{
	// do a list of all available sequences
	Schedule *mySched;
	int i;
	CTRL_SERIAL.println("Schedules");
	for(i=0;i<_sched->_numSchedules;i++)
	{
		mySched = &_sched->_schedule[i];
		CTRL_SERIAL.printf("[%d] Seq:%i (%s)\n",i,mySched->sequenceId,mySched->schedDef);
	}
	CTRL_SERIAL.println("----------------------");
}

void ControlInterface::listRunningSeq()
{
	RunningSequence *ptr;
	int i;
	CTRL_SERIAL.println("Running Sequences");
	for (i=0;i<SCHEDULER_MAX_RUNNING_SEQUENCES;i++)
	{
		ptr = _sched->_currentlyRunningSlots[i];
		if (ptr == NULL)
			continue;
		CTRL_SERIAL.printf("Sequence [%d] started %d\r\n", ptr->running->sequenceId, ptr->milliStarted);
	}
}

void ControlInterface::printTimestamp()
{
  // digital clock display of the time
	CTRL_SERIAL.print("[");
	printTime();
	CTRL_SERIAL.print("] - ");
}

void ControlInterface::setMotor()
{
	char* inDevId = next();
	char* inPercent = next();
	char devId = (char) String(inDevId).toInt();
	unsigned long percent = String(inPercent).toInt();
	char* direction = next();
	if (direction == NULL)
	{	
		_systemController->sendMotorCommand(devId,percent,0);
	} else {
		if (isIt(direction,"FWD"))
		{
			_systemController->sendMotorCommand(devId,percent,0,true);
		} else if (isIt(direction,"REV"))
		{
			_systemController->sendMotorCommand(devId,percent,0,false);
		}
	}
	printTimestamp();
	CTRL_SERIAL.printf("Set motor %d to %d%%\n",devId,percent);
}

void ControlInterface::printLog(String logline)
{
	printTimestamp();
	CTRL_SERIAL.println(logline);
}

void ControlInterface::setClockManager(ClockManager* clockManager)
{
		_clockManager = clockManager;
}

void ControlInterface::setMotorControl(MotorControl* motors)
{
	_motors = motors;
}

void ControlInterface::printMotor()
{
	char* strMotorId = next();
	char devId = String(strMotorId).toInt();
	if (_motors == NULL)
	{
		CTRL_SERIAL.println("No motor controller running!");
		return;
	}
	_motors->printMotor(devId);
}


void ControlInterface::printDmx()
{
	char* param = next();
	int channel = String(param).toInt();
	if (channel == 0)
	{
		_systemController->printDmxChannels();
	} else {
		_systemController->printDmxChannel(channel);
	}
}
void ControlInterface::printI2CDevices()
{
	_systemController->printI2CDevices();
}

void ControlInterface::setDmx()
{
	char* param = next();
	char channel = (char) String(param).toInt();
	param = next();
	char value = (char) String(param).toInt();
	_systemController->setDMX(channel,value);
}