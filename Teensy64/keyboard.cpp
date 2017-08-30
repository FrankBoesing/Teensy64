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
#include "Teensy64.h"
#include "keyboard.h"
#include "cpu.h"
#include "usb_USBHost.h"

USBHost myusb;
USBHub hub1;
USBHub hub2;
USBHub hub3;

KeyboardController keyboard;

const char * hotkeys[] = {"\x45RUN", //F12
								 "\x44LOAD\"$\"\rLIST\r",  //F11
								 "\x43LOAD\"$\",8", //F10
								 "\x42LOAD\"*\",8", //F9
								 NULL };

char * _sendString = NULL;

void sendKey(char key) {
	
	while(true) {
		noInterrupts();
		if (cpu.RAM[198] == 0) break;
		interrupts();
		delay(1);
		//asm volatile("wfi");
	}
	
	cpu.RAM[631] = key;
	cpu.RAM[198] = 1;
	interrupts();
	return;
	
}

void do_sendString() {
	if (_sendString == NULL) return;
	char ch = *_sendString++;
	if (ch != 0)
		sendKey(ch);
	else
		_sendString = NULL;
}

void sendString(const char * p) {
	_sendString = (char *) p;
	
	Serial.print("Send String:");	
	Serial.println(p);
}

static int hotkey(char ch) {
	if (_sendString != NULL ) return 1;
	
	unsigned i = 0;
	while (hotkeys[i] != NULL) {
		if (*hotkeys[i]==ch) {
			sendString(hotkeys[i] + 1);
			return 1;
		}
		i++;
	}
	return 0;	
}

/*******************************************************************************************************************/
/*******************************************************************************************************************/
/*******************************************************************************************************************/

//Array values are keyboard-values returned from USB
const uint8_t ktab[8][8] = {
  {0x2a, 0x28, 0x4f, 0x40, 0x3a, 0x3c, 0x3e, 0x51}, //DEL, Return, Cursor Right, F7, F1, F3, F5, Cursor Down
  {0x20, 0x1a, 0x04, 0x21, 0x1d, 0x16, 0x08, 0x00}, //3, W, A, 4, Z, S, E, LeftShift
  {0x22, 0x15, 0x07, 0x23, 0x06, 0x09, 0x17, 0x1b}, //5, R, D, 6, C, F, T, X
  {0x24, 0x1c, 0x0a, 0x25, 0x05, 0x0B, 0x18, 0x19}, //7, Y, G, 8, B, H, U, V
  {0x26, 0x0c, 0x0D, 0x27, 0x10, 0x0E, 0x12, 0x11}, //9, I, J, 0, M, K, O, N
  {0x57, 0x13, 0x0F, 0x56, 0x37, 0x33, 0x2f, 0x36}, //+(Keypad), P, L, -(Keypad), ",", ":", "@", ","
  {0x49, 0x55, 0x34, 0x4A, 0x00, 0x32, 0x4b, 0x54}, //Pound(ins), *(Keypad), ";", HOME (Pos1), =, UP Arrow (Bild hoch), /(Keypad)
  {0x1e, 0x4e, 0x00, 0x1f, 0x2c, 0x00, 0x14, 0x29} //1,LEFT ARROW(Bild runter) , CTRL, 2, Space, Commodore, Q, RUN/STOP(ESC)
};


struct {
  union {
    uint32_t kv;
    struct {
      uint8_t ke,   //Extratasten SHIFT, STRG, ALT...
              kdummy,
              k,    //Erste gedrückte Taste
              k2;   //Zweite gedrückte Taste
    };
  };
  uint32_t lastkv;
  uint8_t shiftLock;
} kbdData = {0, 0, 0};

