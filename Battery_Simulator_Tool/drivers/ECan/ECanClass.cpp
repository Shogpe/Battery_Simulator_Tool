#ifdef _WIN32
#include <windows.h>
#endif
#include <QDebug>
#include <QDir>
#include "ECANClass.h"

ECanClass::ECanClass() {
    // Init members
    //
    m_bWasLoaded = false;
    m_Library = NULL;

    // Loads the API
    //
    LoadAPI();
}

ECanClass::~ECanClass() {
    // Unloads the API
    //
    UnloadAPI();
}

void ECanClass::LoadAPI() {
    // Initializes pointers
    //
    InitializePointers();

    // Loads the DLL
    //
    if (!LoadDllHandle()) {
        qWarning() << m_Library->errorString();
        return;
    }

    // Loads API functions
    //
    pOpenDevice = (OpenDevice*)GetFunction("OpenDevice");
    pCloseDevice = (CloseDevice*)GetFunction("CloseDevice");
    pResetCAN = (ResetCAN*)GetFunction("ResetCAN");
    pInitCAN = (InitCAN*)GetFunction("InitCAN");
    pReadBoardInfo = (ReadBoardInfo*)GetFunction("ReadBoardInfo");
    pReadErrInfo = (ReadErrInfo*)GetFunction("ReadErrInfo");
    pReadCANStatus = (ReadCANStatus*)GetFunction("ReadCANStatus");
    pGetReference = (GetReference*)GetFunction("GetReference");
    pSetReference = (SetReference*)GetFunction("SetReference");
    pGetReceiveNum = (GetReceiveNum*)GetFunction("GetReceiveNum");
    pClearBuffer = (ClearBuffer*)GetFunction("ClearBuffer");
    pStartCAN = (StartCAN*)GetFunction("StartCAN");
    pTransmit = (Transmit*)GetFunction("Transmit");
    pReceive = (Receive*)GetFunction("Receive");

    m_bWasLoaded = pOpenDevice && pCloseDevice && pResetCAN && pInitCAN && pReadBoardInfo && pReadErrInfo && pReadCANStatus && pGetReference && pSetReference &&
                   pGetReceiveNum && pClearBuffer && pStartCAN && pTransmit && pReceive;

    // If the API was not loaded (Wrong version), an error message is shown.
    //
    if (!m_bWasLoaded) qWarning() << "Error: DLL functions could not be loaded!";
}

void ECanClass::UnloadAPI() {
    // Frees a loaded DLL
    //
    if (m_Library != NULL) m_Library->unload();
    m_Library = NULL;

    // Initializes pointers
    //
    InitializePointers();

    m_bWasLoaded = false;
}

void ECanClass::InitializePointers() {
    // Initializes thepointers for the ECan functions
    //
    pOpenDevice = NULL;
    pCloseDevice = NULL;
    pResetCAN = NULL;
    pInitCAN = NULL;
    pReadBoardInfo = NULL;
    pReadErrInfo = NULL;
    pReadCANStatus = NULL;
    pGetReference = NULL;
    pSetReference = NULL;
    pGetReceiveNum = NULL;
    pClearBuffer = NULL;
    pStartCAN = NULL;
    pTransmit = NULL;
    pReceive = NULL;
}
bool ECanClass::LoadDllHandle() {
    // Was already loaded
    //
    if (m_bWasLoaded) return true;
#ifdef _WIN32
    // windows x86 or x64
    SetDllDirectory((QDir::currentPath() + "/lib").toStdWString().c_str());
#ifdef _WIN64  // x64
    // Loads Dll
    m_Library = new QLibrary("ECanVci64.dll");
#else              // x86
    m_Library = new QLibrary("ECanVci.dll");
#endif             //_WIN64
#else              // unix
#ifdef __x86_64__  // x64
#elif __i386__     // x86
#endif
#endif  //_WIN32
    // Return true if the DLL was loaded or false otherwise
    return m_Library->load();
}

void* ECanClass::GetFunction(const char* strName) {
    // There is no DLL loaded
    //
    if (m_Library == NULL) return NULL;

    // Gets the address of the given function in the loeaded DLL
    //
    return m_Library->resolve(strName);
}

TPCANStatus ECanClass::Initialize(DWORD DeviceType, DWORD DeviceInd, DWORD CANInd) {
    if (!m_bWasLoaded) return STATUS_ERR;
    config.AccCode = 0;
    config.AccMask = 0xffffff;
    config.Filter = 0;
    //    config.Mode = 0;

    m_deviceType = DeviceType;
    m_deviceInd = DeviceInd;
    m_channel = CANInd;
    qDebug() << m_deviceType << m_deviceInd << m_channel;
    qDebug() << pOpenDevice(m_deviceType, m_deviceInd, m_channel);
    qDebug() << pInitCAN(DeviceType, DeviceInd, CANInd, &config);
    ERR_INFO msg;
    pReadErrInfo(m_deviceType, m_deviceInd, m_channel, &msg);
    qDebug() << msg.ErrCode << msg.ArLost_ErrData << msg.Passive_ErrData;
    return pStartCAN(m_deviceType, m_deviceInd, m_channel);
}

TPCANStatus ECanClass::Uninitialize() {
    if (!m_bWasLoaded) return STATUS_ERR;
    return (TPCANStatus)pCloseDevice(m_deviceType, m_deviceInd);
}

