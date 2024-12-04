#include <Audio.h>
#include <Wire.h>
#include <SPI.h>
#include <SD.h>
#include <SerialFlash.h>
#include <FastLED.h>
#include <cmath>

// GUItool: begin automatically generated code
AudioInputI2S i2s1;          // xy=242,357
AudioMixer4 mixer1;          // xy=500,385
AudioAnalyzeFFT256 fft256_1; // xy=675,377
AudioConnection patchCord1(i2s1, 0, mixer1, 0);
AudioConnection patchCord2(i2s1, 1, mixer1, 1);
AudioConnection patchCord3(mixer1, fft256_1);
AudioControlSGTL5000 sgtl5000_1; // xy=309,293

const int myInput = AUDIO_INPUT_LINEIN;

// FastLED setup
#define NUM_LEDS 30
#define DATA_PIN 3
CRGB leds[NUM_LEDS];

// Rolling average size
const int rollingAverageSize = 3; // Adjust this to control the window size for the average

// Rolling average buffers
float redHistory[rollingAverageSize] = {0};
float greenHistory[rollingAverageSize] = {0};
float blueHistory[rollingAverageSize] = {0};
int historyIndex = 0; // Tracks the current position in the history buffer

// Loudness scaling constants
const float weighting[] = {1.0, 0.9, 0.9, 0.8, 0.8, 0.7, 0.7, 0.6}; // Perceptual weighting
const int numBands = 8;                                             // Number of critical bands
const int bandSize = 128 / numBands;                                // FFT bins per band

// Thresholds
const float minBrightness = 0.05; // Minimum loudness for LEDs
const float maxBrightness = 1.0;  // Maximum loudness for full brightness

//===============================================================================
// Initialization
//===============================================================================
void setup()
{
  Serial.begin(9600);
  AudioMemory(12);

  // Initialize audio
  sgtl5000_1.enable();
  sgtl5000_1.inputSelect(myInput);
  sgtl5000_1.volume(1);

  // Initialize FastLED
  FastLED.addLeds<NEOPIXEL, DATA_PIN>(leds, NUM_LEDS);

  delay(1000); // Small delay for system setup
}

//===============================================================================
// Main
//===============================================================================
void loop()
{
  unsigned long startTime = millis();
  if (fft256_1.available()) // Ensure FFT data is available
  {
    // Analyze FFT into perceptual bands
    float bands[numBands] = {0};
    float totalLoudness = 0;

    // Calculate loudness per band
    for (int i = 0; i < numBands; i++)
    {
      bands[i] = fft256_1.read(i * bandSize, (i + 1) * bandSize - 1) * weighting[i];
      totalLoudness += bands[i];
    }

    // Normalize loudness and calculate brightness scale
    float brightnessScale = constrain(totalLoudness, minBrightness, maxBrightness);

    // Map perceptual bands to RGB
    float red = bands[0] + bands[1];              // Low frequencies (bass)
    float green = bands[2] + bands[3] + bands[4]; // Mid frequencies
    float blue = bands[5] + bands[6] + bands[7];  // High frequencies (treble)

    // Normalize RGB values
    float maxColor = max(red, max(green, blue));
    if (maxColor > 0)
    {
      red /= maxColor;
      green /= maxColor;
      blue /= maxColor;
    }

    // Update rolling average buffers
    redHistory[historyIndex] = red;
    greenHistory[historyIndex] = green;
    blueHistory[historyIndex] = blue;

    // Calculate rolling averages
    float redAverage = 0;
    float greenAverage = 0;
    float blueAverage = 0;
    for (int i = 0; i < rollingAverageSize; i++)
    {
      redAverage += redHistory[i];
      greenAverage += greenHistory[i];
      blueAverage += blueHistory[i];
    }
    redAverage /= rollingAverageSize;
    greenAverage /= rollingAverageSize;
    blueAverage /= rollingAverageSize;

    // Increment history index
    historyIndex = (historyIndex + 1) % rollingAverageSize;

    // Scale RGB by brightness
    uint8_t r = static_cast<uint8_t>(redAverage * brightnessScale * 255);
    uint8_t g = static_cast<uint8_t>(greenAverage * brightnessScale * 255);
    uint8_t b = static_cast<uint8_t>(blueAverage * brightnessScale * 255);

    // Set LED colors
    for (int i = 0; i < NUM_LEDS; i++)
    {
      leds[i] = CRGB(r, g, b);
    }

    // Smooth transitions
    fadeToBlackBy(leds, NUM_LEDS, 10);

    // Update LEDs
    FastLED.show();
  }

  // Record the end time and calculate duration
  unsigned long endTime = millis();
  unsigned long loopDuration = endTime - startTime;
  Serial.print("Loop Duration: ");
  Serial.print(loopDuration);
  Serial.println(" ms");
}
