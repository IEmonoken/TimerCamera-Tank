#include <Wire.h>
#include "TimerCametraTank.h"

void setup() {
  Wire.begin();   // 
  delay(100);
}

void loop() {
  Tank.setLeftMotor(192);
  Tank.setRightMotor(0);
  Tank.setFRONT(true);
  Tank.setBACK(false);
  delay(1000);
  Tank.setLeftMotor(0);
  Tank.setRightMotor(192);
  Tank.setFRONT(false);
  Tank.setBACK(true);
  delay(1000);
}
