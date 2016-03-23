#include "ublox.h"


void db_printf(const char *message,...) {
	static char buffer[128];
	va_list args;
	va_start(args, message);
	vsnprintf(buffer,128, message, args);
	SerialUSB.print(buffer);
	va_end(args);
}

uBlox::uBlox (TwoWire& Wire,uint8_t address):
	_Wire(Wire) {
	_address = address;
	//
	pinMode(GPS_ENABLE, OUTPUT);
	pinMode(GPS_TIMEPULSE, INPUT);
}

void uBlox::enable () {
	digitalWrite(GPS_ENABLE, 1);
	db_printf("uBlox enabled\n");
}

void uBlox::disable () {
	digitalWrite(GPS_ENABLE, 0);
	db_printf("uBlox disabled\n");
}

void uBlox::flush() {
	uint16_t bytes = this->available();
	if (bytes) {
		Wire.requestFrom(_address, bytes);
		do {
			(void) Wire.read();
		} while (--bytes);
	}
}

int uBlox::process(uint8_t c) {
	static uint8_t ck_a,ck_b;
	if (state == 0 && c == 0xb5)
		state = 1;
	else if (state == 1) {
		if (c == 0x62)
			state = 2;
		else
			state = 0;
	}
	else if (state == 2) {
		ck_a = c; ck_b = c;
		state = 3;
		Id = (uint16_t) c << 8;
	}
	else if (state == 3) {
		ck_a += c; ck_b += ck_a;
		state = 4;
		Id |= (uint16_t) c;
		// db_printf("Id=%4.4x\n",Id);
	}
	else if (state == 4) {
		ck_a += c; ck_b += ck_a;
		plLength = c;
		state = 5;
	}
	else if (state == 5) {
		ck_a += c; ck_b += ck_a;
		plLength |= (uint16_t) c << 8;
		state = 6;
		// db_printf("length=%4.4x\n",plLength);
		p = buffer;
	}
	else if (state == 6) {
		ck_a += c; ck_b += ck_a;
		*p++ = c;
		if (--plLength == 0) {
			state = 7;
			// db_printf("ck_a=0x%2.2x ck_b=0x%2.2x\n",ck_a,ck_b);
		}
	}
	else if (state == 7) {
		if (c == ck_a)
			state=8;
		else
			state = 0;
	}
	else if (state == 8) {
		state = 0;
		if (ck_b == c) {
			// db_printf("CHK_OK\n");
			if (Id == 0x0107) {
				NavPvt = (NavigationPositionVelocityTimeSolution* )buffer;
				db_printf("%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%d lat=%d lon=%d sats=%d\n",
						  NavPvt->year,NavPvt->month,NavPvt->day,
						  NavPvt->hour,NavPvt->minute,NavPvt->seconds,NavPvt->nano,
						  NavPvt->lat,NavPvt->lon,NavPvt->numSV);
			}
			else
				db_printf("Processed Id=0x%4.4x\n",Id);
			return -1;

		}
	}
	return state;
}

int uBlox::available() {
	_Wire.beginTransmission(_address);
	_Wire.write((uint8_t)0xfd);
	_Wire.endTransmission(false);
	_Wire.requestFrom(_address, 2);//
	uint16_t bytes = (uint16_t) _Wire.read() << 8;
	bytes |= _Wire.read();
	return bytes;
}

int uBlox::send(uint8_t *buffer,int n) {
	uint8_t ck_a,ck_b;
	ck_a = ck_b = 0;
	//
	for (int i=0;i<n;i++) {
		ck_a += buffer[i];
		ck_b += ck_a;
	}
	//
	_Wire.beginTransmission(_address);
	_Wire.write(0xb5);
	_Wire.write(0x62);
	_Wire.write(buffer, n);
	_Wire.write(ck_a);
	_Wire.write(ck_b);
	return _Wire.endTransmission();
}

void uBlox::CfgMsg(uint8_t MsgClass, uint8_t MsgID,uint8_t rate) {
	uint8_t buffer[7];
	//
	buffer[0] = 0x06;
	buffer[1] = 0x01;
	buffer[2] = 0x03;
	buffer[3] = 0;
	buffer[4] = MsgClass;
	buffer[5] = MsgID;
	buffer[6] = rate;
	// Push message on Wire
	int i = uBlox::send(buffer,7);
	//
	uint32_t s = millis(),elapsed;
	uint16_t bytes;
	// Wait for (N)ACK
	while ((elapsed = millis()-s) < 10 && (bytes = this->available()) == 0 );
	if (bytes) {
		db_printf("Waited %d ms for %d bytes\n",elapsed,bytes);
		if (Wire.requestFrom(_address, bytes)) {
			do {
				this->process(_Wire.read());
			} while (--bytes);
			// Ack or Nack ...
		}
	}


}