TPCANStatus ECanClass::Reset() {
    if (!m_bWasLoaded) return STATUS_ERR;

    return (TPCANStatus)pResetCAN(m_deviceType, m_deviceInd, m_channel);
}

TPCANStatus ECanClass::GetStatus(P_CAN_STATUS pCANStatus) {
    if (!m_bWasLoaded) return STATUS_ERR;

    return (TPCANStatus)pReadCANStatus(m_deviceType, m_deviceInd, m_channel, pCANStatus);
}
static void canobj2msg(P_CAN_OBJ pObj, can_message_t& msg) {
    msg.id = pObj->ID;
    msg.dlc = pObj->DataLen;
    msg.IDE = pObj->ExternFlag;
    msg.RTR = pObj->RemoteFlag;
    memcpy(msg.data, pObj->Data, msg.dlc);
}
static QString generateDataString(const can_message_t* cmsg) {
    QString dataString;
    dataString += QString().sprintf("%08X:", cmsg->id);
    if (!cmsg->RTR)
        for (int i = 0; i < cmsg->dlc; i++) {
            dataString += QString().sprintf("%02X ", cmsg->data[i]);
        }
    return dataString;
}
TPCANStatus ECanClass::Read(can_message_t& msg, INT WaitTime) {
    if (!m_bWasLoaded) return STATUS_ERR;
    CAN_OBJ obj;
    TPCANStatus rc = (TPCANStatus)pReceive(m_deviceType, m_deviceInd, m_channel, &obj, 1, WaitTime);
    if (rc == STATUS_OK) {
        canobj2msg(&obj, msg);        
        qDebug() << "<< " << generateDataString(&msg);
    }
    return rc;
}
static void msg2canobj(can_message_t& msg, P_CAN_OBJ pObj) {
    pObj->DataLen = msg.dlc;
    memcpy(pObj->Data, msg.data, msg.dlc);
    pObj->ExternFlag = msg.IDE;
    pObj->RemoteFlag = msg.RTR;
    pObj->ID = msg.id;
}

TPCANStatus ECanClass::Write(can_message_t& msg) {
    if (!m_bWasLoaded) return STATUS_ERR;
    CAN_OBJ obj;
    msg2canobj(msg, &obj);
    if ((TPCANStatus)pTransmit(m_deviceType, m_deviceInd, m_channel, &obj, 1) == 1) {       
        qDebug() << ">> " << generateDataString(&msg);
        return 1;
    }
    return 0;
}
TPCANStatus ECanClass::GetBufferLen() {
    if (!m_bWasLoaded) return STATUS_ERR;
    return (TPCANStatus)pGetReceiveNum(m_deviceType, m_deviceInd, m_channel);
}
TPCANStatus ECanClass::ClearBuf() {
    if (!m_bWasLoaded) return STATUS_ERR;
    return (TPCANStatus)pClearBuffer(m_deviceType, m_deviceInd, m_channel);
}
// TPCANStatus ECanClass::FilterMessages(TPCANHandle Channel,
//                                       DWORD FromID,
//                                       DWORD ToID,
//                                       TPCANMode Mode) {
//   if (!m_bWasLoaded)
//     return STATUS_ERR;

//  return (TPCANStatus)pInitCAN(Channel, FromID, ToID, Mode);
//}

TPCANStatus ECanClass::GetErrorText(P_ERR_INFO pErrInfo) {
    if (!m_bWasLoaded) return STATUS_ERR;

    return (TPCANStatus)pReadErrInfo(m_deviceType, m_deviceInd, m_channel, pErrInfo);
}
TPCANStatus ECanClass::canSetBaudRate(CAN_BAUDRATE baud_rate) {
    switch (baud_rate) {
        case CAN_5K:{
            config.Timing0 = 0xbf;
            config.Timing1 = 0xff;
        }break;
        case CAN_10K: {
            config.Timing0 = 0x31;
            config.Timing1 = 0x1c;
        } break;

        case CAN_20K:{
            config.Timing0 = 0x18;
            config.Timing1 = 0x1c;
        }break;

        case CAN_25K:{
            config.Timing0 = 0x12;
            config.Timing1 = 0x1C;
        }break;

        case CAN_50K: {
            config.Timing0 = 0x09;
            config.Timing1 = 0x1c;
        } break;

        case CAN_100K:{
            config.Timing0 = 0x04;
            config.Timing1 = 0x1c;
        }break;

        case CAN_125K: {
            config.Timing0 = 0x03;
            config.Timing1 = 0x1c;
        } break;

        case CAN_150K:{
            config.Timing0 = 0x06;
            config.Timing1 = 0xff;
        }break;

        case CAN_200K:{
            config.Timing0 = 0x81;
            config.Timing1 = 0xfa;
        }break;

        case CAN_250K: {
            config.Timing0 = 0x01;
            config.Timing1 = 0x1c;
        } break;

        case CAN_500K: {
            config.Timing0 = 0x00;
            config.Timing1 = 0x1c;
        } break;

        case CAN_1M: {
            config.Timing0 = 0x00;
            config.Timing1 = 0x14;
        } break;

        default:  // 125k
        {
            config.Timing0 = 0x03;
            config.Timing1 = 0x1c;
        } break;

    }
    return 0;
}
/**
 * @brief ECanClass::canSetMode
 * @param mode
 * @return
 */
TPCANStatus ECanClass::canSetMode(int mode) {
    config.Mode = mode;
//    switch (mode) {
//        default:
//            config.Mode = mode;
//            break;
//    }
    return 0;
}
