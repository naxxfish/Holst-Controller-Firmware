/*

	ClockManager.h
	
	Deals with the RTC and system time

*/

#ifndef CLOCKMANAGER_H
#define CLOCKMANAGER_H
#include <Time.h>

class ClockManager{
	public:
		ClockManager();
		void setup();
		void loop();
		void setTicking(bool onoff);
		time_t getTeensy3Time();
	private:
		bool _ticking;
		time_t _lastTime;
};

#endif