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

#ifndef _ILI9341_C64H_
#define _ILI9341_C64H_

#ifdef __cplusplus
#include <Arduino.h>
#include <SPI.h>
#include <DMAChannel.h>
#endif

#define ILI9341_TFTWIDTH  320
#define ILI9341_TFTHEIGHT 240

#define ILI9341_NOP     0x00
#define ILI9341_SWRESET 0x01
#define ILI9341_RDDID   0x04
#define ILI9341_RDDST   0x09

#define ILI9341_SLPIN   0x10
#define ILI9341_SLPOUT  0x11
#define ILI9341_PTLON   0x12
#define ILI9341_NORON   0x13

#define ILI9341_RDMODE  0x0A
#define ILI9341_RDMADCTL  0x0B
#define ILI9341_RDPIXFMT  0x0C
#define ILI9341_RDIMGFMT  0x0D
#define ILI9341_RDSELFDIAG  0x0F

#define ILI9341_INVOFF  0x20
#define ILI9341_INVON   0x21
#define ILI9341_GAMMASET 0x26
#define ILI9341_DISPOFF 0x28
#define ILI9341_DISPON  0x29

#define ILI9341_CASET   0x2A
#define ILI9341_PASET   0x2B
#define ILI9341_RAMWR   0x2C
#define ILI9341_RAMRD   0x2E

#define ILI9341_PTLAR    0x30
#define ILI9341_MADCTL   0x36
#define ILI9341_VSCRSADD 0x37
#define ILI9341_PIXFMT   0x3A

#define ILI9341_FRMCTR1 0xB1
#define ILI9341_FRMCTR2 0xB2
#define ILI9341_FRMCTR3 0xB3
#define ILI9341_INVCTR  0xB4
#define ILI9341_DFUNCTR 0xB6

#define ILI9341_PWCTR1  0xC0
#define ILI9341_PWCTR2  0xC1
#define ILI9341_PWCTR3  0xC2
#define ILI9341_PWCTR4  0xC3
#define ILI9341_PWCTR5  0xC4
#define ILI9341_VMCTR1  0xC5
#define ILI9341_VMCTR2  0xC7

#define ILI9341_RDID1   0xDA
#define ILI9341_RDID2   0xDB
#define ILI9341_RDID3   0xDC
#define ILI9341_RDID4   0xDD

#define ILI9341_GMCTRP1 0xE0
#define ILI9341_GMCTRN1 0xE1

#define MADCTL_MY  0x80
#define MADCTL_MX  0x40
#define MADCTL_MV  0x20
#define MADCTL_ML  0x10
#define MADCTL_RGB 0x00
#define MADCTL_BGR 0x08
#define MADCTL_MH  0x04

#ifdef __cplusplus

//#define SCREEN_DMA_MAX_SIZE 65536UL
#define SCREEN_DMA_MAX_SIZE 0xD000
#define SCREEN_DMA_NUM_SETTINGS (((uint32_t)((2 * ILI9341_TFTHEIGHT * ILI9341_TFTWIDTH) / SCREEN_DMA_MAX_SIZE))+1)

typedef struct {
	const unsigned char *index;
	const unsigned char *unicode;
	const unsigned char *data;
	unsigned char version;
	unsigned char reserved;
	unsigned char index1_first;
	unsigned char index1_last;
	unsigned char index2_first;
	unsigned char index2_last;
	unsigned char bits_index;
	unsigned char bits_width;
	unsigned char bits_height;
	unsigned char bits_xoffset;
	unsigned char bits_yoffset;
	unsigned char bits_delta;
	unsigned char line_space;
	unsigned char cap_height;
} ILI9341_t3_font_t;

extern uint32_t * screen32;

class ILI9341_t3DMA : public Print
{
  public:
  	ILI9341_t3DMA(uint8_t _CS, uint8_t _DC, uint8_t _RST = 255, uint8_t _MOSI=11, uint8_t _SCLK=13, uint8_t _MISO=12);
	void begin(void);
	void refresh(void);

	void fillScreen(uint16_t color);
	void writeScreen(const uint16_t *pcolors);
	uint16_t color565(uint8_t r, uint8_t g, uint8_t b) { return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3); }
	
	//color565toRGB		- converts 565 format 16 bit color to 8-Bit RGB
	void color565toRGB(uint16_t color, uint8_t &r, uint8_t &g, uint8_t &b) {
		r = (color>>8)&0x00F8;
		g = (color>>3)&0x00FC;
		b = (color<<3)&0x00F8;
	}

	void start(void);

	void drawPixel(int16_t x, int16_t y, uint16_t color);
	uint16_t getPixel(int16_t x, int16_t y);
	
	void blur();
	void dim();
	
	void setCursor(int16_t x, int16_t y) {cursor_x = x; cursor_y = y;};    
	void setTextColor(uint16_t c) {textcolor = textbgcolor = c;};
	void setTextColor(uint16_t c, uint16_t bg)  {textcolor = c; textbgcolor = bg;};
	
	int16_t getCursorX(void) const { return cursor_x; }
	int16_t getCursorY(void) const { return cursor_y; }
	void setFont(const ILI9341_t3_font_t &f) { font = &f; }	
	
	void drawFontBits(uint32_t bits, uint32_t numbits, uint32_t x, uint32_t y, uint32_t repeat);
	void drawFontChar(unsigned int c);
	size_t write(uint8_t c) { drawFontChar(c);return 1; }
	
 protected:
	DMAChannel dmatx;
	DMASetting dmasettings[SCREEN_DMA_NUM_SETTINGS];
  	uint8_t _rst, _cs, _dc;
	uint8_t _miso, _mosi, _sclk;
	const ILI9341_t3_font_t *font;	
	int16_t  cursor_x, cursor_y;
	uint16_t textcolor, textbgcolor;

};

#endif
#endif
