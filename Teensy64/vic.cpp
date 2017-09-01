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

/*
  TODOs:
  - FLD  - (OK 08/17)
  - Sprite Stretching (requires "MOBcounter")
  - FLI work (doubling badlines)
  - DMA Delay (?)
  - ...
  -
  - Fix Bugs..
  - optimize more
*/



#include <DMAChannel.h>
#include "cpu.h"
#include "settings.h"

#include "util.h"
#include "roms.h"
#include "vic.h"
#include "vic_palette.h"

#define FIRSTDISPLAYLINE (  51 - BORDER )
#define LASTDISPLAYLINE  ( 250 + BORDER )

#define MAXCYCLESSPRITES0_2       7
#define MAXCYCLESSPRITES3_7        10
#define MAXCYCLESSPRITES    (MAXCYCLESSPRITES0_2 + MAXCYCLESSPRITES3_7)
/*
  The VIC has 47 read/write registers for the processor to control its
  functions:

  #| Adr.  |Bit7|Bit6|Bit5|Bit4|Bit3|Bit2|Bit1|Bit0| Function
  --+-------+----+----+----+----+----+----+----+----+------------------------
  0| $d000 |                  M0X                  | X coordinate sprite 0
  --+-------+---------------------------------------+------------------------
  1| $d001 |                  M0Y                  | Y coordinate sprite 0
  --+-------+---------------------------------------+------------------------
  2| $d002 |                  M1X                  | X coordinate sprite 1
  --+-------+---------------------------------------+------------------------
  3| $d003 |                  M1Y                  | Y coordinate sprite 1
  --+-------+---------------------------------------+------------------------
  4| $d004 |                  M2X                  | X coordinate sprite 2
  --+-------+---------------------------------------+------------------------
  5| $d005 |                  M2Y                  | Y coordinate sprite 2
  --+-------+---------------------------------------+------------------------
  6| $d006 |                  M3X                  | X coordinate sprite 3
  --+-------+---------------------------------------+------------------------
  7| $d007 |                  M3Y                  | Y coordinate sprite 3
  --+-------+---------------------------------------+------------------------
  8| $d008 |                  M4X                  | X coordinate sprite 4
  --+-------+---------------------------------------+------------------------
  9| $d009 |                  M4Y                  | Y coordinate sprite 4
  --+-------+---------------------------------------+------------------------
  10| $d00a |                  M5X                  | X coordinate sprite 5
  --+-------+---------------------------------------+------------------------
  11| $d00b |                  M5Y                  | Y coordinate sprite 5
  --+-------+---------------------------------------+------------------------
  12| $d00c |                  M6X                  | X coordinate sprite 6
  --+-------+---------------------------------------+------------------------
  13| $d00d |                  M6Y                  | Y coordinate sprite 6
  --+-------+---------------------------------------+------------------------
  14| $d00e |                  M7X                  | X coordinate sprite 7
  --+-------+---------------------------------------+------------------------
  15| $d00f |                  M7Y                  | Y coordinate sprite 7
  --+-------+----+----+----+----+----+----+----+----+------------------------
  16| $d010 |M7X8|M6X8|M5X8|M4X8|M3X8|M2X8|M1X8|M0X8| MSBs of X coordinates
  --+-------+----+----+----+----+----+----+----+----+------------------------
  17| $d011 |RST8| ECM| BMM| DEN|RSEL|    YSCROLL   | Control register 1
  --+-------+----+----+----+----+----+--------------+------------------------
  18| $d012 |                 RASTER                | Raster counter
  --+-------+---------------------------------------+------------------------
  19| $d013 |                  LPX                  | Light pen X
  --+-------+---------------------------------------+------------------------
  20| $d014 |                  LPY                  | Light pen Y
  --+-------+----+----+----+----+----+----+----+----+------------------------
  21| $d015 | M7E| M6E| M5E| M4E| M3E| M2E| M1E| M0E| Sprite enabled
  --+-------+----+----+----+----+----+----+----+----+------------------------
  22| $d016 |  - |  - | RES| MCM|CSEL|    XSCROLL   | Control register 2
  --+-------+----+----+----+----+----+----+----+----+------------------------
  23| $d017 |M7YE|M6YE|M5YE|M4YE|M3YE|M2YE|M1YE|M0YE| Sprite Y expansion
  --+-------+----+----+----+----+----+----+----+----+------------------------
  24| $d018 |VM13|VM12|VM11|VM10|CB13|CB12|CB11|  - | Memory pointers
  --+-------+----+----+----+----+----+----+----+----+------------------------
  25| $d019 | IRQ|  - |  - |  - | ILP|IMMC|IMBC|IRST| Interrupt register
  --+-------+----+----+----+----+----+----+----+----+------------------------
  26| $d01a |  - |  - |  - |  - | ELP|EMMC|EMBC|ERST| Interrupt enabled
  --+-------+----+----+----+----+----+----+----+----+------------------------
  27| $d01b |M7DP|M6DP|M5DP|M4DP|M3DP|M2DP|M1DP|M0DP| Sprite data priority
  --+-------+----+----+----+----+----+----+----+----+------------------------
  28| $d01c |M7MC|M6MC|M5MC|M4MC|M3MC|M2MC|M1MC|M0MC| Sprite multicolor
  --+-------+----+----+----+----+----+----+----+----+------------------------
  29| $d01d |M7XE|M6XE|M5XE|M4XE|M3XE|M2XE|M1XE|M0XE| Sprite X expansion
  --+-------+----+----+----+----+----+----+----+----+------------------------
  30| $d01e | M7M| M6M| M5M| M4M| M3M| M2M| M1M| M0M| Sprite-sprite collision
  --+-------+----+----+----+----+----+----+----+----+------------------------
  31| $d01f | M7D| M6D| M5D| M4D| M3D| M2D| M1D| M0D| Sprite-data collision
  --+-------+----+----+----+----+----+----+----+----+------------------------
  32| $d020 |  - |  - |  - |  - |         EC        | Border color
  --+-------+----+----+----+----+-------------------+------------------------
  33| $d021 |  - |  - |  - |  - |        B0C        | Background color 0
  --+-------+----+----+----+----+-------------------+------------------------
  34| $d022 |  - |  - |  - |  - |        B1C        | Background color 1
  --+-------+----+----+----+----+-------------------+------------------------
  35| $d023 |  - |  - |  - |  - |        B2C        | Background color 2
  --+-------+----+----+----+----+-------------------+------------------------
  36| $d024 |  - |  - |  - |  - |        B3C        | Background color 3
  --+-------+----+----+----+----+-------------------+------------------------
  37| $d025 |  - |  - |  - |  - |        MM0        | Sprite multicolor 0
  --+-------+----+----+----+----+-------------------+------------------------
  38| $d026 |  - |  - |  - |  - |        MM1        | Sprite multicolor 1
  --+-------+----+----+----+----+-------------------+------------------------
  39| $d027 |  - |  - |  - |  - |        M0C        | Color sprite 0
  --+-------+----+----+----+----+-------------------+------------------------
  40| $d028 |  - |  - |  - |  - |        M1C        | Color sprite 1
  --+-------+----+----+----+----+-------------------+------------------------
  41| $d029 |  - |  - |  - |  - |        M2C        | Color sprite 2
  --+-------+----+----+----+----+-------------------+------------------------
  42| $d02a |  - |  - |  - |  - |        M3C        | Color sprite 3
  --+-------+----+----+----+----+-------------------+------------------------
  43| $d02b |  - |  - |  - |  - |        M4C        | Color sprite 4
  --+-------+----+----+----+----+-------------------+------------------------
  44| $d02c |  - |  - |  - |  - |        M5C        | Color sprite 5
  --+-------+----+----+----+----+-------------------+------------------------
  45| $d02d |  - |  - |  - |  - |        M6C        | Color sprite 6
  --+-------+----+----+----+----+-------------------+------------------------
  46| $d02e |  - |  - |  - |  - |        M7C        | Color sprite 7
  --+-------+----+----+----+----+-------------------+------------------------

*/

/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/

inline __attribute__((always_inline))
void fastFillLine(uint16_t * p, const uint16_t * pe, const uint16_t col, uint16_t * spl);
inline __attribute__((always_inline))
void fastFillLineNoSprites(uint16_t * p, const uint16_t * pe, const uint16_t col);


/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/

#define CHARSETPTR() cpu.vic.charsetPtr = cpu.vic.charsetPtrBase + cpu.vic.rc; //TODO: Besser integrieren

#define CYCLES(x) {if (cpu.vic.badline) {cia_clockt(x);} else {cpu_clock(x);} }

#define BADLINE(x) {if (cpu.vic.badline) { \
      cpu.vic.lineMemChr[x] = cpu.RAM[cpu.vic.videomatrix + vc + x]; \
      cpu.vic.lineMemCol[x] = cpu.vic.COLORRAM[vc + x]; \
      cia_clockt(1); \
    } else { \
      cpu_clock(1); \
    } \
  };

#define SPRITEORFIXEDCOLOR() \
  sprite = *spl++; \
  if (sprite) { \
    *p++ = cpu.vic.palette[sprite & 0x0f];\
  } else { \
    *p++ = col;\
  };

#if 0
#define PRINTOVERFLOW   \
  if (p>pe) { \
    Serial.print("VIC overflow Mode "); \
    Serial.println(mode); \
  }

