/**
BSD 3-Clause License

Copyright (c) 2025, bitaggregat GmbH

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include <Arduino.h>
#include "songs/in_der_weihnachtsbaeckerei.h"

#define LED_1 5
#define LED_3 14
#define LED_4 1
#define LED_5 20
#define LED_6 21

#define RGB_LED_RED 9
#define RGB_LED_GREEN 6
#define RGB_LED_BLUE 10

#define LED1_MASK 0x01
#define LED3_MASK 0x02
#define LED4_MASK 0x04
#define LED5_MASK 0x08
#define LED6_MASK 0x10

#define BUTTON 15
#define BUZZER 2


/* Hardware abstraction functions */
void enable_led(uint8_t led_pin, bool state);
void enable_rgb_led(uint8_t red, uint8_t green, uint8_t blue);

/* Christmas Tree behaviour functions */
void pulsate_gold();
void blink_red_leds();
void play_jingle();

/* Global variables */
//********************************************************************************************
// Timing Globals
long _previousMillis1 = 0;     // stores timestamp for thread 1
long _previousMillis2 = 0;     // stores timestamp for thread 2
long _previousMillis3 = 0;     // stores timestamp for thread 3
long _intervall1 = 10;         // time interval to start thread 1 in millis
const long _intervall2 = 6;    // time interval to start thread 2 in millis
const long _intervall3 = 300;  // time interval to start thread 3 in millis
//********************************************************************************************
//********************************************************************************************
// Sound Globals
bool _do_play_jingle = false;
uint16_t thisNote = 0;
//********************************************************************************************
//********************************************************************************************
// Button Globals
int buttonState;            // the current reading from the input pin
int lastButtonState = LOW;  // the previous reading from the input pin

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastDebounceTime = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 50;    // the debounce time; increase if the output flickers
//********************************************************************************************

/* Helper function to control leds */
void enable_led(uint8_t led_pin, bool state) {
  switch (led_pin) {
    case LED_1:
      digitalWrite(LED_1, !state);
      break;
    case LED_3:
      digitalWrite(LED_3, !state);
      break;
    case LED_4:
      digitalWrite(LED_4, !state);
      break;
    /* LED 5 and 6 need to be handled differently
    *  They are not mapped by arduino like the other pins.
    *  Reason: The pins are reserved for (optional) external clock 
    *  (See atmega328pb documentation)
    */
    case LED_5:
      if (state){
          PORTB |= (1 << 6);
      } else {
          PORTB &= ~(1 << 6);
      };
      break;
    case LED_6:
      if (state){
          PORTB |= (1 << 7);
      } else {
          PORTB &= ~(1 << 7);
      };
      break;
  }
}

/* Helper function to control rgb leds */
void enable_rgb_led(uint8_t red, uint8_t green, uint8_t blue) {
  analogWrite(RGB_LED_RED, red);
  analogWrite(RGB_LED_GREEN, green);
  analogWrite(RGB_LED_BLUE, blue);
}

/* Thread 1 definition */
void play_jingle() {
  const unsigned int half_seconds_in_millis = 500;
  const double user_defined_factor = 1.6;
  int noteDuration = half_seconds_in_millis / song[thisNote].duration;
  int pauseBetweenNotes = noteDuration * user_defined_factor;

  tone(BUZZER, song[thisNote].frequency, noteDuration);

  _intervall1 = pauseBetweenNotes;

  thisNote++;

  if (thisNote >= (sizeof(song) / sizeof(song[0]))) {
    thisNote = 0;
    _do_play_jingle = false;
  }
}

/* Thread 2 definition */
void pulsate_gold() {
  static int16_t brightness = 0;
  static int8_t fade_amount = 3;

  const uint8_t GOLD_R = 255;
  const uint8_t GOLD_G = 100;
  const uint8_t GOLD_B = 0;

  uint8_t red = (GOLD_R * brightness) / 255;
  uint8_t green = (GOLD_G * brightness) / 255;
  uint8_t blue = (GOLD_B * brightness) / 255;

  enable_rgb_led(red, green, blue);

  brightness += fade_amount;

  if (brightness <= 0) {
    brightness = 0;
    fade_amount = -fade_amount;
  } else if (brightness >= 255) {
    brightness = 255;
    fade_amount = -fade_amount;
  }
}

/* Thread 3 definition */
void blink_red_leds() {

  static const uint8_t pattern[] = {
    25, 26, 20, 28, 26, 25,
    17, 26, 28, 28, 10, 25, 
    25, 26, 20, 28, 26, 25,
    25, 10, 28, 28, 18, 25,
  };

  static size_t i = 0;
  uint8_t r = pattern[i % sizeof(pattern)];

  enable_led(LED_1, (r & LED1_MASK) ? 1 : 0);
  enable_led(LED_3, (r & LED3_MASK) ? 1 : 0);
  enable_led(LED_4, (r & LED4_MASK) ? 1 : 0);
  enable_led(LED_5, (r & LED5_MASK) ? 1 : 0);
  enable_led(LED_6, (r & LED6_MASK) ? 1 : 0);
  i++;
}

/* Thread 4 defintion */
void handle_button() {
  // Source: https://docs.arduino.cc/built-in-examples/digital/Debounce/
  // read the state of the switch into a local variable:
  int reading = digitalRead(BUTTON);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    // reset the debouncing timer
    lastDebounceTime = millis();
  }

  if ((millis() - lastDebounceTime) > debounceDelay) {
    // whatever the reading is at, it's been there for longer than the debounce
    // delay, so take it as the actual current state:

    // if the button state has changed:
    if (reading != buttonState) {
      buttonState = reading;

      // only toggle the song state if the new button state is HIGH
      if (buttonState == HIGH) {
        _do_play_jingle = !_do_play_jingle;
      }
    }
  }

  // save the reading. Next time through the loop, it'll be the lastButtonState:
  lastButtonState = reading;
}

/* Setup in/out pins */
void setup() {
  pinMode(LED_1, OUTPUT);
  pinMode(LED_3, OUTPUT);
  pinMode(LED_4, OUTPUT);

  // Init LED 5 and 6
  DDRB |= (1 << 6);
  DDRB |= (1 << 7);
  PORTB |= (1 << 6);
  PORTB |= (1 << 7);

  pinMode(RGB_LED_RED, OUTPUT);
  pinMode(RGB_LED_GREEN, OUTPUT);
  pinMode(RGB_LED_BLUE, OUTPUT);

  pinMode(BUTTON, INPUT);
  pinMode(BUTTON, INPUT_PULLUP);
}

/* Arduino main */
void loop() {
  long currentMillis = millis();

  /* Simple scheduler for 3 threads, switches by time intervalls */
  if (_do_play_jingle && (currentMillis - _previousMillis1 >= _intervall1)) {
    _previousMillis1 = currentMillis;
    /* Run thread 1 */
    play_jingle();
  }

  if (currentMillis - _previousMillis2 >= _intervall2) {
    _previousMillis2 = currentMillis;
    /* Run thread 2 */
    pulsate_gold();
  }

  if (currentMillis - _previousMillis3 >= _intervall3) {
    _previousMillis3 = currentMillis;
    /* Run thread 3 */
    blink_red_leds();
  }

  /* Run thread 4 */
  handle_button();
}
