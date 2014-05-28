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

#include "SI470X.h"

uint8_t SI470X::init (uint8_t sdio, uint8_t sclk, uint8_t sen, uint8_t reset)
{
	uint32_t timeout;

	// copy i/o pin numbers to private vars
	_SDIO = sdio;
	_SCLK = sclk;
	_SEN = sen;
	_RESET = reset;

	digitalWrite (_SDIO, LOW); // sdio low
	digitalWrite (_SCLK, HIGH); // sclk high
	digitalWrite (_SEN, LOW); // sen low
	digitalWrite (_RESET, LOW); // reset low

	pinMode (_SDIO, INPUT); // setup i/i DDR
	pinMode (_SCLK, OUTPUT);
	pinMode (_SEN, OUTPUT);
	pinMode (_RESET, OUTPUT);

	// if SEN is low reset is de-asserted, 3 wire mode is selected
	digitalWrite (_RESET, HIGH); // strobe in SEN low to set 3 wire mode
	_delay_ms (1);

	_readRegisters (); // Read the current register set
	_registers[TEST1] |= _BV (XOSCEN); // Enable the oscillator, from AN230 page 12
	_updateRegisters (); // Update the register set
	_delay_ms (500); // Wait for clock to settle (pg. 12)

	_readRegisters (); // Read the current register set
	_registers[POWERCFG] |= (_BV (DMUTE) | _BV (ENABLE)); // Enable the IC (pg. 12)
	_registers[POWERCFG] &= ~_BV (DISABLE); // why does it have an enable and disable bit?
	_updateRegisters (); // Update the register set

	timeout = 100; // don't lock up if init fails

	while (timeout-- > 1) {
		_readRegisters (); // Read the current register set
		if (_registers[CHIPID] & 0b00011111) {
			break;
		}
	}
	return timeout ? 1 : 0; // 1=success, 0=failure
}

void SI470X::setVolume (uint8_t volume)
{
	_readRegisters (); // Read the current register set
	_registers[SYSCONFIG2] &= ~0x0F; // Clear volume bits
	_registers[SYSCONFIG2] |= (volume & 0x0F); // Set new volume
	_updateRegisters (); // Update
}

uint8_t SI470X::getVolume (void)
{
	_readRegisters (); // Read the current register set
	return (_registers[SYSCONFIG2] & 0x0F);
}

void SI470X::setChannel (uint16_t channel)
{
	_readRegisters ();
	_registers[CHANNEL] &= ~0x01FF; // Clear out the channel bits
	_registers[CHANNEL] |= ((channel -= 875) / 2); // OR in the new channel
	_registers[CHANNEL] |= _BV (TUNE); // Set the TUNE bit to start
	_updateRegisters ();

	while (1) {
		_readRegisters ();
		// stop scanning when "seek complete" is asserted
		if (_registers[STATUSRSSI] & _BV (STC)) {
			break;
		}
	}

	_registers[CHANNEL] &= ~_BV (TUNE); // stop tune operation
	_updateRegisters ();
}

uint16_t SI470X::getChannel (void)
{
	_readRegisters ();
	return (((_registers[READCHANNEL] & 0x01FF) * 2) + 875);
}

void SI470X::setSoftMute (uint8_t on)
{
	_readRegisters ();

	if (on) {
		_registers[POWERCFG] &= ~_BV (DSMUTE);

	} else {
		_registers[POWERCFG] |= _BV (DSMUTE);
	}
	_updateRegisters ();
}

void SI470X::setMute (uint8_t on)
{
	_readRegisters ();

	if (on) {
		_registers[POWERCFG] &= ~_BV (DMUTE);

	} else {
		_registers[POWERCFG] |= _BV (DMUTE);
	}
	_updateRegisters ();
}

void SI470X::setMono (uint8_t on)
{
	_readRegisters ();

	if (on) {
		_registers[POWERCFG] |= _BV (MONO);

	} else {
		_registers[POWERCFG] &= ~_BV (MONO);
	}
	_updateRegisters ();
}

void SI470X::setThreshold (uint8_t th)
{
	_readRegisters (); // Read the current register set
	// highest signal to noise threshold for seek
	_registers[SYSCONFIG3] |= (((th & 0b1111) >> 1) << 4);
	// highest fm impulse detection
	_registers[SYSCONFIG3] |= (th & 0b1111);
	_updateRegisters(); // Update
}

