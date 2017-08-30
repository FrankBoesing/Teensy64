/*
  Copyright Frank Bösing, 2017

  This file is part of Teensy64.

    Teensy64 is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Teensy64 is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Teensy64.  If not, see <http://www.gnu.org/licenses/>.

    Diese Datei ist Teil von Teensy64.

    Teensy64 ist Freie Software: Sie können es unter den Bedingungen
    der GNU General Public License, wie von der Free Software Foundation,
    Version 3 der Lizenz oder (nach Ihrer Wahl) jeder späteren
    veröffentlichten Version, weiterverbreiten und/oder modifizieren.

    Teensy64 wird in der Hoffnung, dass es nützlich sein wird, aber
    OHNE JEDE GEWÄHRLEISTUNG, bereitgestellt; sogar ohne die implizite
    Gewährleistung der MARKTFÄHIGKEIT oder EIGNUNG FÜR EINEN BESTIMMTEN ZWECK.
    Siehe die GNU General Public License für weitere Details.

    Sie sollten eine Kopie der GNU General Public License zusammen mit diesem
    Programm erhalten haben. Wenn nicht, siehe <http://www.gnu.org/licenses/>.

*/
#ifndef Teensy64_h_
#define Teensy64_h_

#include <SD.h>
#include <SPI.h>
#include <Audio.h>
#include <reSID.h>
//#include <XPT2046_Touchscreen.h>

#include "usb_USBHost.h"
#include "ili9341_t64.h"
#include "cpu.h"
#include "logo.h"


void initMachine();
void resetMachine() __attribute__ ((noreturn));
void resetExternal();
unsigned loadFile(const char *filename);

extern uint8_t SDinitialized;
extern ILI9341_t3DMA tft;
extern USBHost myusb;

#endif