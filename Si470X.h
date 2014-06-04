///////////////////////////////////////////////////////////////////////////////
//
//  Silicon Labs Si470x FM Radio Chip Driver Library for Arduino
//  Copyright (c) 2012, 2014 Roger A. Krupski <rakrupski@verizon.net>
//
//  Last update: 04 June 2014
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
//////////////////////////////////////////////////////////////////////////////

#ifndef _SI470X_H
#define _SI470X_H

#if ARDUINO < 100
  #include <WProgram.h>
#else
  #include <Arduino.h>
#endif

#include <util/delay.h> // for _delay_ms ()

// all kinds of defines for the Si470x chip registers and bits
#define DEV_ADDR   0b011 // device address for 3 wire mode
#define RWBIT    _BV (5) // read/write bit location in "address"

// define the register names
#define DEVICEID    0x00
#define CHIPID      0x01
#define POWERCFG    0x02
#define CHANNEL     0x03
#define SYSCONFIG1  0x04
#define SYSCONFIG2  0x05
#define SYSCONFIG3  0x06
#define TEST1       0x07
#define TEST2       0x08
#define BOOTCONFIG  0x09
#define STATUSRSSI  0x0A
#define READCHANNEL 0x0B
#define RDSA        0x0C
#define RDSB        0x0D
#define RDSC        0x0E
#define RDSD        0x0F

// register 0x01 - CHIPID
// firmware and device code bits
#define FIRMWARE  0x003F
#define DEV       0x03C0

// register 0x02 - POWERCFG
#define DSMUTE      0x0F
#define DMUTE       0x0E
#define MONO        0x0D
#define RDSM        0x0B
#define SKMODE      0x0A
#define SEEKUP      0x09
#define SEEK        0x08
#define DISABLE     0x06
#define ENABLE      0x00

// register 0x03 - CHANNEL
#define TUNE        0x0F

// register 0x04 - SYSCONFIG1
#define RDSIEN      0x0F
#define STCIEN      0x0E
#define RDS         0x0C
#define DE          0x0B
#define AGCD        0x0A
#define BLNDADJ     0x06
#define GPIO3       0x04
#define GPIO2       0x02
#define GPIO1       0x00

// register 0x05 - SYSCONFIG2
#define SEEKTH      0x08
#define BAND        0x06
#define SPACE       0x04
#define VOLUME      0x00

// register 0x06 - SYSCONFIG3
#define SMUTER      0x0E
#define SMUTEA      0x0C
#define VOLEXT      0x08
#define SKSNR       0x04
#define SKCNT       0x00

// register 0x07 - TEST1
#define XOSCEN      0x0F
#define AHIZEN      0x0E

// register 0x0A - STATUSRSSI
#define RDSR        0x0F
#define STC         0x0E
#define SFBL        0x0D
#define AFCRL       0x0C
#define RDSS        0x0B
#define BLERA       0x09
#define STEREO      0x08
#define RSSI        0x00

// register 0x0B - READCHANNEL
#define BLERB       0x0E
#define BLERC       0x0C
#define BLERD       0x0A
#define READCHAN    0x00

#ifndef true
  #define true 1
#endif

#ifndef false
  #define false 0
#endif

class SI470X
{
	public:
		uint8_t init (uint8_t, uint8_t, uint8_t, uint8_t, uint8_t = 0);
		void setVolume (uint8_t);
		void setChannel (uint16_t);
		uint16_t getChannel (void);
		uint8_t getSignal (void);
		uint8_t getStereo (void);
		void setTreshold (uint8_t);
		uint8_t muteOn (void);
		uint8_t muteOff (void);
		uint8_t monoOn (void);
		uint8_t monoOff (void);
		uint16_t seekUp (void);
		uint16_t seekDown (void);
		void writeRegisters (void);
		uint16_t *readRegisters (void);
	private:
		uint8_t _SDIO; // chip i/o registers
		uint8_t _SCLK;
		uint8_t _SEN;
		uint8_t _RESET;
		uint8_t _REGION; // region code
		uint16_t _REGISTERS[16]; // chip register shadow
		const uint8_t _CHAN_MULT[2] = { 2, 1 }; // channel multipliers for region
		const uint16_t _CHAN_OFFSET[2] = { 875, 760 }; // channel offsets for region
		uint8_t _setMute (uint8_t);
		uint8_t _setMono (uint8_t);
		uint16_t _setSeek (uint8_t);
		uint16_t _readChip (uint8_t);
		void _writeChip (uint16_t, uint8_t);
		void _pulseSCLK (void);
};

#endif
// end of SI470X.h