#define PRINTOVERFLOWS  \
  if (p>pe) { \
    Serial.print("VIC overflow (Sprite) Mode ");  \
    Serial.println(mode); \
  }
#else
#define PRINTOVERFLOW
#define PRINTOVERFLOWS
#endif

/*****************************************************************************************************/
void mode0 (uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc) {
  // Standard-Textmodus(ECM/BMM/MCM=0/0/0)
  /*
    Standard-Textmodus (ECM / BMM / MCM = 0/0/0)
    In diesem Modus (wie in allen Textmodi) liest der VIC aus der videomatrix 8-Bit-Zeichenzeiger,
    die die Adresse der Punktmatrix des Zeichens im Zeichengenerator angibt. Damit ist ein Zeichensatz
    von 256 Zeichen verfügbar, die jeweils aus 8×8 Pixeln bestehen, die in 8 aufeinanderfolgenden Bytes
    im Zeichengenerator abgelegt sind. Mit den Bits VM10-13 und CB11-13 aus Register $d018 lassen sich
    videomatrix und Zeichengenerator im Speicher verschieben. Im Standard-Textmodus entspricht jedes Bit
    im Zeichengenerator direkt einem Pixel auf dem Bildschirm. Die Vordergrundfarbe ist für jedes Zeichen
    im Farbnibble aus der videomatrix angegeben, die Hintergrundfarbe wird global durch Register $d021 festgelegt.

    +----+----+----+----+----+----+----+----+
    |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
    +----+----+----+----+----+----+----+----+
    |         8 Pixel (1 Bit/Pixel)         |
    |                                       |
    | "0": Hintergrundfarbe 0 ($d021)       |
    | "1": Farbe aus Bits 8-11 der c-Daten  |
    +---------------------------------------+

  */

  uint8_t chr, pixel;
  uint16_t fgcol;
  uint16_t bgcol;
  uint8_t x = 0;

  CHARSETPTR();

  if (cpu.vic.lineHasSprites) {

    do {

      BADLINE(x);

      chr = cpu.vic.charsetPtr[cpu.vic.lineMemChr[x] * 8];
      fgcol = cpu.vic.lineMemCol[x];
      unsigned m = min(8, pe - p);
      for (unsigned i = 0; i < m; i++) {
        int sprite = *spl;

        if (sprite) {     // Sprite: Ja
          int spritenum = 1 << ( (sprite >> 8) & 0x07);
          int spritepixel = sprite & 0x0f;

          if (sprite & 0x4000) {   // Sprite: Hinter Text  MDP = 1
            if (chr & 0x80) {
              cpu.vic.fgcollision |= spritenum;
              pixel = fgcol;
            } else {
              pixel = spritepixel;
            }
          } else {            // Sprite: Vor Text //MDP = 0
            if (chr & 0x80) cpu.vic.fgcollision |= spritenum;
            pixel = spritepixel;
          }
        } else {            // Kein Sprite
          pixel = (chr & 0x80) ? fgcol : cpu.vic.B0C;
        }

        *p++ = cpu.vic.palette[pixel];
        spl++;
        chr = chr << 1;

      }

      x++;

    } while (p < pe);
    PRINTOVERFLOWS
  } else { //Keine Sprites

    while (p < pe - 8) {

      BADLINE(x)

      chr = cpu.vic.charsetPtr[cpu.vic.lineMemChr[x] * 8];
      fgcol = cpu.vic.palette[cpu.vic.lineMemCol[x]];
	  bgcol = cpu.vic.palette[cpu.vic.B0C];
      x++;

      *p++ = (chr & 0x80) ? fgcol : bgcol;
      *p++ = (chr & 0x40) ? fgcol : bgcol;
      *p++ = (chr & 0x20) ? fgcol : bgcol;
      *p++ = (chr & 0x10) ? fgcol : bgcol;
      *p++ = (chr & 0x08) ? fgcol : bgcol;
      *p++ = (chr & 0x04) ? fgcol : bgcol;
      *p++ = (chr & 0x02) ? fgcol : bgcol;
      *p++ = (chr & 0x01) ? fgcol : bgcol;

    };

    while (p < pe) {

      BADLINE(x)

      chr = cpu.vic.charsetPtr[cpu.vic.lineMemChr[x] * 8];
      fgcol = cpu.vic.palette[cpu.vic.lineMemCol[x]];
	  bgcol = cpu.vic.palette[cpu.vic.B0C];
      x++;

      *p++ = (chr & 0x80) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x40) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x20) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x10) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x08) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x04) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x02) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x01) ? fgcol : bgcol;
    };
    PRINTOVERFLOW
  }
  while (x<40) {BADLINE(x); x++;}
};

