/*
MIT License

Copyright (c) 2023 Kveri

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <Arduino.h>
#include <driver/dac.h>


//#define PHASE1_DAC // uncomment to run 1st phase - DAC calibration, if left commented it will run 2nd phase - DAC computation and ADC calibration

// put your measure voltage values from Phase 1 here 
const float DAC_MEASURED_VOLTAGE[17] = { 0.07, 0.26, 0.46, 0.65, 0.83, 1.03, 1.22, 1.41,
                                         1.62, 1.81, 2.00, 2.20, 2.38, 2.58, 2.77, 2.97, 3.15 };

#define ADC_PIN 35  // GPIO 35 = A7


/*##########################################################################################################*/

int DAC_LUT[256];
float DAC_REAL_VALUES[257];
float ADC_VALUES[4096];
float ADC_LUT[4096];

void setup() {
    dac_output_enable(DAC_CHANNEL_1);    // pin 25
    dac_output_voltage(DAC_CHANNEL_1, 0);
    analogReadResolution(12);
    Serial.begin(115200);
    delay(1000);
}

int dac_corrected_voltage(int idx)
{
  return DAC_LUT[idx];
}

void loop() {
  int i, j;
  float x;

#ifdef PHASE1_DAC
  // DAC measurement
  Serial.println("DAC calibration - measure and record voltage on DAC output pin 25 for each step");
  for (i = 0; i < 16; i++) {
    dac_output_voltage(DAC_CHANNEL_1, (i*16) & 0xff);
    Serial.print("Voltage set to ");
    Serial.println(3.3 * (i*16.0) / 256.0);
    delay(5000);
  }
  dac_output_voltage(DAC_CHANNEL_1, 255);
  Serial.println("Voltage set to 3.3");
  while (1) {};
#else
  float target, min_diff;
  int guess_idx, last_pos, min_idx;

  // DAC computation

  // Interpolate 16 real voltage values of DAC to all 257 values
  Serial.println("Interpolate real voltage values of DAC to all 257 values...");
  for (i = 0; i < 256; i++) {
    DAC_REAL_VALUES[i] = DAC_MEASURED_VOLTAGE[i/16] + (DAC_MEASURED_VOLTAGE[i/16 + 1] - DAC_MEASURED_VOLTAGE[i/16]) * (i % 16) / 16;
  }
  
  // Search for best mapping values
  Serial.println("Searching for best matches...");
  for (i = 0; i < 256; i++) {
    // calculate target voltage
    target = 3.3 * i / 256.0;
    
    // search near expected voltage for closest match
    guess_idx = i;
    while (1) {
      if (DAC_REAL_VALUES[guess_idx] == target) { // found exact match
        DAC_LUT[i] = guess_idx;
        break;
      } else if (DAC_REAL_VALUES[guess_idx] > target) { // guess is above target, need to go lower
        if (guess_idx == 0) { // we reached the bottom, return closest (lowest possible) value
          DAC_LUT[i] = 0;
          break;
        }

        // we can still go lower, but is that going to bring us closer to the target?
        if (fabs(DAC_REAL_VALUES[guess_idx] - target) <= fabs(DAC_REAL_VALUES[guess_idx - 1] - target)) { // is the current guess closer to target than new guess?
          // yes, current guess is closest to target, return it
          DAC_LUT[i] = guess_idx;
          break;
        }
        // no, move to next better guess
        guess_idx--;
      } else { // guess is below target, need to go higher
        if (guess_idx == 256) { // we reached the top, return the closest (highest possible) value
          DAC_LUT[i] = 255;
          break;
        }

        // we can still go higher, but is that going to bring us closer to the target?
        if (fabs(DAC_REAL_VALUES[guess_idx] - target) <= fabs(DAC_REAL_VALUES[guess_idx + 1] - target)) { // is the current guess closer to target than new guess?
          // yes, current guess is closest to target, return it
          DAC_LUT[i] = guess_idx;
          break;
        }
        // no, move to next better guess
        guess_idx++;
      }
    }
  }

/*
  // test - compare DAC without and with correction
  for (i = 0; i < 256; i+=16) {
    Serial.print("Setting DAC V to ");
    Serial.print(i);
    Serial.print(" that should be (math) ");
    Serial.println(3.3*i/256.0);

    Serial.println("Setting to not corrected...");
    dac_output_voltage(DAC_CHANNEL_1, i & 0xff);
    delay(3000);
    Serial.println("Setting to CORRECTED...");
    dac_output_voltage(DAC_CHANNEL_1, (dac_corrected_voltage(i) & 0xff));
    Serial.println();
    delay(5000);
  }
*/

  // ADC calibration

  // 1. output a known voltage on DAC
  // 2. measure voltage on ADC
  Serial.print("Getting values from ADC based on corrected DAC ");
  for (i = 0; i < 500; i++) {
    if (i % 100 == 0)
      Serial.print(".");
    
    for (j = 0; j < 256; j++) {
        dac_output_voltage(DAC_CHANNEL_1, (dac_corrected_voltage(j) & 0xff));
        delayMicroseconds(100);
        ADC_VALUES[j * 16] += analogRead(ADC_PIN);

        if (i == 499) {
          ADC_VALUES[j * 16] /= 500;
        }
    }
  }
  Serial.println();
  
  // 3. check if values are ascending and fix if not
  Serial.println("Checking if ADC values are ascending and fixing them if not...");
  for (i = 0; i < 255; i++) {
    if (ADC_VALUES[i * 16] <= ADC_VALUES[i * 16 + 16])
      continue;

    Serial.print("Warning: values not ascending, index: ");
    Serial.println(i*16);
    ADC_VALUES[i * 16] = ADC_VALUES[i * 16 - 16];
  }
  

  // 4. interpolate measured voltages from ADC
  Serial.println("Calculate interpolated values...");
  for (i = 0; i < 4096; i++) {
    if (i % 16 == 0) // skip already existing value
      continue;

    // calculate interpolate value
    ADC_VALUES[i] = ADC_VALUES[i / 16 * 16] + (ADC_VALUES[i / 16 * 16 + 16] - ADC_VALUES[i / 16 * 16]) * (i % 16) / 16.0;
  }

  // 5. generate LUT - reverse array
  Serial.println("Reversing data / Generating LUT...");
  min_idx = 0;
  for (i = 0; i < 4096; i++) {
    min_diff = 99999;
    last_pos = 0;
    while (fabs(ADC_VALUES[last_pos] - i) <= min_diff) {
      min_diff = fabs(ADC_VALUES[last_pos] - i);
      min_idx = last_pos;
      last_pos++;
    }

    ADC_LUT[i] = min_idx;
  }


/*
  // check ADC uncorrected and corrected readings for all 256 DAC values
  for (i = 0; i < 256; i++) {
    Serial.print("Setting DAC to ");
    Serial.println(i);
    dac_output_voltage(DAC_CHANNEL_1, (dac_corrected_voltage(i) & 0xff));
    Serial.print("DAC set to U = ");
    Serial.println(i*3.3/256.0);
  
    x = analogRead(ADC_PIN);
    
    Serial.print("Reading ADC, Uncorrected reading: ");
    Serial.print(x);
    Serial.print(" Voltage: ");
    Serial.println(x*3.3/4096.0);
    Serial.print("Corrected reading: ");
    Serial.print(ADC_LUT[(int)x]);
    Serial.print(" Voltage: ");
    Serial.println(ADC_LUT[(int)x]*3.3/4096.0);
  }
*/


  // try few selected values
  int test_values[5] = { 50, 77, 143, 195, 225 };
  int input;

  Serial.println("Running 5 values quick test, please check that Corrected Voltage is close to 'DAC set to U'.");
  for (i = 0; i < 5; i++) {
    input = test_values[i];
    Serial.print("Setting DAC to ");
    Serial.println(input);
    dac_output_voltage(DAC_CHANNEL_1, (dac_corrected_voltage(input) & 0xff));
    Serial.print("DAC set to U = ");
    Serial.println(input*3.3/256.0);
  
    x = analogRead(ADC_PIN);
    
    Serial.print("Reading ADC, Uncorrected reading: ");
    Serial.print(x);
    Serial.print(" Voltage: ");
    Serial.println(x*3.3/4096.0);
    Serial.print("Corrected reading: ");
    Serial.print(ADC_LUT[(int)x]);
    Serial.print(" Voltage: ");
    Serial.println(ADC_LUT[(int)x]*3.3/4096.0);
    delay(5000);
  }

  // print the look-up table
  Serial.println("Priting the ADC correction look-up table, to be used in your code.");
  Serial.println();
  Serial.println("const float ADC_LUT[4096] = {");
  for (i = 1; i < 4095; i++) {
    Serial.print(ADC_LUT[i]);
    Serial.print(", ");
     if (i % 15 == 0)
       Serial.println();
  }
  Serial.print(ADC_LUT[4095]);
  Serial.println("};");
  Serial.println();

  Serial.println("#####END OF EXECUTION####");
  while(1) {};
#endif

}