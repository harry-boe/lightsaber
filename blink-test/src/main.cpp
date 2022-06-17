#include <Arduino.h>
#include <SPIFFS.h>

#include "blink.h"


const int buttonPin = 26;    // the number of the pushbutton pin

// Variables will change:
int buttonState;             // the current reading from the input pin
int lastButtonState = LOW;   // the previous reading from the input pin
int buttonCount = 0;

// the following variables are unsigned longs because the time, measured in
// milliseconds, will quickly become a bigger number than can be stored in an int.
unsigned long lastStateChange = 0;  // the last time the output pin was toggled
unsigned long now = 0;  // the last time the output pin was toggled
unsigned long debounceDelay = 100;    // the debounce time; increase if the output flickers
unsigned long sequenceDelay = 500;  // end button count sequence after no additional actibity


void setup() {
  // put your setup code here, to run once:
  // initialize digital pin LED_BUILTIN as an output.
  pinMode(LED_BUILTIN, OUTPUT);
  // initialize digital pin button_pin as an input.
  pinMode(buttonPin, INPUT);

  Serial.begin(9600);
  Serial.println("Setup()");

}



void loop() {

  // read the state of the switch into a local variable:
  int reading = digitalRead(buttonPin);

  // check to see if you just pressed the button
  // (i.e. the input went from LOW to HIGH), and you've waited long enough
  // since the last press to ignore any noise:

  // If the switch changed, due to noise or pressing:
  if (reading != lastButtonState) {
    now = millis();

    if ((now - lastStateChange) > debounceDelay) {
      // whatever the reading is at, it's been there for longer than the debounce
      // delay, so take it as the actual current state:

      // if the button state has changed:
      if (reading != buttonState) {
        Serial.printf("%u: got button event %d != %d \n", now - lastStateChange, reading, buttonState);
        buttonState = reading;

        if (buttonState == LOW) {
          Serial.printf("%u: got a button release #%d \n", now - lastStateChange, buttonCount);
          buttonCount++;
        } else {
          Serial.printf("%u: got a button press #%d \n", now - lastStateChange, buttonCount);
        }
      }
    } 

    // save the reading. Next time through the loop, it'll be the lastButtonState:
    lastButtonState = reading; 
    // reset the debouncing timer
    lastStateChange = now;
  }

  /*

  // if there is no state change for some time check for actions
  if (buttonCount > 0) {
    if ((now - lastStateChange) > sequenceDelay) {
      if (buttonState == LOW) {
        Serial.printf("%ul: Long button press \n", now);
      } else {
        Serial.printf("%ul: Reset button count \n", now);
      }
      buttonCount = 0; 
    }
  } 
  */

}