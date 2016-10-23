/*

	Scheduler.cpp
	
	Causes things to happen at the right time.

*/
#include "Scheduler.h"
#include "SystemControl.h"
#include "ClockManager.h"

#include "SystemControl.h"
#include <Time.h>
#include <SdFat.h>
#include <SPI.h>

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

SdFat sd;

void schedPrintDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    Serial.print('0');
  Serial.print(digits);
}

void schedPrintTimestamp()
{
	time_t time = now();
		Serial.print("SCHED>");
		schedPrintDigits(hour(time));
		Serial.print(":");
		schedPrintDigits(minute(time));
		Serial.print(":");
		schedPrintDigits(second(time));
		Serial.print(" ");
		Serial.print(day(time));
		Serial.print("/");
		Serial.print(month(time));
		Serial.print("/");
		Serial.print(year(time));
		Serial.print(" - ");
}


void Scheduler::debug(String log)
{
	if (_debugging)
	{
		schedPrintTimestamp();
		Serial.println(log);
	}
}


void Scheduler::notify(String log)
{
	schedPrintTimestamp();
	Serial.println("[NOTICE] " + log + "\n");
}


void rtrim(char* instr)
{
	char *ptr = instr;
	//wind to the end of the string
	//Serial.printf("before: '%s'",ptr);
	while (*ptr != '\0')
	{ ptr++; }
	while (ptr > instr)
	{
		if (*ptr == '\n' || *ptr == '\r' || *ptr == ' ' || *ptr == '\t')
		{
			*ptr = '\0';
		} 
		ptr--;
	}
	//Serial.printf("after: '%s'",ptr);
}

Scheduler::Scheduler()
{
	int i;
	_message = "Started";
	_numSchedules = 0;
	_numSequences = 0;
	_numRunningSequences = 0;
	// place available running sequence slots into _availableRunningSlots
	for (i=0;i< SCHEDULER_MAX_RUNNING_SEQUENCES;i++)
	{
		_availableRunningSlots[i] = &_runningSequences[i];
	}
	_running = false;
	_debugging = false;
	_controller = NULL;
	scheduleClear();
	sequenceClearAll();
}

void Scheduler::start()
{
	_running = true;
}

void Scheduler::stop()
{
	_running = false;
}

bool Scheduler::loadFromSD()
{
	SdFile scheduleFile;
	SdFile sequenceFile;
	size_t fileCount = 0;
	uint16_t fileIndex[SCHEDULER_MAX_FILE_COUNT];
	dir_t dir;
	int numSequences,numScheds = 0;
	char seqFilename[13];
	char header[12];
	char cueStr[25];
	char schedStr[25];
	long seqId;
	int n;
	
	if (!sd.begin(SDCARD_CHIPSELECT, SPI_QUARTER_SPEED)) {
		sd.initErrorPrint();
		return false;
	}
	
	if (!scheduleFile.open("SCHEDULE.DAT", O_READ))
	{
		sd.errorPrint("couldn't open schedule file");
		return false;
	}
	
	scheduleClear();
	scheduleFile.fgets(header,sizeof(header));
	numScheds = 0;
	while ((n = scheduleFile.fgets(schedStr, sizeof(schedStr))) > 0) {
		char* pEnd;
		seqId = strtol(schedStr,&pEnd,10);
		rtrim(++pEnd);
		if (_debugging)
		{
			schedPrintTimestamp();
			Serial.printf(F("SDLOAD SCHED: [ %i ] %s\n"),seqId,pEnd);
		}
		sequenceAdd(seqId);
		scheduleAdd(seqId,pEnd);
		numScheds++;
	}

	sequenceClearAll();
	// start at beginning of root directory
	sd.vwd()->rewind();
	while (sd.vwd()->readDir(&dir) == sizeof(dir)) {
		if (strncmp((char*)&dir.name[8], "SEQ", 3)) continue;
		if (fileCount >= SCHEDULER_MAX_FILE_COUNT) return false;
		fileIndex[fileCount++] = sd.vwd()->curPosition()/sizeof(dir) - 1;
	}
	numSequences=0;
	for (size_t i = 0; i < fileCount; i++) {
		if (!sequenceFile.open(sd.vwd(), fileIndex[i], O_READ)) return false;
		sequenceFile.getName(seqFilename,13);
		if (_debugging)
		{
			schedPrintTimestamp();
			Serial.printf("Opened: %s\n",seqFilename);
		}
		sequenceFile.fgets(header,sizeof(header));
		seqId = strtol(seqFilename,NULL,10);
		//Serial.printf("Sequence ID: %i\n",seqId);
		sequenceAdd(seqId);
		while ((n = sequenceFile.fgets(cueStr,sizeof(cueStr))) > 0)
		{
			char* pEnd;
			long stepInd = strtol(cueStr,&pEnd,10);
			pEnd++;
			rtrim(pEnd);
			if (_debugging)
			{
				schedPrintTimestamp();
				Serial.printf("SDLOAD SEQ: [ %i/%i ] %s\n",i,stepInd,pEnd);
			}
			sequenceAppendCue(seqId,pEnd);
		}
		numSequences++;
		sequenceFile.close();
	}
	schedPrintTimestamp();
	Serial.printf("Loaded %d sequences and %d schedules", numSequences,numScheds);
	return true;	
}

