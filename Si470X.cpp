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
///////////////////////////////////////////////////////////////////////////////

#include "Si470X.h"

// setup the Arduino pins and pin modes and init the fm radio chip
// region is optional, the default is 0 (USA 200 kHz spacing, 87.5..108 MHz)
// region can be optionally set to 0x01 to select the Europe/Japan (100 kHz spacing, 76..108 MHz)
uint8_t SI470X::init (uint8_t sdio_pin, uint8_t sclk_pin, uint8_t sen_pin, uint8_t reset_pin, uint8_t region)
{
	uint8_t timeout; // prevent lockup if chip not working or wired wrong

	_SDIO = sdio_pin; // serial data i/o (pin 8)
	_SCLK = sclk_pin; // serial clock (pin 7)
	_SEN = sen_pin; // serial enable (pin 6)
	_RESET = reset_pin; // reset (pin 5)
	_REGION = region & 0x01; // region code (default: 0 - USA 200 kHz)

	digitalWrite (_SDIO, HIGH); // sdio high
	digitalWrite (_SCLK, HIGH); // sclk high
	digitalWrite (_SEN, LOW); // sen low
	digitalWrite (_RESET, LOW); // reset low

	pinMode (_SDIO, INPUT_PULLUP); // setup i/o DDR's
	pinMode (_SCLK, OUTPUT);
	pinMode (_SEN, OUTPUT);
	pinMode (_RESET, OUTPUT);

	// if SEN is low reset is de-asserted, 3 wire mode is selected
	digitalWrite (_RESET, HIGH); // de-assert reset
	_delay_ms (1); // tiny delay to assure reset complete

	readRegisters (); // read current chip registers
	_REGISTERS[TEST1] |= _BV (XOSCEN); // enable the oscillator (AN230 pg. 12)
	writeRegisters (); // update chip registers
	_delay_ms (500); // Wait for clock to settle (pg. 12)

	readRegisters (); // read current chip registers
	_REGISTERS[POWERCFG] |= (_BV (DMUTE) | _BV (ENABLE)); // disable mute, enable IC (pg. 12)
	_REGISTERS[POWERCFG] &= ~_BV (DISABLE); // why does it have an enable and disable bit?
	_REGISTERS[SYSCONFIG1] |= (1 << RDS); // enable rds reception

	// set band & channel spacing (AN230, pg. 18)
	_REGISTERS[SYSCONFIG2] |= (_REGION << SPACE); // channel spacing 200 kHz (USA/Europe default)
	_REGISTERS[SYSCONFIG2] |= (_REGION << BAND); // band select 87.5-108 MHz (default)

	writeRegisters (); // update chip registers

	// set seek threshold (AN230, pg. 40)
	setTreshold (0x01); // set seek threshold to "recommended"
	timeout = 20; // don't lock up if init fails

	while (timeout--) {
		readRegisters (); // read current chip registers

		// "firmware" (bits 0..5) and "dev" (bits 6..9) return zero until the
		// chip is powered up - this is how we wait for the chip to be ready.
		if (_REGISTERS[CHIPID] & (FIRMWARE | DEV)) {
			return 0; // return success
		}
	}

	return 1; // return failure
}

// set volume 0 ...100
void SI470X::setVolume (uint8_t volume)
{
	if (volume > 100) {
		return; // if invalid setting, just bail
	}

	volume = ((volume * 100) / 625); // scale 0...100 to 0...15 evenly
	volume = (volume > 15) ? 15 : volume; // clip over 100
	readRegisters (); // read current chip registers
	_REGISTERS[SYSCONFIG2] &= ~0x0F; // Clear volume bits
	_REGISTERS[SYSCONFIG2] |= (volume & 0x0F); // Set new volume
	writeRegisters (); // update chip registers
}

