/*
 * ht16k33.cpp - used to talk to the htk1633 chip to do things like turn on LEDs or scan keys
 * Copyright:  Peter Sjoberg <peters-alib AT techwiz.ca>
 * License: GPLv3
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 3 as 
    published by the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 *
 * History:
 * 2015-10-04  Peter Sjoberg <peters-alib AT techwiz.ca>
 *             Created using https://www.arduino.cc/en/Hacking/LibraryTutorial and ht16k33 datasheet
 * 2015-11-25  Peter Sjoberg <peters-alib AT techwiz DOT ca>
 *	       first check in to github
 *
 *
 *
 *
 *
 */

#include "Arduino.h"
#include "ht16k33.h"
#include <Wire.h>

// "address" is base address 0-7 which becomes 11100xxx = E0-E7
#define BASEHTADDR 0x70

//Commands
#define HT16K33_DDAP          B00000000 // Display data address pointer: 0000xxxx
#define HT16K33_SS            B00100000 // System setup register
#define HT16K33_SS_STANDBY    B00000000 // System setup - oscillator in standby mode
#define HT16K33_SS_NORMAL     B00000001 // System setup - oscillator in normal mode
#define HT16K33_KDAP          B01000000 // Key Address Data Pointer
#define HT16K33_IFAP          B01100000 // Read status of INT flag
#define HT16K33_DSP           B10000000 // Display setup
#define HT16K33_DSP_OFF       B00000000 // Display setup - display off
#define HT16K33_DSP_ON        B00000001 // Display setup - display on
#define HT16K33_DSP_NOBLINK   B00000000 // Display setup - no blink
#define HT16K33_DSP_BLINK2HZ  B00000010 // Display setup - 2hz blink
#define HT16K33_DSP_BLINK1HZ  B00000100 // Display setup - 1hz blink
#define HT16K33_DSP_BLINK05HZ B00000110 // Display setup - 0.5hz blink
#define HT16K33_RIS           B10100000 // ROW/INT Set
#define HT16K33_RIS_OUT       B00000000 // Set INT as row driver output
#define HT16K33_RIS_INTL      B00000001 // Set INT as int active low
#define HT16K33_RIS_INTH      B00000011 // Set INT as int active high
#define HT16K33_DIM           B11100000 // Dimming set
#define HT16K33_DIM_1         B00000000 // Dimming set - 1/16
#define HT16K33_DIM_2         B00000001 // Dimming set - 2/16
#define HT16K33_DIM_3         B00000010 // Dimming set - 3/16
#define HT16K33_DIM_4         B00000011 // Dimming set - 4/16
#define HT16K33_DIM_5         B00000100 // Dimming set - 5/16
#define HT16K33_DIM_6         B00000101 // Dimming set - 6/16
#define HT16K33_DIM_7         B00000110 // Dimming set - 7/16
#define HT16K33_DIM_8         B00000111 // Dimming set - 8/16
#define HT16K33_DIM_9         B00001000 // Dimming set - 9/16
#define HT16K33_DIM_10        B00001001 // Dimming set - 10/16
#define HT16K33_DIM_11        B00001010 // Dimming set - 11/16
#define HT16K33_DIM_12        B00001011 // Dimming set - 12/16
#define HT16K33_DIM_13        B00001100 // Dimming set - 13/16
#define HT16K33_DIM_14        B00001101 // Dimming set - 14/16
#define HT16K33_DIM_15        B00001110 // Dimming set - 15/16
#define HT16K33_DIM_16        B00001111 // Dimming set - 16/16

// Constructor
HT16K33::HT16K33(){
}

/****************************************************************/
// Setup the env
//
void HT16K33::begin(uint8_t address){
  uint8_t i;
  _address=address | BASEHTADDR;
  Wire.begin();
  _i2c_write(HT16K33_SS  | HT16K33_SS_NORMAL); // Wakeup
  _i2c_write(HT16K33_DSP | HT16K33_DSP_ON | HT16K33_DSP_NOBLINK); // Display on and no blinking
  _i2c_write(HT16K33_RIS | HT16K33_RIS_OUT); // INT pin works as row output 
  _i2c_write(HT16K33_DIM | HT16K33_DIM_16);  // Brightness set to max
  //Clear all lights
  for (i = 0 ; i < 16 ; i++){
	displayRam[i] = 0 ;
  }
//  memcpy(displayRam,0,sizeof(displayRam));
  _i2c_write(HT16K33_DDAP, displayRam,sizeof(displayRam),true);
} // begin

/****************************************************************/
// internal function - Write a single byte
//
uint8_t HT16K33::_i2c_write(uint8_t val){
  Wire.beginTransmission(_address);
  Wire.write(val);
  return Wire.endTransmission();
} // _i2c_write

