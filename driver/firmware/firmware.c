#include <xc.h>

// CONFIG1
#pragma config FOSC = INTOSC    // 内部オシレーター使用 CLKINピンはI/Oとして
#pragma config WDTE = OFF       // WDT無効
#pragma config PWRTE = ON       // PWRT有効
#pragma config MCLRE = ON       // RA5をMCLRピンとして使用
#pragma config CP = OFF         // コードプロテクト無効
#pragma config CPD = OFF        // データコードプロテクト無効
#pragma config BOREN = ON       // BOD有効
#pragma config CLKOUTEN = OFF   // クロックを外部に出力しない。
#pragma config IESO = OFF       // 外部クロックなし。クロック切り替え無効
#pragma config FCMEN = OFF      // 外部クロックなし。クロック切り替え無効

// CONFIG2
#pragma config WRT = OFF        // フラッシュメモリ 書き込みプロテクト無効
#pragma config PLLEN = OFF      // PLL無効
#pragma config STVREN = ON      // スタックオーバーフローやスタックアンダーフローでリセット
#pragma config BORV = HI        // BORは2.5Vでリセット
#pragma config LVP = OFF        // LVP無効

#define _XTAL_FREQ      8000000
#define I2C_ADDRESS     0x0f
#define RX_BUF_SIZ      4

// pic16f1827ピン割り当て
// RA0はAN0
#define APAHSE      LATAbits.LATA1      // DRV8835のAPAHSE
#define BPAHSE      LATAbits.LATA2      // DRV8835のBPAHSE
#define APWM        LATAbits.LATA3      // DRV8835のAENABLE
#define BPWM        LATAbits.LATA4      // DRV8835のBENABLE
// RA5はMCLR
#define FRONT       LATAbits.LATA6      // FRONTトランジスタ
#define BACK        LATAbits.LATA7      // BACKトランジスタ
#define BAT_FET     LATBbits.LATB0      // 電源FET
// RB1はSDA
#define AN0_EN      LATBbits.LATB2      // AN0入力は0で有効
#define POWER_SW    PORTBbits.RB3       // 電源スイッチ
// RB4はSCL
#define LED         LATBbits.LATB5      // LED

// ADCシーケンス用
typedef enum {adc_standby = 0, adc_fvr_start, adc_start, adc_go, adc_read} adc_state;

// 共用体でuint32_tをバイト列に分割する
typedef union {
    uint32_t data;
    uint8_t bytes[4];    // xc8コンパイラ(picマイコン)は基本的にリトルエンディアン。
} uint32_packer;

// 関数宣言
void initalize();
int loop();
void finalize();
void i2c_client_isr();
void adc_sequencer();
int check_power_sw();

// グローバル変数
volatile uint32_t syscnt;   // システム時計
volatile uint16_t volt;     // AN0の計測値

uint8_t duty_a = 0;         // DRV8835・AOUTのPWM
uint8_t duty_b = 0;         // DRV8835・BOUTのPWM

uint32_t led_limit;
uint32_t led_interval = 500;
uint32_t adc_limit;
uint32_t adc_interval = 5000;

volatile int cmd_enable;
uint8_t cmd[RX_BUF_SIZ];

// 割り込みハンドラ
void interrupt isr(void) {
    if (PIR1bits.TMR2IF != 0) {
        PIR1bits.TMR2IF = 0;
        syscnt++;
    }
    if (PIR1bits.SSP1IF != 0) {
        PIR1bits.SSP1IF = 0;
        i2c_client_isr();
    }
    if (PIR3bits.TMR4IF != 0) {
        PIR3bits.TMR4IF = 0;
        CCPR3L = duty_a;        // PWMは0から255まで
    }
    if (PIR3bits.TMR6IF != 0) {
        PIR3bits.TMR6IF = 0;
        CCPR4L = duty_b;        // PWMは0から255まで
    }
}

// メイン
void main(void) {
    initalize();
    while (loop());     // loop関数の実行
    finalize();
    for (;;);
}

