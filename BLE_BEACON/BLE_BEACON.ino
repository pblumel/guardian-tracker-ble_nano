//******************************************************************************************************
//******************************************************************************************************
//TITLE: Project Guardian - Child Unit Source Code 
//Version: 1.0
//Authors: Zachary Plato, Philip Blumel and Colton Weese

//Description: 
/* The child unit acts as and is a BLE beacon, continually broadcasting unique BLE packets to be picked 
 * up and read by the parent unit. The child unit software is developed using RedBearLabs Application 
 * Program Interface (API) for the Arduino Integrated Development Environment (IDE).  The software is 
 * based off the event-driven programming approach to keep power consumption as small as possible. 
 * The process behind this event driven approach goes as follows: sleep (run at low power consumption) 
 * until an event, wake up when an event has happened (i.e., software timer overflow), then process the 
 * event and goes back to sleep until the next event. The child unit has two programmed events: 
 * (1) Advertise Packet 
 * (2) Sample Power Button. 
 */
 
//Revision History:
//||=============================================================================================||
//||---VERSION---++-----------------------DESCRIPTION-----------------------++--DATE (MM/DD/YY)--||
//||Version 1.0  ++ This Version is the first working build of the child    ++     11/26/17      ||
//||             ++ unit source code. This version is released as is,       ++                   ||
//||             ++ without liability.                                      ++                   ||
//||-------------++---------------------------------------------------------++-------------------||
//||=============================================================================================||

//******************************************************************************************************
//******************************************************************************************************
 
#include <BLE_API.h> //library header included from RedBearLabs BLE NANO

#define ADVERTISING_INTERVAL  0.1      // 100ms 
#define ADVERTISING_TIMEOUT   0.001    // Time in seconds to advertise (0 = Set to infinity)
#define TX_POWER              4        // -20 to +4 dBm as defined in the datasheet
#define BATT_SAMPLE_RATE      60       // 60 seconds 
#define BATT_SAMPLES          10       // sample ADC 10 times  
#define BUTTON_SAMPLE_RATE    0.1      // sample rate of 100ms 
#define BUTTON_HOLD_TIME      5        // 5 seconds

/* ISR PROTOTYPES */
void sendAdvertisement_ISR(); //Called Every Advertising Interval to Send BLE Packet 
void btn_ISR();               //Called every Buttno Sample Rate to Sample if the button has been pressed 

/* FUNCTION PROTOTYPES */
void updateBattLevel();
void updatePayload(); 

/* GLOBAL VARIABLES DECLARATIONS AND DEFINITIONS */
BLEDevice ble;                            //declare the BLEDevice class ble
Ticker ticker_advertise,ticker_btnsample; //declare ticker interrupts 

//Initialize Bluetooth Packet Payload
uint8_t beaconPayload[] = {
                            0x42,   // Unique Identifier (Must have >1 Byte matching sequence for Android 8.0)
                            0x42,
                            0x00    // Battery level
                          };

const uint8_t device_name[] = "Biscuit";      // Declare Device Name 
bool standby = LOW;                           // Don't start in standby
const int BtnInput = D0;                      // Button Input Pin D0 
const int BattInput = A3;                     // Button Input Pin A3
int btn_samples = 0;                          // Button Samples Counter
int BattLevelWait = 0;                        // Time since last battery update
uint8_t BattLevel = 100;                      // Battery level measured in percent


/* Interrupt Service Routines */
void sendAdvertisement_ISR()
{ 
  //if we are not in standby
  if(!standby)
  {
     ble.startAdvertising(); 
     BattLevelWait++; 
     //wait for the BATT_SAMPLE_RATE defined above, 10 for scaling
     if (BattLevelWait == (BATT_SAMPLE_RATE)*10)
     {
      updateBattLevel();
      updatePayload();
      BattLevelWait = 0; //reset wait counter
     }
  } 
  else //we are in standby 
  {
    ble.stopAdvertising();  //stop advertising since we are in standby
    BattLevelWait = 0;      //reset wait counter
  } 
}