/****************************************************************/
// internal function - Write several bytes
// "size" is amount of data to send excluding the first command byte
// if LSB is true then swap high and low byte to send LSB MSB
// NOTE: Don't send odd amount of data if using LSB, then it will send one to much
//
uint8_t HT16K33::_i2c_write(uint8_t cmd,uint8_t *data,uint8_t size,boolean LSB){
  uint8_t i;
  Wire.beginTransmission(_address);
  Wire.write(cmd);
  i=0;
  while (i<size){
    if (LSB){
      Wire.write(data[i+1]);
      Wire.write(data[i++]);
      i++;
    } else {
      Wire.write(data[i++]);
    }
  }
  return Wire.endTransmission(); // Send out the data
} // _i2c_write

/****************************************************************/
// internal function - read a byte from specific address (send one byte(address to read) and read a byte)
//
uint8_t HT16K33::_i2c_read(uint8_t addr){
  _i2c_write(addr);
  Wire.requestFrom(_address,(uint8_t) 1);
  return Wire.read();    // read one byte
} // _i2c_read

/****************************************************************/
// read an array from specific address (send a byte and read several bytes back)
// return value is how many bytes that where really read
//
uint8_t HT16K33::_i2c_read(uint8_t addr,uint8_t *data,uint8_t size){
  uint8_t i,retcnt,val;
  
  _i2c_write(addr);
  retcnt=Wire.requestFrom(_address, size);
  i=0;
  while(Wire.available() && i<size)    // slave may send less than requested
  {
    data[i++] = Wire.read();    // receive a byte as character
  }

  return retcnt;
} // _i2c_read

/****************************************************************/
// Put the chip to sleep
//
uint8_t HT16K33::sleep(){
  return _i2c_write(HT16K33_SS|HT16K33_SS_STANDBY); // Stop oscillator
} // sleep

/****************************************************************/
// Wake up the chip (after it been a sleep )
//
uint8_t HT16K33::normal(){
  return _i2c_write(HT16K33_SS|HT16K33_SS_NORMAL); // Start oscillator
} // normal

/****************************************************************/
// Turn off one led but only in memory
// To do it on chip a call to "sendLed" is needed
//
uint8_t HT16K33::clearLed(uint8_t ledno){ // 16x8 = 128 LEDs to turn on, 0-127
  if (ledno>=0 && ledno<128){
    bitClear(displayRam[int(ledno/8)],(ledno % 8));
    return 0;
  } else {
    return 1;
  }
} // clearLed

/****************************************************************/
// Turn on one led but only in memory
// To do it on chip a call to "sendLed" is needed
//
uint8_t HT16K33::setLed(uint8_t ledno){ // 16x8 = 128 LEDs to turn on, 0-127
  if (ledno>=0 && ledno<128){
    bitSet(displayRam[int(ledno/8)],(ledno % 8));
    return 0;
  } else {
    return 1;
  }
} // setLed

/****************************************************************/
// check if a specific led is on(true) or off(false)
//
boolean HT16K33::getLed(uint8_t ledno,boolean Fresh){ 

  // get the current state from chip
  if (Fresh) {
    _i2c_read(HT16K33_DDAP, displayRam,sizeof(displayRam));
  }

  if (ledno>=0 && ledno<128){
    return bitRead(displayRam[int(ledno/8)],ledno % 8) != 0;
  }
} // getLed

/****************************************************************/
uint8_t HT16K33::setDisplayRaw(uint8_t pos, uint8_t val) {
  if (pos < sizeof(displayRam)) {
    displayRam[pos] = val;
    return 0;
  } else {
    return 1;
  }
} // setDisplayRaw

/****************************************************************/
// Send the display ram info to chip - kind of commit all changes to the outside world
//
uint8_t HT16K33::sendLed(){
  return _i2c_write(HT16K33_DDAP, displayRam,sizeof(displayRam));
} // sendLed

/****************************************************************/
// set a single LED and update NOW
//
uint8_t HT16K33::setLedNow(uint8_t ledno){
  uint8_t rc;
  rc=setLed(ledno);
  if (rc==0){
    return sendLed();
  } else {
    return rc;
  }
} // setLedNow

/****************************************************************/
// clear a single LED and update NOW
//
uint8_t HT16K33::clearLedNow(uint8_t ledno){
  uint8_t rc;
  rc=clearLed(ledno);
  if (rc==0){
    return sendLed();
  } else {
    return rc;
  }
} // clearLedNow

/****************************************************************/
// Change brightness of the whole display
// level 0-15, 0 means display off
//
uint8_t HT16K33::setBrightness(uint8_t level){
  if (HT16K33_DIM_1>=0 && level <HT16K33_DIM_16){
    return _i2c_write(HT16K33_DIM|level);
  } else {
    return 1;
  }
} // setBrightness

