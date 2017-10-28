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
#include "util.h"
#include "Teensy64.h"

//Attention, don't use WFI-instruction - the CPU does not count cycles during sleep
void enableCycleCounter(void) {
  ARM_DEMCR |= ARM_DEMCR_TRCENA;
  ARM_DWT_CTRL |= ARM_DWT_CTRL_CYCCNTENA;
}

extern "C" volatile uint32_t systick_millis_count;
void mySystick_isr(void) { systick_millis_count++; }
void myUnused_isr(void) {};

void disableEventResponder(void) {
	_VectorsRam[14] = myUnused_isr;  // pendablesrvreq
	_VectorsRam[15] = mySystick_isr; // Short Systick
}

#define PDB_CONFIG (PDB_SC_TRGSEL(15) | PDB_SC_PDBEN | PDB_SC_CONT | PDB_SC_PDBIE | PDB_SC_DMAEN)
static float setDACFreq(float freq) {

  if (!(SIM_SCGC6 & SIM_SCGC6_PDB)) return 0;

  unsigned int t = (float)F_BUS / freq - 0.5f;
  PDB0_SC = 0;
  PDB0_IDLY = 1;
  PDB0_MOD = t;
  PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
  PDB0_CH0C1 = 0x0101;

  return (float)F_BUS / t;
}

float setAudioSampleFreq(float freq) {
  int f;
  f = setDACFreq(freq);
  return f;
}

void setAudioOff(void) {
#if !VGA
  if (!(SIM_SCGC6 & SIM_SCGC6_PDB)) return;
  PDB0_SC = 0;
#endif
  AudioNoInterrupts();
  NVIC_DISABLE_IRQ(IRQ_USBOTG);
  //NVIC_DISABLE_IRQ(IRQ_USBHS);
}

void setAudioOn(void) {
#if !VGA
  if (!(SIM_SCGC6 & SIM_SCGC6_PDB)) return;
  PDB0_SC = PDB_CONFIG | PDB_SC_LDOK;
  PDB0_SC = PDB_CONFIG | PDB_SC_SWTRIG;
#endif
  AudioInterrupts();
  NVIC_ENABLE_IRQ(IRQ_USBOTG);
  //NVIC_ENABLE_IRQ(IRQ_USBHS);
}

void listInterrupts() {

#if defined(__MK66FX1M0__)
const char isrName[][24] = {
	//NMI
	"","reset handler", "nmi", "hard fault", "memmanage fault", "bus fault", "usage fault",
	"unknown fault","unknown fault","unknown fault", "unknown fault",
	"svcall", "debugmonitor", "unknown fault", "pendablesrvreq", "systick timer",
	//ISR
	"dma_ch0","dma_ch1","dma_ch2","dma_ch3","dma_ch4","dma_ch5","dma_ch6","dma_ch7",
	"dma_ch8","dma_ch9","dma_ch10","dma_ch11","dma_ch12","dma_ch13","dma_ch14","dma_ch15",
	"dma_error","mcm","flash_cmd","flash_error","low_voltage","wakeup","watchdog",
	"randnum","i2c0","i2c1","spi0","spi1","i2s0_tx","i2s0_rx","unused 46","uart0_status",
	"uart0_error","uart1_status","uart1_error","uart2_status","uart2_error","uart3_status",
	"uart3_error","adc0","cmp0","cmp1","ftm0","ftm1","ftm2","cmt","rtc_alarm","rtc_seconds",
	"pit0","pit1","pit2","pit3","pdb","usb","usb_charge","unused","dac0","mcg_isr","lptmr",
	"porta","portb","portc","portd","porte","software (audio)","spi2","uart4_status","uart4_error",
	"unused","unused","cmp2","ftm3","dac1","adc1","i2c2","can0_message","can0_bus_off",
	"can0_error","can0_tx_warn","can0_rx_warn","can0_wakeup","sdhc","enet_timer","enet_tx",
	"enet_rx","enet_error","lpuart0_status","tsi0","tpm1","tpm2","usbhs_phy","i2c3","cmp3",
	"usbhs","can1_message","can1_bus_off","can1_error","can1_tx_warn","can1_rx_warn","can1_wakeup"};
#endif

  //unsigned adrFaultNMI = (unsigned)_VectorsRam[3];
  unsigned adrUnusedInt = (unsigned)_VectorsRam[IRQ_FTFL_COLLISION + 16];//IRQ_FTFL_COLLISION is normally unused
  unsigned adr;

  Serial.println("Interrupts in use:");

#if 1
  Serial.println("NMI (non-maskable):");
  for (unsigned i = 1; i < 16; i++) {
    adr = (unsigned)_VectorsRam[i];
    if (adr != adrUnusedInt) {
      Serial.print(i);
      Serial.print(": \t");
      Serial.print(isrName[i]);
      Serial.print("\t0x");
      Serial.print(adr, HEX);
      Serial.println();
    }

  }
#endif

  Serial.println("IRQ:");
  for (unsigned i = 0; i < NVIC_NUM_INTERRUPTS; i++) {
    adr = (unsigned)_VectorsRam[i + 16];
    if (adr != adrUnusedInt) {
      Serial.print(i);
      Serial.print(": ");
      Serial.print("\tPriority:");
      Serial.print(NVIC_GET_PRIORITY(i));
      Serial.print("\t0x");
      Serial.print(adr, HEX);
	  if (adr < 0x10000000) Serial.print("\t");
	  Serial.print("\t");
	  Serial.print(isrName[i + 16]);
      if (NVIC_IS_ENABLED(i)) Serial.print("\t is enabled");
      Serial.println();
    }
  }
  Serial.println();
}