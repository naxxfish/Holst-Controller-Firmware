/*

	SystemControl.h
	
	Tells other things to do things
	
*/
#ifndef SYSTEMCONTROL_H
#define SYSTEMCONTROL_H
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "pcf8574.h"
#include "SystemConfig.h"
#include "MotorControl.h"

class SystemControl{
	public:
		SystemControl();
		void issueCue(String cue);
		void sendMotorCommand(char devId,unsigned long percent, unsigned long duration);
		void sendMotorCommand(char devId,unsigned long percent, unsigned long duration,bool direction);
		void setRelay(unsigned long devId,unsigned long percent, unsigned long duration);
		void setup();
		void enable();
		void loggingEnable(bool logging);
		void setMotorController(MotorControl* motors);
		void eStop();
		void restart();
		void setDMX(unsigned char channel, unsigned char brightness);
		void doStuff();
		void printDmxChannels();
		void printDmxChannel(unsigned char channel);
		void printI2CDevices();
		float getTemperatureC();
		float getInternalTemperatureC();
		bool isStopped();
		
	private:
		time_t _lastTime;
		bool _logging;
		bool _estopped;
		bool _ticking;
		MotorControl* _motors;
		char _dmxChannels[DMX_MAX_CHANNELS];
		void init_AT30TS750A();
		/*void RTC_compensate();*/
};
#endif

