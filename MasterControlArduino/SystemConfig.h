/*

	SystemConfig.h
	
	File that contains all the System Wide configuration parameters for Holst

*/

#ifndef SYSTEM_H
#define SYSTEM_H

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#define HOLST_VERSION "0.2-alpha"
#define HW_REV 2


#define SDCARD_CHIPSELECT 6

#define OK_LED 5

#define CTRL_SERIAL Serial
#define CTRL_BAUD 9600
#define MC_CTRL_SERIAL Serial2
#define MC_BAUD 9600
#define DMX_SERIAL Serial3

#define DMX_MAX_CHANNELS 128
#define DMX_USEPIN 8
#define DMX_TXEN 22
#define DMX_RXEN 23

#define ESTOP_PIN 16

// Motor limiting - set this up to be the maximum sensible speed BEFORE running schedules!
#define MOTOR_MAX_SPEED 100


// debugging

//#define TIMING_DEBUG false
#undef TIMING_DEBUG


#endif