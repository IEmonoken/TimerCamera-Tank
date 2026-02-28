# ドライバ基板について
ドライバ基板はTimerCameraに電源を供給し、TimerCameraからの指令に従ってタンクを動作させる。

## 機能
ドライバ基板はTimerCameraとGROVEインタフェースで接続され、I2Cデバイスとして動作する。
1. 電源制御:　1セルのリポバッテリーを搭載し、TimerCameraに電源を供給する。またバッテリー電圧が低下したらLEDが早く点灯し充電を促す。
1. モータドライバ:　PWMによりキャタピラを駆動するDCモータの回転速度を変える。
1. LEDスイッチ:　タンク前後のLEDを点灯/消灯する。

## プログラミング
Arduino IDEでコードを編集し、プログラムをTimerCameraTankにアップロードする。

### 関数
- 左右のDCモータのスピードを-255から255までの整数で設定する。0でDCモータは停止し、255で最高速度となる。また負の数値を設定するとDCモータは逆転する。
```
// speedは-255から255までの整数。範囲を超えた数値を指定すると最高速度となる。
Tank.setLeftMotor(int speed);
Tank.setRightMotor(int speed);
```
- タンク前後のLEDを点灯/消灯する。
```
// turn_onがtrueでLEDが点灯する。turn_onがfalseでLEDが消灯する。
Tank.setFRONT(bool turn_on);
Tank.setBACK(bool turn_on);
```
