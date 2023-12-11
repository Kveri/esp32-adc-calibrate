# esp32-adc-calibrate
Two stage approach to calibrate ESP-32 ADC

# Introduction
ESP-32 ADC is not perfect, in fact it's imprecise as hell. It isn't just linearity issue, the real voltage measured vs. ADC reported voltage difference is changing with changing voltage (so it's not linear).
I used https://github.com/e-tinkers/esp32-adc-calibrate/ to calibrate the ADC first, but that code has a few bugs and it doesn't account for DAC not being calibrated. That means that the entire ADC calibration process is flawed before it even starts due to uncalibrated DAC.
That's why I wrote my own calibration for ESP-32 DAC and ADC calibration.

Please refer to https://github.com/e-tinkers/esp32-adc-calibrate/ to learn more about why ADC calibration is needed.

ADC - is used to convert analog signal (voltage) to digital values. On ESP-32 ADC is 12-bit by default, meaning there are 4096 possible values.
DAC - is used to convert digital signal to analog signal (voltage). On ESP-32 DAC is 8-bit, which means that DAC can output up to 256 different voltage values.

Voltage on ESP-32 is between 0V and 3.3V, so ADC precision is 0.0008V and DAC precision is 0.012V, at least in theory.

# How to use
1. Download the code
2. Leave #define PHASE1_DAC uncommented
3. Set ADC_PIN to your preferred ADC input pin (I used 35)
4. Connect pin 25 and pin 35 on ESP-32 using a jumper cable
5. With multimeter in hand, negative probe connected to GND and positive probe connected to pin 25 or pin 35, compile & run the code
6. Watch the output of serial monitor and note down all values visible on the multimeter as they change, you should have 17 values
7. put these measured values into DAC_MEASURED_VOLTAGE at the top of the code
8. comment out #define PHASE1_DAC
9. leave the jumper cable connected
10. compile & run the code again
11. before the table is printed there is a quick 5-value test, please check that Corrected Voltage is close to 'DAC set to U'
12. copy&paste the resulting table to your code
13. anytime you read ADC using val = analogRead(), do ADC_LUT[val] and that will give you your corrected ADC reading


# Technical working principle
This ADC calibration works by using the built-in DAC to generate 'known' voltages and then read them back using ADC. In principle, if DAC behaves properly then this process will allow us to create a mapping/correction table for ADC and use it in any code where improved ADC precision is required.
However, ESP-32 DAC also isn't perfect.

To achieve best possible results we first need to calibrate DAC.

This calibration code does the following:
Phase 1a: DAC calibration - measurement
1. the code cycles through 16 voltage values on the output
2. you need to measure the output voltage using a multimeter for each of these 16 values and write them down
3. you then input these 16 values into the code

This is a seed for DAC calibration. This way, we can now compare what DAC thinks it's outputting vs. real world voltage values.
I've chosen 16 values because it doesn't take ages to measure - feel free to do more/less if you like.

Once manually measured voltages are inputted for DAC calibration, we need to perform some DAC calibration computation on these measured values.

Phase 1b: DAC calibration - computation
1. First, those 16 manually measured DAC values need to be interpolated to 256 values (because that's how many values can be set for DAC)
2. Then we cycle through all 256 voltage levels which can be generated using 0-3.3V 256-value DAC and we find the closest matching interpolated value (real-world value) for each of the DAC levels
3. this is saved as DAC_LUT - a DAC correction look-up table, which will be used from now on

The DAC_LUT table allows us to map requested DAC level to a level which is as closest to the output we want.

Imagine needing to output 1.5V. You take 1.5 as you requested voltage output, divide it by 3.3 (max voltage), you get 0.45454545. Now multiply that by 256 (number of possible output values). That gives 116.36. So if you set DAC output to 116, you should get output of around 1.5V. But in reality I get 1.46V. On the other hand if you set 117 to the DAC, you should get (117/256*3.3) 1.508V, right? Well I get 1.47V. So let's go higher. With 118 I get 1.49V and with 119 I get 1.50V. So 119 is the right answer, even though using just math we would use 116.

And this is what the DAC correction look-up table does. It maps 116 to 119 (and so on), correcting the INPUT to the DAC, so that the output matches the math calculation, which is what we expect.

Now that we have DAC correction look-up table calculated, we can plug that as a correct input to our ADC calibration, finally.

Phase 2: ADC calibration
1. Since our DAC is now producing known and relatively precise output, we run ADC through all 256 possible values 500 times and average each output giving 256 ADC readings
2. We need to check if these ADC readings are in ascending order and if not, correct them (sometimes they are not in ascending order, e.x. if you set DAC to 116 the ADC reads value X, but with DAC set to 117 the ADC can potentially read LOWER value than X). As I said - imprecise as hell.
3. Then we linearly interpolate these 256 readings into 4096 (because ADC is 12-bit). Remember, we have 256 values, so between each 2 values we need to interpolate 15 values.
4. Then we cycle through these 4096 values and invert the table. Basically we find which input value (index) we need to produce the expected output (value).
5. That creates the correction look-up table for ADC.

Now, here is the comparison without and with this calibration.

Setting DAC to 50
DAC set to U = 0.64
Reading ADC, Uncorrected reading: 647.00 Voltage: 0.52
Corrected reading: 801.00 Voltage: 0.65

See? Without the calibration the ADC reads 0.52V when we output 0.64V. With the calibration we correct that reading to 0.65V.

Setting DAC to 77
DAC set to U = 0.99
Reading ADC, Uncorrected reading: 1074.00 Voltage: 0.87
Corrected reading: 1235.00 Voltage: 0.99

Setting DAC to 143
DAC set to U = 1.84
Reading ADC, Uncorrected reading: 2147.00 Voltage: 1.73
Corrected reading: 2291.00 Voltage: 1.85

Setting DAC to 195
DAC set to U = 2.51
Reading ADC, Uncorrected reading: 2979.00 Voltage: 2.40
Corrected reading: 3125.00 Voltage: 2.52

Setting DAC to 225
DAC set to U = 2.90
Reading ADC, Uncorrected reading: 3541.00 Voltage: 2.85
Corrected reading: 3603.00 Voltage: 2.90