bool Scheduler::saveToSD()
{
	SdFile schedFile;
	SdFile sequenceFile;
	Schedule *mySched;
	int i,j;
	// Initialize SdFat or print a detailed error message and halt
	// Use half speed like the native library.
	// change to SPI_FULL_SPEED for more performance.
	if (!sd.begin(SDCARD_CHIPSELECT, SPI_QUARTER_SPEED)) {
		sd.initErrorPrint();
		return false;
	}
	// open the file for write at end like the Native SD library
	sd.remove("SCHEDULE.DAT"); // delete the old one first
	if (!schedFile.open("SCHEDULE.DAT", O_RDWR | O_CREAT )) {
		sd.errorPrint("opening schedule for write failed");
		return false;
	}
	schedFile.print("SCHEDULE\r\n");
	for(i=0;i<_numSchedules;i++)
	{
		mySched = &_schedule[i];
		schedFile.printf("%d ",mySched->sequenceId);	
		schedFile.write(mySched->schedDef);
		schedFile.write("\r\n");
	}
	schedFile.close();
	for (i=0;i<_numSequences;i++)
	{
		Sequence *seq = &_sequences[i];
		// save all the sequences to files
		char filename[32];
		sprintf(filename,"%04lu.SEQ",seq->sequenceId);
		sd.remove(filename); // delete the old one first (if it exists)
		if (!sequenceFile.open(filename,O_RDWR | O_CREAT))
		{
			sd.errorPrint("couldn't open sequence for writing");
			return false;
		}
		sequenceFile.printf("%l",seq->sequenceId);
		sequenceFile.write("\r\n");
		for (j=0;j<seq->numCues;j++)
		{
			sequenceFile.printf("%d ",j);
			sequenceFile.write(seq->cues[j]);
			sequenceFile.write("\r\n");
		}
		sequenceFile.close();
	}
	return true;
}

void Scheduler::sequenceAdd(unsigned long sequenceId)
{
	Sequence *seq;
	if (sequenceGet(sequenceId) != NULL)
	{	// duplicate sequence ID - we didn't mean to add the same number twice!
		return;
	}
	seq = &_sequences[_numSequences++];
	seq->sequenceId = sequenceId;
	seq->numCues = 0;
}


void Scheduler::sequenceClearAll()
{
	_numSequences = 0;
	memset(_sequences, 0, SCHEDULER_MAX_SEQUENCES * sizeof(Sequence));
}

void Scheduler::sequenceAppendCue(unsigned long sequenceId, char* strCueDef)
{
	Sequence *seq = sequenceGet(sequenceId);
	if(seq == NULL)
	{
		sequenceAdd(sequenceId);
		seq = sequenceGet(sequenceId);
	}
	strncpy(seq->cues[seq->numCues++],strCueDef,SCHEDULER_MAX_CUE_LENGTH);
}

Sequence* Scheduler::sequenceGet(unsigned long sequenceId)
{
	int i;
	for (i=0;i<_numSequences;i++)
	{
		if (_sequences[i].sequenceId == sequenceId)
		{
			return &_sequences[i];
		}
	}
	return NULL;
}