/*****************************************************************************************************/
void mode1 (uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc) {
  /*
    Multicolor-Textmodus (ECM/BMM/MCM=0/0/1)
    Dieser Modus ermöglicht es, auf Kosten der horizontalen Auflösung vierfarbige Zeichen darzustellen.
    Ist Bit 11 der c-Daten Null, wird das Zeichen wie im Standard-Textmodus dargestellt, wobei aber nur die
    Farben 0-7 für den Vordergrund zur Verfügung stehen. Ist Bit 11 gesetzt, bilden jeweils zwei horizontal
    benachbarte Bits der Punktmatrix ein Pixel. Dadurch ist die Auflösung des Zeichens auf 4×8 reduziert
    (die Pixel sind doppelt so breit, die Gesamtbreite der Zeichen ändert sich also nicht).
    Interessant ist, daß nicht nur die Bitkombination „00”, sondern auch „01” für die Spritepriorität
    und -kollisionserkennung zum "Hintergrund" gezählt wird.

    +----+----+----+----+----+----+----+----+
    |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
    +----+----+----+----+----+----+----+----+
    |         8 Pixel (1 Bit/Pixel)         |
    |                                       | MC-Flag = 0
    | "0": Hintergrundfarbe 0 ($d021)       |
    | "1": Farbe aus Bits 8-10 der c-Daten  |
    +---------------------------------------+
    |         4 Pixel (2 Bit/Pixel)         |
    |                                       |
    | "00": Hintergrundfarbe 0 ($d021)      | MC-Flag = 1
    | "01": Hintergrundfarbe 1 ($d022)      |
    | "10": Hintergrundfarbe 2 ($d023)      |
    | "11": Farbe aus Bits 8-10 der c-Daten |
    +---------------------------------------+

  */

  // POKE 53270,PEEK(53270) OR 16
  // poke 53270,peek(53270) or 16

  uint16_t bgcol, fgcol, pixel;
  uint16_t colors[4];
  uint8_t chr;
  uint8_t x = 0;

  CHARSETPTR();

  if (cpu.vic.lineHasSprites) {

    colors[0] = cpu.vic.B0C;
    colors[1] = cpu.vic.R[0x22];
    colors[2] = cpu.vic.R[0x23];
    do {

      BADLINE(x);

      chr = cpu.vic.charsetPtr[cpu.vic.lineMemChr[x] * 8];
      fgcol = cpu.vic.lineMemCol[x];
      x++;

      colors[3] = fgcol & 0x07;

      if ((fgcol & 0x08) == 0) { //Zeichen ist HIRES

        unsigned m = min(8, pe - p);
        for (unsigned i = 0; i < m; i++) {

          int sprite = *spl;

          if (sprite) {     // Sprite: Ja

            /*
              Sprite-Prioritäten (Anzeige)
              MDP = 1: Grafikhintergrund, Sprite, Vordergrund
              MDP = 0: Grafikhintergrund, Vordergrund, Sprite

              Kollision:
              Eine Kollision zwischen Sprites und anderen Grafikdaten wird erkannt,
              sobald beim Bildaufbau ein nicht transparentes Spritepixel und ein Vordergrundpixel ausgegeben wird.

            */
            int spritenum = 1 << ((sprite >> 8) & 0x07);
            pixel = sprite & 0x0f; //Hintergrundgrafik
            if (sprite & 0x4000) {   // MDP = 1
              if (chr & 0x80) { //Vordergrundpixel ist gesetzt
                cpu.vic.fgcollision |= spritenum;
                pixel = colors[3];
              }
            } else {            // MDP = 0
              if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergrundpixel ist gesetzt
            }

          } else {            // Kein Sprite
            pixel = (chr >> 7) ? colors[3] : colors[0];
          }

          *p++ = cpu.vic.palette[pixel];

          spl++;
          chr = chr << 1;
        }

      } else {//Zeichen ist MULTICOLOR

        for (unsigned i = 0; i < 4; i++) {
          if (p >= pe) break;
          int c = (chr >> 6) & 0x03;
          chr = chr << 2;

          int sprite = *spl;
          if (sprite) {    // Sprite: Ja
            int spritenum = 1 << ((sprite >> 8) & 0x07);

			pixel = sprite & 0x0f; //Hintergrundgrafik
            if (sprite & 0x4000) {  // MDP = 1

              if (chr & 0x80) {  //Vordergrundpixel ist gesetzt
                cpu.vic.fgcollision |= spritenum;
                pixel = colors[c];
              }

            } else {          // MDP = 0
              if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergrundpixel ist gesetzt
            }

          } else { // Kein Sprite
            pixel = colors[c];

          }
          *p++ = cpu.vic.palette[pixel];
          if (p >= pe) break;
          spl++;
          sprite = *spl;
          //Das gleiche nochmal für das nächste Pixel
          if (sprite) {    // Sprite: Ja
            int spritenum = 1 << ((sprite >> 8) & 0x07);
			pixel =  sprite & 0x0f; //Hintergrundgrafik
            if (sprite & 0x4000) {  // MDP = 1

              if (chr & 0x80) { //Vordergrundpixel ist gesetzt
                cpu.vic.fgcollision |= spritenum;
                pixel = colors[c];
              }
            } else {          // MDP = 0
              if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergrundpixel ist gesetzt
            }
          } else { // Kein Sprite
            pixel = colors[c];
          }
          *p++ = cpu.vic.palette[pixel];
          spl++;

        }

      }

    } while (p < pe);
    PRINTOVERFLOWS
  } else { //Keine Sprites

    bgcol = cpu.vic.palette[cpu.vic.B0C];

    colors[0] = bgcol;
    colors[1] = cpu.vic.palette[cpu.vic.R[0x22]];
    colors[2] = cpu.vic.palette[cpu.vic.R[0x23]];

    while (p < pe - 8) {

      BADLINE(x);

      chr = cpu.vic.charsetPtr[cpu.vic.lineMemChr[x] * 8];
      int c = cpu.vic.lineMemCol[x];

      x++;

      if ((c & 0x08) == 0) { //Zeichen ist HIRES
        fgcol = cpu.vic.palette[c & 0x07];
        *p++ = (chr & 0x80) ? fgcol : bgcol;
        *p++ = (chr & 0x40) ? fgcol : bgcol;
        *p++ = (chr & 0x20) ? fgcol : bgcol;
        *p++ = (chr & 0x10) ? fgcol : bgcol;
        *p++ = (chr & 0x08) ? fgcol : bgcol;
        *p++ = (chr & 0x04) ? fgcol : bgcol;
        *p++ = (chr & 0x02) ? fgcol : bgcol;
        *p++ = (chr & 0x01) ? fgcol : bgcol;
      } else {//Zeichen ist MULTICOLOR

        colors[3] = cpu.vic.palette[c & 0x07];
        pixel = colors[(chr >> 6) & 0x03]; *p++ = pixel; *p++ = pixel;
        pixel = colors[(chr >> 4) & 0x03]; *p++ = pixel; *p++ = pixel;
        pixel = colors[(chr >> 2) & 0x03]; *p++ = pixel; *p++ = pixel;
        pixel = colors[(chr     ) & 0x03]; *p++ = pixel; *p++ = pixel;
      }

    };

    while (p < pe) {

      BADLINE(x);

      chr = cpu.vic.charsetPtr[cpu.vic.lineMemChr[x] * 8];
      int c = cpu.vic.lineMemCol[x];

      x++;
      if ((c & 0x08) == 0) { //Zeichen ist HIRES
        fgcol = cpu.vic.palette[c & 0x07];
        *p++ = (chr & 0x80) ? fgcol : bgcol; if (p >= pe) break;
        *p++ = (chr & 0x40) ? fgcol : bgcol; if (p >= pe) break;
        *p++ = (chr & 0x20) ? fgcol : bgcol; if (p >= pe) break;
        *p++ = (chr & 0x10) ? fgcol : bgcol; if (p >= pe) break;
        *p++ = (chr & 0x08) ? fgcol : bgcol; if (p >= pe) break;
        *p++ = (chr & 0x04) ? fgcol : bgcol; if (p >= pe) break;
        *p++ = (chr & 0x02) ? fgcol : bgcol; if (p >= pe) break;
        *p++ = (chr & 0x01) ? fgcol : bgcol;
      } else {//Zeichen ist MULTICOLOR

        colors[3] = cpu.vic.palette[c & 0x07];
        pixel = colors[(chr >> 6) & 0x03]; *p++ = pixel; if (p >= pe) break; *p++ = pixel; if (p >= pe) break;
        pixel = colors[(chr >> 4) & 0x03]; *p++ = pixel; if (p >= pe) break; *p++ = pixel; if (p >= pe) break;
        pixel = colors[(chr >> 2) & 0x03]; *p++ = pixel; if (p >= pe) break; *p++ = pixel; if (p >= pe) break;
        pixel = colors[(chr     ) & 0x03]; *p++ = pixel; if (p >= pe) break; *p++ = pixel;
      }

    };
    PRINTOVERFLOW
  }
  while (x<40) {BADLINE(x); x++;}
}
/*****************************************************************************************************/
void mode2 (uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc) {
  /*
     Standard-Bitmap-Modus (ECM / BMM / MCM = 0/1/0) ("HIRES")
     In diesem Modus (wie in allen Bitmap-Modi) liest der VIC die Grafikdaten aus einer 320×200-Bitmap,
     in der jedes Bit direkt einem Punkt auf dem Bildschirm entspricht. Die Daten aus der videomatrix
     werden für die Farbinformation benutzt. Da die videomatrix weiterhin nur eine 40×25-Matrix ist,
     können die Farben nur für Blöcke von 8×8 Pixeln individuell bestimmt werden (also eine Art YC-8:1-Format).
     Da die Entwickler des VIC-II den Bitmap-Modus mit sowenig zusätzlichem Schaltungsaufwand wie möglich realisieren wollten
     (der VIC-I hatte noch keinen Bitmap-Modus), ist die Bitmap etwas ungewöhnlich im Speicher abgelegt:
     Im Gegensatz zu modernen Videochips, die die Bitmap linear aus dem Speicher lesen, bilden beim VIC jeweils 8 aufeinanderfolgende Bytes einen 8×8-Pixelblock
     auf dem Bildschirm. Mit den Bits VM10-13 und CB13 aus Register $d018 lassen sich videomatrix und Bitmap im Speicher verschieben.
     Im Standard-Bitmap-Modus entspricht jedes Bit in der Bitmap direkt einem Pixel auf dem Bildschirm.
     Für jeden 8×8-Block können Vorder- und Hintergrundfarbe beliebig eingestellt werden.

     +----+----+----+----+----+----+----+----+
     |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
     +----+----+----+----+----+----+----+----+
     |         8 Pixel (1 Bit/Pixel)         |
     |                                       |
     | "0": Farbe aus Bits 0-3 der c-Daten   |
     | "1": Farbe aus Bits 4-7 der c-Daten   |
     +---------------------------------------+


     http://www.devili.iki.fi/Computers/Commodore/C64/Programmers_Reference/Chapter_3/page_127.html
  */

  uint8_t chr;
  uint16_t fgcol, pixel;
  uint16_t bgcol;
  uint8_t x = 0;

  if (cpu.vic.lineHasSprites) {
    do {

      BADLINE(x);

      uint8_t t = cpu.vic.lineMemChr[x];
      fgcol = t >> 4;
      bgcol = t & 0x0f;
      chr = cpu.vic.bitmapPtr[x * 8];

      x++;

      unsigned m = min(8, pe - p);
      for (unsigned i = 0; i < m; i++) {

        int sprite = *spl;

        chr = chr << 1;
        if (sprite) {     // Sprite: Ja
          /*
             Sprite-Prioritäten (Anzeige)
             MDP = 1: Grafikhintergrund, Sprite, Vordergrund
             MDP = 0: Grafikhintergung, Vordergrund, Sprite

             Kollision:
             Eine Kollision zwischen Sprites und anderen Grafikdaten wird erkannt,
             sobald beim Bildaufbau ein nicht transparentes Spritepixel und ein Vordergrundpixel ausgegeben wird.

          */
          int spritenum = 1 << ((sprite >> 8) & 0x07);
          pixel = sprite & 0x0f; //Hintergrundgrafik
          if (sprite & 0x4000) {   // MDP = 1
            if (chr & 0x80) { //Vordergrundpixel ist gesetzt
              cpu.vic.fgcollision |= spritenum;
              pixel = fgcol;
            }
          } else {            // MDP = 0
            if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergrundpixel ist gesetzt
          }

        } else {            // Kein Sprite
          pixel = (chr & 0x80) ? fgcol :cpu.vic.B0C;
        }

        *p++ = cpu.vic.palette[pixel];
        spl++;

      }
    } while (p < pe);
    PRINTOVERFLOWS
  } else { //Keine Sprites

    while (p < pe - 8) {
      //color-ram not used!
      BADLINE(x);

      uint8_t t = cpu.vic.lineMemChr[x];
      fgcol = cpu.vic.palette[t >> 4];
      bgcol = cpu.vic.palette[t & 0x0f];
      chr = cpu.vic.bitmapPtr[x * 8];
      x++;

      *p++ = (chr & 0x80) ? fgcol : bgcol;
      *p++ = (chr & 0x40) ? fgcol : bgcol;
      *p++ = (chr & 0x20) ? fgcol : bgcol;
      *p++ = (chr & 0x10) ? fgcol : bgcol;
      *p++ = (chr & 0x08) ? fgcol : bgcol;
      *p++ = (chr & 0x04) ? fgcol : bgcol;
      *p++ = (chr & 0x02) ? fgcol : bgcol;
      *p++ = (chr & 0x01) ? fgcol : bgcol;
    };
    while (p < pe) {
      //color-ram not used!
      BADLINE(x);

      uint8_t t = cpu.vic.lineMemChr[x];
      fgcol = cpu.vic.palette[t >> 4];
      bgcol = cpu.vic.palette[t & 0x0f];
      chr = cpu.vic.bitmapPtr[x * 8];

      x++;

      *p++ = (chr & 0x80) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x40) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x20) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x10) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x08) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x04) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x02) ? fgcol : bgcol; if (p >= pe) break;
      *p++ = (chr & 0x01) ? fgcol : bgcol;

    };
    PRINTOVERFLOW
  }
  while (x<40) {BADLINE(x); x++;}
}
/*****************************************************************************************************/
void mode3 (uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc) {
  /*
    Multicolor-Bitmap-Modus (ECM/BMM/MCM=0/1/1)

    Ähnlich wie beim Multicolor-Textmodus bilden auch in diesem Modus jeweils
    zwei benachbarte Bits ein (doppelt so breites) Pixel. Die Auflösung
    reduziert sich damit auf 160×200 Pixel.

    Genau wie beim Multicolor-Textmodus wird die Bitkombination "01" für die
    Spritepriorität und -kollisionserkennung zum "Hintergrund" gezählt.

    +----+----+----+----+----+----+----+----+
    |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
    +----+----+----+----+----+----+----+----+
    |         4 Pixel (2 Bit/Pixel)         |
    |                                       |
    | "00": Hintergrundfarbe 0 ($d021)      |
    | "01": Farbe aus Bits 4-7 der c-Daten  |
    | "10": Farbe aus Bits 0-3 der c-Daten  |
    | "11": Farbe aus Bits 8-11 der c-Daten |
    +---------------------------------------+

    POKE 53265,PEEK(53625)OR 32: POKE 53270,PEEK(53270)OR 16
  */

  uint8_t chr, x;
  uint16_t pixel;

  x = 0;

  if (cpu.vic.lineHasSprites) {
    uint16_t colors[4];
    colors[0] = cpu.vic.B0C;
    do {

      BADLINE(x);

      uint8_t t = cpu.vic.lineMemChr[x];
      colors[1] = t >> 4;//10
      colors[2] = t & 0x0f; //01
      colors[3] = cpu.vic.lineMemCol[x];

      chr = cpu.vic.bitmapPtr[x * 8];

      x++;

      for (unsigned i = 0; i < 4; i++) {
        if (p >= pe) break;
        uint32_t c = (chr >> 6) & 0x03;
        chr = chr << 2;

        int sprite = *spl;
        if (sprite) {    // Sprite: Ja
          int spritenum = 1 << ((sprite >> 8) & 0x07);
          pixel = sprite & 0x0f; //Hintergrundgrafik
          if (sprite & 0x4000) {  // MDP = 1
            if (c & 0x02) {  //Vordergrundpixel ist gesetzt
              cpu.vic.fgcollision |= spritenum;
              pixel = colors[c];
            }
          } else {          // MDP = 0
            if (c & 0x02) cpu.vic.fgcollision |= spritenum; //Vordergundpixel ist gesetzt
          }
        } else { // Kein Sprite
          pixel = colors[c];
        }

        *p++ = cpu.vic.palette[pixel];
        if (p >= pe) break;
        spl++;
        sprite = *spl;

        if (sprite) {    // Sprite: Ja
          int spritenum = 1 << ((sprite >> 8) & 0x07);
          pixel = sprite & 0x0f; //Hintergrundgrafik
          if (sprite & 0x4000) {  // MDP = 1

            if (c & 0x02) {  //Vordergrundpixel ist gesetzt
              cpu.vic.fgcollision |= spritenum;
              pixel = colors[c];
            }
          } else {          // MDP = 0
            if (c & 0x02) cpu.vic.fgcollision |= spritenum; //Vordergundpixel ist gesetzt
          }
        } else { // Kein Sprite
          pixel = colors[c];
        }

        *p++ = cpu.vic.palette[pixel];
        spl++;

      }

    } while (p < pe);
    PRINTOVERFLOWS

  } else { //Keine Sprites

    uint16_t colors[4];
    colors[0] = cpu.vic.palette[cpu.vic.B0C];

    while (p < pe - 8) {

      BADLINE(x);

      uint8_t t = cpu.vic.lineMemChr[x];
      colors[1] = cpu.vic.palette[t >> 4];//10
      colors[2] = cpu.vic.palette[t & 0x0f]; //01
      colors[3] = cpu.vic.palette[cpu.vic.lineMemCol[x]];
      chr = cpu.vic.bitmapPtr[x * 8];
      x++;

      pixel = colors[(chr >> 6) & 0x03]; *p++ = pixel; *p++ = pixel;
      pixel = colors[(chr >> 4) & 0x03]; *p++ = pixel; *p++ = pixel;
      pixel = colors[(chr >> 2) & 0x03]; *p++ = pixel; *p++ = pixel;
      pixel = colors[chr        & 0x03]; *p++ = pixel; *p++ = pixel;

    };
    while (p < pe) {

      BADLINE(x);

      uint8_t t = cpu.vic.lineMemChr[x];
      colors[1] = cpu.vic.palette[t >> 4];//10
      colors[2] = cpu.vic.palette[t & 0x0f]; //01
      colors[3] = cpu.vic.palette[cpu.vic.lineMemCol[x]];
      chr = cpu.vic.bitmapPtr[x * 8];

      x++;
      pixel = colors[(chr >> 6) & 0x03]; *p++ = pixel; if (p >= pe) break; *p++ = pixel; if (p >= pe) break;
      pixel = colors[(chr >> 4) & 0x03]; *p++ = pixel; if (p >= pe) break; *p++ = pixel; if (p >= pe) break;
      pixel = colors[(chr >> 2) & 0x03]; *p++ = pixel; if (p >= pe) break; *p++ = pixel; if (p >= pe) break;
      pixel = colors[chr        & 0x03]; *p++ = pixel; if (p >= pe) break; *p++ = pixel;

    };
    PRINTOVERFLOW
  }
  while (x<40) {BADLINE(x); x++;}
}
/*****************************************************************************************************/
void mode4 (uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc) {
  //ECM-Textmodus (ECM/BMM/MCM=1/0/0)
  /*
    Dieser Textmodus entspricht dem Standard-Textmodus, erlaubt es aber, für
    jedes einzelne Zeichen eine von vier Hintergrundfarben auszuwählen. Die
    Auswahl geschieht über die oberen beiden Bits des Zeichenzeigers. Dadurch
    reduziert sich der Zeichenvorrat allerdings von 256 auf 64 Zeichen.

    +----+----+----+----+----+----+----+----+
    |  7 |  6 |  5 |  4 |  3 |  2 |  1 |  0 |
    +----+----+----+----+----+----+----+----+
    |         8 Pixel (1 Bit/Pixel)         |
    |                                       |
    | "0": Je nach Bits 6/7 der c-Daten     |
    |      00: Hintergrundfarbe 0 ($d021)   |
    |      01: Hintergrundfarbe 1 ($d022)   |
    |      10: Hintergrundfarbe 2 ($d023)   |
    |      11: Hintergrundfarbe 3 ($d024)   |
    | "1": Farbe aus Bits 8-11 der c-Daten  |
    +---------------------------------------+
  */
  // https://www.c64-wiki.de/wiki/Hintergrundfarbe
  // POKE 53265, PEEK(53265) OR 64:REM CURSOR BLINKT ROT abc

  uint8_t chr, pixel;
  uint16_t fgcol;
  uint16_t bgcol;
  uint8_t x = 0;

  CHARSETPTR();
  if (cpu.vic.lineHasSprites) {
    do {

      BADLINE(x);

      uint32_t td = cpu.vic.lineMemChr[x];
      bgcol = cpu.vic.R[0x21 + ((td >> 6) & 0x03)];
      chr = cpu.vic.charsetPtr[(td & 0x3f) * 8];
      fgcol = cpu.vic.lineMemCol[x];

      x++;

      unsigned m = min(8, pe - p);
      for (unsigned i = 0; i < m; i++) {

        int sprite = *spl;

        if (sprite) {     // Sprite: Ja
          int spritenum = 1 << ((sprite >> 8) & 0x07);
          if (sprite & 0x4000) {   // Sprite: Hinter Text
            if (chr & 0x80) {
              cpu.vic.fgcollision |= spritenum;
              pixel = fgcol;
            } else pixel = bgcol;
          } else {              // Sprite: Vor Text
            if (chr & 0x80) cpu.vic.fgcollision |= spritenum;
            pixel = sprite & 0x0f;
          }
        } else {                // Kein Sprite
          pixel = (chr & 0x80) ? fgcol : bgcol;
        }
        spl++;
        chr = chr << 1;
        *p++ = cpu.vic.palette[pixel];
      }
    } while (p < pe);
    PRINTOVERFLOWS
  }
  else //Keine Sprites
    while (p < pe - 8) {

      BADLINE(x);

      uint32_t td = cpu.vic.lineMemChr[x];
      bgcol = cpu.vic.palette[cpu.vic.R[0x21 + ((td >> 6) & 0x03)]];
      chr = cpu.vic.charsetPtr[(td & 0x3f) * 8];
      fgcol = cpu.vic.palette[cpu.vic.lineMemCol[x]];
      x++;

      *p++ = (chr & 0x80) ? fgcol : bgcol;
      *p++ = (chr & 0x40) ? fgcol : bgcol;
      *p++ = (chr & 0x20) ? fgcol : bgcol;
      *p++ = (chr & 0x10) ? fgcol : bgcol;
      *p++ = (chr & 0x08) ? fgcol : bgcol;
      *p++ = (chr & 0x04) ? fgcol : bgcol;
      *p++ = (chr & 0x02) ? fgcol : bgcol;
      *p++ = (chr & 0x01) ? fgcol : bgcol;

    };
  while (p < pe) {

    BADLINE(x);

    uint32_t td = cpu.vic.lineMemChr[x];
    bgcol = cpu.vic.palette[cpu.vic.R[0x21 + ((td >> 6) & 0x03)]];
    chr = cpu.vic.charsetPtr[(td & 0x3f) * 8];
    fgcol = cpu.vic.palette[cpu.vic.lineMemCol[x]];

    x++;

    *p++ = (chr & 0x80) ? fgcol : bgcol; if (p >= pe) break;
    *p++ = (chr & 0x40) ? fgcol : bgcol; if (p >= pe) break;
    *p++ = (chr & 0x20) ? fgcol : bgcol; if (p >= pe) break;
    *p++ = (chr & 0x10) ? fgcol : bgcol; if (p >= pe) break;
    *p++ = (chr & 0x08) ? fgcol : bgcol; if (p >= pe) break;
    *p++ = (chr & 0x04) ? fgcol : bgcol; if (p >= pe) break;
    *p++ = (chr & 0x02) ? fgcol : bgcol; if (p >= pe) break;
    *p++ = (chr & 0x01) ? fgcol : bgcol;

  };
  PRINTOVERFLOW
  while (x<40) {BADLINE(x); x++;}
}

