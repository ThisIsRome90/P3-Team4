/*
* File name: main.ino
* Date: 08.01.2023
*
* This is the main Programm. It includes the MPPT Algorithm.
*
*
*/


/*  *   INCLUDE LIBRARIES   *   */
#include <stdint.h>
#include <stdlib.h>
#include <cstring>
#include <vector>
#include <SparkFun_ADS1015_Arduino_Library.h>  //Click here to get the library: http://librarymanager/All#SparkFun_ADS1015
#include <Wire.h>


/*  *   INCLUDE HEADERS   *  */
#include "lcd.h"
#include "interruptFunctions.h"
#include "bluetooth.h"


/*  *   DEFINES   *  */

//define LCD
#define TURN_OFF_TIME (20000)  //20s - Auschaltzeit einstellen  3min ->(180000)
#define ADRESS_ADC (0x48)


//define Input
#define BUTTON_DOWN (33)
#define BUTTON_UP (34)
#define BUTTON_BLUETOOTH (35) 


//define Output
#define SDA_I2C (25)
#define SCL_I2C (26)

#define MPPT_1 (12)
#define MPPT_2_INV (14)
#define TEMPPIN (27)


#define HYSTERESE (0.01)


/*  *   Global Variables*  */
int i = 0;

ADS1015 adcSensor1;

unsigned char *einheiten[] = {(unsigned char *)"V", (unsigned char *)"A", (unsigned char *)"W", (unsigned char *)"V", (unsigned char *)"A", (unsigned char *)"W ", (unsigned char *)"% ", (unsigned char *)"h ", (unsigned char *)"C"};
unsigned char *woerter[] = { (unsigned char *)"U-Panel:    ", (unsigned char *)"I-Panel:    ", (unsigned char *)"P-Panel:    ", (unsigned char *)"U-Batterie: ", (unsigned char *)"I-Batterie: ", (unsigned char *)"P-Batterie: ", (unsigned char *)"Ladezustand:", (unsigned char *)"Prognose:   ", (unsigned char *)"Temperatur: " };
char charstr[16];
double measurements[] = {0,0,0,0,0,0,0,0,0};

unsigned long timeLastAction = 0;
boolean displayOn = true;

TaskHandle_t displayHandle;
TwoWire I2Cone = TwoWire(0);

  
void setup() {
  //MPPT
  pinMode(MPPT_1, OUTPUT);
  pinMode(MPPT_2_INV, OUTPUT);
  digitalWrite(MPPT_1, LOW);
  digitalWrite(MPPT_2_INV, LOW);
  //MPPT

  
  pinMode(TEMPPIN, ANALOG);
  
  // LCD Initialization
  pinMode(SCL_I2C, OUTPUT);
  pinMode(SDA_I2C, OUTPUT);
  initLCD_I2C(SCL_I2C,SDA_I2C);

  setCursor(0x0);
  writeString((unsigned char *)" Starten... ");
  displayHell();

  bluetoothInit();

  Wire.begin();//SDA_I2C, SCL_I2C
  Serial.begin(115200);


  //  Setup the Inputpin with the buttons
  pinMode(BUTTON_DOWN, INPUT_PULLUP);
  pinMode(BUTTON_UP, INPUT_PULLUP);
  pinMode(BUTTON_BLUETOOTH, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON_DOWN), erhoehen, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_UP), verringern, RISING);
  attachInterrupt(digitalPinToInterrupt(BUTTON_BLUETOOTH), bluetoothOn, RISING);


  // ADC Initialization
    I2Cone.begin(SDA_I2C, SCL_I2C);
  if (adcSensor1.begin(ADRESS_ADC, I2Cone) == true) {
    Serial.println("Device found. I2C connections are good.");
    adcSensor1.setGain(ADS1015_CONFIG_PGA_1);
  }else{
    Serial.println("ADC NOT found !!!");
  }



  // anzeige Thread für anzeigen (LCD und Bluetooth)
  xTaskCreatePinnedToCore(
    display,              /* Task function. */
    "display",             /* name of task. */
    10000,                /* Stack size of task */
    NULL,                 /* parameter of the task (void *)measurements*/
    1,                    /* priority of the task */
    &displayHandle,       /* Task handle to keep track of created task */
    1);                   /* pin task to core 0 */
}


