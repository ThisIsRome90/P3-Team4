
#include "BluetoothSerial.h"

#define  INDEX_VOLTAGE_PANEL  (0)
#define  INDEX_CURRENT_PANEL  (1)
#define  INDEX_POWER_PANEL  (2)
#define  INDEX_VOLTAGE_BATTERY  (3)
#define  INDEX_CURRENT_BATTERY  (4)
#define  INDEX_POWER_BATTERY  (5)
#define  INDEX_LOAD_BATTERY  (6)
#define  INDEX_FORECAST_BATTERY  (7)
#define  INDEX_TEMP  (8)


BluetoothSerial SerialBT = BluetoothSerial();
boolean isBluetoothOn = false;

// initialisierung
void bluetoothInit() {
  SerialBT.begin("ESP32_babuskaFamily");  //Name des ESP32
  isBluetoothOn = true;
}

// bluetooth einschalten
void bluetoothOn() {
  if(!isBluetoothOn){
    bluetoothInit();
  }
}

// sendet einen einzelnen Messwert
void sendBT(double value) {
  String s = String(value, 3);
  SerialBT.print(s + "$");
}

// sendet alle Messwerte reihenfolge wichtig
void sendBTall(double measurements[]){
  sendBT(measurements[INDEX_VOLTAGE_PANEL]);
  sendBT(measurements[INDEX_CURRENT_PANEL]);
  sendBT(measurements[INDEX_POWER_PANEL]);
  sendBT(measurements[INDEX_VOLTAGE_BATTERY]);
  sendBT(measurements[INDEX_CURRENT_BATTERY]);
  sendBT(measurements[INDEX_POWER_BATTERY]);
  sendBT(measurements[INDEX_LOAD_BATTERY]);
  sendBT(measurements[INDEX_FORECAST_BATTERY]);
  sendBT(measurements[INDEX_TEMP]);
}

void tryCloseBTconnection(){
  // thread starten und connection beenden wenn client weg
  // geplant aber nicht umgesetzt
    if(!SerialBT.hasClient()){
      SerialBT.end();
      isBluetoothOn = false;
    }
}