/*****************************************************************************************************/
/* Ungültige Modi ************************************************************************************/
/*****************************************************************************************************/

void mode5 (uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc) {
  /*
    Ungültiger Textmodus (ECM/BMM/MCM=1/0/1)

    Das gleichzeitige Setzen der ECM- und MCM-Bits wählt keinen der
    "offiziellen" Grafikmodi des VIC, sondern erzeugt nur schwarze Pixel.
    Nichtsdestotrotz erzeugt der Grafikdatensequenzer auch in diesem Modus
    intern gültige Grafikdaten, die die Spritekollisionserkennung triggern
    können. Über den Umweg der Spritekollisionen kann man die erzeugten Daten
    auch auslesen (sehen kann man nichts, das Bild ist schwarz). Man kann so
    allerdings nur Vordergrund- und Hintergrundpixel unterscheiden, die
    Farbinformation läßt sich aus den Spritekollisionen nicht gewinnen.

    Die erzeugte Grafik entspricht der des Multicolor-Textmodus, allerdings ist
    der Zeichenvorrat genau wie im ECM-Modus auf 64 Zeichen eingeschränkt.
  */
  CHARSETPTR();

  uint8_t chr, pixel;
  uint16_t fgcol;
  uint8_t x = 0;

  if (cpu.vic.lineHasSprites) {

    do {

      BADLINE(x);

      chr = cpu.vic.charsetPtr[(cpu.vic.lineMemChr[x] & 0x3F) * 8];
      fgcol = cpu.vic.lineMemCol[x];

      x++;

      if ((fgcol & 0x08) == 0) { //Zeichen ist HIRES

        unsigned m = min(8, pe - p);
        for (unsigned i = 0; i < m; i++) {

          int sprite = *spl;

          if (sprite) {     // Sprite: Ja

            /*
              Sprite-Prioritäten (Anzeige)
              MDP = 1: Grafikhintergrund, Sprite, Vordergrund
              MDP = 0: Grafikhintergrund, Vordergrund, Sprite

              Kollision:
              Eine Kollision zwischen Sprites und anderen Grafikdaten wird erkannt,
              sobald beim Bildaufbau ein nicht transparentes Spritepixel und ein Vordergrundpixel ausgegeben wird.

            */
            int spritenum = 1 << ((sprite >> 8) & 0x07);
            pixel = sprite & 0x0f; //Hintergrundgrafik

            if (sprite & 0x4000) {   // MDP = 1
              if (chr & 0x80) { //Vordergrundpixel ist gesetzt
                cpu.vic.fgcollision |= spritenum;
                pixel = 0;
              }
            } else {            // MDP = 0
              if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergrundpixel ist gesetzt
            }

          } else {            // Kein Sprite
            pixel = 0;
          }

          *p++ = cpu.vic.palette[pixel];

          spl++;
          chr = chr << 1;
        }

      } else {//Zeichen ist MULTICOLOR

        for (unsigned i = 0; i < 4; i++) {
          if (p >= pe) break;

          chr = chr << 2;

          int sprite = *spl;
          if (sprite) {    // Sprite: Ja
            int spritenum = 1 << ((sprite >> 8) & 0x07);
            pixel = sprite & 0x0f; //Hintergrundgrafik
            if (sprite & 0x4000) {  // MDP = 1

              if (chr & 0x80) {  //Vordergrundpixel ist gesetzt
                cpu.vic.fgcollision |= spritenum;
                pixel = 0;
              }
            } else {          // MDP = 0
              if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergrundpixel ist gesetzt
            }

          } else { // Kein Sprite
            pixel = 0;

          }
          *p++ = cpu.vic.palette[pixel];
          if (p >= pe) break;
          spl++;
          sprite = *spl;
          //Das gleiche nochmal für das nächste Pixel
          if (sprite) {    // Sprite: Ja
            int spritenum = 1 << ((sprite >> 8) & 0x07);
            pixel = sprite & 0x0f; //Hintergrundgrafik
            if (sprite & 0x4000) {  // MDP = 1
              if (chr & 0x80) { //Vordergrundpixel ist gesetzt
                cpu.vic.fgcollision |= spritenum;
                pixel = 0;
              }
            } else {          // MDP = 0
              if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergrundpixel ist gesetzt
            }
          } else { // Kein Sprite
            pixel = 0;
          }
          *p++ = cpu.vic.palette[pixel];
          spl++;

        }

      }

    } while (p < pe);
    PRINTOVERFLOWS

  } else { //Keine Sprites
    //Farbe immer schwarz
    const uint16_t bgcol = palette[0];
    while (p < pe - 8) {

      BADLINE(x);
      x++;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;

    };
    while (p < pe) {

      BADLINE(x);
      x++;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol;

    };
    PRINTOVERFLOW
  }
  while (x<40) {BADLINE(x); x++;}
}
/*****************************************************************************************************/
void mode6 (uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc) {
  /*
    Ungültiger Bitmap-Modus 1 (ECM/BMM/MCM=1/1/0)

    Dieser Modus erzeugt nur ebenfalls nur ein schwarzes Bild, die Pixel lassen
    sich allerdings auch hier mit dem Spritekollisionstrick auslesen.

    Der Aufbau der Grafik ist im Prinzip wie im Standard-Bitmap-Modus, aber die
    Bits 9 und 10 der g-Adressen sind wegen dem gesetzten ECM-Bit immer Null,
    entsprechend besteht auch die Grafik - grob gesagt - aus vier
    "Abschnitten", die jeweils viermal wiederholt dargestellt werden.

  */

  uint8_t chr, pixel;
  uint8_t x = 0;

  if (cpu.vic.lineHasSprites) {

    do {

      BADLINE(x);

      chr = cpu.vic.bitmapPtr[x * 8];

      x++;

      unsigned m = min(8, pe - p);
      for (unsigned i = 0; i < m; i++) {

        int sprite = *spl;

        chr = chr << 1;
        if (sprite) {     // Sprite: Ja
          /*
             Sprite-Prioritäten (Anzeige)
             MDP = 1: Grafikhintergrund, Sprite, Vordergrund
             MDP = 0: Grafikhintergung, Vordergrund, Sprite

             Kollision:
             Eine Kollision zwischen Sprites und anderen Grafikdaten wird erkannt,
             sobald beim Bildaufbau ein nicht transparentes Spritepixel und ein Vordergrundpixel ausgegeben wird.

          */
          int spritenum = 1 << ((sprite >> 8) & 0x07);
          pixel = sprite & 0x0f; //Hintergrundgrafik
          if (sprite & 0x4000) {   // MDP = 1
            if (chr & 0x80) { //Vordergrundpixel ist gesetzt
              cpu.vic.fgcollision |= spritenum;
              pixel = 0;
            }
          } else {            // MDP = 0
            if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergrundpixel ist gesetzt
          }

        } else {            // Kein Sprite
          pixel = 0;
        }

        *p++ = cpu.vic.palette[pixel];
        spl++;

      }

    } while (p < pe);
    PRINTOVERFLOWS

  } else { //Keine Sprites
    //Farbe immer schwarz
    const uint16_t bgcol = palette[0];
    while (p < pe - 8) {

      BADLINE(x);
      x++;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;

    };
    while (p < pe) {

      BADLINE(x);
      x++;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol;

    };
    PRINTOVERFLOW
  }
  while (x<40) {BADLINE(x); x++;}
}
/*****************************************************************************************************/
void mode7 (uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc) {
  /*
    Ungültiger Bitmap-Modus 2 (ECM/BMM/MCM=1/1/1)

    Der letzte ungültige Modus liefert auch ein schwarzes Bild, das sich jedoch
    genauso mit Hilfe der Sprite-Grafik-Kollisionen "abtasten" läßt.

    Der Aufbau der Grafik ist im Prinzip wie im Multicolor-Bitmap-Modus, aber
    die Bits 9 und 10 der g-Adressen sind wegen dem gesetzten ECM-Bit immer
    Null, was sich in der Darstellung genauso wie beim ersten ungültigen
    Bitmap-Modus wiederspiegelt. Die Bitkombination "01" wird wie gewohnt zum
    Hintergrund gezählt.

  */

  uint8_t chr;
  uint8_t x = 0;
  uint16_t pixel;

  if (cpu.vic.lineHasSprites) {

    do {

      BADLINE(x);

      chr = cpu.vic.bitmapPtr[x * 8];
      x++;

      for (unsigned i = 0; i < 4; i++) {
        if (p >= pe) break;

        chr = chr << 2;

        int sprite = *spl;
        if (sprite) {    // Sprite: Ja
          int spritenum = 1 << ((sprite >> 8) & 0x07);
          pixel = sprite & 0x0f;//Hintergrundgrafik
          if (sprite & 0x4000) {  // MDP = 1

            if (chr & 0x80) {  //Vordergrundpixel ist gesetzt
              cpu.vic.fgcollision |= spritenum;
              pixel = 0;
            }
          } else {          // MDP = 0
            if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergundpixel ist gesetzt
          }
        } else { // Kein Sprite
          pixel = 0;
        }

        *p++ = cpu.vic.palette[pixel];
        if (p >= pe) break;
        spl++;
        sprite = *spl;

        if (sprite) {    // Sprite: Ja
          int spritenum = 1 << ((sprite >> 8) & 0x07);
          pixel = sprite & 0x0f;//Hintergrundgrafik
          if (sprite & 0x4000) {  // MDP = 1

            if (chr & 0x80) {  //Vordergrundpixel ist gesetzt
              cpu.vic.fgcollision |= spritenum;
              pixel = 0;
            }
          } else {          // MDP = 0
            if (chr & 0x80) cpu.vic.fgcollision |= spritenum; //Vordergundpixel ist gesetzt
          }
        } else { // Kein Sprite
          pixel = 0;
        }

        *p++ = cpu.vic.palette[pixel];
        spl++;

      }

    } while (p < pe);
    PRINTOVERFLOWS

  } else { //Keine Sprites

    const uint16_t bgcol = palette[0];
    while (p < pe - 8) {

      BADLINE(x);
      x++;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;
      *p++ = bgcol; *p++ = bgcol;

    };
    while (p < pe) {

      BADLINE(x);
      x++;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol; if (p >= pe) break;
      *p++ = bgcol; if (p >= pe) break; *p++ = bgcol;

    };
    PRINTOVERFLOW

  }
  while (x<40) {BADLINE(x); x++;}
}
/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/

