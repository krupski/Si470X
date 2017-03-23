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

#include "SI470X.h"

SI470X::SI470X (
	uint8_t sdio_pin,
	uint8_t sclk_pin,
	uint8_t sen_pin,
	uint8_t reset_pin,
	uint8_t vcc_pin,
	uint8_t gnd_pin
) {

	uint8_t x;

	// pull reset low
	x = digitalPinToPort (reset_pin);
	_RESET_OUT = portOutputRegister (x);
	_RESET_DDR = portModeRegister (x);
	_RESET_BIT = digitalPinToBitMask (reset_pin);

	*_RESET_OUT &= ~_RESET_BIT; // RESET initially low
	*_RESET_DDR |= _RESET_BIT; // set RESET pin as output

	// if a ground pin is specified, configure it
	if (gnd_pin != 99) {
		x = digitalPinToPort (gnd_pin); // pin -> port
		_GND_OUT = portOutputRegister (x); // set gnd pin as output
		_GND_DDR = portModeRegister (x); // get gnd pin's DDR
		_GND_BIT = digitalPinToBitMask (gnd_pin); // get gnd pin's bitmask
		*_GND_OUT &= ~_GND_BIT; // set gnd pin low
		*_GND_DDR |= _GND_BIT; // set gnd pin DDR to output
	}

	// if a vcc pin is specified, configure it
	if (vcc_pin != 99) {
		x = digitalPinToPort (vcc_pin); // pin -> port
		_VCC_OUT = portOutputRegister (x); // set vcc pin as output
		_VCC_DDR = portModeRegister (x); // get vcc pin's DDR
		_VCC_BIT = digitalPinToBitMask (vcc_pin); // get vcc pin's bitmask
		*_VCC_OUT |= _VCC_BIT; // set vcc pin high
		*_VCC_DDR |= _VCC_BIT; // set vcc pin DDR to output
	}

	_init (sdio_pin, sclk_pin, sen_pin); // finish init

	// seek threshold settings (AN230, pg. 40)
	// 0 = default
	// 1 = recommended
	// 2 = more stations
	// 3 = good quality stations only
	// 4 = most stations
//	setThreshold (3);
//	setSeekthreshold (0x3F);

}

void SI470X::setSeekthreshold (uint8_t th)
{
	if (th > 0x7F) { return; } // reject bad value

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[SYSCONFIG2] &= ~(0b11111111 << SEEKTH);
	_REGISTERS[SYSCONFIG2] |= (th << SEEKTH);
	_writeRegisters (_REGISTERS); // update chip registers
}

void SI470X::setSoftmute (uint8_t ar)
{
	if (ar > 3) { return; } // bail if illegal

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[SYSCONFIG3] &= ~(0b11 << SMUTER); // clear setting
	_REGISTERS[SYSCONFIG3] &= ~(0b11 << SMUTEA); // clear setting
	_REGISTERS[SYSCONFIG3] |= (ar << SMUTER); // set soft mute attack/recover
	_REGISTERS[SYSCONFIG3] |= (ar << SMUTEA); // set soft mute attenuation
	_writeRegisters (_REGISTERS); // update chip registers
}

