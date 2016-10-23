#include <Time.h>
#include <Wire.h>
#include <SdFat.h>
#include <SPI.h>
#include <Metro.h>
#include <DmxSimple.h>
#include <Bounce.h>

#include <avr/io.h>
#include <avr/interrupt.h>

#include "SystemConfig.h"
#include "ControlInterface.h"
#include "SystemControl.h"
#include "Scheduler.h"
#include "MotorControl.h"
#include "ClockManager.h"
/* User Interface */
ControlInterface ctrl;

/* Event Scheduler */
Scheduler sched;
Metro schedMetro(50);

/* System Control interface (overall)*/
SystemControl systemControl;
Metro sysControlMetro(250);

/* Motor controll interface */
MotorControl motorControl;
Metro motorRefreshMetro(1000);

/* Clock Manager */
ClockManager clockManager;

elapsedMicros processTimer;

/* LED pulsating */
Metro blinkenMetro(30);
int brightness = 0;    // how bright the LED is
int fadeAmount = 5;    // how many points to fade the LED by

/* ESTOP button */

Bounce eStop = Bounce(ESTOP_PIN,100);;

void setup()
{
	// configure the ESTOP
	// it's quite unlikely the ESTOP code will ever run, as the button immediately cuts mains power.  But if it does... we deal with it! 
	pinMode(ESTOP_PIN, INPUT_PULLUP);
	
	/* LED on the Teensy board */
	pinMode(13,OUTPUT);
	/* OK LED on Holst controller board */
	pinMode(OK_LED,OUTPUT);
	
	digitalWrite(13,HIGH); 		// both LEDs are on during boot
	digitalWrite(OK_LED,HIGH);
	delay(1000);				// wait for a bit for all the subsystems to power up and enable
	
	ctrl.start(CTRL_BAUD);
	/* set up for internal temperature measurement */ 
	analogReference(INTERNAL);
	analogReadResolution(12);
  
	clockManager.setup();
	
	ctrl.printLog("Loading schedule from SD card");
	digitalWrite(13,LOW); // need to turn the LED off before we use the SPI bus
	//sched._debugging = true;
	sched.loadFromSD();
	//sched._debugging = false;
	ctrl.printLog("Schedule loaded.");
	ctrl.printLog("Connecting scheduler to system controller");
	
	// link all of the components to each other
	sched.setController(&systemControl);
	ctrl.setSystemController(&systemControl);
	systemControl.setMotorController(&motorControl);
	ctrl.setMotorControl(&motorControl);
	ctrl.setClockManager(&clockManager);
	
	// initialise each of the components
	ctrl.printLog("Enabling system control");
	systemControl.setup();
	ctrl.printLog("Controller connected");
	ctrl.setSched(&sched);
	ctrl.printLog("Starting scheduler");
	
	// Configure the system now
	ctrl.printLog("Adding motors");
	motorControl.addMotor(10);
	motorControl.addMotor(11);
	motorControl.addMotor(12);
	motorControl.addMotor(13);
	motorControl.addMotor(14);
	motorControl.addMotor(15);
	motorControl.addMotor(16);
	motorControl.addMotor(17);
	motorControl.addMotor(18);
	//motorControl.addMotor(14);
	//motorControl.printMotor(13);

	// Hand control over to systemControl
	systemControl.enable();
	
	// check if eSTOP is engaged
	if (digitalRead(ESTOP_PIN) == LOW)
	{
		systemControl.eStop();
	}
	
	// start the scheduler
	sched.start();
	// allow the control interface to interact with users
	ctrl.interactive();
	
}


void loop()
{
	clockManager.loop();
	ctrl.readSerial();
	eStop.update();
	if (eStop.fallingEdge())
	{
		// emergency stop!!!!
		systemControl.eStop();
		CTRL_SERIAL.println("ESTOP engaged");

		}
	if (eStop.risingEdge())
	{
		systemControl.restart();
		CTRL_SERIAL.println("Reset motors");
		
	}
	
	if (schedMetro.check() == 1)
	{
		processTimer=0;
		sched.execute();
#ifdef TIMING_DEBUG
		CTRL_SERIAL.printf("Sched executed in ");
		CTRL_SERIAL.print(processTimer);
		CTRL_SERIAL.println(" us");
#endif
	}
	if (sysControlMetro.check() == 1)
	{
		processTimer=0;
		systemControl.doStuff();
#ifdef TIMING_DEBUG
		CTRL_SERIAL.printf("SystemControl executed in ");
		CTRL_SERIAL.print(processTimer);
		CTRL_SERIAL.println(" us");
#endif
	}
	if (motorRefreshMetro.check() == 1)
	{
		processTimer=0;
		motorControl.refreshAllMotors();
#ifdef TIMING_DEBUG
		CTRL_SERIAL.printf("MotorControl executed in ");
		CTRL_SERIAL.print(processTimer);
		CTRL_SERIAL.println(" us");
#endif
	}
	
	if (blinkenMetro.check() == 1)
	{
		analogWrite(OK_LED,brightness);
		brightness = brightness + fadeAmount;
		if (brightness == 0 || brightness == 255) {
			fadeAmount = -fadeAmount ;
		}

	}
	
	

	
}