void Scheduler::scheduleAdd(unsigned long sequenceId, char* strSchedDef)
{
	Schedule* sched;
	sched = &_schedule[_numSchedules++];
	sched->sequenceId = sequenceId;
	if (sequenceGet(sequenceId) == NULL)
	{
		// sequence hasn't been created yet - lets create it. 
		sequenceAdd(sequenceId);
		
	}
	strncpy(sched->schedDef, strSchedDef, SCHEDULER_MAX_SCHEDULE_LENGTH);
	rtrim(sched->schedDef);
}

void Scheduler::scheduleClear()
{
	_numSchedules = 0;
	memset(_schedule, 0, SCHEDULER_MAX_SCHEDULES * sizeof(Schedule) );
}

const char* Scheduler::getLastMessage()
{
	return _message;
}

void Scheduler::triggerSchedule(time_t t)
{
	// triggers the execution of sequences
	String candidateSched, tempString;
	Schedule *thisSched;
	int i;
	int dowTemp = 0;
	int pos;
	debug("-----------------");
	for (i=0;i<_numSchedules;i++)
	{
		
		pos = 1;
		thisSched = &_schedule[i];
		candidateSched = String(thisSched->schedDef);
		debug("##### Evaluating " + candidateSched);
		if (thisSched->schedDef[0] != '$') {
			debug("Invalid schedule!!!" + candidateSched);
			continue;
		}
		debug("Schedule OK");
		tempString = candidateSched.substring(pos,pos+4);
		pos+=4;

		if (!(tempString.equals("****") || tempString.equals( String( year(t) ) )))
			continue;
		//debug("Matched year");
		
		tempString = candidateSched.substring(pos,pos+2);
		pos+=2;
		if (!(tempString.equals("**") || (tempString.toInt() == month(t))))
			continue;
		//debug("Matched month");
		
		tempString = candidateSched.substring(pos,pos+2);
		pos+=2;
		if (!(tempString.equals("**") || (tempString.toInt() == day(t))))
			continue;
		//debug("Matched Day (of month)");
		
		tempString = candidateSched.substring(pos,pos+3);
		pos+=3;
		if (!tempString.equals("***")) // if it's "every day" then don't check any further.
		{
			if (tempString.equals("MON"))
				dowTemp = 2;
			else if (tempString.equals("TUE"))
				dowTemp = 3;
			else if (tempString.equals("WED")) 
				dowTemp = 4;
			else if (tempString.equals("THU"))
				dowTemp = 5;
			else if (tempString.equals("FRI"))
				dowTemp = 6;
			else if (tempString.equals("SAT"))
				dowTemp = 7;
			else if (tempString.equals("SUN"))
				dowTemp = 1;
			else if (tempString.equals("WDY"))
				dowTemp = 8; // weekdays
			else {
				// invalid...
				continue;
			}
			if (dowTemp == 8)
			{
				if (!(weekday(t) >=1 && weekday(t) <= 6)) // weekdays
				{
					continue;
				}
			} else 
			{
				if (dowTemp != weekday(t))
				{
					continue;
				}
			}
		}
		//debug("Matched day (of week)");
		
		tempString = candidateSched.substring(pos,pos+2);
		pos+=2;
		if (!(tempString.equals("**") || tempString.toInt() == hour(t)))
		{
			continue;
		}
		debug("Matched hour" + tempString);
		
		tempString = candidateSched.substring(pos,pos+2);
		pos+=2;
		debug("Minute: " + tempString);
		if (!(tempString.equals("**") || tempString.toInt() == minute(t)))
		{
			continue;
		}
		debug("Matched minute " + tempString);
		
		tempString = candidateSched.substring(pos,pos+2);
		pos+=2;
		debug("Second: " + tempString);
		if (!(tempString.equals("**") || tempString.toInt() == second(t)))
		{
			continue;
		}
		debug("Matched second " + tempString);
		
		tempString = candidateSched.substring(pos,pos+1);
		pos+=1;
		if (!tempString.equals("%"))
		{
			debug("So close! not a valid schedule, you left out the %");
			continue;
		}
		debug("Time to run sequence " + String(thisSched->sequenceId));
		if (!isRunningSequence(thisSched->sequenceId)) // if it's not already running
		{
			// start it running!
			startSequence(thisSched->sequenceId);
		} 
	}
}