void keyboardmatrix(void * keys) { //Interrupt
  kbdData.kv = *(uint32_t*) keys;//Use only the first 4 bytes


  if (kbdData.kv != kbdData.lastkv) {
    
	kbdData.lastkv = kbdData.kv;
	if (!kbdData.kv ) return;
	if (hotkey(kbdData.k)) return;
	
    //Serial.printf("0x%x 0x%x\n", kbdData.ke, kbdData.k);
   
    //todo : use a jumptable (?)
	
    //Special Keys
	//RESET
	if (kbdData.ke == 0x05 && kbdData.k == 0x4c) {
		//resetExternal();
		resetMachine();
	}
	
    else if (kbdData.k == 0x46) { //RESTORE - "Druck"
      kbdData.k = kbdData.k2;
      kbdData.k2 = 0;
      cpu_nmi();	  
	  return;
    }
    else if (kbdData.k2 == 0x46) { //RESTORE - "Druck"
      kbdData.k2 = 0;
      cpu_nmi();
	  return;
    }

    //Shift Lock
    if ( kbdData.k == 0x39 ) {
      kbdData.kv = 0;
      kbdData.shiftLock = ~kbdData.shiftLock;
      if (kbdData.shiftLock) {
        Serial.println("ShiftLock: ON");
      } else {
        Serial.println("ShiftLock: OFF");
      }
	  return;
    }
    if (kbdData.shiftLock) kbdData.ke |= 0x02; //Apply shift-lock by pressing left shift

    //Sondertasten
    //Cursor -> kein Shift
    if ( (kbdData.k == 0x4f) || (kbdData.k == 0x51) ) {
      kbdData.ke &= ~0x22;  //Shift entfernen
	  return;
    }
    //Cursor Links => Shift und Cursor Rechts
    else if ( kbdData.k == 0x50 ) {
      kbdData.ke |= 0x20;   //Shift Rechts
      kbdData.k = 0x4f;   //Cursor Rechts
	  return;
    }
    //Cursor Hoch => Shift und Cursor Runter
    else if ( kbdData.k == 0x52 ) {
      kbdData.ke |= 0x20;
      kbdData.k = 0x51;   //Cursor runter
	  return;
    }
	//F2 => SHIFT + F1, F4 => SHIFT + F3, F6 => SHIFT + F5, F8 => SHIFT + F7
	else if( kbdData.k == 0x3b || kbdData.k == 0x3d || kbdData.k == 0x3f || kbdData.k == 0x41 ) { 
  	  kbdData.ke |= 0x20;
	  kbdData.k -= 1;
	  return;
	}

  }


}

inline __attribute__((always_inline))
uint8_t joystick1() {  //Port 1 is on CIA1, PORT B
  return JOYSTICK1() & ~(cpu.cia1.R[0x01] & cpu.cia1.R[0x03]);
}


uint8_t joystick2() { //Port 2 is on CIA1, PORT A  
  return ~JOYSTICK2 & cpu.cia1.R[0x00] & cpu.cia1.R[0x02];
}

void initJoysticks() {
  pinMode(PIN_JOY1_1, INPUT_PULLUP);
  pinMode(PIN_JOY1_2, INPUT_PULLUP);
  pinMode(PIN_JOY1_3, INPUT_PULLUP);
  pinMode(PIN_JOY1_4, INPUT_PULLUP);
  pinMode(PIN_JOY1_BTN, INPUT_PULLUP);

  pinMode(PIN_JOY2_1, INPUT_PULLUP);
  pinMode(PIN_JOY2_2, INPUT_PULLUP);
  pinMode(PIN_JOY2_3, INPUT_PULLUP);
  pinMode(PIN_JOY2_4, INPUT_PULLUP);
  pinMode(PIN_JOY2_BTN, INPUT_PULLUP);
}

void initKeyboard() {
	keyboard.attachC64(keyboardmatrix);
}


