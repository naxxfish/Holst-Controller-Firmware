/*

	Scheduler.h
	
	Causes things to happen at the right time.

*/
#ifndef SCHEDULER_H
#define SCHEDULER_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SystemConfig.h"
#include "SystemControl.h"
#include <Time.h>


#define SCHEDULER_MAX_SCHEDULES 128
#define SCHEDULER_MAX_SCHEDULE_LENGTH 24
#define SCHEDULER_MAX_CUES 48
#define SCHEDULER_MAX_CUE_LENGTH 24
#define SCHEDULER_MAX_SEQUENCES 20
#define SCHEDULER_MAX_FILE_COUNT 999
#define SCHEDULER_MAX_RUNNING_SEQUENCES 5
// make this non-zero to enable serial debugging


typedef struct _seq {
	long unsigned sequenceId;
	char* sequenceLabel;
	char cues[SCHEDULER_MAX_CUES][SCHEDULER_MAX_CUE_LENGTH];
	int numCues;
} Sequence;

typedef struct _sched {
	long unsigned sequenceId;
	char schedDef[SCHEDULER_MAX_SCHEDULE_LENGTH];
} Schedule;

typedef struct _runningSeq {
	Sequence *running;
	unsigned long milliStarted;
	unsigned long milliLast;
} RunningSequence;

class Scheduler {
	public:
		Scheduler();
		bool loadFromSD();
		bool saveToSD();
		void sequenceAdd(unsigned long  sequenceId);
		void sequenceClear(unsigned long  sequenceId);
		void sequenceClearAll();
		void sequenceAppendCue(unsigned long  sequenceId, char* strCueDef);
		Sequence* sequenceGet(unsigned long  sequenceId);
		void scheduleAdd(unsigned long sequenceId, char* strSchedDef);
		void scheduleClear();
		const char* getLastMessage();
		bool execute(); // run this quite often!
		const char* _message;
		int _numSchedules;
		int _numSequences;
		int _numRunningSequences;
		Sequence _sequences[SCHEDULER_MAX_SEQUENCES];
		Schedule _schedule[SCHEDULER_MAX_SCHEDULES];
		RunningSequence _runningSequences[SCHEDULER_MAX_RUNNING_SEQUENCES];
		RunningSequence *_availableRunningSlots[SCHEDULER_MAX_RUNNING_SEQUENCES] = {NULL, NULL, NULL, NULL, NULL};
		RunningSequence *_currentlyRunningSlots[SCHEDULER_MAX_RUNNING_SEQUENCES] = {NULL, NULL, NULL, NULL, NULL};
		bool _debugging;
		void setController(SystemControl *controller);
		void start();
		void stop();
	private:
		bool _running;
		bool isRunningSequence(unsigned long sequenceId);
		void startSequence(unsigned long sequenceId);
		void triggerSchedule(time_t t);
		void triggerSequence();
		void sendCue(String cue);
		void debug(String log);
		void notify(String log);
		SystemControl *_controller;
};

#endif

