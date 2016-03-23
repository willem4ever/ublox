
#ifndef UBLOX_H
#define UBLOX_H

#include "Arduino.h"
#include "Wire.h"
#include <stdio.h>
#include <stdarg.h>
#include <math.h>

typedef struct __attribute__((packed,aligned(1))) NavigationPositionVelocityTimeSolution {
	uint32_t	iTOW;		// GPS time of week of the navigation epoch.
	uint16_t	year;		// Year UTC
	uint8_t		month;		// Month, range 1..12 (UTC)
	uint8_t		day;		// Day of month, range 1..31 (UTC)
	uint8_t		hour;		// Hour of day, range 0..23 (UTC)
	uint8_t		minute;		// Minute of hour, range 0..59 (UTC)
	uint8_t		seconds;	// Seconds of minute, range 0..60 (UTC)
	uint8_t		valid;		// Validity Flags (see graphic below)
	uint32_t	tAcc;		// Time accuracy estimate (UTC)
	int32_t		nano;		// Fraction of second, range -1e9 .. 1e9 (UTC)
	uint8_t		fixType;	// GNSSfix Type, range 0..5
	uint8_t		flags;		// Fix Status Flags
	uint8_t		reserved1;	// Reserved
	uint8_t		numSV;		// Number of satellites used in Nav Solution
	int32_t		lon;		// Longitude
	int32_t		lat;		// Latitude
	int32_t		height;		// Height above Ellipsoid
	int32_t		hMSL;		// Height above mean sea level
	uint32_t	hAcc;		// Horizontal Accuracy Estimate
	uint32_t	vAcc;		// Vertical Accuracy Estimate
	int32_t		velN;		// NED north velocity
	int32_t		velE;		// NED east velocity
	int32_t		velD;		// NED down velocity
	int32_t		gSpeed;		// Ground Speed (2-D)
	int32_t		heading;	// Heading of motion 2-D
	uint32_t	sAcc;		// Speed Accuracy Estimate
	int32_t		headingAcc;	// Heading Accuracy Estimate
	int32_t		pDOP;		// Position DOP
	uint16_t	reserved2;	// Position DOP
	uint32_t	reserved3;	// Reserved
} NavigationPositionVelocityTimeSolution;

class uBlox {

public:
	NavigationPositionVelocityTimeSolution *NavPvt;
	//
	uBlox	(TwoWire& ,uint8_t);
	void	CfgMsg(uint8_t,uint8_t, uint8_t);
	int		available();
	int		process(uint8_t);

private:
	int send(uint8_t *,int);
	TwoWire& _Wire;
	uint8_t _address;
	int	state = 0;
	uint16_t plLength;
	uint16_t Id;
	uint8_t	*p,buffer[128];


};



#endif // UBLOX_H
