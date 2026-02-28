#include "TimerCametraTank.h"
#include <Wire.h>

#define I2C_ADDRESS 0x0f

uint8_t TimerCameraTankDriver::getID() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(0x00);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDRESS, 1, true);
    return Wire.read();
}

// バッテリー電圧を取得する。
uint16_t TimerCameraTankDriver::getVolt() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(0x01);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDRESS, 1, true);
    return (uint16_t)Wire.read() << 8 | (uint16_t)Wire.read();
}

// 起動経過時間(ms)を取得する。
uint32_t TimerCameraTankDriver::millis() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(0x03);
    Wire.endTransmission(false);
    Wire.requestFrom(I2C_ADDRESS, 4, true);
    uint32_t ms = Wire.read() << 8;
    ms = ms | Wire.read();
    ms <<= 8;
    ms = ms | Wire.read();
    ms <<= 8;
    ms = ms | Wire.read();
    return ms;
}

// 電源をOFFする。
void TimerCameraTankDriver::shutdown() {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(0x0f);
    Wire.endTransmission();
}

// モータを停止する。
void TimerCameraTankDriver::stop() {
    setAPWM(0);
    setBPWM(0);
}

// 左モータのスピードを設定する。
void TimerCameraTankDriver::setLeftMotor(int speed) {
    setAPAHSE(speed >= 0);

    uint8_t duty;
    if (speed > 255 || speed < -255) duty = 255;
    else if (speed >= 0) duty = (uint8_t)speed;
    else duty = (uint8_t)(-speed);
    setAPWM(duty);
}

// 右モータのスピードを設定する。
void TimerCameraTankDriver::setRightMotor(int speed) {
    setBPAHSE(speed < 0);

    uint8_t duty;
    if (speed > 255 || speed < -255) duty = 255;
    else if (speed >= 0) duty = (uint8_t)speed;
    else duty = (uint8_t)(-speed);
    setBPWM(duty);
}

// APAHSEを設定する。
void TimerCameraTankDriver::setAPAHSE(bool turn_on) {
    uint8_t cmd = 0x10;
    if (turn_on) cmd |= 0x01;
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(cmd);
    Wire.endTransmission();
}

// BPWMを設定する。
void TimerCameraTankDriver::setAPWM(uint8_t duty) {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(0x12);
    Wire.write(duty);
    Wire.endTransmission();
}

// BPAHSEを設定する。
void TimerCameraTankDriver::setBPAHSE(bool turn_on) {
    uint8_t cmd = 0x20;
    if (turn_on) cmd |= 0x01;
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(cmd);
    Wire.endTransmission();
}

// BPWMを設定する。
void TimerCameraTankDriver::setBPWM(uint8_t duty) {
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(0x22);
    Wire.write(duty);
    Wire.endTransmission();
}

// FRONTを設定する。
void TimerCameraTankDriver::setFRONT(bool turn_on) {
    uint8_t cmd = 0x30;
    if (turn_on) cmd |= 0x01;
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(cmd);
    Wire.endTransmission();
}

// BACKを設定する。
void TimerCameraTankDriver::setBACK(bool turn_on) {
    uint8_t cmd = 0x32;
    if (turn_on) cmd |= 0x01;
    Wire.beginTransmission(I2C_ADDRESS);
    Wire.write(cmd);
    Wire.endTransmission();
}

TimerCameraTankDriver Tank;