// set volume 0 ... 100 (mute...0dBFS in steps of 2)
void SI470X::setVolume (int8_t volume)
{
	uint8_t vol;
	uint8_t ext;

	volume = volume < 0 ? 0 : volume > 100 ? 100 : volume;

	switch (volume) {
		case 0 ... 2: {
			vol = 0;
			ext = 1;
			break;
		}
		case 3 ... 6: {
			vol = 1;
			ext = 1;
			break;
		}
		case 7 ... 9: {
			vol = 2;
			ext = 1;
			break;
		}
		case 10 ... 12: {
			vol = 3;
			ext = 1;
			break;
		}
		case 13 ... 15: {
			vol = 4;
			ext = 1;
			break;
		}
		case 16 ... 18: {
			vol = 5;
			ext = 1;
			break;
		}
		case 19 ... 22: {
			vol = 6;
			ext = 1;
			break;
		}
		case 23 ... 25: {
			vol = 7;
			ext = 1;
			break;
		}
		case 26 ... 28: {
			vol = 8;
			ext = 1;
			break;
		}
		case 29 ... 31: {
			vol = 9;
			ext = 1;
			break;
		}
		case 32 ... 34: {
			vol = 10;
			ext = 1;
			break;
		}
		case 35 ... 38: {
			vol = 11;
			ext = 1;
			break;
		}
		case 39 ... 41: {
			vol = 12;
			ext = 1;
			break;
		}
		case 42 ... 44: {
			vol = 13;
			ext = 1;
			break;
		}
		case 45 ... 47: {
			vol = 14;
			ext = 1;
			break;
		}
		case 48 ... 50: {
			vol = 15;
			ext = 1;
			break;
		}
		case 51 ... 54: {
			vol = 1;
			ext = 0;
			break;
		}
		case 55 ... 57: {
			vol = 2;
			ext = 0;
			break;
		}
		case 58 ... 60: {
			vol = 3;
			ext = 0;
			break;
		}
		case 61 ... 63: {
			vol = 4;
			ext = 0;
			break;
		}
		case 64 ... 66: {
			vol = 5;
			ext = 0;
			break;
		}
		case 67 ... 70: {
			vol = 6;
			ext = 0;
			break;
		}
		case 71 ... 73: {
			vol = 7;
			ext = 0;
			break;
		}
		case 74 ... 76: {
			vol = 8;
			ext = 0;
			break;
		}
		case 77 ... 79: {
			vol = 9;
			ext = 0;
			break;
		}
		case 80 ... 82: {
			vol = 10;
			ext = 0;
			break;
		}
		case 83 ... 86: {
			vol = 11;
			ext = 0;
			break;
		}
		case 87 ... 89: {
			vol = 12;
			ext = 0;
			break;
		}
		case 90 ... 93: {
			vol = 13;
			ext = 0;
			break;
		}
		case 94 ... 96: {
			vol = 14;
			ext = 0;
			break;
		}
		case 97 ... 100: {
			vol = 15;
			ext = 0;
			break;
		}
		default: {
			vol = 0;
			ext = 1;
		}
	}
	_readRegisters (_REGISTERS); // read current chip registers
	ext ? _REGISTERS[SYSCONFIG3] |= VOLEXT : _REGISTERS[SYSCONFIG3] &= ~VOLEXT;
	_REGISTERS[SYSCONFIG2] &= ~0b1111; // Clear volume bits
	_REGISTERS[SYSCONFIG2] |= (vol & 0b1111); // Set new volume
	_writeRegisters (_REGISTERS); // update chip registers
}

uint8_t SI470X::getVolume (void)
{
	uint16_t ext;
	_readRegisters (_REGISTERS); // read current chip registers
	ext = (_REGISTERS[SYSCONFIG3] & VOLEXT) ? 0 : 15; // get ext setting
	return (uint8_t)(((_REGISTERS[SYSCONFIG2] & 0b1111) + ext) * (100.0 / 30.0)); // get volume setting
}

// set FM channel, no decimal point (i.e. 104.1 is sent as 1041)
// we don't check for out of band settings - but these just wrap anyway
void SI470X::setChannel (uint16_t channel)
{
	const uint8_t _CHAN_MULT[] = { 2, 1 }; // channel multipliers for region
	const uint16_t _CHAN_OFFSET[] = { 875, 760 }; // channel offsets for region

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[CHANNEL] &= ~0b111111111; // Clear out the channel bits
	_REGISTERS[CHANNEL] |= ((channel -= _CHAN_OFFSET[_REGION]) / _CHAN_MULT[_REGION]); // OR in the new channel
	_REGISTERS[CHANNEL] |= TUNE; // Set the TUNE bit to start
	_writeRegisters (_REGISTERS); // update chip registers

	while (1) {
		_readRegisters (_REGISTERS); // read current chip registers
		// wait for "seek complete" to be asserted
		if (_REGISTERS[STATUSRSSI] & STC) {
			break;
		}
	}

	_REGISTERS[CHANNEL] &= ~TUNE; // tune complete, clear tune bit
	_writeRegisters (_REGISTERS); // update chip registers
}