typedef void (*modes_t)( uint16_t *p, const uint16_t *pe, const uint16_t *spl, const uint16_t vc ); //Funktionspointer
const modes_t modes[8] = {mode0, mode1, mode2, mode3, mode4, mode5, mode6, mode7};


void vic_do(void) {

  uint16_t vc;
  uint16_t fgcol, xscroll;
  uint16_t *pe;
  uint16_t *p;
  uint16_t *spl;

  /*****************************************************************************************************/
  /* Linecounter ***************************************************************************************/
  /*****************************************************************************************************/
  /*
    ?PEEK(678) NTSC =0
    ?PEEK(678) PAL = 1
  */

  if ( cpu.vic.rasterLine >= LINECNT ) {

    //reSID sound needs much time - too much to keep everything in sync and with stable refreshrate
    //but it is not called very often, so most of the time, we have more time than needed.
    //We can measure the time needed for a frame and calc a correction factor to speed things up.
    unsigned long m = micros();
    cpu.vic.neededTime = (m - cpu.vic.timeStart);
    cpu.vic.timeStart = m;
    cpu.vic.lineClock.setInterval(LINETIMER_DEFAULT_FREQ - ((float)cpu.vic.neededTime / (float)LINECNT - LINETIMER_DEFAULT_FREQ ));

    cpu.vic.rasterLine = 0;
    cpu.vic.vcbase = 0;
    cpu.vic.denLatch = 0;

  } else  cpu.vic.rasterLine++;

  int r = cpu.vic.rasterLine;

  if (r == cpu.vic.intRasterLine )//Set Rasterline-Interrupt
    cpu.vic.R[0x19] |= 1 | ((cpu.vic.R[0x1a] & 1) << 7);



  /*****************************************************************************************************/
  /* Badlines ******************************************************************************************/
  /*****************************************************************************************************/
  /*
    Ein Bad-Line-Zustand liegt in einem beliebigen Taktzyklus vor, wenn an der
    negativen Flanke von ø0 zu Beginn des Zyklus RASTER >= $30 und RASTER <=
    $f7 und die unteren drei Bits von RASTER mit YSCROLL übereinstimmen und in
    einem beliebigen Zyklus von Rasterzeile $30 das DEN-Bit gesetzt war.

    (default 3)
    yscroll : POKE 53265, PEEK(53265) AND 248 OR 1:POKE 1024,0
    yscroll : poke 53265, peek(53265) and 248 or 1

    DEN : POKE 53265, PEEK(53265) AND 224 Bildschirm aus

    Die einzige Verwendung von YSCROLL ist der Vergleich mit r in der Badline

  */

  if (r == 0x30 ) cpu.vic.denLatch |= cpu.vic.DEN;

  /* 3.7.2
    2. In der ersten Phase von Zyklus 14 jeder Zeile wird VC mit VCBASE geladen
     (VCBASE->VC) und VMLI gelöscht. Wenn zu diesem Zeitpunkt ein
     Bad-Line-Zustand vorliegt, wird zusätzlich RC auf Null gesetzt.
  */

  cpu.vic.badline = (cpu.vic.denLatch && (r >= 0x30) && (r <= 0xf7) && ( (r & 0x07) == cpu.vic.YSCROLL));

  vc = cpu.vic.vcbase;
  if (cpu.vic.badline) {
    cpu.vic.idle = 0;
    cpu.vic.rc = 0;
  }

  /*****************************************************************************************************/
  /*****************************************************************************************************/

  {
    int t = MAXCYCLESSPRITES3_7 - cpu.vic.spriteCycles3_7;
    if (t > 0) cpu_clock(t);
    if (cpu.vic.spriteCycles3_7 > 0) cia_clockt(cpu.vic.spriteCycles3_7);
  }

#if 1
  cpu_clock(2); //BUGBUG THIS IS NEEDED FOR the demo " DELIRIOUS "
  cpu_clock(2); //Two more cycles  for " COMMANDO " to prevent problems when scrolling
#endif

  //cpu.vic.videomatrix =  cpu.vic.bank + (unsigned)(cpu.vic.R[0x18] & 0xf0) * 64;

  /* Rand oben /unten **********************************************************************************/
  /*
    RSEL  Höhe des Anzeigefensters  Erste Zeile   Letzte Zeile
    0 24 Textzeilen/192 Pixel 55 ($37)  246 ($f6) = 192 sichtbare Zeilen, der Rest ist Rand oder unsichtbar
    1 25 Textzeilen/200 Pixel 51 ($33)  250 ($fa) = 200 sichtbare Zeilen, der Rest ist Rand oder unsichtbar
  */

  if (cpu.vic.borderFlag) {
    int firstLine = (cpu.vic.RSEL) ? 0x33 : 0x37;
    if ((cpu.vic.DEN) && (r == firstLine)) cpu.vic.borderFlag = false;
  } else {
    int lastLine = (cpu.vic.RSEL) ? 0xfb : 0xf7;
    if (r == lastLine) cpu.vic.borderFlag = true;
  }

  if (r < FIRSTDISPLAYLINE || r > LASTDISPLAYLINE ) {
    if (r == 0)
      cpu_clock(CYCLESPERRASTERLINE - MAXCYCLESSPRITES - 1);
    else
      cpu_clock(CYCLESPERRASTERLINE - MAXCYCLESSPRITES);
    goto noDisplayIncRC;
  }

  //max_x =  (!cpu.vic.CSEL) ? 40:38;
  spl = &cpu.vic.spriteLine[24];
  p = (uint16_t*)&screen[r - FIRSTDISPLAYLINE][0];
  pe = (uint16_t*)&screen[r - FIRSTDISPLAYLINE][SCREEN_WIDTH];

  cpu_clock(6); //left screenborder, now 40 max cycles left.

  if (cpu.vic.borderFlag) {
    fastFillLineNoSprites(p, pe, cpu.vic.palette[cpu.vic.EC]);
    goto noDisplayIncRC ;
  }

  /*
    if (!cpu.vic.CSEL) {
      pe -= 9;
    }
  */

  /*****************************************************************************************************/
  /* DISPLAY *******************************************************************************************/
  /*****************************************************************************************************/

  //max_x =  (!cpu.vic.CSEL) ? 40:38;
  //X-Scrolling:

  xscroll = cpu.vic.XSCROLL;

  if (xscroll > 0) {
    uint16_t col = cpu.vic.palette[cpu.vic.EC];

    if (!cpu.vic.CSEL) {
      cpu_clock(1);
      uint16_t sprite;
      for (int i = 0; i < xscroll; i++) {
        SPRITEORFIXEDCOLOR();
      }
    } else {
      spl += xscroll;
      for (unsigned i = 0; i < xscroll; i++) {
        *p++ = col;
      }

    }
  }

  /*****************************************************************************************************/
  /*****************************************************************************************************/
  /*****************************************************************************************************/


  cpu.vic.fgcollision = 0;

 if (1 | !cpu.vic.idle)  {

    uint8_t mode = (cpu.vic.ECM << 2) | (cpu.vic.BMM << 1) | cpu.vic.MCM;

    if (cpu.vic.BMM) {
      if (cpu.vic.ECM) {
        cpu.vic.bitmapPtr = (uint8_t*) &cpu.RAM[cpu.vic.bank | ((((unsigned)(cpu.vic.R[0x18] & 0x08) * 0x400) + vc * 8) & 0xf9FF)] + cpu.vic.rc;
      } else {
        cpu.vic.bitmapPtr = (uint8_t*) &cpu.RAM[cpu.vic.bank | ((unsigned)(cpu.vic.R[0x18] & 0x08) * 0x400) | vc * 8] + cpu.vic.rc;
      }
    }

#if 0
    static uint8_t omode = 99;
    if (mode != omode) {
      Serial.print("Graphicsmode:");
      Serial.println(mode);
      omode = mode;
    }
#endif

    modes[mode](p, pe, spl, vc);

    /*if (!cpu.vic.idle)*/ vc = (vc + 40) & 0x3ff;

  } else {
    //IDLE Mode
	//TODO: 3.7.3.9. Idle-Zustand
    fastFillLine(p, pe, cpu.vic.palette[0], spl);
  }

  /*
    Bei den MBC- und MMC-Interrupts löst jeweils nur die erste Kollision einen
    Interrupt aus (d.h. wenn die Kollisionsregister $d01e bzw. $d01f vor der
    Kollision den Inhalt Null hatten). Um nach einer Kollision weitere
    Interrupts auszulösen, muß das betreffende Register erst durch Auslesen
    gelöscht werden.
  */

  if (cpu.vic.fgcollision) {
    if (cpu.vic.MD == 0) {
      cpu.vic.R[0x19] |= 2 | ( (cpu.vic.R[0x1a] & 2) << 6);
    }
    cpu.vic.MD |= cpu.vic.fgcollision;
  }

  /*****************************************************************************************************/

  if (!cpu.vic.CSEL) {
    cpu_clock(1);
    uint16_t col = cpu.vic.palette[cpu.vic.EC];
    p = &screen[r - FIRSTDISPLAYLINE][0];

#if 0
    // Sprites im Rand
    uint16_t sprite;
    uint16_t * spl;
    spl = &cpu.vic.spriteLine[24 + xscroll];

    SPRITEORFIXEDCOLOR()
    SPRITEORFIXEDCOLOR()
    SPRITEORFIXEDCOLOR()
    SPRITEORFIXEDCOLOR()
    SPRITEORFIXEDCOLOR()
    SPRITEORFIXEDCOLOR()
    SPRITEORFIXEDCOLOR() //7
#else
    //keine Sprites im Rand
    *p++ = col; *p++ = col; *p++ = col; *p++ = col;
    *p++ = col; *p++ = col; *p = col;

#endif

    //Rand rechts:
    p = &screen[r - FIRSTDISPLAYLINE][SCREEN_WIDTH - 9];
    pe = (uint16_t*)&screen[r - FIRSTDISPLAYLINE][SCREEN_WIDTH];

#if 0
    // Sprites im Rand
    spl = &cpu.vic.spriteLine[24 + SCREEN_WIDTH - 9 + xscroll];
    while (p < pe) {
      SPRITEORFIXEDCOLOR();
    }
#else
    //keine Sprites im Rand
    while (p < pe) {
      *p++ = col;
    }
#endif

  }

noDisplayIncRC:
  /* 3.7.2
    5. In der ersten Phase von Zyklus 58 wird geprüft, ob RC=7 ist. Wenn ja,
     geht die Videologik in den Idle-Zustand und VCBASE wird mit VC geladen
     (VC->VCBASE). Ist die Videologik danach im Display-Zustand (liegt ein
     Bad-Line-Zustand vor, ist dies immer der Fall), wird RC erhöht.
  */

  if (cpu.vic.rc == 7) {
    cpu.vic.idle = 1;
    cpu.vic.vcbase = vc;
  }
  //Ist dies richtig ?? 
  if ((!cpu.vic.idle) || (cpu.vic.denLatch && (r >= 0x30) && (r <= 0xf7) && ( (r & 0x07) == cpu.vic.YSCROLL))) {
    cpu.vic.rc = (cpu.vic.rc + 1) & 0x07;
  }


  /*****************************************************************************************************/
  /* Sprites *******************************************************************************************/
  /*****************************************************************************************************/
#define SPRITENUM(data) (1 << ((data >> 8) & 0x07)

  cpu.vic.spriteCycles0_2 = 0;
  cpu.vic.spriteCycles3_7 = 0;

  if (cpu.vic.lineHasSprites) {
    memset(cpu.vic.spriteLine, 0, sizeof(cpu.vic.spriteLine) );
    cpu.vic.lineHasSprites = 0;
  }

  uint32_t spriteYCheck = cpu.vic.R[0x15]; //Sprite enabled Register

  if (spriteYCheck) {

    unsigned short R17 = cpu.vic.R[0x17]; //Sprite-y-expansion
    unsigned char collision = 0;
    short lastSpriteNum = 0;

    for (unsigned short i = 0; i < 8; i++) {
      if (!spriteYCheck) break;

      unsigned b = 1 << i;

      if (spriteYCheck & b )  {
        spriteYCheck &= ~b;
        short y = cpu.vic.R[i * 2 + 1];

        if ( (r >= y ) && //y-Position > Sprite-y ?
             (((r < y + 21) && (~R17 & b )) || // ohne y-expansion
              ((r < y + 2 * 21 ) && (R17 & b ))) ) //mit y-expansion
        {

          //Sprite Cycles
          if (i < 3) {
            if (!lastSpriteNum) cpu.vic.spriteCycles0_2 += 1;
            cpu.vic.spriteCycles0_2 += 2;
          } else {
            if (!lastSpriteNum) cpu.vic.spriteCycles3_7 += 1;
            cpu.vic.spriteCycles3_7 += 2;
          }
          lastSpriteNum = i;
          //Sprite Cycles END


          if (r < FIRSTDISPLAYLINE || r > LASTDISPLAYLINE ) continue;

          uint16_t x =  (((cpu.vic.R[0x10] >> i) & 1) << 8) | cpu.vic.R[i * 2];
          if (x >= 320 + 24) continue;

          unsigned short lineOfSprite = r - y;
          if (R17 & b) lineOfSprite = lineOfSprite / 2; // Y-Expansion
          unsigned short spriteadr = cpu.vic.bank | cpu.RAM[cpu.vic.videomatrix + (1024 - 8) + i] << 6 | (lineOfSprite * 3);
          unsigned spriteData = ((unsigned)cpu.RAM[ spriteadr ] << 16) | ((unsigned)cpu.RAM[ spriteadr + 1 ] << 8) | ((unsigned)cpu.RAM[ spriteadr + 2 ]);

          if (!spriteData) continue;
          cpu.vic.lineHasSprites = 1;

          uint16_t * slp = &cpu.vic.spriteLine[x]; //Sprite-Line-Pointer
          unsigned short upperByte = ( 0x80 | ( (cpu.vic.MDP & b) ? 0x40 : 0 ) | i ) << 8; //Bit7 = Sprite "da", Bit 6 = Sprite-Priorität vor Grafik/Text, Bits 3..0 = Spritenummer

          //Sprite in Spritezeile schreiben:
          if ((cpu.vic.MMC & b) == 0) { // NO MULTICOLOR

            uint16_t color = upperByte | cpu.vic.R[0x27 + i];

            if ((cpu.vic.MXE & b) == 0) { // NO MULTICOLOR, NO SPRITE-EXPANSION

              for (unsigned cnt = 0; (spriteData > 0) && (cnt < 24); cnt++) {
                int c = (spriteData >> 23) & 0x01;
                spriteData = (spriteData << 1);

                if (c) {
                  if (*slp == 0) *slp = color;
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                }
                slp++;

              }

            } else {    // NO MULTICOLOR, SPRITE-EXPANSION

              for (unsigned cnt = 0; (spriteData > 0) && (cnt < 24); cnt++) {
                int c = (spriteData >> 23) & 0x01;
                spriteData = (spriteData << 1);
                //So wie oben, aber zwei gleiche Pixel

                if (c) {
                  if (*slp == 0) *slp = color;
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                  slp++;
                  if (*slp == 0) *slp = color;
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                  slp++;
                } else {
                  slp += 2;
                }

              }
            }



          } else { // MULTICOLOR
            /* Im Mehrfarbenmodus (Multicolor-Modus) bekommen alle Sprites zwei zusätzliche gemeinsame Farben.
              Die horizontale Auflösung wird von 24 auf 12 halbiert, da bei der Sprite-Definition jeweils zwei Bits zusammengefasst werden.
            */
            uint16_t colors[4];
            //colors[0] = 1; //dummy, color 0 is transparent
            colors[1] = upperByte | cpu.vic.R[0x25];
            colors[2] = upperByte | cpu.vic.R[0x27 + i];
            colors[3] = upperByte | cpu.vic.R[0x26];

            if ((cpu.vic.MXE & b) == 0) { // MULTICOLOR, NO SPRITE-EXPANSION
              for (unsigned cnt = 0; (spriteData > 0) && (cnt < 24); cnt++) {
                int c = (spriteData >> 22) & 0x03;
                spriteData = (spriteData << 2);

                if (c) {
                  if (*slp == 0) *slp = colors[c];
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                  slp++;
                  if (*slp == 0) *slp = colors[c];
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                  slp++;
                } else {
                  slp += 2;
                }

              }

            } else {    // MULTICOLOR, SPRITE-EXPANSION
              for (unsigned cnt = 0; (spriteData > 0) && (cnt < 24); cnt++) {
                int c = (spriteData >> 22) & 0x03;
                spriteData = (spriteData << 2);

                //So wie oben, aber vier gleiche Pixel
                if (c) {
                  if (*slp == 0) *slp = colors[c];
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                  slp++;
                  if (*slp == 0) *slp = colors[c];
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                  slp++;
                  if (*slp == 0) *slp = colors[c];
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                  slp++;
                  if (*slp == 0) *slp = colors[c];
                  else collision |= b | (1 << ((*slp >> 8) & 0x07));
                  slp++;
                } else {
                  slp += 4;
                }

              }

            }
          }

        }
        else lastSpriteNum = 0;
      }

    }

    if (collision) {
      if (cpu.vic.MM == 0) {
        cpu.vic.R[0x19] |= 4 | ((cpu.vic.R[0x1a] & 4) << 5 );
      }
      cpu.vic.MM |= collision;
    }

  }
  /*****************************************************************************************************/
  {
    int t = MAXCYCLESSPRITES0_2 - cpu.vic.spriteCycles0_2;
    if (t > 0) cpu_clock(t);
    if (cpu.vic.spriteCycles0_2 > 0) cia_clockt(cpu.vic.spriteCycles0_2);
  }

  return;
}