// set FM channel, no decimal point (i.e. 104.1 is sent as 1041)
// we don't check for out of band settings - but these just wrap anyway
void SI470X::setChannel (uint16_t channel)
{
	readRegisters (); // read current chip registers
	_REGISTERS[CHANNEL] &= ~0x01FF; // Clear out the channel bits
	_REGISTERS[CHANNEL] |= ((channel -= _CHAN_OFFSET[_REGION]) / _CHAN_MULT[_REGION]); // OR in the new channel
	_REGISTERS[CHANNEL] |= _BV (TUNE); // Set the TUNE bit to start
	writeRegisters (); // update chip registers

	while (1) {
		readRegisters (); // read current chip registers

		// wait for "seek complete" to be asserted
		if (_REGISTERS[STATUSRSSI] & _BV (STC)) {
			break;
		}
	}

	_REGISTERS[CHANNEL] &= ~_BV (TUNE); // tune complete, clear tune bit
	writeRegisters (); // update chip registers
}

// get FM channel (returned without decimal point (i.e. 104.1 returns as 1041)
uint16_t SI470X::getChannel (void)
{
	readRegisters (); // read current chip registers
	return (((_REGISTERS[READCHANNEL] & 0x01FF) * _CHAN_MULT[_REGION]) + _CHAN_OFFSET[_REGION]);
}

// returns received signal strength (RSSI) in dB microvolts
uint8_t SI470X::getSignal (void)
{
	readRegisters (); // read current chip registers
	return (_REGISTERS[STATUSRSSI] & 0xFF); // received signal strength indicator is 8 bits
}

// returns true if station is stereo and chip is actually decoding stereo
uint8_t SI470X::getStereo (void)
{
	readRegisters (); // read current chip registers
	return (_REGISTERS[STATUSRSSI] & _BV (STEREO)) ? true : false;
}

// seek threshold settings (AN230, pg. 40)
// 0 = default
// 1 = recommended
// 2 = more stations
// 3 = good quality stations only
// 4 = most stations
void SI470X::setTreshold (uint8_t th)
{
	if (th > 4) {
		return; // if invalid setting, just bail
	}

	uint8_t thr[5] = { 0x19, 0x19, 0x0C, 0x0C, 0x00 };
	uint8_t snr[5] = { 0x00, 0x04, 0x04, 0x07, 0x04 };
	uint8_t cnt[5] = { 0x00, 0x08, 0x08, 0x0F, 0x0F };

	readRegisters (); // read current chip registers
	_REGISTERS[SYSCONFIG2] |= (thr[th] << SEEKTH); // set seek threshold
	_REGISTERS[SYSCONFIG3] |= (snr[th] << SKSNR); // set seek s/n ratio
	_REGISTERS[SYSCONFIG3] |= (cnt[th] << SKCNT); // set seek fm impulse detect
	writeRegisters (); // update chip registers
}

// mute the audio
uint8_t SI470X::muteOn (void)
{
	return _setMute (true);
}

// unmute the audio
uint8_t SI470X::muteOff (void)
{
	return _setMute (false);
}

// force mono mode (less noise on weak stations)
uint8_t SI470X::monoOn (void)
{
	return _setMono (true);
}

// mono mode off (i.e. stereo)
uint8_t SI470X::monoOff (void)
{
	return _setMono (false);
}

// seek UP to the next highest channel
uint16_t SI470X::seekUp (void)
{
	return _setSeek (true);
}

// seek DOWN to the next lowest channel
uint16_t SI470X::seekDown (void)
{
	return _setSeek (false);
}

// note: the 9 bit "address" contains:
//
// bit: [ 8   7   6 ]  [ 5 ]  [ 4 ]  [ 3    2    1    0 ]
// val: [ 0   1   1 ]  [R/W]  [ 0 ]  [register 0x00-0x0F]
//
// note: WRITE: R/W = 0, READ: R/W = 1
// note: This also applies to "readRegisters()" below.
//
// write all 16 register shadows to the chip (update)
void SI470X::writeRegisters (void)
{
	uint8_t x = 16;

	while (x--) { // writing registers 15..0
		pinMode (_SDIO, OUTPUT); // setup i/o pin as output
		digitalWrite (_SEN, LOW); // enable chip select
		_writeChip (((DEV_ADDR << 6) | x), 9); // send "address" (9 bits)
		_writeChip (_REGISTERS[x], 16); // write data (16 bits)
		digitalWrite (_SEN, HIGH); // disable chip select
		pinMode (_SDIO, INPUT_PULLUP); // set i/o pin as input
		_pulseSCLK (); // send the required 26th clock
	} // note: _SDIO pin is always left as an input when finished
}