bool Scheduler::isRunningSequence(unsigned long sequenceId)
{
	RunningSequence *ptr;
	int i;	
	for (i=0;(i < _numRunningSequences) && (i < SCHEDULER_MAX_RUNNING_SEQUENCES);i++)
	{
		ptr = _currentlyRunningSlots[i];
		if (ptr->running->sequenceId == sequenceId)
			return true;
	}
	return false;
}

void Scheduler::startSequence(unsigned long sequenceId)
{
	RunningSequence *runSeqPtr;
	Sequence *seq;
	unsigned long milliNow = millis();
	int slot;
	if (_debugging)
	{
		schedPrintTimestamp();
		Serial.printf(F("Starting sequence %i\n"),sequenceId);
	}
	if (_numRunningSequences >= SCHEDULER_MAX_RUNNING_SEQUENCES)
	{
		debug("Too many sequences running!");
		return; // nope, too many sequences running
	}
	// find the first free slot
	runSeqPtr = NULL;
	debug("Looking for a free slot");
	//delay(100);
	for (slot=0;slot<SCHEDULER_MAX_RUNNING_SEQUENCES;slot++)
	{
		if (_availableRunningSlots[slot] != NULL)
		{
			debug("Found a free running sequence slot, " + String(slot) );
			runSeqPtr = _availableRunningSlots[slot];
			_currentlyRunningSlots[slot] = runSeqPtr;  // move the pointer from available slots to running slots.
			_availableRunningSlots[slot] = NULL; // remove it from the list.
			break;
		}
	}
	
	if (runSeqPtr == NULL)
	{
		// no free slots
		notify("No free slots for running sequences!\n");
		return;
	}
	if (_debugging)
	{
		schedPrintTimestamp();
		Serial.printf("Allocated slot %d\n",slot);
	}
	
	_numRunningSequences++;

	seq = sequenceGet(sequenceId);
	runSeqPtr->running = seq;
	runSeqPtr->milliStarted = milliNow;
	runSeqPtr->milliLast = milliNow;
	//notify("Added RunningSequence to list of sequences");
}

void Scheduler::triggerSequence()
{
	// triggers the execution of cues in a sequence
	// don't forget, millis() wraps around after 50 days!
	int i,j;
	unsigned long t = millis();
	unsigned long offset;
	unsigned long absoluteOffset;
	bool stillGoing = false;
	String thisCue;
	String strOffset;
	RunningSequence *ptr;
	debug("Scheduler::triggerSequence()");

	for (i=0;i<SCHEDULER_MAX_RUNNING_SEQUENCES;i++)
	{
		ptr = _currentlyRunningSlots[i];
		if (ptr != NULL)
		{
			//notify("Scheduler::triggerSequence - slot contains sequence " + String(ptr->running->sequenceId));
			// an actual running sequence is in this slot!
			// now check whether there are any cues that should be sent that weren't last time
			stillGoing = false;
			for (j=0;j<ptr->running->numCues;j++)
			{
				thisCue = String(ptr->running->cues[j]);
				strOffset = thisCue.substring(1,6);
				offset = strOffset.toInt()*10;
				absoluteOffset = (offset + ptr->milliStarted);

				if (absoluteOffset >= ptr->milliLast && absoluteOffset < t)
				{
					debug("==== SENDING CUE");
					sendCue(thisCue);
				} else {
					debug("Not sending");
				}
				// if the cue is in the future, we're still going.
				if (absoluteOffset > t)
				{
					debug("Cue in future");
					stillGoing = true;
				}
			}
			ptr->milliLast = t;
			if (!stillGoing)
			{
				debug("Sequence " + String(ptr->running->sequenceId) + " has ended");
				// put the pointer back into the array of available slots. 
				_currentlyRunningSlots[i] = NULL; // make this slot available again
				_availableRunningSlots[i] = ptr; // put the pointer back into the list of available slots

				_numRunningSequences--;
			}
		} else {
			debug("Scheduler::triggerSequence - slot empty");
		}
	}
	
}

void Scheduler::sendCue(String cue)
{
	if (_controller != NULL)
	{
		_controller->issueCue(cue);
	}	
}

void Scheduler::setController(SystemControl *controller)
{
	_controller = controller;
}



bool Scheduler::execute()
{
	time_t time = now();
	if (!_running) // do nothing if we're not running
		return true;
	_message = "Executed!";
	// get the time
	// go through each schedule and see if we're supposed to be executing one of them. 
	triggerSchedule(time);
	triggerSequence();
	return true;
}