// 開始処理
void initalize() {
    uint8_t dummy;

    OSCCON = 0b01110010;    // 内部オシレータ使用、8MHzクロック

    // ポートの設定
    ANSELA = 0b00000001;        // RA0のみアナログ入力に設定
    ANSELB = 0b00000000;        // すべてデジタルピン
    PORTA = 0;
    PORTB = 0;
    TRISA  = 0b00100001;        // RA1-2:右モータ,RA3-4:左モータ,RA6:バック,RA7:フロント
    TRISB  = 0b11011010;        // RB0:電源EN,RB5:電源スイッチ
    BAT_FET = 1;                // 電源FETをONにする。
    AN0_EN = 1;                 // ADC入力無効

    // タイマ2はシステムを動作させるために1ms周期の割り込みを発生させる。
    T2CONbits.T2CKPS = 0b10;    // 1/16プリスケーラ
    PR2 = 124;
    TMR2 = 0;
    PIR1bits.TMR2IF = 0;
    PIE1bits.TMR2IE = 1;        // タイマー2割込みを許可する
    T2CONbits.TMR2ON = 1;       // タイマ2 ON

    // タイマ4はCCP3にクロックを与え、PWM信号を発生させる。
    T4CONbits.T4CKPS = 0b10;    // 1/16プリスケーラ
    PR4 = 255;
    TMR4 = 0;
    PIR3bits.TMR4IF = 0;
    PIE3bits.TMR4IE = 1;        // タイマー4割込みを許可する

    // タイマ6はCCP4にクロックを与え、PWM信号を発生させる。
    T6CONbits.T6CKPS = 0b10;    // 1/16プリスケーラ
    PR6 = 255;
    TMR6 = 0;
    PIR3bits.TMR6IF = 0;
    PIE3bits.TMR6IE = 1;        // タイマー6割込みを許可する

    // CCP3のベースタイマをタイマ4に、CCP4のベースタイマをタイマ6に指定
    CCPTMRS = 0b10010000;   

    // I2Cクライアント処理を開始する。
    SSP1ADD = I2C_ADDRESS << 1; // 7ビットアドレスを左詰めで設定する
    SSP1STAT = 0xC0;            // 100kbpsスルーレート、SMバス規格対応
    SSP1CON1 = 0x36;            // I2Cスレーブモード、7ビットアドレス

    SSP1CON2bits.SEN = 1;       // ストレッチを許可
    SSP1CON3bits.PCIE = 1;      // ストップビット検出割り込みを許可
    SSP1CON3bits.SCIE = 0;
    SSP1CON3bits.AHEN = 0;
    SSP1CON3bits.DHEN = 0;
    dummy = SSP1BUF;

    PIR1bits.SSP1IF = 0;
    PIE1bits.SSP1IE = 1;        // I2C割込みを許可する

    // 割り込み許可
    INTCONbits.PEIE = 1;
    INTCONbits.GIE = 1;

    syscnt = 0;
    led_limit = 500;
    led_interval = 500;
    cmd_enable = 0;
}

// 主処理
int loop() {
    static uint8_t led = 0;

    // LED点滅
    if (syscnt >= led_limit) {
        led_limit += led_interval;
        led ^= 1;
        LED = led;
    }
    // GROVEによるコマンド操作
    if (cmd_enable > 0) {
        cmd_enable = 0;
        switch (cmd[0]) {
        case 0x0f:         // 電源をOFFする。
            finalize();
            break;

        case 0x10:         // APAHSEをOFFする。
            APAHSE = 0;
            break;

        case 0x11:         // APAHSEをONする。
            APAHSE = 1;
            break;

        case 0x12:         // APWMを設定する。
            duty_a = cmd[1];
            T4CONbits.TMR4ON = 0;       // タイマ4 Off
            if (duty_a == 255 || duty_a == 0) {
                CCP3CONbits.CCP3M = 0b0000; // CCP3 off (resets ECCP3 module)
                APWM = (duty_a == 0) ? 0 : 1;   // RA3をデジタルポートとして使用する
            }
            else {
                CCPR3L = duty_a;
                CCP3CONbits.CCP3M = 0b1100; // CCP3設定 (PWMモード)
                TMR4 = 0;
                T4CONbits.TMR4ON = 1;       // タイマ4 ON
            }
            break;

        case 0x20:         // BPAHSEをOFFする。
            BPAHSE = 0;
            break;

        case 0x21:         // BPAHSEをONする。
            BPAHSE = 1;
            break;

        case 0x22:         // BPWMを設定する。
            duty_b = cmd[1];
            T6CONbits.TMR6ON = 0;       // タイマ6 Off
            if (duty_b == 255 || duty_b == 0) {
                CCP4CONbits.CCP4M = 0b0000; // CCP4 off (resets ECCP3 module)
                APWM = (duty_b == 0) ? 0 : 1;   // RA3をデジタルポートとして使用する
            }
            else {
                CCPR4L = duty_b;
                CCP4CONbits.CCP4M = 0b1100; // CCP4設定 (PWMモード)
                TMR6 = 0;
                T6CONbits.TMR6ON = 1;       // タイマ6 ON
            }
            break;

        case 0x30:         // FRONTをOFFする。
            FRONT = 0;
            break;

        case 0x31:         // FRONTをONする。
            FRONT = 1;
            break;

        case 0x32:         // BACKをOFFする。
            BACK = 0;
            break;

        case 0x33:         // BACKをONする。
            BACK = 1;
            break;
        }
    }
    // バッテリー電圧監視
    adc_sequencer();

    // 電源スイッチ監視
    if (check_power_sw() != 0) return 0;
    return 1;
}

