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

#include "ILI9341_t64.h"
#include "vic_palette.h"

#define SPICLOCK 144e6 //Just a number..max speed

//#define SCREEN_DMA_MAX_SIZE 65536UL
#define SCREEN_DMA_MAX_SIZE 0xD000
#define SCREEN_DMA_NUM_SETTINGS (((uint32_t)((2 * ILI9341_TFTHEIGHT * ILI9341_TFTWIDTH) / SCREEN_DMA_MAX_SIZE))+1)

//const int dma_linestarts[SCREEN_DMA_NUM_SETTINGS] =

DMAMEM uint16_t screen[ILI9341_TFTHEIGHT][ILI9341_TFTWIDTH];
DMASetting dmasettings[SCREEN_DMA_NUM_SETTINGS];
DMAChannel dmatx;

uint16_t * screen16 = (uint16_t*)&screen[0][0];
uint32_t * screen32 = (uint32_t*)&screen[0][0];
const uint32_t * screen32e = (uint32_t*)&screen[0][0] + sizeof(screen) / 4;

static const uint8_t init_commands[] = {
  4, 0xEF, 0x03, 0x80, 0x02,
  4, 0xCF, 0x00, 0XC1, 0X30,
  5, 0xED, 0x64, 0x03, 0X12, 0X81,
  4, 0xE8, 0x85, 0x00, 0x78,
  6, 0xCB, 0x39, 0x2C, 0x00, 0x34, 0x02,
  2, 0xF7, 0x20,
  3, 0xEA, 0x00, 0x00,
  2, ILI9341_PWCTR1, 0x23, // Power control
  2, ILI9341_PWCTR2, 0x10, // Power control
  3, ILI9341_VMCTR1, 0x3e, 0x28, // VCM control
  2, ILI9341_VMCTR2, 0x86, // VCM control2
  2, ILI9341_MADCTL, 0x48, // Memory Access Control
  2, ILI9341_PIXFMT, 0x55,
  3, ILI9341_FRMCTR1, 0x00, 0x18,
  4, ILI9341_DFUNCTR, 0x08, 0x82, 0x27, // Display Function Control
  2, 0xF2, 0x00, // Gamma Function Disable
  2, ILI9341_GAMMASET, 0x01, // Gamma curve selected
  16, ILI9341_GMCTRP1, 0x0F, 0x31, 0x2B, 0x0C, 0x0E, 0x08,
  0x4E, 0xF1, 0x37, 0x07, 0x10, 0x03, 0x0E, 0x09, 0x00, // Set Gamma
  16, ILI9341_GMCTRN1, 0x00, 0x0E, 0x14, 0x03, 0x11, 0x07,
  0x31, 0xC1, 0x48, 0x08, 0x0F, 0x0C, 0x31, 0x36, 0x0F, // Set Gamma
//  3, 0xb1, 0x00, 0x1f, // FrameRate Control 61Hz
  3, 0xb1, 0x00, 0x10, // FrameRate Control 119Hz
  2, ILI9341_MADCTL, MADCTL_MX | MADCTL_MY | MADCTL_MV | MADCTL_BGR,
  0
};

ILI9341_t3DMA::ILI9341_t3DMA(uint8_t cs, uint8_t dc, uint8_t rst, uint8_t mosi, uint8_t sclk, uint8_t miso)
{
  _cs   = cs;
  _dc   = dc;
  _rst  = rst;
  _mosi = mosi;
  _sclk = sclk;
  _miso = miso;
  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);
  digitalWrite(_cs, 1);
  digitalWrite(_dc, 1);

}

