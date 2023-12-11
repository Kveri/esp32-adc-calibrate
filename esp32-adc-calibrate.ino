#include <Arduino.h>
#include <driver/dac.h>


//#define PHASE1_DAC // uncomment to run 1st phase - DAC calibration, if left commented it will run 2nd phase - ADC calibration

const float DAC_MEASURED_VOLTAGE[17] = { 0.07, 0.26, 0.46, 0.65, 0.83, 1.03, 1.22, 1.41,
                                         1.62, 1.81, 2.00, 2.20, 2.38, 2.58, 2.77, 2.97, 3.15 };

#define ADC_PIN 35    // GPIO 35 = A7, uses any valid Ax pin as you wish

int DAC_LUT[256];
float DAC_REAL_VALUES[257];
float ADC_VALUES[4096];
float ADC_LUT[4096];

/*
void dumpResults() {
  for (int i=0; i<4096; i++) {
    if (i % 16 == 0) {
      Serial.println();
      Serial.print(i); Serial.print(" - ");
    }
    Serial.print(Results[i], 2); Serial.print(", ");
  }
  Serial.println();
}

void dumpRes2() {
  Serial.println(F("Dump Res2 data..."));
  for (int i=0; i<(5*4096); i++) {
      if (i % 16 == 0) {
        Serial.println(); Serial.print(i); Serial.print(" - ");
      }
      Serial.print(Res2[i],3); Serial.print(", ");
    }
    Serial.println();
}
*/
void setup() {
    dac_output_enable(DAC_CHANNEL_1);    // pin 25
    dac_output_voltage(DAC_CHANNEL_1, 0);
    analogReadResolution(12);
    Serial.begin(115200);
    delay(1000);
}

int dac_corrected_voltage(int idx)
{
//  int better_idx;
//  float v_out, v_out_better;
  // get better index (closer value)
//  better_idx = DAC_LUT[idx];
//  Serial.print("requested idx = ");
//  Serial.print(idx);
//  Serial.print(" better idx = ");
//  Serial.println(better_idx);

  // find what voltage we will be outputting by using this index
//  v_out = DAC_REAL_VALUES[idx];
//  v_out_better = DAC_REAL_VALUES[better_idx];

//  Serial.print("requested idx would result in ");
//  Serial.print(v_out);
//  Serial.print(" V, better idx results in ");
//  Serial.print(v_out_better);
//  Serial.println(" V");
  
  return DAC_LUT[idx];
}

