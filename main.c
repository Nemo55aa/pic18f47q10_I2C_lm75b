#include "mcc_generated_files/system/system.h"

#define LM75B_address 0x48          // A0=A1=A2=Low
#define temp_reg      0x00          //Temperture register
#define conf_reg      0x01          //Configuration register
#define thyst_reg     0x02          //Hysterisis register
#define tos_reg       0x03          //Overtemperature shutdown register

const double tos = 30.0;                  //割り込み発生温度（高） 0.5℃刻み  デフォルトでは80℃
const double thyst = 28.0;                //割り込み発生温度（低） 0.5℃刻み　デフォルトでは75℃
int tos_data = (int)(tos / 0.5) << 7;     //レジスタ用に変換
int thyst_data = (int)(thyst / 0.5) << 7; //レジスタ用に変換
const uint8_t osPin = 2;                  //LM75BのOSピンと接続するピン

const i2c_host_interface_t *I2C = &I2C1_Host;

#define BUFMAX 128
uint8_t sendBuf[BUFMAX];
uint8_t dataRead[BUFMAX];

uint8_t loopCnt = 0;
static bool gb_didProceedData = false;

void initLM75B(void)
{
    printf(" ---- initLM75B start ---- \n");
    //tosの温度設定    
    sendBuf[0] = tos_reg;
    sendBuf[1] = tos_data >> 8;
    sendBuf[2] = tos_data;
    I2C->Write(LM75B_address, sendBuf, 3);
    while(I2C->IsBusy())
    {
        printf("I2C busy, Do task!\n");
        I2C->Tasks();
    }
    //thystの温度設定
    sendBuf[0] = thyst_reg;
    sendBuf[1] = thyst_data >> 8;
    sendBuf[2] = thyst_data;
    I2C->Write(LM75B_address, sendBuf, 3);
    while(I2C->IsBusy())
    {
        printf("I2C busy, Do task!\n");
        I2C->Tasks();
    }
    
    //! LM75Bを温度読み出しモードに設定
    sendBuf[0] = temp_reg;
    I2C->Write(LM75B_address, sendBuf, 1);
    while(I2C->IsBusy())
    {
        printf("I2C busy, Do task!\n");
        I2C->Tasks();
    }
}
#define NOPRINTF (1)

int main(void)
{
    static uint16_t rawTempValue;   //!< 読み出し値のビット抽出結果格納用
    static float tempCelsius;       //!< ↑をセルシウス温度に変換後の値を代入
    
    static i2c_host_error_t er = I2C_ERROR_NONE;
    
    SYSTEM_Initialize();
    
    //! LM75Bのレジスタ設定（温度読み出しモードに設定した状態で終了される）
    initLM75B();
    
    while (1)
    {
        IO_RD2_Toggle();
        
        #if NOPRINTF != 1
        printf("\n\n--------\n %d \n--------\n", loopCnt++);
        #endif
        
        if(!I2C->IsBusy() && gb_didProceedData)     //! I2Cbusyでなく、かつ、受信データを処理済みであるか、
        {   //! 忙しくなくて、かつ受信データの処理が終わっている場合、
            I2C->Read(LM75B_address, dataRead, 2);  //! 温度センサから2バイト読み込む   
            gb_didProceedData = false;              //! 受信データ処理済みフラグをクリア
        }
        
        if(!I2C->IsBusy())                          //! I2Cbusyかチェック
        {   //! I2Cが忙しくないとき、
            er = I2C->ErrorGet();                   //! エラー起きてるか取得する
            if (er == I2C_ERROR_NONE)               //! 取得した値でエラーかチェックする
            {   //! エラーじゃなかった場合、
                #if NOPRINTF != 1
                printf("i2cReadSuccessful\n");
                #endif
                rawTempValue = (dataRead[0] << 8 | dataRead[1]);    //! 受信データから温度データ抽出
                tempCelsius = (float)(rawTempValue >> 5) * 0.125;   //! セルシウス温度に変換する
                // ^From line 60 of the sample:"https://github.com/SWITCHSCIENCE/samplecodes/blob/master/LM75B_breakout/Arduino/LM75B_breakout/LM75B_breakout.ino"
                #if NOPRINTF != 1
                printf("rawTempValue:%d\n", rawTempValue);
                printf("tempCelsius:%f\n", tempCelsius);
                #endif
            }
            else
            {   //! エラーだった場合、
                #if NOPRINTF != 1
                printf("!!!! i2cReadError:%d !!!! \n", er);
                #endif
                //! （エラー処理をする）
            }
            //! 受信データを処理したので、
            gb_didProceedData = true;   //! 受信データ処理済みフラグをセットする
        }
        else
        {   //! I2Cが忙しいときは、
            I2C->Tasks();   //! I2Cタスクを処理する
        }
    }
}
