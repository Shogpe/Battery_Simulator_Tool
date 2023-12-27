#ifndef __ECANCLASSH_
#define __ECANCLASSH_

// Inclusion of the ECanVci header file
//
#ifndef _V_ECANVCI_H_
#include "ECanVci.h"
#endif
#include <QLibrary.h>
#include "CanAdapter/can_message.h"

#define TPCANHandle      WORD   // Represents a PCAN hardware channel handle
#define TPCANStatus      DWORD  // Represents a PCAN status/error code
#define TPCANParameter   BYTE   // Represents a PCAN parameter to be read or set
#define TPCANDevice      BYTE   // Represents a PCAN device
#define TPCANMessageType BYTE   // Represents the type of a PCAN message
#define TPCANType        BYTE   // Represents the type of PCAN hardware to be initialized
#define TPCANMode        BYTE   // Represents a PCAN filter mode
#define TPCANBaudrate    WORD   // Represents a PCAN Baud rate register value
#define TPCANBitrateFD   LPSTR  // Represents a PCAN-FD bit rate string

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
enum CAN_BAUDRATE {
    CAN_5K = 0,
    CAN_10K,
    CAN_20K,
    CAN_25K,
    CAN_50K,
    CAN_100K,
    CAN_125K,
    CAN_150K,
    CAN_200K,
    CAN_250K,
    CAN_500K,
    CAN_1M,
};
enum CAN_MODE {
    VCI_MODE_NORMAL = 0,  // 正常模式
    VCI_MODE_LISTEN,      // 只听模式
};
// ECAN dynamic-load class
//
class ECanClass {
   private:
    // DLL ECAN
    QLibrary* m_Library;

    // Function pointers
    OpenDevice* pOpenDevice;
    ResetCAN* pResetCAN;
    CloseDevice* pCloseDevice;
    InitCAN* pInitCAN;
    StartCAN* pStartCAN;
    Receive* pReceive;
    GetReceiveNum* pGetReceiveNum;
    ClearBuffer* pClearBuffer;
    ReadErrInfo* pReadErrInfoCAN;
    ReadCANStatus* pVCI_ReadCANStatus;
    ReadBoardInfo* pReadBoardInfo;
    ReadErrInfo* pReadErrInfo;
    ReadCANStatus* pReadCANStatus;
    GetReference* pGetReference;
    SetReference* pSetReference;
    Transmit* pTransmit;

    INIT_CONFIG config;
    DWORD m_deviceType;
    DWORD m_deviceInd;
    DWORD m_channel;
    // Load flag
    bool m_bWasLoaded;

    // Load API
    void LoadAPI();

    // Releases the loaded API
    //
    void UnloadAPI();

    // Initializes the pointers for the PCANBasic functions
    //
    void InitializePointers();

    // Loads the DLL
    //
    bool LoadDllHandle();

    // Gets the address of a given function name in a loaded DLL
    //
    void* GetFunction(const char* szName);

   public:
    // PCANBasicClass constructor
    //
    ECanClass();
    // PCANBasicClass destructor
    //
    ~ECanClass();

    TPCANStatus Initialize(DWORD DeviceType, DWORD DeviceInd, DWORD CANInd);

    TPCANStatus Uninitialize();

    TPCANStatus Reset();

    TPCANStatus GetStatus(P_CAN_STATUS pCANStatus);
    TPCANStatus GetBufferLen();
    TPCANStatus ClearBuf();

    TPCANStatus Read(can_message_t& msg, INT WaitTime);

    TPCANStatus Write(can_message_t& msg);


    /// <summary>
    /// Configures the reception filter.
    /// </summary>
    /// <remarks>The message filter will be expanded with every call to
    /// this function. If it is desired to reset the filter, please use
    /// the CAN_SetParameter function</remarks>
    /// <param name="Channel">"The handle of a PCAN Channel"</param>
    /// <param name="FromID">"The lowest CAN ID to be received"</param>
    /// <param name="ToID">"The highest CAN ID to be received"</param>
    /// <param name="Mode">"Message type, Standard (11-bit identifier) or
    /// Extended (29-bit identifier)"</param>
    /// <returns>"A TPCANStatus error code"</returns>
    TPCANStatus FilterMessages(TPCANHandle Channel, DWORD FromID, DWORD ToID, TPCANMode Mode);

    TPCANStatus GetErrorText(P_ERR_INFO pErrInfo);
    TPCANStatus canSetBaudRate(CAN_BAUDRATE baud_rate);
    TPCANStatus canSetMode(int mode);
};
#endif
