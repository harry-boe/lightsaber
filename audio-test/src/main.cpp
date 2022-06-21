/**
 * @file streams-sd_mp3-metadata.ino
 * @author Phil Schatzmann
 * @brief read MP3 stream from a SD drive and output metadata and audio! 
 * The used mp3 file contains ID3 Metadata!
 * @date 2021-11-07
 * 
 * @copyright Copyright (c) 2021
 */

// install https://github.com/pschatzmann/arduino-libhelix.git
// install https://github.com/greiman/SdFat.git


#include <SPIFFS.h>
#include <ArduinoJson.h>
#include <EasyButton.h>
#include <Adafruit_NeoPixel.h>

#include "AudioTools.h"
#include "AudioCodecs/CodecMP3Helix.h"


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

const char *cfgfile = "/config.json";  // <- SD library uses 8.3 filenames
Config config;                          // <- global configuration object


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
#define NUMPIXELS 50 // Popular NeoPixel ring size
#define DELAYVAL 10 // Time (in milliseconds) to pause between pixels

boolean  doPixel = false;


Adafruit_NeoPixel pixels(NUMPIXELS, PIN, NEO_GRB + NEO_KHZ800);



File audioFile;
MetaDataPrint outMeta; // final output of metadata
I2SStream i2s; // I2S output
EncodedAudioStream out2dec(&i2s, new MP3DecoderHelix()); // Decoding stream
MultiOutput out(outMeta, out2dec);
StreamCopy copier(out, audioFile); // copy url to decoder

void playFile(const char * filename);

// random hum sounds
long randNumber;


void pixelLoop() {

  if (!doPixel) return;

  if ((millis() & DELAYVAL) ==0) {
    Serial.printf("[%u] do pixel Action Loop\n", millis());
  }
  
}


// Callback function to be called when the button is pressed.
void onPressed()
{
    Serial.printf("Button pressed\n");

    if (!bladeIsOn) {
      Serial.printf("Turn on Blade '%c' %s\n", config.color[0], config.color );


      playFile("/sw4lightsabre.mp3");
      copier.copy();

      pixels.clear();
      doPixel = true;

      // The first NeoPixel in a strand is #0, second is 1, all the way up
      // to the count of pixels minus one.
      for(int i=0; i<NUMPIXELS; i++) { // For each pixel...

        pixelLoop();

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
      doPixel = false;
      bladeIsOn = true;
    }
}

// Callback function to be called when the button is pressed.
void onPressedForDuration()
{
  Serial.println("Button long pressed");

  
  

  if (bladeIsOn) {
    Serial.println("Turn off Blade");

    doPixel = true;
    
    for(int i=NUMPIXELS; i>=0; i--) { // For each pixel...

          // pixels.Color() takes RGB values, from 0,0,0 up to 255,255,255
          // Here we're using a moderately bright green color:
          pixels.setPixelColor(i, pixels.Color(0, 0, 0));
        
          pixels.show();   // Send the updated pixel colors to the hardware.
          delay(DELAYVAL); // Pause before next pass through loop
    }
  }

  doPixel = false;
  bladeIsOn = false;
}


// callback for meta data
void printMetaData(MetaDataType type, const char* str, int len){
  Serial.print("==> ");
  Serial.print(toStr(type));
  Serial.print(": ");
  Serial.println(str);
}


void playFile(const char * filename) {

  // close the old file
  audioFile.close();
  // load the new one
  audioFile = SPIFFS.open(filename, "r");

  if (audioFile.available()) {
    // setup I2S based on sampling rate provided by decoder
    out2dec.setNotifyAudioChange(i2s);
    out2dec.begin();
  } else {
    Serial.println("failed to read sound file");
  }
}


void initConfig(const char * filename) {

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



void setup(){
  Serial.begin(115200);

  Serial.println();
  Serial.println(">>> EasyButton multiple onSequence example <<<");

  // mount SPIFF
  SPIFFS.begin();

  // read config file
  initConfig(cfgfile);

  // Initialize the button.
  button.begin();
  button.onPressed(onPressed);
  button.onPressedFor(1000, onPressedForDuration);

  // init neopixel
  pixels.begin(); // INITIALIZE NeoPixel strip object (REQUIRED)

  // --------- Audio Init Section -------
  //  AudioLogger::instance().begin(Serial, AudioLogger::Info);  
  // setup metadata
  outMeta.setCallback(printMetaData);
  outMeta.begin();
  // setup i2s
  auto i2scfg = i2s.defaultConfig(TX_MODE);
  i2scfg.pin_ws = 25;
  i2scfg.pin_bck = 26;
  i2scfg.pin_data = 27;
  i2s.begin(i2scfg);
  // setup I2S based on sampling rate provided by decoder
  out2dec.setNotifyAudioChange(i2s);
  out2dec.begin();
  // --------- End Audio Init Section -------
  
}


void loop(){
  
  // Continuously read the status of the button.
  button.read();

  // pixel
  pixelLoop();

  if (bladeIsOn) {
    // sound stream 
    copier.copy();

    // no file loaded play random hum sounds
    if (!copier.available()) {
      randNumber = random(5);
      Serial.printf("Play file no %d\n", (int)randNumber);
      switch ((int) randNumber) {
        case 0: playFile("/Hum 1.mp3");
                break;
        case 1: playFile("/Hum 2.mp3");
                break;
        case 2: playFile("/Hum 4.mp3");
                break;
        case 3: playFile("/Hum 5.mp3");
                break;
        case 4: playFile("/SlowSabr.mp3");
                break;
      }
    }
  }


}


