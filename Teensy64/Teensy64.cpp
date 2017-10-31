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
#include <core_pins.h>
#include "Teensy64.h"



#if VGA
#include <uVGA.h>
#include <uVGA_valid_settings.h>
UVGA_STATIC_FRAME_BUFFER(uvga_fb);
uVGA uvga;
uint8_t * VGA_frame_buffer = uvga_fb;

#else
ILI9341_t3DMA tft = ILI9341_t3DMA(TFT_CS, TFT_DC, TFT_RST, TFT_MOSI, TFT_SCLK, TFT_MISO);
#endif

#include "vic_palette.h"


AudioPlaySID        playSID;  //xy=105,306
AudioOutputAnalog   audioout; //xy=476,333
AudioConnection     patchCord1(playSID, 0 , audioout, 0);


SdFatSdio SD;
uint8_t SDinitialized = 0;


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
    cpu.lineCycles = cpu.lineCyclesAbs = 0;

    if (!cpu.exactTiming) {
		vic_do();
	} else {
		vic_do_simple();
	}

    if (--lc == 0) {
      lc = LINEFREQ / 10; // 10Hz
      cia1_checkRTCAlarm();
      cia2_checkRTCAlarm();
    }

    //Switch "ExactTiming" Mode off after a while:
    if (!cpu.exactTiming) break;
    if (ARM_DWT_CYCCNT - cpu.exactTimingStartTime >= EXACTTIMINGDURATION * (F_CPU / 1000)) {
	  cpu_disableExactTiming();
      break;
    }
  };

}


/*
	The USB HOST feature causes pixel-flicker on VGA, because it with GPIO on the same periphal bus.
	These functions sync to the HSYNC-Signal, and enable USB on the right edge of the screen only.
*/

#define USBHS_PERIODIC_TRANSFERS_DISABLE 0

#if VGA && USBHOST && USBHS_PERIODIC_TRANSFERS_DISABLE
//As above, but with enable/disable USB Periodic transfers
FASTRUN void ftm0_isr(void) {

	int16_t c = rasterLineCounterVGA;
	uint32_t i1 = FTM0_C2SC;
	if (i1 & FTM_CSC_CHF) {
		FTM0_C2SC = i1 & ~FTM_CSC_CHF;
		//Enable USB Host Periodic Transfers
		USBHS_USBCMD |= ( USBHS_USBCMD_PSE /* | USBHS_USBCMD_ASE */);
		c++;
		if (c == modeline.vtotal) {c = 0;}
		rasterLineCounterVGA = c;
//		digitalWriteFast(13,0);
	} else {
		uint32_t i2 = FTM0_C3SC;
		FTM0_C3SC = i2 & ~FTM_CSC_CHF;
		//Disable USB Host Periodic Transfers
		if (c > 51 && c < 593) USBHS_USBCMD &= ~(USBHS_USBCMD_PSE /* | USBHS_USBCMD_ASE */);
//  	digitalWriteFast(13,1);
	}
}
#endif

void add_uVGAhsync(void) {
 
  USBHS_ASYNC_OFF;

  Serial.println("uVGA: Add hSync interrupt.");

#if VGA && USBHOST && USBHS_PERIODIC_TRANSFERS_DISABLE

  // Add channels 2 + 3 to FTM 0 as triggers for FTM0 interrupt

  uint32_t sc = FTM0_SC;
  FTM0_SC = 0;

#if 0
  Serial.print("FTM 0 Channel 0 SC: 0x");
  Serial.println(FTM0_C0SC, HEX);
  Serial.print("FTM 0 Channel 0 V:");
  Serial.println(FTM0_C0V);
  Serial.print("FTM 0 Channel 1 SC: 0x");
  Serial.println(FTM0_C1SC, HEX);
  Serial.print("FTM 0 Channel 1 V:");
  Serial.println(FTM0_C1V);
#endif


  //Add channels 2+3
  FTM0_C2SC = FTM0_C0SC | FTM_CSC_CHIE; // With interrupt
  FTM0_C2V = FTM0_C0V - 260; //Value determined experimentally

  FTM0_C3SC = FTM0_C1SC;
  FTM0_C3V = FTM0_C1V + 145; //Value determined experimentally

  const uint32_t channel_shift = (2 >> 1) << 3;	// combine bits is at position (channel pair number (=channel number /2) * 8)
  FTM0_COMBINE |= ((FTM0_COMBINE & ~(0x000000FF << channel_shift)) | ((FTM_COMBINE_COMBINE0 | FTM_COMBINE_COMP0) << channel_shift));

  // re-start FTM0
  FTM0_SC = sc;

  noInterrupts();
  NVIC_SET_PRIORITY(IRQ_FTM0, 0);
  NVIC_ENABLE_IRQ(IRQ_FTM0);
  uvga.waitSync();
  rasterLineCounterVGA = 0;
  interrupts();

#endif

}



DMAChannel dma_gpio(false);

