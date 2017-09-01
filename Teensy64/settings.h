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

#ifndef settings_h_
#define settings_h_

#define PAL         1 //use 0 for NTSC
#define FASTBOOT      1 //0 to disable fastboot
#define EXACTTIMINGDURATION 1600ul //ms
#ifndef M1N
#define M1N 734544.2f
#endif
#ifndef M2N
#define M2N 412155.4f
#endif

//
// Do not edit values below this line
//
//Automatic values :
#if PAL == 1
#define CRYSTAL       M1N
#define CLOCKSPEED      ( CRYSTAL / 18.0f) // 985248,61 Hz
#define CYCLESPERRASTERLINE 63
#define LINECNT       312 //Rasterlines
#define VBLANK_FIRST    300
#define VBLANK_LAST     15
#define NTSC        0

#else
#define CRYSTAL       M2N
#define CLOCKSPEED      ( CRYSTAL / 14.0f) // 1022727,14 Hz
#define CYCLESPERRASTERLINE 64
#define LINECNT       263 //Rasterlines
#define VBLANK_FIRST    13
#define VBLANK_LAST     40
#define NTSC        1
#endif

#define LINEFREQ      (CLOCKSPEED / CYCLESPERRASTERLINE) //Hz
#define REFRESHRATE       (LINEFREQ / LINECNT) //Hz
#define LINETIMER_DEFAULT_FREQ (1000000.0f/LINEFREQ)


#define MCU_C64_RATIO   (F_CPU / CLOCKSPEED) //MCU Cycles per C64 Cycle
#define US_C64_CYCLE    (1000000.0f / CLOCKSPEED) // Duration (µs) of a C64 Cycle
#define nanos()       (ARM_DWT_CYCCNT / (int)(F_CPU / 1000000))


//#define AUDIOSAMPLERATE     ((int)CLOCKSPEED / 16)// (~62kHz)
#define AUDIOSAMPLERATE     ((int)CLOCKSPEED / 32)// (~31kHz)
//#define AUDIOSAMPLERATE     ((int)CLOCKSPEED / 64)// (~15kHz)
//#define AUDIOSAMPLERATE     (LINEFREQ * 2)// (~15kHz)

#define ISR_DAC           0
#define ISR_USB           53

#ifndef I1P
#define ISR_PRIORITY_DAC      200
#define ISR_PRIORITY_AUDIO      180
#define ISR_PRIORITY_RASTERLINE   32
#define ISR_PRIORITY_USB_OTG    78
#else
#define ISR_PRIORITY_DAC      I1P
#define ISR_PRIORITY_AUDIO      I2P
#define ISR_PRIORITY_RASTERLINE   I3P
#define ISR_PRIORITY_USB_OTG    I4P
#endif



//Teensy Pins

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

#define LED_INIT  {pinMode(13,OUTPUT);}
#define LED_ON    {digitalWriteFast(13,1);}
#define LED_OFF   {digitalWriteFast(13,0);}
#define LED_TOGGLE  {GPIOC_PTOR=32;} // This toggles the Teensy Builtin LED pin 13

#define PIN_RESET       25 //PTA5
#define PIN_SERIAL_ATN   4 //PTA13
#define PIN_SERIAL_CLK  26 //PTA14
#define PIN_SERIAL_DATA 27 //PTA15

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


#define PIN_JOY1_BTN    24 //PTE26
#define PIN_JOY1_1       0 //PTB16
#define PIN_JOY1_2       1 //PTB17
#define PIN_JOY1_3      29 //PTB18
#define PIN_JOY1_4      30 //PTB19
#define PIN_JOY1_A1     A14
#define PIN_JOY1_A2     A15

#define PIN_JOY2_BTN     5 //PTD7
#define PIN_JOY2_1       2 //PTD0
#define PIN_JOY2_2       7 //PTD2
#define PIN_JOY2_3       8 //PTD3
#define PIN_JOY2_4       6 //PTD4
#define PIN_JOY2_A1     A12
#define PIN_JOY2_A2     A13

#define JOYSTICK1() (((~GPIOB_PDIR >> 16) & 0x0f) | (((~GPIOE_PDIR >> 26) & 0x01) << 4)) //joystick hoch, runter, links, rechts, feuer
#define JOYSTICK2   ({uint32_t v = GPIOD_PDIR;v =( (~v & 0x01) | ((~v & 0x1c) >> 1) | ((~v & 0x80) >> 3) ) & 0x1f;})  // PTD0, PTD2, PTD3, PTD4, PTD7



#if 0 // userport
#define PIN_PA2_2        3
#define PIN_PB2_0       15
#define PIN_PB2_1       22
#define PIN_PB2_2       23
#define PIN_PB2_3        9
#define PIN_PB2_4       10
#define PIN_PB2_6       11
#define PIN_PB2_7       12
#define PIN_PB2_5       13
#define PIN_CNT1        16
#define PIN_CNT2        19
#define PIN_SP1         17
#define PIN_SP2         18
#define PIN_PC2         35
#define PIN_FLAG2       36
#endif

#endif