void ILI9341_t3DMA::begin(void) {

  SPI.setMOSI(_mosi);
  SPI.setMISO(_miso);
  SPI.setSCK(_sclk);
  SPI.begin();

  if (_rst < 255) { // toggle RST low to reset
    pinMode(_rst, OUTPUT);
    digitalWrite(_rst, HIGH);
    delay(5);
    digitalWrite(_rst, LOW);
    delay(20);
    digitalWrite(_rst, HIGH);
	delay(120);
  }
  SPI.beginTransaction(SPISettings(SPICLOCK, MSBFIRST, SPI_MODE0));
  const uint8_t *addr = init_commands;

  digitalWrite(_cs, 0);

  while (1) {
    uint8_t count = *addr++;
    if (count-- == 0) break;

    digitalWrite(_dc, 0);
    SPI.transfer(*addr++);

    while (count-- > 0) {
      digitalWrite(_dc, 1);
      SPI.transfer(*addr++);
    }
  }


  digitalWrite(_dc, 0);
  SPI.transfer(ILI9341_SLPOUT);
  digitalWrite(_dc, 1);
  digitalWrite(_cs, 1);
  SPI.endTransaction();

  //Set C64 Palette

  digitalWrite(_dc, 1);
  digitalWrite(_cs, 1);
  SPI.endTransaction();

  SPI.beginTransaction(SPISettings(SPICLOCK, MSBFIRST, SPI_MODE0));
  digitalWrite(_dc, 0);
  digitalWrite(_cs, 0);
  SPI.transfer(ILI9341_DISPON);
  digitalWrite(_dc, 1);
  digitalWrite(_cs, 1);
  SPI.endTransaction();

  const uint32_t bytesPerLine = ILI9341_TFTWIDTH * 2;
  const uint32_t maxLines = (SCREEN_DMA_MAX_SIZE / bytesPerLine);
  uint32_t i = 0, sum = 0, lines;
  do {

    //Source:
    lines = min(maxLines, ILI9341_TFTHEIGHT - sum);
    int32_t len = lines * bytesPerLine;
    dmasettings[i].TCD->CSR = 0;
    dmasettings[i].TCD->SADDR = &screen[sum][0];

    dmasettings[i].TCD->SOFF = 2;
    dmasettings[i].TCD->ATTR_SRC = 1;
    dmasettings[i].TCD->NBYTES = 2;
    dmasettings[i].TCD->SLAST = -len;
    dmasettings[i].TCD->BITER = len / 2;
    dmasettings[i].TCD->CITER = len / 2;

    //Destination:
    dmasettings[i].TCD->DADDR = &SPI0_PUSHR;
    dmasettings[i].TCD->DOFF = 0;
    dmasettings[i].TCD->ATTR_DST = 1;
    dmasettings[i].TCD->DLASTSGA = 0;

    dmasettings[i].replaceSettingsOnCompletion(dmasettings[i + 1]);

    sum += lines;
  } while (++i < SCREEN_DMA_NUM_SETTINGS);

  dmasettings[SCREEN_DMA_NUM_SETTINGS - 1].replaceSettingsOnCompletion(dmasettings[0]);
  dmatx.begin(false);
  dmatx.triggerAtHardwareEvent(DMAMUX_SOURCE_SPI0_TX );
  dmatx = dmasettings[0];

};

void ILI9341_t3DMA::start(void) {

  pinMode(_dc, OUTPUT);
  pinMode(_cs, OUTPUT);

  //SPI.usingInterrupt(IRQ_DMA_CH1);

  SPI.beginTransaction(SPISettings(SPICLOCK, MSBFIRST, SPI_MODE0));
  digitalWrite(_cs, 0);
  digitalWrite(_dc, 0);

  SPI.transfer(ILI9341_CASET);
  digitalWrite(_dc, 1);
  SPI.transfer16(0);
  //SPI.transfer16(_width);
  SPI.transfer16(319);

  SPI.transfer(ILI9341_PASET);
  digitalWrite(_dc, 1);
  SPI.transfer16(0);
  //SPI.transfer16(_height);
  SPI.transfer16(239);

  digitalWrite(_dc, 0);
  SPI.transfer(ILI9341_RAMWR);

  digitalWrite(_dc, 1);
  digitalWrite(_cs, 0);

  SPI0_RSER |= SPI_RSER_TFFF_DIRS | SPI_RSER_TFFF_RE;  // Set ILI_DMA Interrupt Request Select and Enable register
  SPI0_MCR &= ~SPI_MCR_HALT;  //Start transfers.
  SPI0_CTAR0 = SPI0_CTAR1;
  (*(volatile uint16_t *)((int)&SPI0_PUSHR + 2)) = (SPI_PUSHR_CTAS(1) | SPI_PUSHR_CONT) >> 16; //Enable 16 Bit Transfers + Continue-Bit

}


void ILI9341_t3DMA::refresh(void) {
  start();
  dmasettings[SCREEN_DMA_NUM_SETTINGS - 1].TCD->CSR &= ~DMA_TCD_CSR_DREQ; //disable "disableOnCompletion"
  dmatx = dmasettings[0];
  dmatx.enable();
}

/*******************************************************************************************************************/
/*******************************************************************************************************************/
/*******************************************************************************************************************/

void ILI9341_t3DMA::fillScreen(uint16_t color) {
  uint32_t col32 = (color << 16) | color;
  uint32_t * p = screen32;
  do {
    *p++ = col32;
  } while (p < screen32e);
}

void ILI9341_t3DMA::writeScreen(const uint16_t *pcolors) {
  memcpy(&screen, pcolors, sizeof(screen));
}
