#include <U8x8lib.h>       // Ug8g2 by oliver https://github.com/olikraus/u8g2 . Needed for display
#include <LoRa.h>          // Lora by Sandeep Mistry https://github.com/sandeepmistry/arduino-LoRa
                           // See ~/Arduino/libraries/LoRa/API.md for help on LoRa. functions
#include <SPI.h>           // needed for Lora
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);  // Define pins (15,4,16) used for OLED


// ****************************************************************
// SETUP
// ****************************************************************
void setup() {

// *** Serial ***
   Serial.begin(115200);
   //while (!Serial);  // makes the board pause until you open the serial port, so you get to see that initial bit of data

  
// *** Display ***
   u8x8.begin();
   u8x8.setFont(u8x8_font_8x13B_1x2_r);  // 4 lines on display -> setCursor 0,2,4,6

// *** Lora ***
   LoRa.setPins(18, 14, 26);             // defines the pins used for Lora module (ss,rst,dio0)
   LoRa.setSpreadingFactor(7);           // 7-12 [default 7]
   LoRa.setSyncWord(0x9A);               // sync word must match with sender [default 0x34]
   LoRa.setSignalBandwidth(125000);      // [default 125000] in Hz. For other supported values, see API.md
   while (!LoRa.begin(869525000)) { Serial.println("Waiting on Lora");delay(500); }    // start Lora at 869.525MHz and wait until available
   Serial.println("Lora read: 869.525 MHz | SF 7 | SyncWord 0x9A");

}


// ****************************************************************
// LOOP
// ****************************************************************
void loop() {

  //int dbValueArray[10];  // Array for 10 Elements (index 0..9)
  //int dbValueMax = dbValueArray.Max;  NOT Working on Arduino
  String transmitString;
  u8x8.clear();
  for (int k=1; k<=6; k++) {       // Compose transmitString from six 10-second blocks
    int dbValueMax = 0;            // Reset dbValueMax after one 10-second block
    for (int i=1; i<=80; i++) {    // Take the maximum db value of 80 125ms-measurements = 10s
      float  voltageValue = ReadVoltage(32);      // Number of analog GPIO pin. See function below.
      float  dbValueFloat = voltageValue * 50.0 - 1.0;  // convert voltage to decibel value (50.0 is the sensitivity factor of the sensor)
      int    dbValueInt      = round(dbValueFloat);     // -1.0 according to calibration with NTI XL2 Audio device (5.4.2019)
      dbValueMax = max(dbValueMax,dbValueInt);
      // *** Display ***
      u8x8.setCursor(0,0); u8x8.print(voltageValue,2); u8x8.print(" V ");
      u8x8.setCursor(0,2); u8x8.print(dbValueInt);     u8x8.print(" dBA ");
      u8x8.setCursor(0,4); u8x8.print(dbValueMax);     u8x8.print(" dBA ");
      delay(19);   // This value must be set by trial and error until one pass takes 125ms.
                   // This is given by the hardware characteristic of the SoundLevelMeasurement device.
                   // To measure, look at the elapsed time beetween LoRa messages. It should be one minute.
                   // delay =  125ms without OLED. The code execution is almost instantly
                   //         ~ 19ms with    OLED. The display strongly slows down the execution
                   //                              This value must be adjusted on every change on the display!
    }
    transmitString = transmitString + String(dbValueMax,HEX);
    u8x8.setCursor(0,6); u8x8.print(transmitString);
  }
  // *** Send LoRa packet ***
  LoRa.beginPacket(); LoRa.print(transmitString); LoRa.endPacket();
}


// ****************************************************************
// ReadVoltage (improvement of analogRead() )
// ****************************************************************
// From https://github.com/G6EJD/ESP32-ADC-Accuracy-Improvement-function
// Call function with 'ReadVoltage(32)'. It improves 'analogRead(32) / 4096.0 * 3.3' which is not accurate.
double ReadVoltage(byte pin){
   double reading = analogRead(pin); // Reference voltage is 3v3 so maximum reading is 3v3 = 4095 in range 0 to 4095
   if(reading < 1 || reading > 4095) return 0;
   return -0.000000000000016 * pow(reading,4) + 0.000000000118171 * pow(reading,3)- 0.000000301211691 * pow(reading,2)+ 0.001109019271794 * reading + 0.034143524634089;
}
