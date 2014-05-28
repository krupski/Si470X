///////////////////////////////////////////////////////////////////////////////
//
//  Silicon Labs Si470x FM Radio Chip Driver Library for Arduino
//  Copyright (c) 2012, 2014 Roger A. Krupski <rakrupski@verizon.net>
//
//  Last update: 25 May 2014
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU General Public License as published by
//  the Free Software Foundation, either version 3 of the License, or
//  (at your option) any later version.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
//  GNU General Public License for more details.
//
//  You should have received a copy of the GNU General Public License
//  along with this program. If not, see <http://www.gnu.org/licenses/>.
//
///////////////////////////////////////////////////////////////////////////////

#ifndef _SI470X_H
#define _SI470X_H

#if ARDUINO < 100
	#include <WProgram.h>
#else
	#include <Arduino.h>
#endif

#include <util/delay.h> // for _delay_ms ()

// all kinds of defines for the Si470x chip registers and bits - lots of them

// chip I/O
#define RD                1
#define WR                0
#define DEV_ADDR      0b011 // 3 wire device address for standard mode
#define RWBIT       _BV (5) // 3 wire mode read/write bit (hi=read, lo=write)

#define FAIL           0x00
#define SUCCESS        0x01

// define the register names
#define DEVICEID       0x00
#define CHIPID         0x01
#define POWERCFG       0x02
#define CHANNEL        0x03
#define SYSCONFIG1     0x04
#define SYSCONFIG2     0x05
#define SYSCONFIG3     0x06
#define TEST1          0x07
#define TEST2          0x08
#define BOOTCONFIG     0x09
#define STATUSRSSI     0x0A
#define READCHANNEL    0x0B
#define RDSA           0x0C
#define RDSB           0x0D
#define RDSC           0x0E
#define RDSD           0x0F

// register 0x02 - POWERCFG
#define DSMUTE         0x0F
#define DMUTE          0x0E
#define MONO           0x0D
#define RDSM           0x0B
#define SKMODE         0x0A
#define SEEKUP         0x09
#define SEEK           0x08
#define DISABLE        0x06
#define ENABLE         0x00

// register 0x03 - CHANNEL
#define TUNE           0x0F

// register 0x04 - SYSCONFIG1
#define RDSIEN         0x0F
#define STCIEN         0x0E
#define RDS            0x0C
#define DE             0x0B
#define AGCD           0x0A
#define BLNDADJ0       0x06
#define BLNDADJ1       0x07
#define GPIO30         0x05
#define GPIO31         0x04
#define GPIO20         0x03
#define GPIO21         0x02
#define GPIO10         0x01
#define GPIO11         0x00

// register 0x05 - SYSCONFIG2
#define SEEKTH7        0x0F
#define SEEKTH6        0x0E
#define SEEKTH5        0x0D
#define SEEKTH4        0x0C
#define SEEKTH3        0x0B
#define SEEKTH2        0x0A
#define SEEKTH1        0x09
#define SEEKTH0        0x08
#define BAND1          0x07
#define BAND0          0x06
#define SPACE1         0x05
#define SPACE0         0x04
#define VOLUME3        0x03
#define VOLUME2        0x02
#define VOLUME1        0x01
#define VOLUME0        0x00

// register 0x06 - SYSCONFIG3
#define SMUTER1        0x0F
#define SMUTER0        0x0E
#define SMUTEA1        0x0D
#define SMUTEA0        0x0C
#define VOLEXT         0x08
#define SKSNR3         0x07
#define SKSNR2         0x06
#define SKSNR1         0x05
#define SKSNR0         0x04
#define SKCNT3         0x03
#define SKCNT2         0x02
#define SKCNT1         0x01
#define SKCNT0         0x00

// register 0x07 - TEST1
#define XOSCEN         0x0F
#define AHIZEN         0x0E

// register 0x0A - STATUSRSSI
#define RDSR           0x0F
#define STC            0x0E
#define SFBL           0x0D
#define AFCRL          0x0C
#define RDSS           0x0B
#define BLERA          0x0A
#define STEREO         0x08
#define RSSI           0x07

// register 0x0B - READCHANNEL
#define BLERB          0x0F
#define BLERC          0x0D
#define BLERD          0x0B
#define READCHAN       0x09

// program with commands - Si4702/03 Rev C or later

// register 0x0C - COMMAND1
#define COMMAND1       0x0C
#define RESPONSE1      0x0C

// register 0x0D - COMMAND2
#define COMMAND2       0x0D
#define RESPONSE2      0x0D

// register 0x0E - COMMAND3
#define COMMAND3       0x0E
#define RESPONSE3      0x0E

// register 0x0F - COMMAND4
#define COMMAND4       0x0F
#define RESPONSE4      0x0F

// this stuff is not supported by the driver
// program with commands - command & properties
#define SET_PROPERTY   0x07
#define GET_PROPERTY   0x08
#define VERIFY_COMMAND 0xFF

// available properties (Si4702/03 Rev C or Later Device Only)
#define FM_DETECTOR_SNR   0x0200 // r/w, default 0x00,  8 bit value
#define BLEND_MONO_RSSI   0x0300 // r/w, default 0x1F,  8 bit value
#define BLEND_STEREO_RSSI 0x0301 // r/w, default 0x32,  8 bit value
#define CALCODE           0x0700 // ro,  default  n/a, 16 bit value
#define SNRDB             0x0C00 // ro,  default  n/a,  8 bit value

class SI470X
{
public:
	uint8_t init (uint8_t, uint8_t, uint8_t, uint8_t);
	void setVolume (uint8_t);
	uint8_t getVolume (void);
	void setChannel (uint16_t);
	uint16_t getChannel (void);
	void setSoftMute (uint8_t);
	void setMute (uint8_t);
	void setMono (uint8_t);
	void setThreshold (uint8_t);
	uint8_t getThreshold (void);
	uint8_t isStereo (void);
	uint8_t getSignal (void);
	uint16_t setSeek (uint8_t, uint8_t = 1); // seek wrap enabled by default
private:
	uint8_t _SDIO; // chip i/o registers
	uint8_t _SCLK;
	uint8_t _SEN;
	uint8_t _RESET;
	uint16_t _registers[16];
	void _updateRegisters (void);
	void _readRegisters (void);
	void _ctrl (uint8_t, uint8_t);
	uint16_t _read (uint8_t);
	void _write (uint16_t, uint8_t);
	uint8_t _recv (void);
	void _send (uint8_t);
	void _sclk (void);
};

#endif


