
#include <Wire.h>
#include <Sodaq_RN2483.h>
#include <ublox.h>
#include <stdio.h>
#include <stdarg.h>
#include <time.h>

#define loraSerial Serial
#define debugSerial SerialUSB

enum LedColor {
  RED = 1,
  GREEN,
  BLUE
};

uint8_t buffer[128];
uint32_t epoch  = 0;
uint32_t uptime = 0;

UBlox uBlox;

void scan () {

  int nDevices = 0;
  for (int address = 1; address < 127; address++ )
  {
    Wire.beginTransmission(address);
    int error = Wire.endTransmission();

    if (error == 0)
    {
      SerialUSB.print("I2C device found at address 0x");
      if (address < 16)
        SerialUSB.print("0");
      SerialUSB.print(address, HEX);
      SerialUSB.println("  !");

      nDevices++;
    }
    else if (error == 4)
    {
      SerialUSB.print("Unknow error at address 0x");
      if (address < 16)
        SerialUSB.print("0");
      SerialUSB.println(address, HEX);
    }
  }
  if (nDevices == 0)
    SerialUSB.println("No I2C devices found\n");
  else
    SerialUSB.println("done\n");

}

// Fast way to manipulate RGB leds
void Led (uint8_t color) {
  // Switch Off
  REG_PORT_OUTSET0 = (uint32_t) 0x8000;     // RED
  REG_PORT_OUTSET1 = (uint32_t) 0x0c00;     // GREEN & BLUE
  // Switch Requested color on
  if (color == RED)
    REG_PORT_OUTCLR0 = (uint32_t) 0x8000;   // RED
  else if (color == GREEN)
    REG_PORT_OUTCLR1 = (uint32_t) 0x0400;   // GREEN
  else if (color == BLUE)
    REG_PORT_OUTCLR1 = (uint32_t) 0x0800;   // BLUE

}

void ISR () {
  epoch++;
  uptime++;
}

void setup() {
  while ( !SerialUSB &&  millis() < 7500);
  delay(500);
  //
  pinMode(LED_RED, OUTPUT);
  pinMode(LED_GREEN, OUTPUT);
  pinMode(LED_BLUE, OUTPUT);
  Led(0);
  //
  uBlox.enable();
  attachInterrupt(GPS_TIMEPULSE, ISR, RISING);
  //
  Wire.begin();
  // Wire.setClock(400000);
  scan();
  delay(100);
  //
  uBlox.flush();
  //

  TimePulseParameters xnew;
  if (uBlox.getTimePulseParameters(0, &xnew)) {
    xnew.freqPeriod    = 1000000;
    xnew.pulseLenRatio = 100000;
    uBlox.db_printf(">>freqPeriod=%d freqPeriodLock=%d pulseLenRatio=%d pulseLenRatioLock=%d flags=%4.4x\n", xnew.freqPeriod, xnew.freqPeriodLock, xnew.pulseLenRatio, xnew.pulseLenRatioLock, xnew.flags);
    uBlox.setTimePulseParameters(&xnew);
    //
    memset(&xnew, 0, sizeof(TimePulseParameters));      // Wipe structure
    if (uBlox.getTimePulseParameters(0, &xnew))         // Reread TimePulseParameters
      uBlox.db_printf("<<freqPeriod=%d freqPeriodLock=%d pulseLenRatio=%d pulseLenRatioLock=%d flags=%4.4x\n", xnew.freqPeriod, xnew.freqPeriodLock, xnew.pulseLenRatio, xnew.pulseLenRatioLock, xnew.flags);
  }
  
  PortConfigurationDDC pcd;
  if (uBlox.getPortConfigurationDDC(&pcd)) {
    uBlox.db_printf("portID=%x txReady=%x mode=%x inProtoMask=%4.4x outProtoMask=%4.4x flags=%4.4x reserved5=%x\n", pcd.portID, pcd.txReady, pcd.mode, pcd.inProtoMask, pcd.outProtoMask, pcd.flags, pcd.reserved5);
    pcd.outProtoMask = 1;           // Disable NMEA
    uBlox.setPortConfigurationDDC(&pcd);
  }
  else
    uBlox.db_printf("uBlox.getPortConfigurationDDC(&pcd) == false\n");

  uBlox.CfgMsg(UBX_NAV_PVT, 1);      // Navigation Position Velocity TimeSolution
  uBlox.funcNavPvt = delegateNavPvt;
  //
  SerialUSB.println("Hit any key to enter loop or wait 5000 ms");
  uint32_t s = millis();
  while (SerialUSB.read() == -1 && millis() - s < 5000);
  //
  SerialUSB.println("Entering loop() now");
}

void delegateNavPvt(NavigationPositionVelocityTimeSolution* NavPvt) {

  uBlox.db_printf("%4.4d-%2.2d-%2.2d %2.2d:%2.2d:%2.2d.%d valid=%2.2x lat=%d lon=%d sats=%d fixType=%2.2x\n",
                  NavPvt->year, NavPvt->month, NavPvt->day,
                  NavPvt->hour, NavPvt->minute, NavPvt->seconds, NavPvt->nano, NavPvt->valid,
                  NavPvt->lat, NavPvt->lon, NavPvt->numSV, NavPvt->fixType);

  if ((NavPvt->valid & 3) == 3 && epoch == uptime) {     // Valid date & time
    struct tm tm;
    // Calculate epoch from UTC time ...
    tm.tm_isdst = -1;
    tm.tm_year  = NavPvt->year - 1900;
    tm.tm_mon   = NavPvt->month - 1;
    tm.tm_mday  = NavPvt->day;
    tm.tm_hour  = NavPvt->hour;
    tm.tm_min   = NavPvt->minute;
    tm.tm_sec   = NavPvt->seconds;
    //
    epoch = mktime(&tm);
    uBlox.db_printf("set epoch=%d\n", epoch);
  }
}

void loop() {
  static uint32_t _uptime = 0;
  if (SerialUSB.available()) {
    char c = SerialUSB.read();
    if (c == 's') {
      SerialUSB.println("Holding");
      while (SerialUSB.read() == -1);
    }
    else if (c == 'u') {
      PortConfigurationDDC pcd;
      if (uBlox.getPortConfigurationDDC(&pcd)) {
        uBlox.db_printf("portID=%x txReady=%x mode=%x inProtoMask=%4.4x outProtoMask=%4.4x flags=%4.4x reserved5=%x\n", pcd.portID, pcd.txReady, pcd.mode, pcd.inProtoMask, pcd.outProtoMask, pcd.flags, pcd.reserved5);
        pcd.outProtoMask ^= 2;   // NMEA toggle
        uBlox.setPortConfigurationDDC(&pcd);
      }
      else
        uBlox.db_printf("uBlox.getPortConfigurationDDC(&pcd) == false\n");
    }
  }
  //
  if (uptime != _uptime) {
    uBlox.db_printf("Up since %d for %d seconds \n", epoch - uptime, uptime);
    uBlox.GetPeriodic();
    _uptime = uptime;
  }
  //
  Led((millis() / 500) & 3);

}