/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
void fastFillLineNoSprites(uint16_t * p, const uint16_t * pe, const uint16_t col) {
  // 16Bit align to 32Bit
  while ( ( (uintptr_t)p & 0x03) != 0 && p < pe) {
    *p++ = col;
  }
  // Fast 32BIT aligned fill
  while ( p <= pe - 2 )  {
    *(uint32_t *)p = col << 16 | col;
    p += 2;
  }
  // Fill remaining
  while ( p < pe ) {
    *p++ = col;
  }

  CYCLES(40);
}

void fastFillLine(uint16_t * p, const uint16_t * pe, const uint16_t col, uint16_t *  spl) {
  uint16_t sprite;

  if (spl != NULL && cpu.vic.lineHasSprites) {

    while ( p < pe ) {
      SPRITEORFIXEDCOLOR();
    };
    CYCLES(40);

  } else {

    fastFillLineNoSprites(p, pe, col);

  }
}

/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/
void vic_adrchange(void) {
  uint8_t r18 = cpu.vic.R[0x18];
  cpu.vic.videomatrix =  cpu.vic.bank + (unsigned)(r18 & 0xf0) * 64;

  unsigned charsetAddr = r18 & 0x0e;
  if  ((cpu.vic.bank & 0x4000) == 0) {
    if (charsetAddr == 0x04) cpu.vic.charsetPtrBase =  ((uint8_t *)&rom_characters);
    else if (charsetAddr == 0x06) cpu.vic.charsetPtrBase =  ((uint8_t *)&rom_characters) + 0x800;
    else
      cpu.vic.charsetPtrBase = &cpu.RAM[charsetAddr * 0x400 + cpu.vic.bank] ;
  } else
    cpu.vic.charsetPtrBase = &cpu.RAM[charsetAddr * 0x400 + cpu.vic.bank];

}
/*****************************************************************************************************/
void vic_write(uint32_t address, uint8_t value) {

  address &= 0x3F;

  switch (address) {
    case 0x11 :
      cpu.vic.intRasterLine = (cpu.vic.intRasterLine & 0xff) | ((((uint16_t) value) << 1) & 0x100);
      if (cpu.vic.rasterLine == 0x30 ) cpu.vic.denLatch |= value & 0x10;
      cpu.vic.badline = (cpu.vic.denLatch && (cpu.vic.rasterLine >= 0x30) && (cpu.vic.rasterLine <= 0xf7) && ( (cpu.vic.rasterLine & 0x07) == (value & 0x07)));
	  if (cpu.vic.badline) { cpu.vic.idle = 0;}
      cpu.vic.R[address] = value;
      break;
    case 0x12 :
      cpu.vic.intRasterLine = (cpu.vic.intRasterLine & 0x100) | value;
      cpu.vic.R[address] = value;
      break;
    case 0x18 :
      cpu.vic.R[address] = value;
      vic_adrchange();
      break;
    case 0x19 : //IRQs
      cpu.vic.R[0x19] &= (~value & 0x0f);
      //ggf Interrupt triggern

#if 0 // Dies sollte eigentlich so sein - funktioniert aber beim intro von "commando95" nicht!
      if (cpu.vic.R[0x19] & cpu.vic.R[0x1a])
        cpu.vic.R[0x19] |= 0x80;
      else
        cpu.vic.R[0x19] &= 0x7f;
#endif

      break;
    case 0x1A : //IRQ Mask
      cpu.vic.R[address] = value & 0x0f;
      //ggf Interrupt triggern
      if (cpu.vic.R[0x19] & cpu.vic.R[0x1a])
        cpu.vic.R[0x19] |= 0x80;
      else
        cpu.vic.R[0x19] &= 0x7f;

      break;
    case 0x1e:
    case 0x1f:
      cpu.vic.R[address] = 0;
      break;
    case 0x20 ... 0x2E:
      cpu.vic.R[address] = value & 0x0f;
      // cpu.vic.colors[address - 0x20] = cpu.vic.palette[value & 0x0f];
      break;
    case 0x2F ... 0x3F:
      break;
    default :
      cpu.vic.R[address] = value;
      break;
  }

  //#if DEBUGVIC
#if 0
  Serial.print("VIC ");
  Serial.print(address, HEX);
  Serial.print("=");
  Serial.println(value, HEX);
  //logAddr(address, value, 1);
#endif
}

