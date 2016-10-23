/*

	SystemControl.h
	
	Tells other things to do things
	
*/

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif

#include "SystemConfig.h"
#include "SystemControl.h"
#include <Time.h>
#include "pcf8574.h"
#include "MotorControl.h"
#include <DmxSimple.h>
#include <Wire.h>
PCF8574 relays(0x38);
void init_AT30TS750A();

SystemControl::SystemControl () 
{
	unsigned char i;
	_logging = false;
	// enable DMX
	pinMode(DMX_TXEN,OUTPUT);
	pinMode(DMX_RXEN,OUTPUT);
	
	for (i=0;i<DMX_MAX_CHANNELS;i++)
	{
		_dmxChannels[i] = 0x00;
		//DmxSimple.write(i,0x00);
	}
	_logging = false;
	_ticking = false;
	_estopped = false;
}

void SystemControl::loggingEnable(bool logging)
{
	_logging = logging;
}
void sysCtrlPrintDigits(int digits){
  // utility function for digital clock display: prints preceding colon and leading 0
  if(digits < 10)
    CTRL_SERIAL.print('0');
  CTRL_SERIAL.print(digits);
}

void SystemControl::setMotorController(MotorControl* motors)
{
	_motors = motors;
}

void SystemControl::sendMotorCommand(char devId,unsigned long percent, unsigned long duration)
{
	if (this->_estopped)
		return; // don't sent a command if we're estopped
	_motors->setMotor(devId,true,percent);
}

void SystemControl::sendMotorCommand(char devId,unsigned long percent, unsigned long duration,bool direction)
{
	if (this->_estopped)
		return; // don't sent a command if we're estopped
	_motors->setMotor(devId,direction,percent);	
}

bool SystemControl::isStopped()
{
	return this->_estopped;
}

void SystemControl::eStop()
{
	// STOP EVERYTHING RIGHT NOW
	
	this->_estopped = true;
	_motors->eStopAllMotors();
	CTRL_SERIAL.println("STOPPED MOTORS");
}

void SystemControl::restart()
{
	this->_estopped = false;
	// UNSTOP EVERYTHING RIGHT NOW
	_motors->safeStartAllMotors();
}

void printTimestamp()
{
	time_t time = now();
	CTRL_SERIAL.print("CONTROLLER>");
	sysCtrlPrintDigits(hour(time));
	CTRL_SERIAL.print(":");
	sysCtrlPrintDigits(minute(time));
	CTRL_SERIAL.print(":");
	sysCtrlPrintDigits(second(time));
	CTRL_SERIAL.print(" ");
	CTRL_SERIAL.print(day(time));
	CTRL_SERIAL.print("/");
	CTRL_SERIAL.print(month(time));
	CTRL_SERIAL.print("/");
	CTRL_SERIAL.print(year(time));
	CTRL_SERIAL.print(" - ");
	
}

void SystemControl::printI2CDevices()
{
	byte error, address;
	unsigned int nDevices = 0;
	for(address = 1; address < 127; address++ )
	{
		// The i2c_scanner uses the return value of
		// the Write.endTransmisstion to see if
		// a device did acknowledge to the address.
		Wire.beginTransmission(address);
		error = Wire.endTransmission();

		if (error == 0)
		{
		  CTRL_SERIAL.print(F("I2C device found at address 0x"));
		  if (address<16)
			CTRL_SERIAL.print(F("0"));
		  CTRL_SERIAL.print(address,HEX);
		  CTRL_SERIAL.println(F("  !"));

		  nDevices++;
		}
		else if (error==4)
		{
		  CTRL_SERIAL.print(F("Unknow error at address 0x"));
		  if (address<16)
			CTRL_SERIAL.print("0");
		  CTRL_SERIAL.println(address,HEX);
		}    
	}

	if (nDevices == 0)
		CTRL_SERIAL.println(F("No I2C devices found\n"));
	else
		CTRL_SERIAL.println(F("done\n"));
}

void SystemControl::setRelay(unsigned long devId,unsigned long percent, unsigned long duration)
{
	if (this->_estopped)
		return; // don't sent a command if we're estopped
	if (_logging)
	{
		printTimestamp();
		CTRL_SERIAL.print(F("Setting relay ") + String(devId));
	}
	bool state = false;
	if (devId <= 0 || devId >= 8)
		return;
	devId--;
	if (percent > 0)
	{
		
		state = true;
	}
	// enable on non-zero
	if (state)
	{
		if (_logging)
			CTRL_SERIAL.println(F(" to on"));
		relays.write((uint8_t)devId,0);
	} else {
		if (_logging)
			CTRL_SERIAL.println(F(" to off"));
		relays.write((uint8_t)devId,1);
	}
	
}

void SystemControl::setup()
{
	_motors->motorsInitialise(MC_BAUD);
	init_AT30TS750A();
	digitalWrite(DMX_TXEN,1);
	digitalWrite(DMX_RXEN,1);
	// set up DmxSimple
	DmxSimple.usePin(DMX_USEPIN);
	DmxSimple.maxChannel(DMX_MAX_CHANNELS);

}

