/*TR-01 Rotary Compression Tester
  Originally distributed by John Doss 2017, modified for output in bar and
  normalized values for 250rpm by Miro Bogner 2018

  This code assumes you're using a 0-200 psi - 0.5-4.5 vdc pressure transducer
  connected to the AI0 pin. You can use sensors with other pressure or voltage
  ranges but you'll need to modify the code.
*/
#include "LiquidCrystal_I2C.h"
#include <Wire.h>
#define SENSOR 0                                                // Analog input pin that sensor is connected too

LiquidCrystal_I2C lcd(0x27, 16, 2);

const float dead_space = 1.2;                                   // correction factor to match values to the Mazda OEM Tester
const float bar_factor = 0.1683290354778;                       // factor to calculate bar (x10) from analog sensor values
const float a0 = 8.5944582043344;                               // polynom Fit a0 to norm values to 250rpm
const float a1 = -4.8028702270382E-02;                          // polynom Fit a1 to norm values to 250rpm
const float a2 = 5.4250515995872E-05;                           // polynom Fit a2 to norm values to 250rpm
const int baseline = 200;                                       // If sensor reading are below baseline we assume there is no measurement
const int no_measurement_series = 5;                            // We record 5 sets of values as we consider the first and last two unreliable
const int reliable_measurement = no_measurement_series / 2;
const int no_chambers = 3;                                      // The rotary engine has three chambers
const int max_threshold = 15;                                   // Threshold to look for max and min values, cannot be zero due to noise
const int min_threshold = 15;

int max_buffer[no_measurement_series][no_chambers];             // Buffer to store pressure values
unsigned long rpm[no_measurement_series];                       // Buffer to store rpm values

char buf1[5], buf2[75];                                         // Definde char buffers for temp and output
int testcham=0;

float analogReadTest()
{
  float readval=0.0;
  if ( testcham == 0 )
  {
    readval=random(200, 220);
  }
  if ( testcham == 1 )
  {
    readval=random(220, 240);
  }
  if ( testcham == 2 )
  {
    testcham=-1;
    readval=random(240, 260);
  }
  testcham++;
  return readval;
}

int find_max() {                                                // Function to look for pressure peaks
  int current_max = 0;
  int sensor_measurement = analogReadTest();
  /*
   * max_threshold=15
   * 
   */
  while ((current_max - sensor_measurement) < max_threshold) 
  {
    Serial.println("in find_max < max_threshold");  
    if (sensor_measurement > current_max)
    {
      current_max = sensor_measurement;
      Serial.println(current_max);   
    }
    sensor_measurement = (analogReadTest());
  }
  int current_min = sensor_measurement;
  while ((sensor_measurement - current_min) < min_threshold) {
    sensor_measurement = (analogReadTest());
    if (sensor_measurement < current_min)
      current_min = sensor_measurement;
  }
  return current_max;
}

void setup()
{
  Serial.begin(38400);
  // initialize the LCD
  lcd.begin();
  
  delay(500);

  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.print("Tester Ready");
}
void loop() {
  Serial.println("enter loop");
  if (analogReadTest() < baseline) {                          // Only start measuring once we exceed the baseline
    Serial.println("Only start measuring once we exceed the baseline");
    return;
  }

  Serial.println("first for");
  for (int i = 0; i < no_measurement_series; ++i) {
    unsigned long start_time = millis();                        // record cycle begining time for RPM calculation
    Serial.println("second for");
    for (int chamber = 0; chamber < no_chambers; ++chamber) {
      Serial.println("chamber");
      max_buffer[i][chamber] = find_max();
      //max_buffer[i][chamber] = 2;
    }
    unsigned long end_time = millis();                          // record cycle end time for RPM calculation
    rpm[i] = round(180000.f / (end_time - start_time));
  }

  Serial.println("third for");
  float pressure_chamber[no_chambers];
  float correction = a0 + a1 * rpm[reliable_measurement] + a2 * sq(rpm[reliable_measurement]);
  for (int chamber = 0; chamber < no_chambers; ++chamber) {
    pressure_chamber[chamber] = (float) round((max_buffer[reliable_measurement][chamber] - 102.4) * bar_factor * dead_space) / 10.f;
  }

  strcpy(buf2, "BAR: ");
  for (int chamber = 0; chamber < no_chambers; ++chamber) {
    dtostrf(pressure_chamber[chamber], 3, 1, buf1);
    strcat(buf2, buf1);
    strcat(buf2, " ");
  }
  strcat(buf2, "RPM: ");
  itoa(rpm[reliable_measurement], buf1, 10);
  strcat(buf2, buf1);
  strcat(buf2, " BAR: ");
  for (int chamber = 0; chamber < no_chambers; ++chamber) {
    dtostrf(pressure_chamber[chamber] + correction, 3, 1, buf1);
    strcat(buf2, buf1);
    strcat(buf2, " ");
  }
  strcat(buf2, "@250 RPM");
  //lcd.begin(0x27, 16, 2);
  //lcd.backlight();
  lcd.setCursor(0, 1);
  lcd.print("Ergebnisse sind");
  lcd.setCursor(2, 2);
  lcd.print(buf2);
  Serial.println(buf2);
  delay(2000);
}