// get FM channel (returned without decimal point (i.e. 104.1 returns as 1041)
uint16_t SI470X::getChannel (void)
{
	const uint8_t _CHAN_MULT[] = { 2, 1 }; // channel multipliers for region
	const uint16_t _CHAN_OFFSET[] = { 875, 760 }; // channel offsets for region

	_readRegisters (_REGISTERS); // read current chip registers
	return (((_REGISTERS[READCHANNEL] & 0b111111111) * _CHAN_MULT[_REGION]) + _CHAN_OFFSET[_REGION]);
}

// returns received signal strength (RSSI) in dB microvolts
uint8_t SI470X::getSignal (void)
{
	_readRegisters (_REGISTERS); // read current chip registers
	// received signal strength indicator is 8 bits but 75 dbuV max
	return (_REGISTERS[STATUSRSSI] & 0b1111111);
}

// returns true if station is stereo and chip is actually decoding stereo
uint8_t SI470X::getStereo (void)
{
	_readRegisters (_REGISTERS); // read current chip registers
	return (_REGISTERS[STATUSRSSI] & STEREO) ? 1 : 0;
}

// seek threshold settings (AN230, pg. 40)
// 0 = default
// 1 = recommended
// 2 = more stations
// 3 = good quality stations only
// 4 = most stations
void SI470X::setThreshold (uint8_t th)
{
	if (th > 4) {
		return; // if invalid setting, just bail
	}

	const uint8_t thr[] = { 0x19, 0x19, 0x0C, 0x0C, 0x00 };
	const uint8_t snr[] = { 0x00, 0x04, 0x04, 0x07, 0x04 };
	const uint8_t cnt[] = { 0x00, 0x08, 0x08, 0x0F, 0x0F };

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[SYSCONFIG2] &= ~(0b11111111 << SEEKTH); //
	_REGISTERS[SYSCONFIG3] &= ~(0b1111 << SKSNR);  // clear bits
	_REGISTERS[SYSCONFIG3] &= ~(0b1111 << SKCNT);  //
	_REGISTERS[SYSCONFIG2] |= (thr[th] << SEEKTH); // set seek threshold
	_REGISTERS[SYSCONFIG3] |= (snr[th] << SKSNR);  // set seek s/n ratio
	_REGISTERS[SYSCONFIG3] |= (cnt[th] << SKCNT);  // set seek fm impulse detect
	_writeRegisters (_REGISTERS); // update chip registers
}

// mute audio on/off
void SI470X::setMute (uint8_t on)
{
	_readRegisters (_REGISTERS); // read current chip registers
	// clear or set "disable mute" bit
	on ? _REGISTERS[POWERCFG] &= ~DMUTE : _REGISTERS[POWERCFG] |= DMUTE;
	_writeRegisters (_REGISTERS); // update chip registers
}

// force mono mode (less noise on really weak stations)
void SI470X::setMono (uint8_t on)
{
	_readRegisters (_REGISTERS); // read current chip registers
	// set of clear "mono" bit
	on ? _REGISTERS[POWERCFG] |= MONO : _REGISTERS[POWERCFG] &= ~MONO;
	_writeRegisters (_REGISTERS); // update chip registers
}

// seek to the next (up) or previous (down) active channel
uint16_t SI470X::setSeek (uint8_t updown)
{
	_readRegisters (_REGISTERS); // read current chip registers
	updown ? _REGISTERS[POWERCFG] |= SEEKUP : _REGISTERS[POWERCFG] &= ~SEEKUP; // set seek up / down
	_REGISTERS[POWERCFG] |= SEEK; // enable seeking
	_writeRegisters (_REGISTERS); // update chip registers

	while (! (_REGISTERS[STATUSRSSI] & SFBL)) { // search until whole band is searched
		_readRegisters (_REGISTERS); // read current chip registers
		// stop scanning when "seek complete" is asserted
		if (_REGISTERS[STATUSRSSI] & STC) {
			break;
		}
	}

	_REGISTERS[POWERCFG] &= ~SEEK; // seek done, clear seek bit
	_writeRegisters (_REGISTERS); // update chip registers
	return getChannel(); // return channel found
}

