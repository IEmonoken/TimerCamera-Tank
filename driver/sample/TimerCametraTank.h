#include <Arduino.h>

#ifndef _TimerCameraTankDriver
#define _TimerCameraTankDriver
 
#define I2C_ADDRESS 0x0f

class TimerCameraTankDriver {
public:
    uint8_t getID();                // IDを取得する。
    uint16_t getVolt();             // バッテリー電圧を取得する。
    uint32_t millis();              // 起動経過時間(ms)を取得する。
    void shutdown();                // 電源をOFFする。
    void stop();                    // モータを停止する。
    void setLeftMotor(int speed);   // 左モータのスピードを設定する。
    void setRightMotor(int speed);  // 右モータのスピードを設定する。
    void setAPAHSE(bool turn_on);   // APAHSEを設定する。
    void setAPWM(uint8_t duty);     // BPWMを設定する。
    void setBPAHSE(bool turn_on);   // BPAHSEを設定する。
    void setBPWM(uint8_t duty);     // BPWMを設定する。
    void setFRONT(bool turn_on);    // FRONTを設定する。
    void setBACK(bool turn_on);     // BACKを設定する。
};

extern TimerCameraTankDriver Tank;
    
#endif
