#include "mcc_generated_files/system/system.h"

#define LM75B_address 0x48          // A0=A1=A2=Low
#define temp_reg      0x00          //Temperture register
#define conf_reg      0x01          //Configuration register
#define thyst_reg     0x02          //Hysterisis register
#define tos_reg       0x03          //Overtemperature shutdown register

const double tos = 30.0;                  //���荞�ݔ������x�i���j 0.5������  �f�t�H���g�ł�80��
const double thyst = 28.0;                //���荞�ݔ������x�i��j 0.5�����݁@�f�t�H���g�ł�75��
int tos_data = (int)(tos / 0.5) << 7;     //���W�X�^�p�ɕϊ�
int thyst_data = (int)(thyst / 0.5) << 7; //���W�X�^�p�ɕϊ�
const uint8_t osPin = 2;                  //LM75B��OS�s���Ɛڑ�����s��

const i2c_host_interface_t *I2C = &I2C1_Host;

#define BUFMAX 128
uint8_t sendBuf[BUFMAX];
uint8_t dataRead[BUFMAX];

uint8_t loopCnt = 0;
static bool gb_didProceedData = false;

void initLM75B(void)
{
    printf(" ---- initLM75B start ---- \n");
    //tos�̉��x�ݒ�    
    sendBuf[0] = tos_reg;
    sendBuf[1] = tos_data >> 8;
    sendBuf[2] = tos_data;
    I2C->Write(LM75B_address, sendBuf, 3);
    while(I2C->IsBusy())
    {
        printf("I2C busy, Do task!\n");
        I2C->Tasks();
    }
    //thyst�̉��x�ݒ�
    sendBuf[0] = thyst_reg;
    sendBuf[1] = thyst_data >> 8;
    sendBuf[2] = thyst_data;
    I2C->Write(LM75B_address, sendBuf, 3);
    while(I2C->IsBusy())
    {
        printf("I2C busy, Do task!\n");
        I2C->Tasks();
    }
    
    //! LM75B�����x�ǂݏo�����[�h�ɐݒ�
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
    static uint16_t rawTempValue;   //!< �ǂݏo���l�̃r�b�g���o���ʊi�[�p
    static float tempCelsius;       //!< �����Z���V�E�X���x�ɕϊ���̒l����
    
    static i2c_host_error_t er = I2C_ERROR_NONE;
    
    SYSTEM_Initialize();
    
    //! LM75B�̃��W�X�^�ݒ�i���x�ǂݏo�����[�h�ɐݒ肵����ԂŏI�������j
    initLM75B();
    
    while (1)
    {
        IO_RD2_Toggle();
        
        #if NOPRINTF != 1
        printf("\n\n--------\n %d \n--------\n", loopCnt++);
        #endif
        
        if(!I2C->IsBusy() && gb_didProceedData)     //! I2Cbusy�łȂ��A���A��M�f�[�^�������ς݂ł��邩�A
        {   //! �Z�����Ȃ��āA����M�f�[�^�̏������I����Ă���ꍇ�A
            I2C->Read(LM75B_address, dataRead, 2);  //! ���x�Z���T����2�o�C�g�ǂݍ���   
            gb_didProceedData = false;              //! ��M�f�[�^�����ς݃t���O���N���A
        }
        
        if(!I2C->IsBusy())                          //! I2Cbusy���`�F�b�N
        {   //! I2C���Z�����Ȃ��Ƃ��A
            er = I2C->ErrorGet();                   //! �G���[�N���Ă邩�擾����
            if (er == I2C_ERROR_NONE)               //! �擾�����l�ŃG���[���`�F�b�N����
            {   //! �G���[����Ȃ������ꍇ�A
                #if NOPRINTF != 1
                printf("i2cReadSuccessful\n");
                #endif
                rawTempValue = (dataRead[0] << 8 | dataRead[1]);    //! ��M�f�[�^���牷�x�f�[�^���o
                tempCelsius = (float)(rawTempValue >> 5) * 0.125;   //! �Z���V�E�X���x�ɕϊ�����
                // ^From line 60 of the sample:"https://github.com/SWITCHSCIENCE/samplecodes/blob/master/LM75B_breakout/Arduino/LM75B_breakout/LM75B_breakout.ino"
                #if NOPRINTF != 1
                printf("rawTempValue:%d\n", rawTempValue);
                printf("tempCelsius:%f\n", tempCelsius);
                #endif
            }
            else
            {   //! �G���[�������ꍇ�A
                #if NOPRINTF != 1
                printf("!!!! i2cReadError:%d !!!! \n", er);
                #endif
                //! �i�G���[����������j
            }
            //! ��M�f�[�^�����������̂ŁA
            gb_didProceedData = true;   //! ��M�f�[�^�����ς݃t���O���Z�b�g����
        }
        else
        {   //! I2C���Z�����Ƃ��́A
            I2C->Tasks();   //! I2C�^�X�N����������
        }
    }
}
