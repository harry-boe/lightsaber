/*
  Name:    Interrupts.ino
  Created:  8/11/2019 11:45:52 AM
  Author: José Gabriel Companioni Benítez (https://github.com/elC0mpa)
  Description: Example to demostrate how to use interrupts in order to improve performance
*/

#include <Arduino.h>
#include <EasyButton.h>
#include <Adafruit_NeoPixel.h>

#include <SPIFFS.h>
#include <ArduinoJson.h>

#include <AudioTools.h>
#include "AudioCodecs/CodecMP3Helix.h"


// forward declarations
void callbackInit();
Stream* callbackStream();

// data
// const int chipSelect=PIN_CS;
AudioSourceCallback source(callbackStream,callbackInit);
I2SStream i2s;
MP3DecoderHelix decoder;
AudioPlayer player(source, i2s, decoder);
File audioFile;

const char *fileOnSound = "/SaberOn.mp3";

void callbackInit() {

  audioFile = SPIFFS.open(fileOnSound, "r");
  if (audioFile) {
    Serial.printf("Load Sound File %s\n", fileOnSound);    
  } else {
    Serial.println("failed to read sound file)");
  }
  // open the audio file 

}


Stream* callbackStream() {
  auto lastFile = audioFile;
  audioFile = audioFile.openNextFile();
  lastFile.close();
  return &audioFile;
}


void callbackPrintMetaData(MetaDataType type, const char* str, int len){
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
}



// Arduino pin where the buttons are connected to.
#define BUTTON_PIN 10
#define BAUDRATE 9600


// When setting up the NeoPixel library, we tell it how many pixels,
// and which pin to use to send signals. Note that for older NeoPixel
// strips you might need to change the third parameter -- see the
// strandtest example for more information on possible values.
// Which pin on the Arduino is connected to the NeoPixels?

#define PIN        9 // On Trinket or Gemma, suggest changing this to 1
// How many NeoPixels are attached to the Arduino?
#define NUMPIXELS 50 // Popular NeoPixel ring size
#define DELAYVAL 10 // Time (in milliseconds) to pause between pixels


Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);


// Our configuration structure.
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

const char *filename = "/config.json";  // <- SD library uses 8.3 filenames
Config config;                         // <- global configuration object


boolean bladeIsOn = false;
unsigned long lastEvent = millis();


// Callback function to be called when the button is pressed.
void onPressed()
{
  if (millis() - lastEvent > 5000) {
    Serial.printf("%u Button pressed\n", millis() - lastEvent);
    lastEvent = millis();
    if (!bladeIsOn) {
      Serial.printf("Turn on Blade '%c' %s\n", config.color[0], config.color );

      pixels.clear();

      // The first NeoPixel in a strand is #0, second is 1, all the way up
      // to the count of pixels minus one.
      for(int i=0; i<NUMPIXELS; i++) { // For each pixel...

        // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
        // Here we're using a moderately bright green color:
        switch (config.color[0]) {
          case 'r':
        pixels.setPixelColor(i, pixels.Color(20, 0, 0));
        break;
          case 'g':
        pixels.setPixelColor(i, pixels.Color(0, 20, 0));
        break;
          case 'b':
        pixels.setPixelColor(i, pixels.Color(0, 0, 20));
        break;
          default:
            pixels.setPixelColor(i, pixels.Color(20, 20, 20));
        }

        pixels.show();   // Send the updated pixel colors to the hardware.

        delay(DELAYVAL); // Pause before next pass through loop
      }
      bladeIsOn = true;
    }
  }
}


// Callback function to be called when the button is pressed.
void onPressedForDuration()
{
  Serial.println("Button long pressed");
  if (bladeIsOn) {
    Serial.println("Turn off Blade");
    
for(int i=NUMPIXELS; i>=0; i--) { // For each pixel...

        // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
        // Here we're using a moderately bright green color:
        pixels.setPixelColor(i, pixels.Color(0, 0, 0));
       
        pixels.show();   // Send the updated pixel colors to the hardware.
        delay(DELAYVAL); // Pause before next pass through loop
      }


    bladeIsOn = false;
  }

}

// Instance of the button.
EasyButton button(BUTTON_PIN);


void initConfig() {

 File file = SPIFFS.open(filename, "r");

  if (file) {

  // Serial.println(gif.readString());

  // Allocate a temporary JsonDocument
  // Don't forget to change the capacity to match your requirements.
  // Use https://arduinojson.org/v6/assistant to compute the capacity.
  StaticJsonDocument<512> doc;

  // Deserialize the JSON document
  DeserializationError error = deserializeJson(doc, file);
  if (error)
    Serial.println(F("Failed to read file, using default configuration"));

    // Copy values from the JsonDocument to the Config
    config.port = doc["port"] | 8080;
    strlcpy(config.hostname,                  // <- destination
            doc["hostname"] | "hbonet.ch",  // <- source
            sizeof(config.hostname));         // <- destination's capacity
    strlcpy(config.color,                  // <- destination
            doc["color"] | "red",  // <- source
            sizeof(config.color));         // <- destination's capacity
    config.brightness = doc["brightness"] | 20;

    Serial.println(config.hostname);
    Serial.println(config.color);
    Serial.printf("color %s\n", config.color);
    Serial.printf("brightness %d\n", config.brightness);

    file.close();

  } else {
    Serial.println("failed to read config file)");
  }
}




void setup()
{
  // Initialize Serial for debuging purposes.
  Serial.begin(BAUDRATE);

  Serial.println();
  Serial.println(">>> EasyButton multiple onSequence example <<<");

  SPIFFS.begin();
  initConfig();

  // Initialize the button.
  button.begin();

  button.onPressed(onPressed);
  button.onPressedFor(1000, onPressedForDuration);

  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)


  AudioLogger::instance().begin(Serial, AudioLogger::Info);

  // setup output
  auto cfg = i2s.defaultConfig(TX_MODE);
  i2s.begin(cfg);

  // setup player
  player.setMetadataCallback(callbackPrintMetaData);
  player.begin();

}




void loop()
{
  // Continuously read the status of the button.
  button.read();

  player.copy();

}