uint8_t keypress() {

  uint8_t v;
/*
  //Joystick 1
  v |= ((~GPIOB_PDIR >> 16) & 0x0f); //joystick hoch, runter, links, rechts
  v |= ((~GPIOE_PDIR >> 26) & 0x01) << 4; //Feuer
  v &= ~(cpu.cia1.R[0x01] & cpu.cia1.R[0x03]);
*/  
  v = joystick1();
  
  if (!kbdData.kv) return ~v; //Keine Taste gedrückt

 
  uint8_t porta = cpu.cia1.R[0x00] ; //CIA1, PORTA
  if (porta == 0) return 0;

  uint8_t ke = kbdData.ke;  //Extratasten SHIFT, STRG, ALT...
  uint8_t k = kbdData.k;    //Erste gedrückte Taste
  uint8_t k2 = kbdData.k2;  //Zweite gedrückte Taste

 // Serial.printf("0x%x - 0x%x - 0x%x %d\n", ke, k, k2, porta);

  const uint8_t * p;
  const uint8_t * kp;

  porta = ~porta;
  if (porta & 0x01) {
    if (k) {
      kp = &ktab[0][0];
      p = (uint8_t*)memchr(kp, k, 8);
      if (p) v |= 1 << (p - kp);

      if (k2) {
        p = (uint8_t*)memchr(kp, k2, 8);
        if (p) v |= 1 << (p - kp);
      }
    }
  }
  if (porta & 0x02) {
    if ((ke & 0x02) > 0) {
      v |= 0x80; //Left Shift or Shift Lock ?
    }
    if (k) {
      kp = &ktab[1][0];
      p = (uint8_t*)memchr(kp, k, 8);
      if (p) v |= 1 << (p - kp);

      if (k2) {
        p = (uint8_t*)memchr(kp, k2, 8);
        if (p) v |= 1 << (p - kp);
      }
    }
  }
  if (porta & 0x04) {
    if (k) {
      kp = &ktab[2][0];
      p = (uint8_t*)memchr(kp, k, 8);
      if (p) v |= 1 << (p - kp);

      if (k2) {
        p = (uint8_t*)memchr(kp, k2, 8);
        if (p) v |= 1 << (p - kp);
      }
    }
  }
  if (porta & 0x08) {
    if (k) {
      kp = &ktab[3][0];
      p = (uint8_t*)memchr(kp, k, 8);
      if (p) v |= 1 << (p - kp);

      if (k2) {
        p = (uint8_t*)memchr(kp, k2, 8);
        if (p) v |= 1 << (p - kp);
      }
    }
  }
  if (porta & 0x10) {
    if (k) {
      kp = &ktab[4][0];
      p = (uint8_t*)memchr(kp, k, 8);
      if (p) v |= 1 << (p - kp);

      if (k2) {
        p = (uint8_t*)memchr(kp, k2, 8);
        if (p) v |= 1 << (p - kp);
      }
    }
  }
  if (porta & 0x20) {
    if (k) {
      kp = &ktab[5][0];
      p = (uint8_t*)memchr(kp, k, 8);
      if (p) v |= 1 << (p - kp);

      if (k2) {
        p = (uint8_t*)memchr(kp, k2, 8);
        if (p) v |= 1 << (p - kp);
      }
    }
  }
  if (porta & 0x40) {
    if ((ke & 0x20) > 0) {
      v |= 0x10; //Right Shift ?
    }
    if (k) {
      kp = &ktab[6][0];
      p = (uint8_t*)memchr(kp, k, 8);
      if (p) v |= 1 << (p - kp);

      if (k2) {
        p = (uint8_t*)memchr(kp, k2, 8);
        if (p) v |= 1 << (p - kp);
      }
    }
  }
  if (porta & 0x80) {
    if ((ke & 0x11) > 0) {
      v |= 0x04; //Ctrl ?
    }
    if ((ke & 0x88) > 0) {
      v |= 0x20; //Commodore (WIN) ?
    }
    if (k) {
      kp = &ktab[7][0];
      p = (uint8_t*)memchr(kp, k, 8);
      if (p) v |= 1 << (p - kp);

      if (k2) {
        p = (uint8_t*)memchr(kp, k2, 8);
        if (p) v |= 1 << (p - kp);
      }
    }
  }

  return ~v;//Return PortB (CIA1)

}
