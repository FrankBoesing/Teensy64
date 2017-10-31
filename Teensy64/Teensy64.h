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

#include <Arduino.h>
#include <DMAChannel.h>
#include "settings.h"

#define VERSION "09"
#define NTSC (!PAL)
#define USBHOST (!PS2KEYBOARD)


#if VGA
#include <uVGA.h>
extern uint8_t * VGA_frame_buffer;
extern uVGA uvga;
#else

#include "logo.h"
#include "ili9341_t64.h"
extern ILI9341_t3DMA tft;
//#include <XPT2046_Touchscreen.h> //Not used
#endif


#if USBHOST
#include "keyboard_usb.h"
extern USBHost myusb;
#endif

#if VGA && USBHOST
#define USBHS_ASYNC_ON {\
		USBHS_USBCMD |= USBHS_USBCMD_ASE;\
		Serial.println("USBHS: Async ON.");\
}		

#define USBHS_ASYNC_OFF {\
		USBHS_USBCMD &= ~(USBHS_USBCMD_ASE);\
		Serial.println("USBHS: Async OFF.");\
}		
#else
#define USBHS_ASYNC_ON
#define USBHS_ASYNC_OFF
#endif

void initMachine();
void resetMachine() __attribute__ ((noreturn));
void resetExternal();
unsigned loadFile(const char *filename);

extern uint8_t SDinitialized;


#if PAL == 1
#define CRYSTAL       	17734475.0f
#define CLOCKSPEED      ( CRYSTAL / 18.0f) // 985248,61 Hz
#define CYCLESPERRASTERLINE 63
#define LINECNT         312 //Rasterlines
#define VBLANK_FIRST    300
#define VBLANK_LAST     15

#else
#define CRYSTAL       	14318180.0f
#define CLOCKSPEED      ( CRYSTAL / 14.0f) // 1022727,14 Hz
#define CYCLESPERRASTERLINE 64
#define LINECNT       	263 //Rasterlines
#define VBLANK_FIRST    13
#define VBLANK_LAST     40
#endif

#define LINEFREQ      			(CLOCKSPEED / CYCLESPERRASTERLINE) //Hz
#define REFRESHRATE       		(LINEFREQ / LINECNT) //Hz
#define LINETIMER_DEFAULT_FREQ (1000000.0f/LINEFREQ)


#define MCU_C64_RATIO   ((float)F_CPU / CLOCKSPEED) //MCU Cycles per C64 Cycle
#define US_C64_CYCLE    (1000000.0f / CLOCKSPEED) // Duration (µs) of a C64 Cycle

#define AUDIOSAMPLERATE     (LINEFREQ * 2)// (~32kHz)

#define ISR_PRIORITY_RASTERLINE   255


//Pins

#define LED_INIT  {pinMode(13,OUTPUT);}
#define LED_ON    {digitalWriteFast(13,1);}
#define LED_OFF   {digitalWriteFast(13,0);}
#define LED_TOGGLE  {GPIOC_PTOR=32;} // This toggles the Teensy Builtin LED pin 13

#if PS2KEYBOARD

#define PIN_PS2DATA 27
#define PIN_PS2INT  24

#endif

#if VGA
#define UVGA_240M_452X300
#define PIN_VSYNC       29
#define PIN_HSYNC       22
#define PIN_RESET       25 //PTA5
#define PIN_SERIAL_ATN   3 //PTA13
#define PIN_SERIAL_CLK  26 //PTA14
#define PIN_SERIAL_DATA 28 //PTA15
#define PIN_SERIAL_SRQ   4 //PTC9



#if 1
#define WRITE_ATN_CLK_DATA(value) { \
    digitalWriteFast(PIN_SERIAL_ATN, (~value & 0x08));\
	digitalWriteFast(PIN_SERIAL_CLK, (~value & 0x10));\
	digitalWriteFast(PIN_SERIAL_DATA, (~value & 0x20));\
}
/*
#define READ_CLK_DATA() \
  ((digitalReadFast(PIN_SERIAL_CLK) << 6) | \
   (digitalReadFast(PIN_SERIAL_DATA) << 7))
  */
#define READ_CLK_DATA() 0

#else
#define WRITE_ATN_CLK_DATA(value) { \
    GPIOA_PCOR = ((value >> 3) & 0x07) << 13; \
    GPIOA_PSOR = ((~value >> 3) & 0x07) << 13;\
  }
#define READ_CLK_DATA() \
  (((GPIOA_PDIR >> 14) & 0x03) << 6)
#endif

#define PIN_JOY1_BTN     30
#define PIN_JOY1_1       16
#define PIN_JOY1_2       17
#define PIN_JOY1_3       18
#define PIN_JOY1_4       19
#define PIN_JOY1_A1     A12
#define PIN_JOY1_A2     A19

#define PIN_JOY2_BTN    37
#define PIN_JOY2_1      15
#define PIN_JOY2_2      23
#define PIN_JOY2_3      35
#define PIN_JOY2_4      36
#define PIN_JOY2_A1     A13
#define PIN_JOY2_A2     A20

#else //ILI9341

#define SCK       14
#define MISO      39
#define MOSI      28
#define TFT_TOUCH_CS    38
#define TFT_TOUCH_INT   37
#define TFT_DC          20
#define TFT_CS          21
#define TFT_RST         255  // 255 = unused, connected to 3.3V
#define TFT_SCLK        SCK
#define TFT_MOSI        MOSI
#define TFT_MISO        MISO

#define PIN_RESET       25 //PTA5
#define PIN_SERIAL_ATN   4 //PTA13
#define PIN_SERIAL_CLK  26 //PTA14
#define PIN_SERIAL_DATA 27 //PTA15
#define PIN_SERIAL_SRQ  36 //PTC9

#if 0
#define WRITE_ATN_CLK_DATA(value) { \
    digitalWriteFast(PIN_SERIAL_ATN, (~value & 0x08));\//PTA13 IEC ATN 3
digitalWriteFast(PIN_SERIAL_CLK, (~value & 0x10)); \ //PTA14 IEC CLK 4
digitalWriteFast(PIN_SERIAL_DATA, (~value & 0x20)); \ //PTA15 IEC DATA 5
}
#define READ_CLK_DATA() \
  ((digitalReadFast(PIN_SERIAL_CLK) << 6) | \
   (digitalReadFast(PIN_SERIAL_DATA) << 7))

#else
#define WRITE_ATN_CLK_DATA(value) { \
    GPIOA_PCOR = ((value >> 3) & 0x07) << 13; \
    GPIOA_PSOR = ((~value >> 3) & 0x07) << 13;\
  }
#define READ_CLK_DATA() \
  (((GPIOA_PDIR >> 14) & 0x03) << 6)
#endif

#define PIN_JOY1_BTN     5 //PTD7
#define PIN_JOY1_1       2 //PTD0 up
#define PIN_JOY1_2       7 //PTD2 down
#define PIN_JOY1_3       8 //PTD3 left
#define PIN_JOY1_4       6 //PTD4 right
#define PIN_JOY1_A1     A12
#define PIN_JOY1_A2     A13

#define PIN_JOY2_BTN    24 //PTE26
#define PIN_JOY2_1       0 //PTB16 up
#define PIN_JOY2_2       1 //PTB17 down
#define PIN_JOY2_3      29 //PTB18 left
#define PIN_JOY2_4      30 //PTB19 right
#define PIN_JOY2_A1     A14
#define PIN_JOY2_A2     A15

#endif

#include <SdFat.h>
#include "output_dac.h"
#include <reSID.h>
#include "cpu.h"
#endif