void btn_ISR()
{ 
  //if we have pressed the button
  if(digitalRead(BtnInput))
  {
    btn_samples++; 
    //if we have pressed it for (BUTTON_HOLD_TIME)
    if (btn_samples == (BUTTON_HOLD_TIME*10)) //10 is a scaling factor
    { 
      standby = !standby; //toggle standby 
      digitalWrite(LED, !digitalRead(LED));  //toggle LED
      btn_samples = 0; 
    }
  }
  else
    btn_samples = 0; 
}


/* FUNCTIONS */ 
//Update the Bluetooth Packets Payload 
void updatePayload()
{
    ble.stopAdvertising();                                                                                                  // Stop Broadcasting Packets
    ble.clearAdvertisingPayload();                                                                                          // Clear Packet Payload
    ble.accumulateAdvertisingPayload(GapAdvertisingData::SHORTENED_LOCAL_NAME, 
    device_name, sizeof(device_name)-1);   // Packet Data Unit 1
    ble.accumulateAdvertisingPayload(GapAdvertisingData::MANUFACTURER_SPECIFIC_DATA, 
    beaconPayload, sizeof(beaconPayload)); // Packet Data Unit 2
    ble.startAdvertising();                                                                                                 // Start Advertising 
}

//Updates the battery level by sampling and averaging the ADC input pin from the battery BattInput 
void updateBattLevel()
{
  int BattLevelSum = 0; //Holds the Sum of ADC Samples
  
  //Sum over BATT_SAMPLES readings
  for (int i = 0; i < BATT_SAMPLES; i++) { BattLevelSum += analogRead(BattInput); }
  //Average Samples and Interpret Voltage
  BattLevel = (((BattLevelSum/BATT_SAMPLES)-740)*9 >> 4); //Avoid floating point notation
  //Interpreted voltage is based on 3.7V Li-Poly battery with range 4.2 - 3.0 (full - empty) 

  //Set Upper and Lower Battery Level Rails (accounts for small discrepancies)
  if (BattLevel < 0)
    BattLevel = 0;
  else if (BattLevel > 100)
    BattLevel = 100;
     
  beaconPayload[sizeof(beaconPayload)-1] = BattLevel;  // Write battery percent to payload
}

/* SETUP - CALLED ONCE UPON INITIAL RUNTIME */
void setup() 
{
  /* PERIPHERIAL SETTINGS */
  NRF_POWER->DCDCEN = 0x00000001; //ENABLE DC/DC REGULATOR  
  NRF_POWER->TASKS_LOWPWR = 1;    //ENABLE LOW POWER MODE
  NRF_TIMER1->POWER = 0;          //TURN TIMER1 POWER OFF
  NRF_TIMER2->POWER = 0;          //TURN TIMER2 POWER OFF
  NRF_UART0->POWER  = 0;          //TURN UART POWER OFF
  NRF_UART0->TASKS_STOPTX = 1;    //STOP UART Tx 
  NRF_UART0->TASKS_STOPRX = 1;    //STOP UART Rx
  NRF_UART0->ENABLE = 0;          //DISABLE UART

  /* INPUTS */
  pinMode(BtnInput,INPUT);
  pinMode(BattInput,INPUT);

  /* OUTPUTS */
  pinMode(LED, OUTPUT);
  digitalWrite(LED,LOW);  // LED active LOW
                          // Default on if not in standby by default
  /* INTERRUPTS */
  ticker_advertise.attach(sendAdvertisement_ISR,ADVERTISING_INTERVAL); 
  ticker_btnsample.attach(btn_ISR,BUTTON_SAMPLE_RATE); 

  /* BLE STACK INIT */
  ble.init();
  ble.setAdvertisingType(GapAdvertisingParams::ADV_NON_CONNECTABLE_UNDIRECTED);  //Not Connectable
  ble.gap().setTxPower(TX_POWER);
  ble.setAdvertisingInterval(ADVERTISING_INTERVAL*1000);  
  ble.setAdvertisingTimeout(ADVERTISING_TIMEOUT); // Set adv_timeout, in seconds

  /* PAYLOAD INIT */
  updateBattLevel();
  updatePayload(); 
}

/* MAIN LOOP */ 
void loop() 
{
    ble.waitForEvent(); //sleep - doesnt stop advertising
}
}