void SI470X::setRDS (uint8_t on) // enable RDS on/off
{
	_readRegisters (_REGISTERS); // read current chip registers
	on ? _REGISTERS[SYSCONFIG1] |= RDS : _REGISTERS[SYSCONFIG1] &= ~RDS;
	_writeRegisters (_REGISTERS); // update chip registers
}

void SI470X::setRDSmode (uint8_t mode) // RDS mode standard / verbose
{
	_readRegisters (_REGISTERS); // read current chip registers
	mode ? _REGISTERS[POWERCFG] |= RDSM : _REGISTERS[POWERCFG] &= ~RDSM;
	_writeRegisters (_REGISTERS); // update chip registers
}

void SI470X::setDE (uint8_t on) // enable de-emphasis on/off
{
	_readRegisters (_REGISTERS); // read current chip registers
	on ? _REGISTERS[SYSCONFIG1] &= ~DE : _REGISTERS[SYSCONFIG1] |= DE;
	_writeRegisters (_REGISTERS); // update chip registers
}

void SI470X::setRegion (uint8_t region) // 0=87.5->108[200kHz], 1=76->108[100kHz]
{
	_REGION = (region % 2); // save a copy for library use
	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[SYSCONFIG2] |= (_REGION << SPACE); // channel spacing 200 kHz (USA/Europe default)
	_REGISTERS[SYSCONFIG2] |= (_REGION << BAND); // band select 87.5-108 MHz (default)
	_writeRegisters (_REGISTERS); // update chip registers
}

void SI470X::setAGC (uint8_t on) // enable agc on/off
{
	_readRegisters (_REGISTERS); // read current chip registers
	on ? _REGISTERS[SYSCONFIG1] &= ~AGCD : _REGISTERS[SYSCONFIG1] |= AGCD;
	_writeRegisters (_REGISTERS); // update chip registers
}

void SI470X::setBlendadj (uint8_t level) // adjust stereo blend
{
	if (level > 3) { return; } // bail if illegal

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[SYSCONFIG1] &= ~(0b11 << BLNDADJ); // clear setting
	_REGISTERS[SYSCONFIG3] |= (level << BLNDADJ); // set blend adjust
	_writeRegisters (_REGISTERS); // update chip registers
}

void SI470X::getRDSdata (void)
{
//	uint16_t x;
//	x = 1000; // timeout
//	while (x--) {
_REGISTERS[RDSA] = 0;
_REGISTERS[RDSB] = 0;
_REGISTERS[RDSC] = 0;
_REGISTERS[RDSD] = 0;
		_readRegisters (_REGISTERS); // read current chip registers
		if (_REGISTERS[STATUSRSSI] & RDSR) { // wait for RDS data
			processData (_REGISTERS[RDSA], _REGISTERS[RDSB], _REGISTERS[RDSC], _REGISTERS[RDSD]);
//			return;
		}
//	}
//	processData (0, 0, 0, 0);
}


//////////////////////////////////////////////////////////////////////
////////////////// private functions from here down //////////////////
//////////////////////////////////////////////////////////////////////

// note: the 9 bit "address" contains:
//
// bit: [ 8  7  6 ] [ 5 ] [ 4 ] [  3   2   1   0   ]
// val: [ 0  1  1 ] [R/W] [ 0 ] [register 0x00-0x0F]
//
// note: WRITE: R/W = 0, READ: R/W = 1
// note: This also applies to "_readRegisters()" below.
//
// write all 16 register shadows to the chip (update)
//read 0x0A...0x0F, 0x00...0x09
void SI470X::_writeRegisters (uint16_t *_REGS)
{
	uint8_t regs = 16;

	while (regs--) {
		_spi_transfer ((DEV_WR | regs), 9); // write address (9 bits)
		_spi_transfer (_REGS[regs], 16); // write data (16 bits)
		*_SCLK_OUT |= _SCLK_BIT; // send the required 26th clock
		__builtin_avr_delay_cycles ((F_CPU / 1e6) * 10); // 10 usec delay
		*_SCLK_OUT &= ~_SCLK_BIT;
	}
}

