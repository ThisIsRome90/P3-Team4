/*
* File name: interruptFunctions.h
* Date: 08.01.2023
*
* This file includes the function which get executed (via interrupt) when a button getspressed.
*
*
*/

#define ANZ_MESSWERTE (9)

extern int i;
extern unsigned long  timeLastAction;

// aufwärts button gedrückt wurde
void verringern() {
  static unsigned long lastInterrupt = 0;
  if ((millis() - lastInterrupt) > 100) {
    lastInterrupt = millis();
    i = i - 1;
    if (i == -1) {
      i = ANZ_MESSWERTE-1;
    }
    timeLastAction = millis();
  }
}

// abwärts button gedrückt wurde
void erhoehen() {
  static unsigned long lastInterrupt = 0;
  if ((millis() - lastInterrupt) > 100) {
    lastInterrupt = millis();
    i = abs(i + 1) % ANZ_MESSWERTE;

    timeLastAction = millis();
  }
}