void SystemControl::enable()
{
	_motors->eStopAllMotors();
	_motors->safeStartAllMotors();	
}

void SystemControl::setDMX(unsigned char channel, unsigned char brightness)
{
	if (this->_estopped)
		return; // don't change anything if if we're estopped
	if (channel < DMX_MAX_CHANNELS)
	{
		DmxSimple.write(channel, brightness);
		_dmxChannels[channel] = brightness;
	}
}

void SystemControl::printDmxChannels()
{
	unsigned int i;
	for (i=0;i<DMX_MAX_CHANNELS;i++)
	{
		CTRL_SERIAL.printf(F("Ch: %02d : 0x%02x\r\n"),i,_dmxChannels[i]);
	}
}
void SystemControl::printDmxChannel(unsigned char channel)
{
	if (channel < DMX_MAX_CHANNELS) {
		CTRL_SERIAL.printf(F("Ch: %02d : 0x%02x\r\n"),channel,_dmxChannels[channel]);
	}
}

void SystemControl::doStuff()
{
	//unsigned int i;
	if (_ticking)
	{
		if(_lastTime != now())
		{
			printTimestamp();
			CTRL_SERIAL.println();
		}
	}
	_lastTime = now();

}


/*void SystemControl::RTC_compensate()
{
	float temperature = this->getTemperatureC();
	
	
}*/

float SystemControl::getTemperatureC()
{
	byte   tempLSByte       = 0;
	byte   tempMSByte       = 0; 
	float floatTemperature        = 0.00;
	Wire.beginTransmission (0x48);    //Select the AT30TS750 for writing
	Wire.write             (0x00);    //Select the temperature register
	Wire.endTransmission   ();
	Wire.requestFrom       (0x48, 2); //Retrieve two bytes of data
	tempMSByte             = Wire.read();
	tempLSByte             = Wire.read() >> 4;
	floatTemperature       = tempMSByte + (tempLSByte * .0625);
	if (_logging)
	{
		CTRL_SERIAL.print    (F("Raw Output Byte 01: 0x"));
		CTRL_SERIAL.println  (tempMSByte, HEX);
		CTRL_SERIAL.print    (F("Raw Output Byte 02: 0x"));
		CTRL_SERIAL.println  (tempLSByte, HEX);  
		CTRL_SERIAL.print    (F("Raw Output Value 01: "));
		CTRL_SERIAL.println  (tempMSByte, DEC);
		CTRL_SERIAL.print    (F("Raw Output Value 02: "));
		CTRL_SERIAL.println  (tempLSByte * .0625, DEC);  
	}
	Wire.endTransmission();
	return floatTemperature;
}

float SystemControl::getInternalTemperatureC()
{
	float average = 0;
	for (byte counter =0;counter<255;counter++){    
		average = analogRead(38)+average;  
	}
	average= average/255;
	float C = 25.0 + 0.17083 * (2454.19 - average);
	return C;
}


void SystemControl::init_AT30TS750A()
{
	Wire.beginTransmission (0x48);  //Select the AT30TS750 for writing
	Wire.write             (0x01);  //Select the configuration register
	Wire.write             (0x60);  //0x00 = 9bit resolution, 0x60 = 12bit resolution
	Wire.endTransmission   ();	
	this->getTemperatureC(); // read the first time
}



void SystemControl::issueCue(String cue)
{
	String type;
	unsigned long offset;
	unsigned long devId;
	unsigned long percent;
	unsigned long duration;
	
	if (_logging)
	{
		printTimestamp();
		CTRL_SERIAL.println( cue);
	}
	// check that the cue is the right size:
	if (cue.length() != 20)
	{
		if (_logging)
			CTRL_SERIAL.printf(F("Invalid cue! Length is %d\n"),cue.length());
		return;
	}
	// cool.  Does it start with a $ and end with %
	if (!cue.substring(0,1).equals("$"))
	{
		if (_logging)
			CTRL_SERIAL.println(F("Doesn't start with $\n"));
		return;
	}
	if (!cue.substring(19).equals("%"))
	{
		if (_logging)
			CTRL_SERIAL.println(F("Doesn't end with %\n"));
		return;
	}
	// unpack the values;
	offset = cue.substring(1,6).toInt()*10; // offsets are in 10's of ms
	type = cue.substring(6,9);
	devId = cue.substring(9,11).toInt();
	percent = cue.substring(11,14).toInt();
	duration = cue.substring(14,19).toInt();
	if (_logging)
	{
		printTimestamp();
		CTRL_SERIAL.printf(F("Offset: %d; Type: %s; DevID: %d; Percent: %d; Duration: %d\n"),offset,type.c_str(),devId,percent,duration);
	}
	if(type.equals(F("MOT")))
	{
		sendMotorCommand(devId,percent,duration);
	} else if (type.equals(F("REL")))
	{
		setRelay(devId,percent,duration);
	} else if (type.equals(F("DMX")))
	{
		setDMX(devId,percent);
	}
}