void SI470X::_readRegisters (uint16_t *_REGS)
{
	uint8_t regs = 16;

	while (regs--) {
		_spi_transfer ((DEV_RD | regs), 9); // write address (9 bits)
		_REGS[regs] = _spi_transfer (0, 16); // read data (16 bits)
		*_SCLK_OUT |= _SCLK_BIT; // send the required 26th clock
		__builtin_avr_delay_cycles ((F_CPU / 1e6) * 10); // 10 usec delay
		*_SCLK_OUT &= ~_SCLK_BIT;
	}
}

// SPI mode 0 data transfer
uint16_t SI470X::_spi_transfer (uint16_t data, uint8_t bits)
{
	*_SEN_OUT &= ~_SEN_BIT; // SEN = low (enable chip select)
	__builtin_avr_delay_cycles ((F_CPU / 1e6) * 10); // 10 usec delay

	while (bits--) {
		// send a bit
		*_SDIO_DDR |= _SDIO_BIT; // set SDIO as output
		data & _BV(bits) ? *_SDIO_OUT |= _SDIO_BIT : *_SDIO_OUT &= ~_SDIO_BIT; // put data on bus
		*_SCLK_OUT |= _SCLK_BIT;  // SCLK high
		// receive a bit
		*_SDIO_DDR &= ~_SDIO_BIT; // set SDIO as input
		*_SDIO_INP & _SDIO_BIT ? data |= _BV(bits) : data &= ~_BV(bits); // get data from bus
		*_SCLK_OUT &= ~_SCLK_BIT;  // SCLK low
	}

	__builtin_avr_delay_cycles ((F_CPU / 1e6) * 10); // 10 usec delay
	*_SEN_OUT |= _SEN_BIT; // SEN = high (disable chip select)

	return data;
}

// setup the Arduino pins and pin modes and init the fm radio chip
uint8_t SI470X::_init (uint8_t sdio_pin, uint8_t sclk_pin, uint8_t sen_pin)
{
	uint8_t x;

	// set ports, pins & ddr's
	x = digitalPinToPort (sdio_pin);
	_SDIO_OUT = portOutputRegister (x);
	_SDIO_INP = portInputRegister (x);
	_SDIO_DDR = portModeRegister (x);
	_SDIO_BIT = digitalPinToBitMask (sdio_pin);

	x = digitalPinToPort (sclk_pin);
	_SCLK_OUT = portOutputRegister (x);
	_SCLK_DDR = portModeRegister (x);
	_SCLK_BIT = digitalPinToBitMask (sclk_pin);

	x = digitalPinToPort (sen_pin);
	_SEN_OUT = portOutputRegister (x);
	_SEN_DDR = portModeRegister (x);
	_SEN_BIT = digitalPinToBitMask (sen_pin);

	// set initial pin values
	*_SEN_OUT  |= _SEN_BIT;    // SEN initially high
	*_SCLK_OUT &= ~_SCLK_BIT;  // SCLK idles low
	*_SDIO_OUT |= _SDIO_BIT;   // SDIO initially high

	*_SEN_DDR  |= _SEN_BIT;   // set SEN pin as output
	*_SCLK_DDR |= _SCLK_BIT;  // set SCLK pin as output
	*_SDIO_DDR &= ~_SDIO_BIT; // set SDIO pin as input

	// if SEN is low when reset is de-asserted, 3 wire mode is selected
	*_SEN_OUT &= ~_SEN_BIT; // set SEN low to select 3 wire mode
	__builtin_avr_delay_cycles ((F_CPU / 1e6) * 100); // 100 usec wait
	*_RESET_OUT |= _RESET_BIT; // set RESET high (enable chip in 3 wire mode)
	__builtin_avr_delay_cycles ((F_CPU / 1e6) * 100); // 100 usec wait
	*_SEN_OUT |= _SEN_BIT; // set SEN high
	__builtin_avr_delay_cycles ((F_CPU / 1e6) * 100); // 100 usec wait

	_readRegisters (_REGISTERS); // read current chip registers (AN230 pg. 12)
	_REGISTERS[TEST1] |= XOSCEN; // enable the oscillator
	_writeRegisters (_REGISTERS); // update chip registers

	__builtin_avr_delay_cycles ((F_CPU / 1e3) * 500); // let oscillator stabilize

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[POWERCFG] |= DMUTE; // disable mute
	_REGISTERS[POWERCFG] |= ENABLE; // set powerup state
	_REGISTERS[POWERCFG] &= ~DISABLE; // set powerup state
	_writeRegisters (_REGISTERS); // update chip registers

	__builtin_avr_delay_cycles ((F_CPU / 1e3) * 110); // datasheet pg 13

	x = 100; // timeout (don't lock up if init fails)
	while (x--) {
		_readRegisters (_REGISTERS); // read current chip registers
		if (_REGISTERS[CHIPID] == ENABLED) {
			x = 0; // load "success" code
			break;
		}
	}

	return x; // return success or failure
}