void loop() {
  int i, j;
  float x;


#ifdef PHASE1_DAC
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
    //Serial.print("Target: ");
    //Serial.println(target);
    
    // search near expected voltage for closest match
    guess_idx = i;
    //Serial.print("Guess idx: ");
    //Serial.print(guess_idx);
    //Serial.print(" Value: ");
    //Serial.println(DAC_REAL_VALUES[guess_idx]);
    while (1) {
      if (DAC_REAL_VALUES[guess_idx] == target) { // found exact match
        DAC_LUT[i] = guess_idx;
        //Serial.print("Exact match ");
        //Serial.print(guess_idx);
        //Serial.print(" Value: ");
        //Serial.println(DAC_REAL_VALUES[guess_idx]);
        break;
      } else if (DAC_REAL_VALUES[guess_idx] > target) { // guess is above target, need to go lower
        //Serial.println("Guess above target, going lower ");
        if (guess_idx == 0) { // we reached the bottom, return closest (lowest possible) value
          DAC_LUT[i] = 0;
          //Serial.println("CANT go lower, zero");
          break;
        }

        //Serial.println("Can go lower, closer to target?");
        
        // we can still go lower, but is that going to bring us closer to the target?
        //Serial.print("fabs(");
        //Serial.print(DAC_REAL_VALUES[guess_idx]);
        //Serial.print(" - ");
        //Serial.print(target);
        //Serial.print(") <= fabs(");
        //Serial.print(DAC_REAL_VALUES[guess_idx - 1]);
        //Serial.print(" - ");
        //Serial.print(target);
        //Serial.print(") = ");
        if (fabs(DAC_REAL_VALUES[guess_idx] - target) <= fabs(DAC_REAL_VALUES[guess_idx - 1] - target)) { // is the current guess closer to targe than new guess?
          // yes, current guess is closest to target, return it
          DAC_LUT[i] = guess_idx;
          //Serial.print("yes, best guess is this, ");
          //Serial.print(guess_idx);
          //Serial.print(" Value: ");
          //Serial.println(DAC_REAL_VALUES[guess_idx]);
          break;
        }
        // no, move to next better guess
        //Serial.println("No, going to better guess");
        guess_idx--;
      } else { // guess is below target, need to go higher
        //Serial.println("Guess below target, going higher ");
        if (guess_idx == 256) { // we reached the top, return the closest (highest possible) value
          DAC_LUT[i] = 255;
          //Serial.println("CANT go higher, 255");
          break;
        }

        //Serial.println("Can go higher, closer to target?");
        
        // we can still go lower, but is that going to bring us closer to the target?
        //Serial.print("fabs(");
        //Serial.print(DAC_REAL_VALUES[guess_idx]);
        //Serial.print(" - ");
        //Serial.print(target);
        //Serial.print(") <= fabs(");
        //Serial.print(DAC_REAL_VALUES[guess_idx - 1]);
        //Serial.print(" - ");
        //Serial.print(target);
        //Serial.print(") = ");

        // we can still go higher, but is that going to bring us closer to the target?
        if (fabs(DAC_REAL_VALUES[guess_idx] - target) <= fabs(DAC_REAL_VALUES[guess_idx + 1] - target)) { // is the current guess closer to targe than new guess?
          // yes, current guess is closest to target, return it
          DAC_LUT[i] = guess_idx;
          //Serial.print("yes, best guess is this, ");
          //Serial.print(guess_idx);
          //Serial.print(" Value: ");
          //Serial.println(DAC_REAL_VALUES[guess_idx]);
          break;
        }
        // no, move to next better guess
        guess_idx++;
        //Serial.println("No, going to better guess");
      }
    }
  }

  // print out best DAC LUT
  /*
  for (i = 0; i < 256; i++) {
    Serial.print(DAC_LUT[i]);
    Serial.print(", ");
    if (i % 16 == 15)
      Serial.println();      
  }
  */

/*
  // test - compare DAC without and with correction
  for (i = 0; i < 256; i+=16) {
    Serial.print("Setting DAC V to ");
    Serial.print(i);
    Serial.print(" that should be ");
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

  // 1. output a known voltage on DAC
  // 2. measure voltage on ADC


  Serial.print("Getting values from ADC based on corrected DAC...");
  for (i = 0; i < 500; i++) {
    if (i % 100 == 0)
      Serial.print(".");
    
    for (j = 0; j < 256; j++) {
        //Serial.print("Setting DAC voltage to: ");
        //Serial.print(j);
        //Serial.print(" corrected: ");
        //Serial.print(dac_corrected_voltage(j));
        //Serial.print(" index: ");
        //Serial.println(j*16);
        dac_output_voltage(DAC_CHANNEL_1, (dac_corrected_voltage(j) & 0xff));
        delayMicroseconds(100);
        //ADC_VALUES[j * 16] = 0.9 * ADC_VALUES[j * 16] + 0.1 * analogRead(ADC_PIN);
        ADC_VALUES[j * 16] += analogRead(ADC_PIN);

        if (i == 499) {
          ADC_VALUES[j * 16] /= 500;
        }
    }
  }
  Serial.println();
/*
  for (i = 0; i < 4096; i++) {
    if (i % 16 == 0) {
      Serial.print(i);
      Serial.print(": ");
    }
    Serial.print(ADC_VALUES[i]);
    Serial.print(", ");
    if (i % 16 == 15) // skip already existing value
      Serial.println();
  }
*/
  /*
  dac_output_voltage(DAC_CHANNEL_1, (190 & 0xff));
  delay(5000);
  dac_output_voltage(DAC_CHANNEL_1, (191 & 0xff));
  delay(5000);
  dac_output_voltage(DAC_CHANNEL_1, (192 & 0xff));

  while (1) {};
*/
  //dumpResults();

  // check if values are ascending and fix if not
  Serial.println("Checking if ADC values are ascending and fixing them if not...");
  for (i = 0; i < 255; i++) {
    if (ADC_VALUES[i * 16] <= ADC_VALUES[i * 16 + 16])
      continue;

    Serial.print("ERROR: values not ascending, index: ");
    Serial.println(i*16);
    ADC_VALUES[i * 16] = ADC_VALUES[i * 16 - 16];
    //while (1) {};
  }
  

  // 3. interpolate measured voltages from ADC

  Serial.println("Calculate interpolated values...");
  //Results[4096] = 4095.0;
  
  for (i = 0; i < 4096; i++) {
    if (i % 16 == 0) // skip already existing value
      continue;

    // calculate interpolated values
    ADC_VALUES[i] = ADC_VALUES[i / 16 * 16] + (ADC_VALUES[i / 16 * 16 + 16] - ADC_VALUES[i / 16 * 16]) * (i % 16) / 16.0;
  }