/****************************************************************/
// Check the chips interrupt flag
// 0 if no new key is pressed
// !0 if some key is pressed and not yet read
//
uint8_t HT16K33::keyINTflag(){ 
  return _i2c_read(HT16K33_IFAP);
} // keyINTflag

/****************************************************************/
// Check if any key is pressed
// returns how many keys that are currently pressed
// 

//From http://stackoverflow.com/questions/109023/how-to-count-the-number-of-set-bits-in-a-32-bit-integer
#ifdef __GNUC__
  uint16_t _popcount(uint16_t x) {
    return __builtin_popcount(x);
  }
#else
  uint16_t _popcount(uint16_t i) {
    i = i - ((i >> 1) & 0x55555555);
    i = (i & 0x33333333) + ((i >> 2) & 0x33333333);
    return (((i + (i >> 4)) & 0x0F0F0F0F) * 0x01010101) >> 24;
  }
#endif

uint8_t HT16K33::keysPressed(){ 
  //  Serial.println(_keyram[0]|_keyram[1]|_keyram[2],HEX);
  return (_popcount(_keyram[0])+_popcount(_keyram[1])+_popcount(_keyram[2]));
} // keysPressed

/****************************************************************/
// Internal function - update cached key array
//
void HT16K33::_updateKeyram(){
  uint8_t curkeyram[6];

  _i2c_read(HT16K33_KDAP, curkeyram, 6);
  _keyram[0]=curkeyram[1]<<8 | curkeyram[0]; // datasheet page 21, 41H is high 40H is low
  _keyram[1]=curkeyram[3]<<8 | curkeyram[2]; // or LSB MSB
  _keyram[2]=curkeyram[5]<<8 | curkeyram[4];
  return;
} // _updateKeyram

/****************************************************************/
// return the key status
//
void HT16K33::readKeyRaw(HT16K33::KEYDATA keydata,boolean Fresh){
  int8_t i;

  // get the current state
  if (Fresh) {_updateKeyram();}

  for (i=0;i<3;i++){
    keydata[i]=_keyram[i];
  }

  return;
} // readKeyRaw

/****************************************************************
 * read the keys and return the key that changed state
 * if more than one is pressed (compared to last scan) 
 * only one is returned, the first one found
 * 0 means no key pressed.
 * "1" means the key #1 is pressed
 * "-1" means the key #1 is released 
 * "clear"=true means it will only look keys currently pressed down.
 *     this is so you can detect what key is still pressed down after
 *     several keys are pressed down and then all but one is released
 *     (without keeping track of up/down separately)
 *
 *Observations:
 * As long as the key is pressed the keyram bit is set
 * the flag is set when key is pressed down but then cleared at first
 * read of key ram.
 * When released the key corresponding bit is cleared but the flag is NOT set
 * This means that the only way a key release can be detected is
 * by only polling readKey and ignoring flag
 * 
 */

int8_t HT16K33::readKey(boolean clear){
  static HT16K33::KEYDATA oldKeyData;
  uint16_t diff;
  uint8_t key;
  int8_t i,j;

    // save the current state
  for (i=0;i<3;i++){
    if (clear){
      oldKeyData[i]=0;
    } else {
      oldKeyData[i]=_keyram[i];
    }
  }
    
  _updateKeyram();

  key=0; //the key that changed state
  for (i=0;i<3;i++){
    diff=_keyram[i] ^ oldKeyData[i]; //XOR old and new, any changed bit is set.
    if ( diff !=0 ){ // something did change
      for (j=0;j<13;j++){
	key++;
	if (((diff >> j) & 1) == 1){
	  if (((_keyram[i] >> j) & 1)==0){
	    return -key;
	  }else{
	    return key;
	  }
	} // if keyram differs
      } // for j in bits
    } else {
      key+=13;
    } // if diff
  } // for i
  return 0; //apperently no new key was pressed - old might still be held down, pass clear=true to see it
} // readKey

/****************************************************************/
// Make the display blink
//
uint8_t HT16K33::setBlinkRate(uint8_t rate){
  switch (rate) {
    case HT16K33_DSP_NOBLINK:
    case HT16K33_DSP_BLINK2HZ:
    case HT16K33_DSP_BLINK1HZ:
    case HT16K33_DSP_BLINK05HZ:
      _i2c_write(HT16K33_DSP | HT16K33_DSP_ON |rate);
      return 0;
      ;;
  default:
    return 1;
  }
} //setBlinkRate

/****************************************************************/
// turn on the display
//
void HT16K33::displayOn(){
  _i2c_write(HT16K33_DSP |HT16K33_DSP_ON);
} // displayOn

/****************************************************************/
// turn off the display
//
void HT16K33::displayOff(){
  _i2c_write(HT16K33_DSP |HT16K33_DSP_OFF);
} // displayOff


