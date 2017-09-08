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

#ifndef Teensy64_cpu_h_
#define Teensy64_cpu_h_

#include <Arduino.h>
#include <arm_math.h>

#define INLINE 			static inline __attribute__((always_inline)) 
#define INLINEOP 		static inline __attribute__((always_inline, flatten))
#define OPCODE			static 

#define RAMSIZE 		65536	//Bytes


#include "settings.h"
#include "roms.h"
#include "patches.h"
#include "util.h"
#include "pla.h" 
#include "vic.h"
#include "keyboard.h"
#include "cia1.h"
#include "cia2.h"

#include <Audio.h>
#include <reSID.h>

extern AudioPlaySID  playSID;

#define BASE_STACK     0x100

struct tcpu {
  unsigned long exactTiming; 

  //6502 CPU registers
  uint8_t sp, a, x, y, cpustatus;
  uint8_t penaltyop, penaltyaddr;
  uint8_t nmi;
  uint16_t pc;  
 
  //helper variables
  uint16_t reladdr;  
  uint16_t ea;

  unsigned ticks;
  unsigned lineCycles;
  unsigned long lineStartTime;

  r_rarr_ptr_t plamap_r; //Memory-Mapping read
  w_rarr_ptr_t plamap_w; //Memory-Mapping write
  uint8_t _exrom:1, _game:1;
    
  tvic vic;
  tcia cia1;
  tcia cia2;

  union {
	uint8_t RAM[RAMSIZE];
	uint16_t RAM16[RAMSIZE/2];
	uint32_t RAM32[RAMSIZE/4];
  };
   
  uint8_t cartrigeLO[1]; //TODO
  uint8_t cartrigeHI[1]; //TODO

};

extern struct tcpu cpu;

void cpu_reset();
void cpu_nmi();
void cpu_clearNmi();
void cpu_clock(int cycles);
void cpu_setExactTiming();

void cia_clockt(int ticks);
#endif