void SI470X::processData (uint16_t block1, uint16_t block2, uint16_t block3, uint16_t block4)
{
	uint8_t idx; // index of rdsText

	char c1, c2;
	char *p;


	if (block1 == 0) {
//		// reset all the RDS info.
//		init();
//
//		// Send out empty data
//		if (_sendServiceName) {
//			_sendServiceName (programServiceName);
//		}
//
//		if (_sendText) {
//			_sendText ("");
//		}
//
//		fprintf (stdout, "RDS: No Data\n");
		return;
	}

	// analyzing Block 2
	rdsGroupType = 0x0A | ((block2 & 0xF000) >> 8) | ((block2 & 0x0800) >> 11);

	if (rdsGroupType == 0x0A || rdsGroupType == 0x0B) {
		// The data received is part of the Service Station Name
		idx = 2 * (block2 & 0x0003);

		// new data is 2 chars from block 4
		c1 = block4 >> 8;
		c2 = block4 & 0x00FF;

		// check that the data was received successfully twice
		// before publishing the station name

		if ((_PSName1[idx] == c1) && (_PSName1[idx + 1] == c2)) {
			// retrieved the text a second time: store to _PSName2
			_PSName2[idx] = c1;
			_PSName2[idx + 1] = c2;
			_PSName2[8] = '\0';

			if ((idx == 6) && strcmp (_PSName1, _PSName2) == 0) {
				if (strcmp (_PSName2, programServiceName) != 0) {
					// publish station name
					strcpy (programServiceName, _PSName2);

//	fprintf (stdout, "[%s]\n", programServiceName);


				} // if
			} // if
		} // if

		if ((_PSName1[idx] != c1) || (_PSName1[idx + 1] != c2)) {
			_PSName1[idx] = c1;
			_PSName1[idx + 1] = c2;
			_PSName1[8] = '\0';
			// Serial.println(_PSName1);
		} // if
	}


	if (rdsGroupType == 0x2A) {

		// The data received is part of the RDS Text.
		_textAB = (block2 & 0x0010);
		idx = 4 * (block2 & 0x000F);

		if (idx < _lastTextIDX) {
			// the existing text might be complete because the index is starting at the beginning again.
			// now send it to the possible listener.
		//	if (_sendText) {
		//		_sendText (_RDSText);
		//	}
//fprintf (stdout, "CNT: %u\n", cnt);
//if (cnt == 3) {
	fprintf (stdout, "%s\n", _RDSText);
//	cnt = 0;
//}
//cnt++;
//fprintf (stdout, "%s\n", _RDSText);

		}

		_lastTextIDX = idx;

		if (_textAB != _last_textAB) {
			// when this bit is toggled the whole buffer should be cleared.
			_last_textAB = _textAB;
			memset (_RDSText, 0, sizeof (_RDSText));
		}


		// new data is 2 chars from block 3
		_RDSText[idx] = (block3 >> 8);
		idx++;
		_RDSText[idx] = (block3 & 0x00FF);
		idx++;

		// new data is 2 chars from block 4
		_RDSText[idx] = (block4 >> 8);
		idx++;
		_RDSText[idx] = (block4 & 0x00FF);
		idx++;

	}
}
// end of SI470X.cpp