void display(void * values) {//
  Serial.print("Task1 running on core 1");

  while(1) {

  // messwerte einlesen
    TwoWire *I2Clightmeter; 
    I2Clightmeter = new TwoWire(1);
    I2Clightmeter->begin(SDA_I2C, SCL_I2C);// wire verbindung wird aufgebaut
    measurements[INDEX_VOLTAGE_PANEL] = (double)(adcSensor1.getSingleEnded(2))*(double)(0.002)*6.47;
    measurements[INDEX_CURRENT_PANEL] = (((double)adcSensor1.getSingleEnded(1))*(double)(0.002)-1.9202)/0.0862; 
    measurements[INDEX_POWER_PANEL] = measurements[INDEX_VOLTAGE_PANEL] * measurements[INDEX_CURRENT_PANEL];
    measurements[INDEX_VOLTAGE_BATTERY] = (double)(adcSensor1.getSingleEnded(3))*(double)(0.002)*4.95;
    measurements[INDEX_CURRENT_BATTERY] = (((double)adcSensor1.getSingleEnded(0))*(double)(0.002)-1.9202)/0.0862;
    measurements[INDEX_POWER_BATTERY] = measurements[INDEX_VOLTAGE_BATTERY] * measurements[INDEX_CURRENT_BATTERY];
    measurements[INDEX_LOAD_BATTERY] = ladezustand(measurements[INDEX_VOLTAGE_BATTERY], measurements[INDEX_CURRENT_BATTERY]);
    measurements[INDEX_TEMP] = analogRead(TEMPPIN) * 21/571;
    measurements[INDEX_FORECAST_BATTERY] = calcChargingForecast(measurements[INDEX_LOAD_BATTERY], measurements[INDEX_CURRENT_BATTERY]);
    measurements[INDEX_CURRENT_BATTERY] = (((double)adcSensor1.getSingleEnded(0))*(double)(0.002)-1.9202)/0.0862;
   
    // messwerte auf Serieller Schnittstelle ausgeben
    Serial.print("A0:");
    Serial.println(adcSensor1.getSingleEndedSigned(0));
    Serial.print("A1:");
    Serial.println(adcSensor1.getSingleEndedSigned(1));
    Serial.print("A2:");
    Serial.println(adcSensor1.getSingleEndedSigned(2));
    Serial.print("A3:");
    Serial.println(adcSensor1.getSingleEndedSigned(3));
    Serial.print("A0:-----");
    Serial.println(adcSensor1.getSingleEndedSigned(0));
    delete I2Clightmeter;

    sendBTall(measurements);

    // LCD Libary funktioniert nicht mit wire.h sonders setzt die I2C direkt,
    // desshalb müssen sie "uminizialisiert werden". Etwas unschön, wäre zeit nicht knapp könnte dies noch verbessert werden
    pinMode(SCL_I2C, OUTPUT);
    pinMode(SDA_I2C, OUTPUT);


    // 1. Messwert anzeigen
    setCursor(0x0);
    writeString(woerter[(i) % ANZ_MESSWERTE]);
    setCursor(0x0C);
    dtostrf(measurements[(i) % ANZ_MESSWERTE], 3, 3, charstr);
    writeString((unsigned char *)charstr);
    setCursor(0x11);
    writeString(einheiten[(i) % ANZ_MESSWERTE]);

    // 2. Messwert anzeigen
    setCursor(0x40);
    writeString(woerter[(i + 1) % ANZ_MESSWERTE]);
    setCursor(0x4C);
    dtostrf(measurements[(i + 1) % ANZ_MESSWERTE], 3, 3, charstr);
    writeString((unsigned char *)charstr);
    setCursor(0x51);
    writeString(einheiten[(i+1) % ANZ_MESSWERTE]);


    // 3. Messwert anzeigen
    setCursor(0x14);
    writeString(woerter[(i + 2) % ANZ_MESSWERTE]);
    setCursor(0x20);
    dtostrf(measurements[(i + 2) % ANZ_MESSWERTE], 3, 3, charstr);
    writeString((unsigned char *)charstr);
    setCursor(0x25);
    writeString(einheiten[(i+2) % ANZ_MESSWERTE]);

    // 4. Messwert anzeigen
    setCursor(0x54);
    writeString(woerter[(i + 3) % ANZ_MESSWERTE]);
    setCursor(0x60);
    dtostrf(measurements[(i + 3) % ANZ_MESSWERTE], 3, 3, charstr);
    writeString((unsigned char *)charstr);
    setCursor(0x65);
    writeString(einheiten[(i+3) % ANZ_MESSWERTE]);


    // wenn bluetooth eingeschalten ist wenn man sich mit dem Handy verbinden kann 
    // oder schon verbunden ist, wird ein B ober rechts angezeigt.
    setCursor(0x13);
    if(isBluetoothOn){
      writeString((unsigned char *)"B");
    }else{
      writeString((unsigned char *)" ");
    }

    if ((timeLastAction) + TURN_OFF_TIME < (millis())) {
      if (displayOn == true) {
        displayDunkel();
        tryCloseBTconnection();
        
        displayOn = false;
      }
    } else {
      if (displayOn == false) {
        displayHell();
        displayOn = true;
        bluetoothOn();
      }
    }
  }
  
  vTaskDelete(displayHandle);

}


void loop() {
  //MPPT

}


double ladezustand(double spannung, double strom){
  //double U100 = 1.464 * pow(2, -1*strom) + 11.2;
  double U100 = -0.443 * strom + 12.63;
  double U20 = U100 - 0.5;

  if(strom < HYSTERESE && strom  > -1 * HYSTERESE){
      U100 = 12.7;
      U20 = 12.2;
  }

  return abs(((spannung - U20) / 0.5) * 100);
}

double calcChargingForecast(double stateOfCharge, double batteryCurrent){
  const int capacity = 18;

  if(batteryCurrent > 0){
    return (100-stateOfCharge)/100*capacity/batteryCurrent;
  }else{
    return -1*stateOfCharge/100*capacity/batteryCurrent;
  }
}

// nur für debug
void text() {
  setCursor(0x0);
  writeString((unsigned char *)"hello");
}

