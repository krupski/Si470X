///////////////////////////////////////////////////////////////////////////////
//
//  Silicon Labs Si470x FM Radio Chip Driver Library for Arduino
//  Copyright (c) 2012, 2017 Roger A. Krupski <rakrupski@verizon.net>
//
//  Last update: 03 March 2017
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

#ifndef SI470X_H
#define SI470X_H

#if ARDUINO < 100
#include <WProgram.h>
#else
#include <Arduino.h>
#endif

#define DEV_WR  0b011000000 // device address for 3 wire mode write
#define DEV_RD  0b011100000 // device address for 3 wire mode read

// define the register names
#define DEVICEID         (0x00)
#define CHIPID           (0x01)
#define POWERCFG         (0x02)
#define CHANNEL          (0x03)
#define SYSCONFIG1       (0x04)
#define SYSCONFIG2       (0x05)
#define SYSCONFIG3       (0x06)
#define TEST1            (0x07)
#define TEST2            (0x08)
#define BOOTCONFIG       (0x09)
#define STATUSRSSI       (0x0A)
#define READCHANNEL      (0x0B)
#define RDSA             (0x0C)
#define RDSB             (0x0D)
#define RDSC             (0x0E)
#define RDSD             (0x0F)

// register 0x01 - CHIPID
// firmware and device code bits
#define ENABLED2       (0x1053) // Si4702 id
#define ENABLED3       (0x1253) // Si4703 id

// register 0x02 - POWERCFG
#define DSMUTE    (1UL << 0x0F)
#define DMUTE     (1UL << 0x0E)
#define MONO      (1UL << 0x0D)
#define RDSM      (1UL << 0x0B)
#define SKMODE    (1UL << 0x0A)
#define SEEKUP    (1UL << 0x09)
#define SEEK      (1UL << 0x08)
#define DISABLE   (1UL << 0x06)
#define ENABLE    (1UL << 0x00)

// register 0x03 - CHANNEL
#define TUNE      (1UL << 0x0F)

// register 0x04 - SYSCONFIG1
#define RDSIEN    (1UL << 0x0F)
#define STCIEN    (1UL << 0x0E)
#define RDS       (1UL << 0x0C)
#define DE        (1UL << 0x0B)
#define AGCD      (1UL << 0x0A)
#define BLNDADJ          (0x06)
#define GPIO3            (0x04)
#define GPIO2            (0x02)
#define GPIO1            (0x00)

// register 0x05 - SYSCONFIG2
#define SEEKTH           (0x08)
#define BAND             (0x06)
#define SPACE            (0x04)
#define VOLUME    (1UL << 0x00)

// register 0x06 - SYSCONFIG3
#define SMUTER           (0x0E)
#define SMUTEA           (0x0C)
#define VOLEXT    (1UL << 0x08)
#define SKSNR            (0x04)
#define SKCNT            (0x00)

// register 0x07 - TEST1
#define XOSCEN    (1UL << 0x0F)
#define AHIZEN    (1UL << 0x0E)

// register 0x0A - STATUSRSSI
#define RDSR      (1UL << 0x0F)
#define STC       (1UL << 0x0E)
#define SFBL      (1UL << 0x0D)
#define AFCRL     (1UL << 0x0C)
#define RDSS      (1UL << 0x0B)
#define BLERA            (0x09)
#define STEREO    (1UL << 0x08)
#define RSSI      (1UL << 0x00)

// register 0x0B - READCHANNEL
#define BLERB            (0x0E)
#define BLERC            (0x0C)
#define BLERD            (0x0A)
#define READCHAN  (1UL << 0x00)

// RDS defines
#define RADIO_TEXT_GROUP_CODE 2
#define TOGGLE_FLAG_POSITION 5
#define CHARS_PER_SEGMENT 2
#define MAX_MESSAGE_LENGTH 64
#define MAX_SEGMENTS 16
#define MAX_CHARS_PER_GROUP 4
#define VERSION_A_TEXT_SEGMENT_PER_GROUP 2
#define VERSION_B_TEXT_SEGMENT_PER_GROUP 1

class SI470X
{
	public:
		SI470X (uint8_t, uint8_t, uint8_t, uint8_t);
		uint8_t ready (void);
		void setSeekthreshold (uint8_t);
		void setSoftmute (uint8_t);
		uint8_t setVolume (int8_t);
		uint8_t getVolume (void);
		uint16_t setChannel (uint16_t);
		uint16_t getChannel (void);
		uint8_t getSignal (void);
		uint8_t getStereo (void);
		void setThreshold (uint8_t);
		void setMute (uint8_t);
		void setMono (uint8_t);
		uint16_t setSeek (uint8_t);
		uint8_t getRDS (void);
		void setDE (uint8_t);
		void setRegion (uint8_t);
		void setAGC (uint8_t);
		void setBlendadj (uint8_t);
		char *getRDSdata (void);
//		uint8_t getRDSdata (char *);

	private:
		uint8_t _pass;
		uint16_t _filled;
		// bitmasks
		uint8_t _SDIO_BIT;
		uint8_t _SCLK_BIT;
		uint8_t _SEN_BIT;
		uint8_t _RST_BIT;
		// outputs
		volatile uint8_t *_SDIO_OUT;
		volatile uint8_t *_SCLK_OUT;
		volatile uint8_t *_SEN_OUT;
		volatile uint8_t *_RST_OUT;
		// inputs
		volatile uint8_t *_SDIO_INP;
		// DDR's
		volatile uint8_t *_SDIO_DDR;
		volatile uint8_t *_SCLK_DDR;
		volatile uint8_t *_SEN_DDR;
		volatile uint8_t *_RST_DDR;
		// vars & private functions
		uint8_t _REGION=0;
		uint16_t _REGISTERS[16]; // chip register shadow
		char _rdsBuffer[MAX_MESSAGE_LENGTH];
		void _writeRegisters (uint16_t *);
		void _readRegisters (uint16_t *);
		uint16_t _spi_transfer (uint16_t, uint8_t);
};

#endif
// end of SI470X.h
