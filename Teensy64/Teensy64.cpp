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

#include <Arduino.h>
#include "Teensy64.h"

uint8_t SDinitialized = 0;

ILI9341_t3DMA tft = ILI9341_t3DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);

AudioPlaySID        playSID;  //xy=105,306
AudioOutputAnalog   audioout; //xy=476,333
AudioConnection     patchCord1(playSID, 0 , audioout, 0);

void resetMachine() {
  *(volatile uint32_t *)0xE000ED0C = 0x5FA0004;
  while (true) {
    ;
  }
}

void resetExternal() {
  //perform a Reset for external devices on IEC-Bus,
  //   detachInterrupt(digitalPinToInterrupt(PIN_RESET));

  digitalWriteFast(PIN_RESET, 0);
  delay(50);
  digitalWriteFast(PIN_RESET, 1);

}


void oneRasterLine(void) {
  static unsigned short lc = 1;

  while (true) {

    cpu.lineStartTime = ARM_DWT_CYCCNT;
    cpu.lineCycles = 0;

    vic_do();

    if (--lc == 0) {
      lc = LINEFREQ / 10; // 10Hz
      cia1_checkRTCAlarm();
      cia2_checkRTCAlarm();
    }

    //Switch "ExactTiming" Mode off after a while:
    if (!cpu.exactTiming) break;
    if (ARM_DWT_CYCCNT - cpu.exactTiming >= EXACTTIMINGDURATION * (F_CPU / 1000)) {
      cpu.exactTiming = 0;
      setAudioOn();
      LED_OFF;
      break;
    }
  };

}

void initMachine() {
#if F_BUS < 120000000
#error Teensy64: Please select F_CPU=240MHz and F_BUS=120MHz
#endif
  unsigned long m = millis();

#if 1
  //enable sd-card pullups early
  PORTE_PCR0 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.D1  */
  PORTE_PCR1 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.D0  */
  PORTE_PCR3 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.CMD */
  PORTE_PCR4 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.D3  */
  PORTE_PCR5 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.D2  */
#endif

  pinMode(PIN_RESET, OUTPUT_OPENDRAIN);
  digitalWriteFast(PIN_RESET, 1);

  pinMode(TFT_TOUCH_CS, OUTPUT);
  digitalWriteFast(TFT_TOUCH_CS, 1);

  LED_INIT;
  LED_ON;

  Serial.begin(9600);

  myusb.begin();

  tft.begin();
  tft.writeScreen((uint16_t*)logo_320x240);
  tft.refresh();

  SDinitialized = SD.begin(BUILTIN_SDCARD);

  unsigned audioSampleFreq;
  audioSampleFreq = setAudioSampleFreq(AUDIOSAMPLERATE);
  playSID.setSampleParameters((int) CLOCKSPEED, audioSampleFreq);

  LED_OFF;

  delay(250);

  while (!Serial && ((millis () - m) <= 1500)) {
    ;
  }

  Serial.println("=============================\n");
  Serial.println("Teensy64 " __DATE__ " " __TIME__ "\n");

  Serial.print("SD Card ");
  Serial.println(SDinitialized ? "initialized." : "failed, or not present.");

  Serial.println();
  Serial.print("F_CPU (MHz): ");
  Serial.print((int)(F_CPU / 1e6));
  Serial.println();
  Serial.print("F_BUS (MHz): ");
  Serial.print((int)(F_BUS / 1e6));
  Serial.println();

  Serial.println();
  Serial.print("Emulated Video: ");
  Serial.println(PAL ? "PAL" : "NTSC");
  Serial.print("Video line frequency (Hz): ");
  Serial.println(LINEFREQ, 3);
  Serial.print("Video refresh rate (Hz): ");
  Serial.println(REFRESHRATE, 3);

  Serial.println();
  Serial.print("Audioblock (Samples): ");
  Serial.print(AUDIO_BLOCK_SAMPLES);
  Serial.println();
  Serial.print("Audio Samplerate (Hz): ");
  Serial.println(audioSampleFreq);
  Serial.println();

  Serial.print("sizeof(tcpu) (Bytes): ");
  Serial.println(sizeof(tcpu));

  Serial.println();

  resetPLA();
  resetCia1();
  resetCia2();
  resetVic();
  cpu_reset();

  resetExternal();

  while ((millis () - m) <= 1500) {
    ;
  }

  Serial.println("Starting.\n\n");

#if FASTBOOT
  cpu_clock(2e6);
  cpu.RAM[678] = (PAL == 1) ? 1 : 0; //PAL/NTSC switch, C64-Autodetection does not work with FASTBOOT
#endif

  cpu.vic.lineClock.begin( oneRasterLine, LINETIMER_DEFAULT_FREQ);
  cpu.vic.lineClock.priority( ISR_PRIORITY_RASTERLINE );

  attachInterrupt(digitalPinToInterrupt(PIN_RESET), resetMachine, RISING);

  listInterrupts();
}

void yield(void) {
  static volatile uint8_t running = 0;
  if (running) return;
  running = 1;

  //Input via terminal to keyboardbuffer (for BASIC only)
  if ( Serial.available() ) {
    uint8_t r = Serial.read();
    sendKey(r);
    Serial.write(r);
  }
  do_sendString();

  running = 0;
};

