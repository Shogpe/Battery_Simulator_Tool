#include "can_protocol.h"
#include <QDebug>




can_message_t MakeUpCanPacketId(uint8_t code , uint8_t page , uint8_t src , uint8_t dst , uint8_t rw )
{
    can_message_t msg;
    canMsgId_T unpackMsg;
    msg.IDE = 1U;
    msg.dlc = 8U;

    if(CMD_MODE_R == rw)
    {
        msg.RTR = CMD_MODE_R;
    }
    else
    {
        msg.RTR = CMD_MODE_W;
    }


    unpackMsg.Bits.unUsed  = 0U;
    unpackMsg.Bits.reserve = 0U;
    unpackMsg.Bits.subpack = 0U;
    unpackMsg.Bits.cmdCode = code;
    unpackMsg.Bits.cmdPage = page;
    unpackMsg.Bits.srcAddr = src;
    unpackMsg.Bits.dstAddr = dst;

    msg.id = unpackMsg.u32Word;

    return msg;
}

void ClearMsgData(uint8_t * data)
{
    for(int i = 0; i < 8; i++)
    {
        data[i] = 0U;
    }
}

double TransformVoltDataMV(uint8_t * data)
{
    double dbRes = 0;

    dbRes += data[0];
    dbRes += data[1]<<8;
    dbRes += data[2]<16;
    dbRes *= 0.1;

    return dbRes-0.1;
}

double TransformCurrDataMA(uint8_t * data)
{
    double dbRes = 0;

    dbRes += data[0];
    dbRes += data[1]<<8;
    dbRes += data[2]<16;
    dbRes *= 0.1;

    if((data[3]&0x01) == 1)
    {
        dbRes /= 1000;
    }
    return dbRes-0.1;
}





can_message_t setVoltageDataPacket(uint8_t addr , double val)
{
    can_message_t msg;
    msg = MakeUpCanPacketId(CMD_CODE_GEN_VOLTAGE , CMD_PAGE_GEN , UPPER_PC_ADDR , addr , CMD_MODE_W );
    msg.dlc = DATA_W_LEN_GEN_VOLTAGE;
    ClearMsgData(msg.data);

    union
    {
        uint8_t buf[4];
        int32_t word;
    }u32Temp;

    if(val<0)
    {
        val = 0;
    }
    else if(val>5500)
    {
        val = 5500;
    }

    u32Temp.word = val;
    msg.data[0] = u32Temp.buf[0];
    msg.data[1] = u32Temp.buf[1];
    msg.data[2] = u32Temp.buf[2];






    return msg;
}

can_message_t setCurrentDataPacket(uint8_t addr , double val)
{
    can_message_t msg;
    msg = MakeUpCanPacketId(CMD_CODE_GEN_CURRENT , CMD_PAGE_GEN , UPPER_PC_ADDR , addr , CMD_MODE_W );
    msg.dlc = DATA_W_LEN_GEN_CURRENT;
    ClearMsgData(msg.data);

    union
    {
        uint8_t buf[4];
        int32_t word;
    }u32Temp;

    if(val<0)
    {
        val = 0;
    }
    else if(val>1100)
    {
        val = 1100;
    }



    u32Temp.word = val;
    msg.data[0] = u32Temp.buf[0];
    msg.data[1] = u32Temp.buf[1];
    msg.data[2] = u32Temp.buf[2];


    return msg;
}

can_message_t setAddrDataPacket(uint8_t addr , double val)
{
    can_message_t msg;
    msg = MakeUpCanPacketId(CMD_CODE_SET_SET_ADDR , CMD_PAGE_SET , UPPER_PC_ADDR , addr , CMD_MODE_W );
    msg.dlc = DATA_W_LEN_SET_SET_ADDR;
    ClearMsgData(msg.data);

    if(0)//(val<=0&&val>60)
    {
        addr = 99;//地址不能乱设，这里保护一下，强行让这条没法接收
        return msg;
    }

    msg.data[0] = (uint8_t)val;


    return msg;
}

can_message_t setRelayDataPacket(uint8_t addr , double val)
{
    can_message_t msg;
    msg = MakeUpCanPacketId(CMD_CODE_GEN_OUT_RELY , CMD_PAGE_GEN , UPPER_PC_ADDR , addr , CMD_MODE_W );
    msg.dlc = DATA_W_LEN_GEN_OUT_RELY;
    ClearMsgData(msg.data);
    if(val==0)
    {
        msg.data[0] = 0;
    }
    else
    {
        msg.data[0] = 1;
    }

    return msg;

}
