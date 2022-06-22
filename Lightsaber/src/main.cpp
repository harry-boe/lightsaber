

#include <Arduino.h>
#include <SPIFFS.h>
#include <EasyButton.h>
#include <Adafruit_NeoPixel.h>


#include "AudioTools.h"


//
// Never use a JsonDocument to store the configuration!
// A JsonDocument is *not* a permanent storage; it's only a temporary storage
// used during the serialization phase. See:
// https://arduinojson.org/v6/faq/why-must-i-create-a-separate-config-object/
struct Config {
  char hostname[64];
  int port;
  char color[6];
  int brightness;
};

const char *cfgfile = "/config.json";  // <- SD library uses 8.3 filenames
Config cfg;                          // <- global configuration object


File audioFile;
I2SStream i2s;                        // I2S stream 
WAVDecoder decoder(i2s);              // decode wav to pcm and send it to I2S
EncodedAudioStream out(i2s, decoder); // Decoder stream
StreamCopy copier(out, audioFile); // copy url to decoder

// Arduino pin where the buttons are connected to.
#define BUTTON_PIN 10
// Instance of the button.
EasyButton button(BUTTON_PIN);
boolean bladeIsOn = false;


// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
// Which pin on the Arduino is connected to the NeoPixels?
#define PIN        9 // On Trinket or Gemma, suggest changing this to 1
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 50  // Popular NeoPixel ring size
#define DURATION 1000 // Time (in milliseconds) to animate lightstrip

Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);

void playFile(const char * filename) {

  // close the old file
  audioFile.close();
  // load the new one
  audioFile = SPIFFS.open(filename, "r");

  if (audioFile.available()) {
    // setup I2S based on sampling rate provided by decoder
//    out2dec.setNotifyAudioChange(i2s);
    Serial.print("Start playing ");
    Serial.println(filename);
    i2s.begin();
  } else {
    Serial.println("failed to read sound file");
  }
}


int prev;
long startTime =0;

void power(const char* sound, unsigned long duration, boolean reverse) {

    Serial.printf("Start power(%s, %ul, %i\n", sound, duration, reverse);

    unsigned long elapsed;
    float fraction;
    float threshold;
    int num;

    if (reverse)
        prev = NUMPIXELS;
    else
        prev = 0;

    if (startTime == 0) {               // nothing Playing ?
      startTime = millis();             // Save audio start time
      playFile(sound);                  // play sound
    }

    while (true) {
      elapsed = millis() - startTime;   // Time spent playing sound
      Serial.printf("elapsed %u starttime %u \n", elapsed, startTime);
      if (elapsed > duration)           // Past sound duration?
            break;                      // Stop animating

      fraction = (float)elapsed / duration;            // Animation time, 0.0 to 1.0
      if (reverse) {
        fraction = 1.0 - fraction;            // 1.0 to 0.0 if reverse
      };
      fraction = pow(fraction, 0.5);       // Apply nonlinear curve
      threshold = int(NUMPIXELS * fraction + 0.5);
      num = threshold - prev;                   // Number of pixels to light on this pass

      if (num == prev)                  // no changes -> break
          break;

      if (!reverse) {
        for (int i=0; i<num; i++) {
          pixels.setPixelColor(i, pixels.Color(0, 0, 20));
        }
      } else {
        int poff =  NUMPIXELS+num;
        for (int i=NUMPIXELS; i>=poff; i--) {
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        }
      } 
      pixels.show();   // Send the updated pixel colors to the hardware.
      copier.copy();
    }

    Serial.printf("end animation  %u starttime %u \n", elapsed, startTime);
    while (copier.available()) {  //  # Wait until audio done
         copier.copy(); 
    }      

    Serial.print("End Power");
    if (reverse) {
      pixels.clear();      // if there are any remaining pixels due to POV function
      pixels.show();   // Send the updated pixel colors to the hardware.
    }
//    Serial.printf("power done (%s, %ul, %i\n", sound, duration, reverse);
    startTime = 0;                    // reset startTime
}


boolean isOn = false;

// Callback function to be called when the button is pressed.
void onPressed()
{
    Serial.printf("Button pressed\n");

    if (!isOn) {
//      Serial.printf("Turn on Blade '%c' %s\n", config.color[0], config.color );
      Serial.println("turn on blade ..");
      power("/on.wav", 980, false);
      Serial.print("turn on blade ..");
      Serial.println("done ..");
      isOn = true;     
    }
}

// Callback function to be called when the button is pressed.
void onPressedForDuration()
{
  Serial.println("Button long pressed");

  if (isOn) {
    Serial.println("Turn off Blade ..");
    power("/off.wav", 930, true);
    Serial.print("Turn off Blade ..");
    Serial.println("done ..");
    isOn = false;
  }

}



void setup() {
// Init Serial output
  Serial.begin(115200);
  AudioLogger::instance().begin(Serial, AudioLogger::Info);  

// setup i2s
  auto config = i2s.defaultConfig(TX_MODE);
  config.pin_ws = 25;
  config.pin_bck = 26;
  config.pin_data = 27;
  config.sample_rate = 16000; 
  config.bits_per_sample = 32;
  config.channels = 1;
  i2s.begin(config);

// mount SPIFF
  SPIFFS.begin();

// Initialize the button.
  button.begin();
  button.onPressed(onPressed);
  button.onPressedFor(1000, onPressedForDuration);


 // init neopixel
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)


}


void loop() {
  // put your main code here, to run repeatedly:

    // Continuously read the status of the button.
  button.read();

  copier.copy();

  // no file loaded play random hum sounds
  if (isOn && !copier.available()) {
    playFile("/Hum-4.wav");
  }

}