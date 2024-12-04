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
#define NUM_LEDS 28 // Total LEDs (14 mirrored on each side)
#define DATA_PIN 3
CRGB leds[NUM_LEDS];

// Frequency bin ranges for low, mid, and high
const int lowStart = 0, lowEnd = 5;      // Low frequencies (bass)
const int midStart = 6, midEnd = 20;     // Mid frequencies
const int highStart = 21, highEnd = 127; // High frequencies

const int halfLeds = NUM_LEDS / 2; // Half the LEDs (14 for mirroring)

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
// Main Loop
//===============================================================================
void loop()
{
  if (fft256_1.available()) // Ensure FFT data is available
  {
    // Analyze frequency ranges
    float low = fft256_1.read(lowStart, lowEnd);    // Low frequencies
    float mid = fft256_1.read(midStart, midEnd);    // Mid frequencies
    float high = fft256_1.read(highStart, highEnd); // High frequencies

    // Normalize frequency bands
    float total = low + mid + high;
    if (total > 0)
    {
      low /= total;
      mid /= total;
      high /= total;
    }

    // Map frequency ranges to LED colors for half the LEDs
    for (int i = 0; i < halfLeds; i++)
    {
      float position = float(i) / (halfLeds - 1); // LED position normalized [0, 1]
      uint8_t r, g, b;

      if (position < 0.33) // Low frequencies (red)
      {
        float intensity = low * (1.0 - position / 0.33);
        r = static_cast<uint8_t>(intensity * 255);
        g = b = 0;
      }
      else if (position < 0.66) // Mid frequencies (green)
      {
        float intensity = mid * (1.0 - (position - 0.33) / 0.33);
        g = static_cast<uint8_t>(intensity * 255);
        r = b = 0;
      }
      else // High frequencies (blue)
      {
        float intensity = high * (1.0 - (position - 0.66) / 0.34);
        b = static_cast<uint8_t>(intensity * 255);
        r = g = 0;
      }

      // Assign color to this LED
      leds[halfLeds - 1 - i] = CRGB(r, g, b); // Left side
      leds[halfLeds + i] = CRGB(r, g, b);     // Mirrored right side
    }

    // Update LEDs
    FastLED.show();

    // Debugging output
    Serial.print("Low: ");
    Serial.print(low);
    Serial.print(" Mid: ");
    Serial.print(mid);
    Serial.print(" High: ");
    Serial.println(high);
  }
}
