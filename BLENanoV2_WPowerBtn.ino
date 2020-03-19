/*
 * Copyright (c) 2016 RedBear
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
#include <nRF5x_BLE_API.h>

BLEDevice ble;

const uint8_t device_name[] = "Biscuit";

/* Original IBeacon Packet Payload
const static uint8_t beaconPayload[] = {
  0x4C,0x00,                                                                       // Company Identifier Code = Apple
  0x02,                                                                            // Type = iBeacon
  0x15,                                                                            // Following data length
  0x71,0x3d,0x00,0x00,0x50,0x3e,0x4c,0x75,0xba,0x94,0x31,0x48,0xf1,0x8d,0x94,0x1e, // Beacon UUID
  0x00,0x00,                                                                       // Major
  0x00,0x00,                                                                       // Minor
  0xC5                                                                             // Calibrated RSSI@1m 
};*/

//Project Guardian Packet Payload 
uint8_t beaconPayload[] = {
  0x00,   // Must have at least one byte of UUID
  0xC5,   // Calibrated RSSI@1m
  0x00    // Battery level
};

bool standby = LOW;
const byte BtnInput = D0;
bool BtnStatus = LOW;
int BattPin = A4;
uint8_t BattLevel = 100;

void setup() {

  // close peripheral power
  NRF_POWER->DCDCEN = 0x00000001; 
  //NRF_POWER->SYSTEMOFF = 1; 
  NRF_POWER->TASKS_LOWPWR = 1; //enable low power
  NRF_UART0->TASKS_STOPTX = 1;
  NRF_UART0->TASKS_STOPRX = 1;
  NRF_UART0->ENABLE = 0;


  //Button Pin
  pinMode(BtnInput,INPUT);

  //LED Pin
  pinMode(LED, OUTPUT);
  digitalWrite(LED,HIGH); //default on (beacon defaults on)
  
  // set advertisement
  ble.init(); 
  ble.setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);  //Not Connectable
  ble.gap().setTxPower(4); //set Tx power - (-20 to +4 dbm as defined in the datasheet) 
  ble.setAdvertisingInterval(400); //broadcast a packet every 200ms 
  ble.setAdvertisingTimeout(0);   // set adv_timeout, in seconds 
}

void UpdatePacket()
{
      /* Start/Restart advertising with new battery level */
    ble.stopAdvertising();
    ble.clearAdvertisingPayload();
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME, device_name, sizeof(device_name)-1);         // PDU1
    ble.accumulateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA, beaconPayload, sizeof(beaconPayload)); // PDU2
    ble.startAdvertising();
}

//button interrupt 
void handle_irq1()
{

}

void loop() {

  BtnStatus = digitalRead(BtnInput);                      // Read button (Pressed = HIGH)
  delay (5000);                                           // Wait to sample again
  standby = standby^(BtnStatus & digitalRead(BtnInput));  // Toggle standby if button is still pressed
  digitalWrite(LED,standby);                              // Set LED
  
  if (standby)
  {
    //ble.stopAdvertising();
  }
  else
  {
    BattLevel = ((analogRead(BattPin)-740)*9) >> 4;
    if (BattLevel < 0)
      BattLevel = 0;
    else if (BattLevel > 100)
      BattLevel = BattLevel;
      
    beaconPayload[2] = BattLevel;             // Read in lower byte of battery voltage
    UpdatePacket(); 
  }
  
}