// 停止処理
void finalize() {
    LED = 0;
    BAT_FET = 0;    // 電源FETをOFFにする。
}

// I2Cクライアントの割込み処理
void i2c_client_isr() {
    static int rx_idx = 0;
    static int tx_idx = 0;
    static uint32_packer tx_buf;
    uint8_t data;

    // ストップビット(P)の検出
    if (SSP1STATbits.P != 0) {
        if (rx_idx > 0) {
            switch (cmd[0]) {
            case 0x00:          // idを取得する。
                tx_buf.data = (uint32_t)I2C_ADDRESS;
                break;

            case 0x01:          // バッテリー電圧を取得する。
                tx_buf.data = (uint32_t)volt;
                break;

            case 0x02:          // 起動経過時間を取得する。
                tx_buf.data = syscnt;
                break;

            default:
                cmd_enable = rx_idx;
            }
            tx_idx = 0;
        }
    }
    // マスターがライトを送信
    if (SSP1STATbits.R_nW == 0) {
        if (SSP1STATbits.D_nA == 0) {   // アドレス受信
            data = SSP1BUF;             // ダミーリードが必要(BFをクリア)
            rx_idx = 0;
        }
        else if (SSP1STATbits.BF == 1) {
            data = SSP1BUF;             // データ受信
            if (rx_idx < RX_BUF_SIZ) cmd[rx_idx++] = data;
        }
    }
    // マスターがリードを送信
    else {
        if (SSP1STATbits.D_nA == 0) {   // アドレス受信
            data = SSP1BUF;             // ダミーリードが必要(BFをクリア)
        }
        else {                          // データ送信はtx_bufの下位バイトから送信する。
            if (tx_idx < 4)
                SSP1BUF = tx_buf.bytes[tx_idx++];
            else
                SSP1BUF = 0;
        }
    }
    SSP1CON1bits.CKP = 1;   // クロック・ストレッチ解除
}

// バッテリー電圧監視（1msec間隔で呼ばれる）
void adc_sequencer()
{
    static adc_state next = adc_standby;

    switch (next) {
    case adc_standby:
        if  (adc_limit <= syscnt) {
            adc_limit = syscnt + adc_interval;
            next = adc_fvr_start;
        }
        break;

    case adc_fvr_start:
        FVRCONbits.ADFVR = 0b10;    // ADC用FVRを2.048Vに設定
        FVRCONbits.FVREN = 1;       // FVR有効
        next = adc_start;
        break;

    case adc_start:
        if (!FVRCONbits.FVRRDY) break;
        AN0_EN = 0;                 // ADC入力開始
        ADCON1bits.ADFM = 1;        // 右詰め形式
        ADCON1bits.ADPREF = 0b11;   // 参照電圧+をFVRに設定
        ADCON1bits.ADNREF = 0;      // 参照電圧-をVSSに設定
        ADCON1bits.ADCS = 0b001;    // ADCクロックをFosc/8に設定
        ADCON0bits.CHS = 0b00000;   // AN0の選択
        ADCON0bits.ADON = 1;        // ADC有効
        next = adc_go;
        break;

    case adc_go:
        ADCON0bits.GO = 1;          // ADC変換開始
        next = adc_read;
        break;

    case adc_read:
        if (ADCON0bits.GO) break;   // 変換終了待ち
        volt = (ADRESH << 8) | ADRESL; // 10ビットの結果
        // バッテリー電圧が約3.75V(理論値3.9V)以下になると、LEDの点滅が早くなる。
        led_interval = (volt >= 600) ? 500 : 200;

    default:
        ADCON0bits.ADON = 0;        // ADCモジュールを無効
        FVRCONbits.FVREN = 0;       // FVR無効
        AN0_EN = 1;                 // ADC入力終了
        next = adc_standby;
    }
}

// 電源スイッチ監視（1msec間隔で呼ばれる）
int check_power_sw() {
    static int before = 1;
    static uint32_t limit;
    int now;

    now = POWER_SW;
    if (now != before) {
        before = now;
        if (now == 0) {
            limit = syscnt + 3000;  // 長押し3秒
        }
    }
    else if (now == 0) {
        if (syscnt >= limit) return 1;
    }
    return 0;
}
