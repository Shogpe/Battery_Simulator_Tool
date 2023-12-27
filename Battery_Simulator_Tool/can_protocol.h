#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>
#include "./drivers/CanAdapter/can_message.h"

/*--- 基本地址 ---*/
#define BROADCAST_ADDR          100U
#define UPPER_PC_ADDR           99U

/*--- 命令页 ---*/
#define CMD_PAGE_GEN          0x00//常规命令页
#define CMD_PAGE_SET          0x01//设置命令页
#define CMD_PAGE_SYS          0x03//系统命令页
#define CMD_PAGE_LOG          0x04//状态码命令页

/*--- 命令代码 ---*/
#define CMD_CODE_GEN_VOLTAGE         0U
#define CMD_CODE_GEN_CURRENT         1U
#define CMD_CODE_GEN_CURR_RANGE      2U
#define CMD_CODE_GEN_PARA_METER      3U
#define CMD_CODE_GEN_AUTO_SENDE      4U
#define CMD_CODE_GEN_AUTO_SENDD      5U
#define CMD_CODE_GEN_SEL_AD_FST      6U
#define CMD_CODE_GEN_SEL_AD_END      7U
#define CMD_CODE_GEN_SEL_ADDR        8U
#define CMD_CODE_GEN_OUT_RELY        9U
#define CMD_CODE_GEN_READ_TEMP       10U
#define CMD_CODE_GEN_READ_PARA       12U

#define CMD_CODE_SET_SET_ADDR        0U

#define CMD_CODE_SYS_SET_BAUD        4U

#define CMD_CODE_LOG_OK              0U
#define CMD_CODE_LOG_WARNING         1U
#define CMD_CODE_LOG_ERROR           2U

/*--- 命令代码对应的写数据长度 ---*/
#define DATA_W_LEN_GEN_VOLTAGE         3U
#define DATA_W_LEN_GEN_CURRENT         3U
#define DATA_W_LEN_GEN_CURR_RANGE      1U
#define DATA_W_LEN_GEN_PARA_METER      7U
#define DATA_W_LEN_GEN_AUTO_SENDE      0U
#define DATA_W_LEN_GEN_AUTO_SENDD      0U
#define DATA_W_LEN_GEN_SEL_AD_FST      1U
#define DATA_W_LEN_GEN_SEL_AD_END      1U
#define DATA_W_LEN_GEN_SEL_ADDR        2U
#define DATA_W_LEN_GEN_OUT_RELY        1U

#define DATA_W_LEN_SET_SET_ADDR        1U

#define DATA_W_LEN_SYS_SET_BAUD        1U

/*--- 命令代码对应的读数据长度 ---*/
#define DATA_R_LEN_GEN_VOLTAGE         3U
#define DATA_R_LEN_GEN_CURRENT         4U
#define DATA_R_LEN_GEN_PARA_METER      7U
#define DATA_R_LEN_GEN_OUT_RELY        1U
#define DATA_R_LEN_GEN_READ_TEMP       1U
#define DATA_R_LEN_GEN_READ_PARA       8U

/*--- 模拟器型号 ---*/
#define SIMULATOR_JCY2200              0U

#define CMD_MODE_R                    0x01
#define CMD_MODE_W                    0x00

/*--- 其他 ---*/
#define BAUD_RATE_5K                    0U
#define BAUD_RATE_10K                   1U
#define BAUD_RATE_20K                   2U
#define BAUD_RATE_25K                   3U
#define BAUD_RATE_50K                   4U
#define BAUD_RATE_100K                  5U
#define BAUD_RATE_125K                  6U
#define BAUD_RATE_150K                  7U
#define BAUD_RATE_200K                  8U
#define BAUD_RATE_250K                  9U
#define BAUD_RATE_500K                  10U
#define BAUD_RATE_1000K                 11U

/*--- 其他 ---*/
#define CELL_NUM_MAX                  0x7F
#define SIMULATOR_NUM_MAX             60U

typedef enum
{
    SimId_Invalid = 0x00,
    SimId_Valid   = 0xA5,
}SimIdValid_T;



typedef union
{
    uint32_t u32Word;
    struct
    {
        unsigned dstAddr:7;    //目的地址
        unsigned srcAddr:7;    //源地址
        unsigned cmdPage:3;    //命令页
        unsigned cmdCode:7;    //命令码
        unsigned subpack:1;    //分包标志
        unsigned reserve:4;    //保留域
        unsigned unUsed: 3;
    }Bits;
}canMsgId_T;


typedef struct
{
    double       dbCellVolt[CELL_NUM_MAX];
    double       dbCellCurr[CELL_NUM_MAX];
    double       dbCellCurrSet[CELL_NUM_MAX];
    double       dbCellTemperature[CELL_NUM_MAX];
    uint8_t      u8ReleyState[CELL_NUM_MAX];
    uint8_t      u8Addr[CELL_NUM_MAX];
    SimIdValid_T simIdValid[CELL_NUM_MAX];
    uint8_t      u8DeviceNum;
}simulatorData_T;










can_message_t MakeUpCanPacketId(uint8_t code , uint8_t page , uint8_t src , uint8_t dst , uint8_t rw );

void ClearMsgData(uint8_t * data);

double TransformVoltDataMV(uint8_t * data);
double TransformCurrDataMA(uint8_t * data);

can_message_t setVoltageDataPacket(uint8_t addr , double val);
can_message_t setCurrentDataPacket(uint8_t addr , double val);
can_message_t setAddrDataPacket(uint8_t addr , double val);
can_message_t setRelayDataPacket(uint8_t addr , double val);

#endif // CAN_PROTOCOL_H