// read all 16 chip registers into the register shadows.
// returns a pointer to the register shadows so that an advanced user
// can manipulate registers and bits not supported by this driver.
// see notes on "address" above.
uint16_t *SI470X::readRegisters (void)
{
	uint8_t x = 16;

	while (x--) { // reading registers 15..0
		pinMode (_SDIO, OUTPUT); // setup i/o pin as output
		digitalWrite (_SEN, LOW); // enable chip select
		_writeChip (((DEV_ADDR << 6) | RWBIT | x), 9); // send "address" (9 bits)
		pinMode (_SDIO, INPUT_PULLUP); // set i/o pin as input
		_REGISTERS[x] = _readChip (16); // read data (16 bits)
		digitalWrite (_SEN, HIGH); // disable chip select
		_pulseSCLK (); // send the required 26th clock
	} // note: _SDIO pin is always left as an input when finished

	return _REGISTERS;
}

//////////////////////////////////////////////////////////////////////
////////////////// private functions from here down //////////////////
//////////////////////////////////////////////////////////////////////

// mute audio on/off
uint8_t SI470X::_setMute (uint8_t on)
{
	readRegisters (); // read current chip registers
	// clear or set "disable mute" bit
	on ? _REGISTERS[POWERCFG] &= ~_BV (DMUTE) : _REGISTERS[POWERCFG] |= _BV (DMUTE);
	writeRegisters (); // update chip registers
	readRegisters (); // read current chip registers
	return _REGISTERS[POWERCFG] & _BV (DMUTE) ? false : true; // 1=muted, 0=not muted
}

// force mono mode (less noise on really weak stations)
uint8_t SI470X::_setMono (uint8_t on)
{
	readRegisters (); // read current chip registers
	// set of clear "mono" bit
	on ? _REGISTERS[POWERCFG] |= _BV (MONO) : _REGISTERS[POWERCFG] &= ~_BV (MONO);
	writeRegisters (); // update chip registers
	readRegisters (); // read current chip registers
	return _REGISTERS[POWERCFG] & _BV (MONO) ? true : false; // return mono bit state
}

// seek to the next (up) or previous (down) active channel
uint16_t SI470X::_setSeek (uint8_t updown)
{
	readRegisters (); // read current chip registers
	_REGISTERS[POWERCFG] &= ~_BV (SEEKUP); // seek direction bit
	_REGISTERS[POWERCFG] |= updown ? _BV (SEEKUP) : 0; // set seek up / down
	_REGISTERS[POWERCFG] |= _BV (SEEK); // enable seeking
	writeRegisters (); // update chip registers

	while (! (_REGISTERS[STATUSRSSI] & _BV (SFBL))) { // search until whole band is searched
		readRegisters (); // read current chip registers

		// stop scanning when "seek complete" is asserted
		if (_REGISTERS[STATUSRSSI] & _BV (STC)) {
			break;
		}
	}

	_REGISTERS[POWERCFG] &= ~_BV (SEEK); // seek done, clear seek bit
	writeRegisters (); // update chip registers
	return getChannel (); // return channel found
}

// bit-banger 3-wire receive (MSB first)
uint16_t SI470X::_readChip (uint8_t bits)
{
	uint16_t data = 0;

	while (bits--) {
		_pulseSCLK (); // send one SCLK pulse
		data |= (digitalRead (_SDIO) << bits);
	}

	return data;
}

// bit-banger 3-wire send (MSB first)
void SI470X::_writeChip (uint16_t data, uint8_t bits)
{
	while (bits--) {
		digitalWrite (_SDIO, (data & _BV (bits)) ? HIGH : LOW); // put data on bus
		_pulseSCLK (); // send one SCLK pulse
	}
}

// send out one sclk pulse
void SI470X::_pulseSCLK (void)
{
	digitalWrite (_SCLK, HIGH);
	digitalWrite (_SCLK, LOW);
}

// end of SI470X.cpp
