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

#include "Si470X.h"

SI470X::SI470X (uint8_t sdio_pin, uint8_t sclk_pin, uint8_t sen_pin, uint8_t rst_pin)
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

	x = digitalPinToPort (rst_pin);
	_RST_OUT = portOutputRegister (x);
	_RST_DDR = portModeRegister (x);
	_RST_BIT = digitalPinToBitMask (rst_pin);

	// set initial pin values
	*_SDIO_OUT &= ~_SDIO_BIT; // SDIO initially low
	*_SCLK_OUT &= ~_SCLK_BIT; // SCLK idles low
	*_SEN_OUT  &= ~_SEN_BIT;  // SEN initially low
	*_RST_OUT  &= ~_RST_BIT;  // RESET initially low

	*_SDIO_DDR &= ~_SDIO_BIT; // set SDIO pin as input
	*_SCLK_DDR |= _SCLK_BIT;  // set SCLK pin as output
	*_SEN_DDR  |= _SEN_BIT;   // set SEN pin as output
	*_RST_DDR  |= _RST_BIT;   // set RESET pin as output

	*_RST_OUT |= _RST_BIT; // set RESET high (set 3 wire mode because SEN is low)
	*_SEN_OUT |= _SEN_BIT; // set SEN high (deselect chip)

	_readRegisters (_REGISTERS); // read current chip registers (AN230 pg. 12)
	_REGISTERS[TEST1] |= XOSCEN; // enable the oscillator
	_writeRegisters (_REGISTERS); // update chip registers

	__builtin_avr_delay_cycles ((((double)(F_CPU)/(double)(1e3))+0.5)*500); // let oscillator stabilize

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[RDSD] = 0; // clear RDS data as per errata
	_REGISTERS[POWERCFG] |= DMUTE; // disable mute
	_REGISTERS[POWERCFG] |= ENABLE; // set powerup state
	_REGISTERS[POWERCFG] &= ~DISABLE; // set powerup state
	_writeRegisters (_REGISTERS); // update chip registers

	x = 100; // timeout (don't lock up if init fails)
	while (x--) {
		if (ready()) {
			break;
		}
	}

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[SYSCONFIG1] |= RDS; // enable RDS
	_REGISTERS[POWERCFG] &= ~RDSM; // set RDS standard mode
	_writeRegisters (_REGISTERS); // update chip registers

	_pass = 0; // init RDS vars
	_filled = 0; // init RDS vars
}

