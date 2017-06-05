/* c64_boardtestv01 code is placed under the MIT license
 * Copyright (c) 2014,2015 Frank Bösing
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
 
#define PIN_JOY1_1       0
#define PIN_JOY1_2       1
#define PIN_JOY2_1       2
#define PIN_PA2_2        3
#define PIN_SERIAL_ATN   4
#define PIN_JOY2_BTN     5
#define PIN_JOY2_4       6
#define PIN_JOY2_2       7     
#define PIN_JOY2_3       8
#define PIN_PB2_3        9
#define PIN_PB2_4       10
#define PIN_PB2_6       11
#define PIN_PB2_7       12
#define PIN_PB2_5       13
#define SCK             14
#define PIN_PB2_0       15
#define PIN_CNT1        16
#define PIN_SP1         17
#define PIN_SP2         18
#define PIN_CNT2        19
#define TFT_DC          20
#define TFT_CS          21
#define PIN_PB2_1       22
#define PIN_PB2_2       23
#define PIN_JOY1_BTN    24
#define PIN_RESET       25
#define PIN_SERIAL_CLK  26
#define PIN_SERIAL_DATA 27
#define MOSI            28
#define PIN_JOY1_3      29
#define PIN_JOY1_4      30
#define PIN_JOY2_A1     A12
#define PIN_JOY2_A2     A13
#define PIN_JOY1_A1     A14
#define PIN_JOY1_A2     A15
#define PIN_PC2         35
#define PIN_FLAG2       36
#define TFT_TOUCH_INT   37
#define TFT_TOUCH_CS    38
#define MISO            39


void setup() {
  delay(1000);
  Serial.begin(9600);
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
  
  pinMode(PIN_RESET, OUTPUT_OPENDRAIN);  digitalWrite(PIN_RESET,1);
  pinMode(PIN_SERIAL_ATN, OUTPUT_OPENDRAIN);   digitalWrite(PIN_SERIAL_ATN,1); //ATN OUT (CIA2 PA3 OUT)
  pinMode(PIN_SERIAL_CLK, OUTPUT_OPENDRAIN);   digitalWrite(PIN_SERIAL_ATN,1); //CLK   (CIA2 PA6:IN PA4: OUT)
  pinMode(PIN_SERIAL_DATA, OUTPUT_OPENDRAIN);  digitalWrite(PIN_SERIAL_ATN,1); //DATA  (CIA2 PA7:IN PA5: OUT)   

  pinMode(PIN_PA2_2, INPUT_PULLUP);
  pinMode(PIN_PB2_0, INPUT_PULLUP);
  pinMode(PIN_PB2_1, INPUT_PULLUP);
  pinMode(PIN_PB2_2, INPUT_PULLUP);
  pinMode(PIN_PB2_3, INPUT_PULLUP);
  pinMode(PIN_PB2_4, INPUT_PULLUP);
  pinMode(PIN_PB2_5, INPUT_PULLUP);
  pinMode(PIN_PB2_6, INPUT_PULLUP);
  pinMode(PIN_PB2_7, INPUT_PULLUP);
  pinMode(PIN_CNT1, INPUT_PULLUP);
  pinMode(PIN_CNT2, INPUT_PULLUP);
  pinMode(PIN_SP1, INPUT_PULLUP);
  pinMode(PIN_SP2, INPUT_PULLUP);
  pinMode(PIN_FLAG2, INPUT_PULLUP);
  pinMode(PIN_PC2, INPUT_PULLUP);
  
}

void loop() {
  static int b = 1;
  int joy1 = digitalRead(PIN_JOY1_1) + digitalRead(PIN_JOY1_2) * 2 + digitalRead(PIN_JOY1_3) * 4 + digitalRead(PIN_JOY1_4) * 8 + digitalRead(PIN_JOY1_BTN) * 16;
  int joy2 = digitalRead(PIN_JOY2_1) + digitalRead(PIN_JOY2_2) * 2 + digitalRead(PIN_JOY2_3) * 4 + digitalRead(PIN_JOY2_4) * 8 + digitalRead(PIN_JOY2_BTN) * 16;
  Serial.printf("JOY1: 0x%x\n", joy1);
  Serial.printf("JOY1: 0x%x\n", joy2);

 
  Serial.printf("SERIAL_RESET: %d\n", digitalRead(PIN_RESET) );
  digitalWrite(PIN_SERIAL_ATN,b); Serial.printf("SERIAL_ATN: %d\n", digitalRead(PIN_SERIAL_ATN) );
  digitalWrite(PIN_SERIAL_CLK,b); Serial.printf("SERIAL_CLK: %d\n", digitalRead(PIN_SERIAL_CLK) );
  digitalWrite(PIN_SERIAL_DATA,b); Serial.printf("SERIAL_DATA: %d\n", digitalRead(PIN_SERIAL_DATA) );
  
  delay(300);
  Serial.println();
  b = (b + 1) & 1;
}
