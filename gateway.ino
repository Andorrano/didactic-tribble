#include <U8x8lib.h>       // Ug8g2 by oliver https://github.com/olikraus/u8g2 . Needed for display
#include <LoRa.h>          // Lora by Sandeep Mistry https://github.com/sandeepmistry/arduino-LoRa
                           // See ~/Arduino/libraries/LoRa/API.md for help on LoRa. functions
#include <SPI.h>           // for Lora
#include <WiFi.h>          // from folder ~/Arduino/hardware/heltec/esp32/libraries/WiFi/src/
#include <HTTPClient.h>    // for HTTP POST, from folder ~/Arduino/hardware/heltec/esp32/libraries/WiFi/src/
 
U8X8_SSD1306_128X64_NONAME_SW_I2C u8x8(/* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);  // Define pins (15,4,16) used for OLED -> global object 'u8x8'
 
 
// ****************************************************************
// SETUP
// ****************************************************************
void setup() {
 
// *** Serial ***
   Serial.begin(115200);
   //while (!Serial);  // makes the board pause until you open the serial port, so you get to see that initial bit of data
 
 
// *** Display ***
   u8x8.begin();
   u8x8.setFont(u8x8_font_8x13B_1x2_r);   // 4 lines on display -> setCursor 0,2,4,6
 
// *** LoRa ***
   LoRa.setPins(18, 14, 31); //or 26 ?    // defines the pins used for Lora module (ss,rst,dio0) [default 10,9,2]
   LoRa.setSpreadingFactor(7);            // 7-12 [default 7]
   LoRa.setSyncWord(0x9A);                // sync word must match with sender [default 0x34]
   LoRa.setSignalBandwidth(125000);       // [default 125000] in Hz. For other supported values, see API.md
   while (!LoRa.begin(869525000)) { Serial.println("Waiting on Lora");delay(500); }    // start Lora at 869.525MHz and wait until available
   Serial.println("Lora OK: 869.525 MHz | SF 7 | SyncWord 0x9A");
 
 
// *** Wifi ***
   WiFi.begin("ssid", "password");
   while (WiFi.status() != WL_CONNECTED) { Serial.println(WiFi.status());u8x8.setCursor(0,0); u8x8.print("Waiting for Wifi");delay(1000); }
   Serial.print("WiFi connected. IP address: "); Serial.println(WiFi.localIP());
   u8x8.setCursor(0,0); u8x8.print(WiFi.localIP());
   delay(500);
}
 
 
 
// ****************************************************************
// LOOP
// ****************************************************************
void loop() {
   // One loop takes about 20ms. Shouldn't exceed about 100ms, otherwise it could miss arriving Lora packets
   String receivedChar;
   String receivedText;
   String receivedRssi;
 
   // *** Check for LoRa packet ***
   if (LoRa.parsePacket()) { // returns 0 if no packet or packet size in bytes (one byte per character, ex. john = 4 bytes)
      // this if-condition is only entered if a LoRa packet has been received
      while (LoRa.available()) {  // returns number of remaining bytes to read, starts with packet size (ex. 4 bytes) and decrease with every read()
         // this while-loop is needed to read the LoRa message byte by byte
         receivedChar = (char)LoRa.read(); // reads the next byte from the packet (ex. 'j' from john)
         char currentid[64]; receivedChar.toCharArray(currentid, 64);  // ? doesn't seem to make anything (ex. still 'j')
         receivedText += receivedChar;     // compose received text
      }
      receivedRssi = LoRa.packetRssi();       // returns the RSSI of the received packet
      char currentrs[64]; receivedRssi.toCharArray(currentrs, 64);  // ? doesn't seem to make anything
      // *** Print info on Screen ***
      u8x8.clear(); u8x8.print(receivedText); // clear OLED and set cursor to home and print received text
      u8x8.setCursor(0,2); u8x8.print("RSSI ");u8x8.print(receivedRssi);
      Serial.print(receivedText);Serial.print(", RSSI ");Serial.println(receivedRssi);
      // *** Send HTTP Post ***
      httpPost(receivedText);
   }
}
 
 
// ****************************************************************
// HTTP Post
// ****************************************************************
void httpPost(String dataToSend) {
   if (WiFi.status()== WL_CONNECTED) {
      HTTPClient http;
      http.begin("http://myServer.com:8080/upload");                                                   // destination for HTTP request
      //http.addHeader("Content-Type", "text/plain");                                                  // content-type header
      int httpResponseCode = http.POST("password=secret&data="+dataToSend);                            // Send POST request
      if (httpResponseCode>0) {
         u8x8.setCursor(0,4); u8x8.print(httpResponseCode); u8x8.print(" "); u8x8.print(http.getString()); }
         // Print return code (ex. 200 or 400) and request answer (ex. OK  or Bad Request (HTML-Page :-)
      else {
         u8x8.setCursor(0,4); u8x8.print("Error POST: "); u8x8.print(httpResponseCode); }
         // ex. -1 (if no server running)
      http.end(); }                                                                                    // Free resources
   else {    u8x8.setCursor(0,4); u8x8.print("Error WiFi"); }
}
