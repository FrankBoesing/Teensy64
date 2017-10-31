/* USB EHCI Host for Teensy 3.6
 * Copyright 2017 Paul Stoffregen (paul@pjrc.com)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
 
//Modified for Teensy64 (c) F.Bösing

#include "keyboard_usb.h"


extern "C" {
void __keyboardmatrixEmptyFunction(void *keys) {} ;//FB
}

static void ((*keyboardmatrixFunc)(void *keys)) = __keyboardmatrixEmptyFunction;

void c64USBKeyboard::init()
{
  contribute_Pipes(mypipes, sizeof(mypipes) / sizeof(Pipe_t));
  contribute_Transfers(mytransfers, sizeof(mytransfers) / sizeof(Transfer_t));
  driver_ready_for_device(this);
}

void  c64USBKeyboard::attachC64(void (*keyboardmatrix)(void *keys)) {//FB
      keyboardmatrixFunc = keyboardmatrixFunction = keyboardmatrix;
    };
	
bool c64USBKeyboard::claim(Device_t *dev, int type, const uint8_t *descriptors, uint32_t len)
{
  println("c64USBKeyboard claim this=", (uint32_t)this, HEX);

  // only claim at interface level
  if (type != 1) return false;
  if (len < 9 + 9 + 7) return false;

  uint32_t numendpoint = descriptors[4];
  if (numendpoint < 1) return false;
  if (descriptors[5] != 3) return false; // bInterfaceClass, 3 = HID
  if (descriptors[6] != 1) return false; // bInterfaceSubClass, 1 = Boot Device
  if (descriptors[7] != 1) return false; // bInterfaceProtocol, 1 = Keyboard
  if (descriptors[9] != 9) return false;
  if (descriptors[10] != 33) return false; // HID descriptor (ignored, Boot Protocol)
  if (descriptors[18] != 7) return false;
  if (descriptors[19] != 5) return false; // endpoint descriptor
  uint32_t endpoint = descriptors[20];
  if ((endpoint & 0xF0) != 0x80) return false; // must be IN direction
  endpoint &= 0x0F;
  if (endpoint == 0) return false;
  if (descriptors[21] != 3) return false; // must be interrupt type
  uint32_t size = descriptors[22] | (descriptors[23] << 8);
  if (size != 8 && size != 64) { //Size==64 by "menno" - thank you.
    return false; // must be 8 bytes for Keyboard Boot Protocol
  }
  //uint32_t interval = descriptors[24];
  //datapipe = new_Pipe(dev, 3, endpoint, 1, 8, interval);
  datapipe = new_Pipe(dev, 3, endpoint, 1, 8, 8);
  datapipe->callback_function = callback;
  queue_Data_Transfer(datapipe, report, 8, this);
  mk_setup(setup, 0x21, 10, 0, 0, 0); // 10=SET_IDLE
  queue_Control_Transfer(dev, &setup, NULL, this);
  keyboardmatrixFunction = keyboardmatrixFunc;//FB
  Serial.println("USB Keyboard connected.");

  return true;
}

void c64USBKeyboard::control(const Transfer_t *transfer)
{
}

void c64USBKeyboard::callback(const Transfer_t *transfer)
{
  if (transfer->driver) {
    ((c64USBKeyboard *)(transfer->driver))->new_data(transfer);
  }
}

void c64USBKeyboard::disconnect()
{
  // TODO: free resources
  Serial.println("USB Keyboard disconnected.");
}


// Arduino defined this static weak symbol callback, and their
// examples use it as the only way to detect new key presses,
// so unfortunate as static weak callbacks are, it probably
// needs to be supported for compatibility
extern "C" {
  void __c64USBKeyboardEmptyCallback() { }  
}

void c64USBKeyboard::new_data(const Transfer_t *transfer)
{
  //Serial.println("Keypress");
  keyboardmatrixFunction((void*) transfer->buffer); //FB

  queue_Data_Transfer(datapipe, report, 8, this);

    // See if we have any outstanding leds to update
    if (update_leds_) {
      updateLEDS();
    }
  
}


void c64USBKeyboard::numLock(bool f) {
  if (leds_.numLock != f) {
    leds_.numLock = f;
    updateLEDS();
  }
}

void c64USBKeyboard::capsLock(bool f) {
  if (leds_.capsLock != f) {
    leds_.capsLock = f;
    updateLEDS();
  }
}

void c64USBKeyboard::scrollLock(bool f) {
  if (leds_.scrollLock != f) {
    leds_.scrollLock = f;
    updateLEDS();
  }
}

void c64USBKeyboard::LEDS(uint8_t leds) {
  println("Keyboard setLEDS ", leds, HEX);
  leds_.byte = leds;
  updateLEDS();
}

void c64USBKeyboard::updateLEDS() {
  println("KBD: Update LEDS", leds_.byte, HEX);
  // Now lets tell keyboard new state.
  static uint8_t keyboard_keys_report[1] = {0};
  setup_t keys_setup;
  keyboard_keys_report[0] = leds_.byte;
  queue_Data_Transfer(datapipe, report, 8, this);
  mk_setup(keys_setup, 0x21, 9, 0x200, 0, sizeof(keyboard_keys_report)); // hopefully this sets leds
  queue_Control_Transfer(device, &keys_setup, keyboard_keys_report, this);

  update_leds_ = false;
}