/*
  for (i = 0; i < 4096; i++) {
    if (i % 16 == 0) {
      Serial.print(i);
      Serial.print(": ");
    }
    Serial.print(ADC_VALUES[i]);
    Serial.print(", ");
    if (i % 16 == 15) // skip already existing value
      Serial.println();
  }
*/
  // 4. generate LUT - reverse & expand array

  //while (1) {};

  Serial.println("Reversing data / Generating LUT...");

  min_idx = 0;
  for (i = 0; i < 4096; i++) {
    //Serial.print("looking for ");
    //Serial.println(i);
    //Serial.print("diff: ");

    min_diff = 99999;
    last_pos = 0;
    while (fabs(ADC_VALUES[last_pos] - i) <= min_diff) {
      //Serial.print("last_pos: ");
      //Serial.print(last_pos);
      //Serial.print(" -> ");
      //Serial.print(ADC_VALUES[last_pos]);
      //Serial.print(" - ");
      //Serial.print(i);
      //Serial.print(" = ");
      //Serial.print(fabs(ADC_VALUES[last_pos] - i));
      //Serial.print(" < ");
      //Serial.println(min_diff);
      min_diff = fabs(ADC_VALUES[last_pos] - i);
      min_idx = last_pos;
      last_pos++;
    }

    //Serial.print("best pos = ");
    //Serial.println(min_idx);

    ADC_LUT[i] = min_idx;
    //Serial.print("ADC_LUT[");
    //Serial.print(i);
    //Serial.print("] = ADC_VALUES[");
    //Serial.print(min_idx);
    //Serial.print("] (val: ");
    //Serial.print(ADC_VALUES[min_idx]);
    //Serial.println(")");
  }

/*
  for (i = 0; i < 4096; i++) {
    if (i % 16 == 0) {
      Serial.print(i);
      Serial.print(": ");
    }
    Serial.print(ADC_LUT[i]);
    Serial.print(", ");
    if (i % 16 == 15) // skip already existing value
      Serial.println();
  }
*/

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

  // try few selected values
  int test_values[5] = { 50, 77, 143, 195, 225 };
  int input;

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

  while (1) {};
/*
  for (int i=0; i<256; i++) {
     for (int j=1; j<16; j++) {
        Results[i*16+j] = Results[i*16] + (Results[(i+1)*16] - Results[(i)*16])*(float)j / (float)16.0;
     }
  }
    //dumpResults();

   

    Serial.println(F("Generating LUT .."));
    for (int i=0; i<4096; i++) {
        Results[i]=0.5 + Results[i];
    }
    // dumpResults();

    Results[4096]=4095.5000;
    for (int i=0; i<4096; i++) {
       for (int j=0; j<5; j++) {
          Res2[i*5+j] = Results[i] + (Results[(i+1)] - Results[i]) * (float)j / (float)5.0;
       }
    }
    dumpRes2();
    while (1);

    for (int i=1; i<4096; i++) {
        int index=0;
        float minDiff=99999.0;
        for (int j=0; j<(5*4096); j++) {
            float diff=fabs((float)(i) - Res2[j]);
            if(diff<minDiff) {
                minDiff=diff;
                index=j;
            }
        }
        Results[i]=(float)index;
    }
    // dumpResults();

    for (int i=0; i<(4096); i++) {
        Results[i]/=5;
    }

  
  Serial.println("DONE");
  Serial.println("DONE");
  Serial.println("DONE");
//  while(1);


    Serial.println();
    

  Serial.println("const float ADC_LUT[4096] = { 0,");
  for (int i=1; i<4095; i++) {
    Serial.print(Results[i],4); Serial.print(",");
     if ((i%15)==0) Serial.println();
  }
  Serial.println(Results[4095]);
  Serial.println("};");

  while(1);
*/
#endif

}