/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/

uint8_t vic_read(uint32_t address) {
  uint8_t ret;

  address &= 0x3F;
  switch (address) {

    case 0x11:
      ret = (cpu.vic.R[address] & 0x7F) | ((cpu.vic.rasterLine & 0x100) >> 1);
      break;
    case 0x12:
      ret = cpu.vic.rasterLine;
      break;
    case 0x16:
      ret = cpu.vic.R[address] | 0xC0;
      break;
    case 0x18:
      ret = cpu.vic.R[address] | 0x01;
      break;
    case 0x19:
      ret = cpu.vic.R[address] | 0x70;
      break;
    case 0x1a:
      ret = cpu.vic.R[address] | 0xF0;
      break;
    case 0x1e:
    case 0x1f:
      ret = cpu.vic.R[address];
      cpu.vic.R[address] = 0;
      break;
    case 0x20 ... 0x2E:
      ret = cpu.vic.R[address] | 0xF0;
      break;
    case 0x2F ... 0x3F:
      ret = 0xFF;
      break;
    default:
      ret = cpu.vic.R[address];
      break;
  }

#if DEBUGVIC
  Serial.print("VIC ");
  logAddr(address, ret, 0);
#endif
  return ret;
}

/*****************************************************************************************************/
/*****************************************************************************************************/
/*****************************************************************************************************/

