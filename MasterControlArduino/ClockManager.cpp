/*

	ClockManager.cpp
	
	Deals with the RTC and system time
	
*/
#include <TimeLib.h>
#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SystemConfig.h"
#include "ClockManager.h"



ClockManager::ClockManager()
{
	// do stuff
	_ticking = false;
	_lastTime = now();
	return;
}

void clockManPrintDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    CTRL_SERIAL.print('0');
  CTRL_SERIAL.print(digits);
}

void clockManPrintTime()
{
	time_t time = now();
	CTRL_SERIAL.print("[");
	clockManPrintDigits(hour(time));
	CTRL_SERIAL.print(":");
	clockManPrintDigits(minute(time));
	CTRL_SERIAL.print(":");
	clockManPrintDigits(second(time));
	CTRL_SERIAL.print(" ");
	CTRL_SERIAL.print(day(time));
	CTRL_SERIAL.print("/");
	CTRL_SERIAL.print(month(time));
	CTRL_SERIAL.print("/");
	CTRL_SERIAL.print(year(time));
	CTRL_SERIAL.println("] ");
}

time_t ClockManager::getTeensy3Time()
{
  return Teensy3Clock.get();
}

void ClockManager::setup()
{
	CTRL_SERIAL.println("Setting the RTC sync provider");
	setSyncProvider((getExternalTime) Teensy3Clock.get);   // the function to get the time from the RTC
	setSyncInterval(60);
	CTRL_SERIAL.print("Syncing time : ");
	while(timeStatus()!= timeSet)
	{
		CTRL_SERIAL.print(".");
	}
	CTRL_SERIAL.println(" done!");
}

void ClockManager::loop()
{
	// do things
	if (_ticking)
	{
		if(_lastTime != now())
		{
			clockManPrintTime();
		}
	}
	_lastTime = now();
}

void ClockManager::setTicking(bool onoff)
{
	_ticking = onoff;
}