void setupGPIO_DMA(void) {
	dma_gpio.begin(true);

	dma_gpio.TCD->CSR = 0;
	dma_gpio.TCD->SADDR = &GPIOA_PDIR;
	dma_gpio.TCD->SOFF = (uintptr_t) &GPIOB_PDIR - (uintptr_t) &GPIOA_PDIR;
	dma_gpio.TCD->ATTR = DMA_TCD_ATTR_SSIZE(DMA_TCD_ATTR_SIZE_32BIT) | DMA_TCD_ATTR_DSIZE(DMA_TCD_ATTR_SIZE_32BIT);
	dma_gpio.TCD->NBYTES =  sizeof(uint32_t);
	dma_gpio.TCD->SLAST = -5 * ( (uintptr_t) &GPIOB_PDIR - (uintptr_t) &GPIOA_PDIR );

	dma_gpio.TCD->DADDR = &io;
	dma_gpio.TCD->DOFF = sizeof(uint32_t);
	dma_gpio.TCD->DLASTSGA = -5 * sizeof(uint32_t);

	dma_gpio.TCD->CITER = 5;
	dma_gpio.TCD->BITER = 5;

	//Audio-DAC is triggered with PDB(NO-VGA) or FTM-Channel 7
	//Use The Audio-DMA a trigger for this DMA-Channel:
	dma_gpio.triggerAtCompletionOf(audioout.dma);
	dma_gpio.enable();

}


void initMachine() {

#if F_CPU < 240000000
#error Teensy64: Please select F_CPU=240MHz
#endif

#if !VGA && F_BUS < 120000000
#error Teensy64: Please select F_BUS=120MHz
#endif

#if VGA && !( defined(USB_RAWHID) || defined(USB_DISABLED) )
#pragma message "Teensy64: Please select USB Type Raw HID"
#endif

#if AUDIO_BLOCK_SAMPLES > 32
#error Teensy64: Set AUDIO_BLOCK_SAMPLES to 32
#endif

  unsigned long m = millis();

  //enable sd-card pullups early
  PORTE_PCR0 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.D1  */
  PORTE_PCR1 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.D0  */
  PORTE_PCR3 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.CMD */
  PORTE_PCR4 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.D3  */
  PORTE_PCR5 = PORT_PCR_MUX(1) | PORT_PCR_PE | PORT_PCR_PS;   /* PULLUP SDHC.D2  */


  // turn on USB host power for VGA Board
  PORTE_PCR6 = PORT_PCR_MUX(1);
  GPIOE_PDDR |= (1<<6);
  GPIOE_PSOR = (1<<6);

  pinMode(PIN_RESET, OUTPUT_OPENDRAIN);
  digitalWriteFast(PIN_RESET, 1);

#if !VGA
  pinMode(TFT_TOUCH_CS, OUTPUT);
  digitalWriteFast(TFT_TOUCH_CS, 1);
#endif

  LED_INIT;
  LED_ON;
  disableEventResponder();

  Serial.begin(9600);
  Serial.println("Init");
#if USBHOST
  myusb.begin();
#endif

#if VGA

  uvga.set_static_framebuffer(VGA_frame_buffer);
  uvga.begin(&modeline);
  uvga.clear(0x00);
 // for (int i =0; i<299;i++) memset(VGA_frame_buffer + i*464, palette[14], 452-(37));

#else

  tft.begin();
  tft.writeScreen((uint16_t*)logo_320x240);
  tft.refresh();

#endif

  SDinitialized = SD.begin();
  float audioSampleFreq;

#if !VGA
  audioSampleFreq = AUDIOSAMPLERATE;
  audioSampleFreq = setAudioSampleFreq(audioSampleFreq);
#else
  audioSampleFreq = ((float)modeline.pixel_clock / (float) modeline.htotal);
  FTM0_C7SC |= FTM_CSC_DMA; //Enable Audio-DMA-Trigger on every VGA-Rasterline. FTM0 is VGA Timer.
#endif
  playSID.setSampleParameters(CLOCKSPEED, audioSampleFreq);

  delay(250);

  while (!Serial && ((millis() - m) <= 1500)) {
    ;
  }

  LED_OFF;

  Serial.println("=============================\n");
  Serial.println("Teensy64 v." VERSION " " __DATE__ " " __TIME__ "\n");

  Serial.print("SD Card ");
  Serial.println(SDinitialized ? "initialized." : "failed, or not present.");

  Serial.println();
  Serial.print("F_CPU (MHz): ");
  Serial.print((int)(F_CPU / 1e6));
  Serial.println();
  Serial.print("F_BUS (MHz): ");
  Serial.print((int)(F_BUS / 1e6));
  Serial.println();
  Serial.print("Display: ");
  Serial.println((VGA>0) ? "VGA":"TFT");
  Serial.println();

  Serial.print("Emulated video: ");
  Serial.println(PAL ? "PAL" : "NTSC");
  Serial.print("Emulated video line frequency (Hz): ");
  Serial.println(LINEFREQ, 3);
  Serial.print("Emulated video refresh rate (Hz): ");
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

  while ((millis() - m) <= 1500) {
    ;
  }

  add_uVGAhsync();
  setupGPIO_DMA();

  Serial.println("Starting.\n");

#if FASTBOOT
  cpu_clock(2e6);
  cpu.RAM[678] = (PAL == 1) ? 1 : 0; //PAL/NTSC switch, C64-Autodetection does not work with FASTBOOT
#endif

  cpu.vic.lineClock.begin( oneRasterLine, LINETIMER_DEFAULT_FREQ);
  cpu.vic.lineClock.priority( ISR_PRIORITY_RASTERLINE );

  attachInterrupt(digitalPinToInterrupt(PIN_RESET), resetMachine, RISING);

  listInterrupts();

}


// Switch off/replace Teensyduinos` yield and systick stuff

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

  myusb.Task();
  
  running = 0;
};

