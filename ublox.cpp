/**
 * Copyright 2016 Willem Eradus
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 **/

#include "ublox.h"

#include <Arduino.h>
#include <stdio.h>
#include <stdarg.h>

void uBlox::db_printf(const char *message,...) {
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
    this->reset();
    digitalWrite(GPS_ENABLE, 1);
    db_printf("uBlox enabled\n");
}

void uBlox::disable () {
    this->reset();
    digitalWrite(GPS_ENABLE, 0);
    db_printf("uBlox disabled\n");
}

void uBlox::flush() {
    this->reset();
    uint16_t bytes = this->available();
    if (bytes) {
        Wire.requestFrom(_address, bytes);
        do {
            (void) Wire.read();
        } while (--bytes);
    }
}

void uBlox::reset() {
    state = 0;
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
        if (Id == 0x500 || Id == 0x501) // ACK/NAK buffer
            p = (uint8_t *) &AckedId;
        else {
            payLoad.length = plLength;
            p = payLoad.buffer;
        }
    }
    else if (state == 6) {
        ck_a += c; ck_b += ck_a;
        *p++ = c;
        if (--plLength == 0) {
            state = 7;
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
            if (Id == 0x0501 || Id == 0x500) {
                AckedId = (AckedId >> 8) | (AckedId << 8);  // Change Endianess
                // db_printf("Id=%4.4x Ack=%4.4x\n",Id,AckedId);
            }
            return Id;
        }
    }
    return (0-state);
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

void uBlox::CfgMsg(uint16_t Msg,uint8_t rate) {
    uint8_t buffer[7];
    //
    buffer[0] = 0x06;
    buffer[1] = 0x01;
    buffer[2] = 0x03;
    buffer[3] = 0;
    buffer[4] = (Msg >> 8) & 0xff;
    buffer[5] = Msg & 0xff;
    buffer[6] = rate;
    // Push message on Wire
    int i = this->send(buffer,7);
    this->wait();
}

int uBlox::CfgPrt () {
    uint8_t buffer[4];
    //
    buffer[0] = 0x06;
    buffer[1] = 0x00;
    buffer[2] = 0;
    buffer[3] = 0;
    // Push message on Wire
    int i = this->send(buffer,4);
    return this->wait();
}

int uBlox::CfgPrt (PortConfigurationDDC *pcd) {
    // Warning this overwrites the receive buffer !!!
    payLoad.buffer[0] = 0x06;
    payLoad.buffer[1] = 0x00;
    payLoad.buffer[2] = 20;
    payLoad.buffer[3] = 0;
    memcpy(&payLoad.buffer[4],(uint8_t*)pcd,20);
    // Push message on Wire
    int i = this->send(payLoad.buffer,24);
    return this->wait();
}


int uBlox::CfgTp5 (uint8_t tpIdx) {
    uint8_t buffer[5];
    //
    buffer[0] = 0x06;
    buffer[1] = 0x31;
    buffer[2] = 1;
    buffer[3] = 0;
    buffer[4] = tpIdx;
    // Push message on Wire
    int i = this->send(buffer,5);
    return this->wait();
}

int uBlox::CfgTp5 (TimePulseParameters *Tpp) {
    // Warning this overwrites the receive buffer !!!
    payLoad.buffer[0] = 0x06;
    payLoad.buffer[1] = 0x31;
    payLoad.buffer[2] = 32;
    payLoad.buffer[3] = 0;
    memcpy(&payLoad.buffer[4],(uint8_t*)Tpp,32);
    // Push message on Wire
    int i = this->send(payLoad.buffer,36);
    return this->wait();
}

void *uBlox::getBuffer (uint16_t required) {
    if (payLoad.length == required) // Should at least match
        return payLoad.buffer;
    else {
        this->db_printf("Required=%d, available=%d\n",required,payLoad.length);
        return NULL;
    }
}

uint8_t uBlox::GetResponse(void *d,uint16_t required) {
    if (payLoad.length == required) {
        memcpy(d,payLoad.buffer,required);
        return required;
    }
    return 0;
}

uint16_t uBlox::getAckedId () {
    return AckedId;
}

int uBlox::wait() {
    uint32_t s = millis(),elapsed;
    uint16_t bytes;
    int16_t id = 0;
    // Wait 50 ms for response
    while ((elapsed = millis()-s) < 50 && (bytes = this->available()) == 0 );
    if (bytes) {
        // db_printf("Waited %d ms for %d bytes\n",elapsed,bytes);
        if (Wire.requestFrom(_address, bytes)) {
            do {
                id = this->process(_Wire.read());
            } while (--bytes);
        }
    }
    return id;
}