void resetVic(void) {

  cpu.vic.intRasterLine = 0;
  cpu.vic.rasterLine = 0;
  cpu.vic.lineHasSprites = 0;
  memset(&cpu.RAM[0x400], 0, 1000);
  memset(&cpu.vic, 0, sizeof(cpu.vic));
  memcpy(cpu.vic.palette, (void*)palette, sizeof(cpu.vic.palette));

  //http://dustlayer.com/vic-ii/2013/4/22/when-visibility-matters
  cpu.vic.R[0x11] = 0x9B;
  cpu.vic.R[0x16] = 0x08;
  cpu.vic.R[0x18] = 0x14;
  cpu.vic.R[0x19] = 0x0f;

  for (unsigned i = 0; i < sizeof(cpu.vic.COLORRAM); i++)
    cpu.vic.COLORRAM[i] = (rand() & 0x0F);

  cpu.RAM[0x39FF] = 0x0;
  cpu.RAM[0x3FFF] = 0x0;
  cpu.RAM[0x39FF + 16384] = 0x0;
  cpu.RAM[0x3FFF + 16384] = 0x0;
  cpu.RAM[0x39FF + 32768] = 0x0;
  cpu.RAM[0x3FFF + 32768] = 0x0;
  cpu.RAM[0x39FF + 49152] = 0x0;
  cpu.RAM[0x3FFF + 49152] = 0x0;

  vic_adrchange();
}


/*
  ?PEEK(678) NTSC =0
  ?PEEK(678) PAL = 1
  PRINT TIME$
*/
/*
          Raster-  Takt-   sichtb.  sichtbare
  VIC-II  System  zeilen   zyklen  Zeilen   Pixel/Zeile
  -------------------------------------------------------
  6569    PAL    312     63    284     403
  6567R8  NTSC   263     65    235     418
  6567R56A  NTSC   262   64    234     411
*/