uint8_t SI470X::ready (void)
{
	_readRegisters (_REGISTERS); // read current chip registers
	return ((_REGISTERS[CHIPID] == ENABLED2) || (_REGISTERS[CHIPID] == ENABLED3)) ? 1 : 0;
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

// set volume 0 ... 99 (mute...0dB)
uint8_t SI470X::setVolume (int8_t volume)
{
	uint8_t vol, ext;

	volume = (volume < 0) ? 0 : (volume > 99) ? 99 : volume;
	vol = (volume / (100.0 / 32.0));
	vol = (vol == 16) ? 17 : vol;
	ext = (volume < 50) ? 1 : 0;

	_readRegisters (_REGISTERS); // read current chip registers
	ext ? _REGISTERS[SYSCONFIG3] |= VOLEXT : _REGISTERS[SYSCONFIG3] &= ~VOLEXT;
	_REGISTERS[SYSCONFIG2] &= ~0b1111; // Clear volume bits
	_REGISTERS[SYSCONFIG2] |= (vol & 0b1111); // Set new volume
	_writeRegisters (_REGISTERS); // update chip registers

	return getVolume();
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
uint16_t SI470X::setChannel (uint16_t channel)
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

	return getChannel();
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

	const uint8_t _thr[] = { 0x19, 0x19, 0x0C, 0x0C, 0x00 };
	const uint8_t _snr[] = { 0x00, 0x04, 0x04, 0x07, 0x04 };
	const uint8_t _cnt[] = { 0x00, 0x08, 0x08, 0x0F, 0x0F };

	_readRegisters (_REGISTERS); // read current chip registers
	_REGISTERS[SYSCONFIG2] &= ~(0b11111111 << SEEKTH); //
	_REGISTERS[SYSCONFIG3] &= ~(0b1111 << SKSNR);  // clear bits
	_REGISTERS[SYSCONFIG3] &= ~(0b1111 << SKCNT);  //
	_REGISTERS[SYSCONFIG2] |= (_thr[th] << SEEKTH); // set seek threshold
	_REGISTERS[SYSCONFIG3] |= (_snr[th] << SKSNR);  // set seek s/n ratio
	_REGISTERS[SYSCONFIG3] |= (_cnt[th] << SKCNT);  // set seek fm impulse detect
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

uint8_t SI470X::getRDS (void)
{
	uint8_t timeout = 25;
	while (timeout--) {
		_readRegisters (_REGISTERS); // read current chip registers
		if (_REGISTERS[STATUSRSSI] & RDSR) {
			return timeout;
		}
	}
	return 0;
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

char *SI470X::getRDSdata (void)
{
	uint8_t c0, c1, c2, c3, n, group, idx, addr, err;

	_readRegisters (_REGISTERS); // read current chip registers

	if (_REGISTERS[STATUSRSSI] & RDSR) {

		group = ((_REGISTERS[RDSB] & 0xF000) >> 8); // get group base
		group |= (_REGISTERS[RDSB] & 0x0800) ? 0x0B : 0x0A; // add in a/b code
		err = 0;

		if (group == 0x2A) {

			idx = (_REGISTERS[RDSB] & 0x000F); // get block index
			addr = (idx * CHARS_PER_SEGMENT * VERSION_A_TEXT_SEGMENT_PER_GROUP);

			c0 = (_REGISTERS[RDSC] >> 8); // block char 0
			c1 = (_REGISTERS[RDSC] & 0x00FF); // block char 1
			c2 = (_REGISTERS[RDSD] >> 8); // block char 2
			c3 = (_REGISTERS[RDSD] & 0x00FF); // block char 3

			_rdsBuffer[addr + 0] = c0;
			_rdsBuffer[addr + 1] = c1;
			_rdsBuffer[addr + 2] = c2;
			_rdsBuffer[addr + 3] = c3;

			_filled |= (1UL << idx); // mark which segment we got

			if (_pass == 1) {
				if (_rdsBuffer[addr + 0] != c0) { err++; }
				if (_rdsBuffer[addr + 1] != c1) { err++; }
				if (_rdsBuffer[addr + 2] != c2) { err++; }
				if (_rdsBuffer[addr + 3] != c3) { err++; }
			}

			if (err != 0) {
				_filled = 0;
				_pass = 0;
				idx = MAX_MESSAGE_LENGTH;
				while (idx--) {
					_rdsBuffer[idx] = 0x20;
				}
				_rdsBuffer[MAX_MESSAGE_LENGTH-1] = 0;
				return NULL;
			}

			if (_filled == 0xFFFF) {
				if (_pass == 0) {
					_filled = 0;
					_pass = 1;
				} else {
					_filled = 0;
					_pass = 0;
					_rdsBuffer[MAX_MESSAGE_LENGTH-1] = 0;
					return _rdsBuffer;
				}
			}
		}

		if (group == 0x2B) {

			idx = (_REGISTERS[RDSB] & 0x000F); // get block index
			addr = (idx * CHARS_PER_SEGMENT * VERSION_B_TEXT_SEGMENT_PER_GROUP);

			c0 = (_REGISTERS[RDSD] >> 8); // block char 0
			c1 = (_REGISTERS[RDSD] & 0x00FF); // block char 1

			_rdsBuffer[addr + 0] = c0;
			_rdsBuffer[addr + 1] = c1;
			_filled |= (1UL << idx); // mark which segment we got

			if (_pass == 1) {
				if (_rdsBuffer[addr + 0] != c0) { err++; }
				if (_rdsBuffer[addr + 1] != c1) { err++; }
			}

			if (err != 0) {
				_filled = 0;
				_pass = 0;
				idx = MAX_MESSAGE_LENGTH;
				while (idx--) {
					_rdsBuffer[idx] = 0x20;
				}
				_rdsBuffer[MAX_MESSAGE_LENGTH-1] = 0;
				return NULL;
			}

			if (_filled == 0x00FF) {
				if (_pass == 0) {
					_filled = 0;
					_pass = 1;
				} else {
					_filled = 0;
					_pass = 0;
					_rdsBuffer[MAX_MESSAGE_LENGTH-1] = 0;
					return _rdsBuffer;
				}
			}
		}

	}
	return NULL;
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
//		__builtin_avr_delay_cycles ((((double)(F_CPU)/(double)(1e6))+0.5)*1); // 1 usec delay
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
//		__builtin_avr_delay_cycles ((((double)(F_CPU)/(double)(1e6))+0.5)*1); // 1 usec delay
		*_SCLK_OUT &= ~_SCLK_BIT;
	}
}

// SPI mode 0 data transfer
uint16_t SI470X::_spi_transfer (uint16_t data, uint8_t bits)
{
	*_SEN_OUT &= ~_SEN_BIT; // SEN = low (enable chip select)

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

	*_SEN_OUT |= _SEN_BIT; // SEN = high (disable chip select)

	return data;
}
// end of SI470X.cpp