uint8_t SI470X::getThreshold (void)
{
	_readRegisters ();
	return (_registers[SYSCONFIG3] & 0b1111) | (((_registers[SYSCONFIG3] & 0b0111) << 1) >> 4);
}

uint8_t SI470X::isStereo (void)
{
	_readRegisters ();
	return (_registers[STATUSRSSI] & _BV (STEREO)) ? 1 : 0;
}

uint8_t SI470X::getSignal (void)
{
	_readRegisters ();
	return (_registers[STATUSRSSI] & 0xFF);
}

uint16_t SI470X::setSeek (uint8_t updown, uint8_t wrap)
{
	_readRegisters ();

	if (updown) {
		_registers[POWERCFG] |= _BV (SEEKUP); // set seek-up bit

	} else {
		_registers[POWERCFG] &= ~_BV (SEEKUP); // clear seek up bit (seek down)
	}

	if (wrap) {
		_registers[POWERCFG] |= _BV (SKMODE); // Allow seek wrap-around

	} else {
		_registers[POWERCFG] &= ~_BV (SKMODE); // Disallow seek wrap-around
	}

	_registers[POWERCFG] |= _BV (SEEK); // enable seeking
	_updateRegisters (); // send command to chip

	do {
		_readRegisters ();
		// stop scanning when "seek complete" is asserted
		if (_registers[STATUSRSSI] & _BV (STC)) {
			break;
		}
	} while (!(_registers[STATUSRSSI] & _BV (SFBL))); // quit if the whole band was searched and nothing found

	_registers[POWERCFG] &= ~_BV (SEEK); // disable seeking
	_updateRegisters ();

	return getChannel ();
}

void SI470X::_updateRegisters (void)
{
	uint8_t x;
	for (x = 2; x < 8; x++) { // write registers 2...7
		_write (_registers[x], x);
	}
}

void SI470X::_readRegisters (void)
{
	uint8_t x;
	for (x = 0; x < 16; x++) { // read registers 0...15
		_registers[x] = _read (x);
	}
}

void SI470X::_ctrl (uint8_t rw, uint8_t reg)
{
	uint8_t bits;
	uint16_t data = 0;

	data |= (DEV_ADDR << 6); // add in chip address
	data |= reg; // add in register address
	data |= rw ? RWBIT : 0;

	bits = 9; // sending 9 bits

	pinMode (_SDIO, OUTPUT); // setup bus for write

	while (bits--) {
		digitalWrite (_SDIO, data & _BV (bits) ? HIGH : LOW); // put data on bus
		_sclk (); // pulse SCLK
	}
	pinMode (_SDIO, INPUT_PULLUP); // leave bus as input
}

uint16_t SI470X::_read (uint8_t reg)
{
	uint16_t data = 0;
	digitalWrite (_SEN, LOW); // enable chip select
	_ctrl (RD, reg); // send command (9 bits)
	data |= (_recv () << 8); // get data hi byte (8 bits)
	data |= _recv (); // get data lo byte (8 bits)
	digitalWrite (_SEN, HIGH); // disable chip select
	_sclk (); // send the required 26th clock
	return data;
}

void SI470X::_write (uint16_t data, uint8_t reg)
{
	digitalWrite (_SEN, LOW); // enable chip select
	_ctrl (WR, reg); // send command (9 bits)
	_send (data >> 8); // send data hi byte (8 bits)
	_send (data & 0x00FF); // send data lo byte (8 bits)
	digitalWrite (_SEN, HIGH); // disable chip select
	_sclk (); // send the required 26th clock
}

// bit-banger 3-wire receive
uint8_t SI470X::_recv (void)
{
	uint8_t bits = 8;
	uint8_t data = 0;

	pinMode (_SDIO, INPUT_PULLUP); // setup bus for read

	while (bits--) {
		_sclk (); // send one SCLK pulse
		digitalRead (_SDIO) ? data |= _BV (bits) : data &= ~_BV (bits); // get data bit
	}

	return data;
}

// bit-banger 3-wire send
void SI470X::_send (uint8_t data)
{
	uint8_t bits = 8;
	pinMode (_SDIO, OUTPUT); // setup bus for write

	while (bits--) {
		digitalWrite (_SDIO, data & _BV (bits) ? HIGH : LOW); // put data on bus
		_sclk (); // send one SCLK pulse
	}
	pinMode (_SDIO, INPUT_PULLUP); // leave bus as input
}

void SI470X::_sclk (void)
{
	digitalWrite (_SCLK, HIGH);
	digitalWrite (_SCLK, LOW); // pulse sclk
}

