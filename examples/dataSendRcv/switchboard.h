#ifndef _SWITCH_BOARD_H_
#define _SWITCH_BOARD_H_

typedef unsigned char uint8;
// Message Types
#define SB_BOARD_INFO_REQ   0x01
#define SB_BOARD_INFO_RSP   0x02
#define SB_STATE_CHANGE_REQ 0x03 
#define SB_STATE_CHANGE_RSP 0x04

//Message Lengths
#define SB_BOARD_INFO_REQ_LEN   (2)
#define SB_BOARD_INFO_RSP_LEN   (10)
#define SB_STATE_CHANGE_REQ_LEN (6) 
#define SB_STATE_CHANGE_RSP_LEN (6)

typedef struct
{
  uint8 message_type;
} sbMessageHdr_t;


// Switch Board Types
#define SB_TYPE_PLUG  0x01
#define SB_TYPE_4X4   0x02
typedef struct
{
  uint8 type;
} switchBoardType_t;


// Switch state change request defines
#define SW_TURN_OFF  0x0
#define SW_TURN_ON   0x1
#define SW_KEEP_OFF  0x2
#define SW_KEEP_ON   0x3
#define SW_DONT_CARE 0x4
// Switch state change reply defines
#define SW_STATEC_SUCCESS        0x1
#define SW_STATEC_FAILED         0x2
#define SW_STATEC_NOT_IN_USE     0x3
#define SW_STATEC_NOT_WORKING    0x4
#define SW_STATEC_DONT_CARE      0x5
typedef struct
{
    struct
    {
      uint8 switch1 :4;
      uint8 switch2 :4;
      uint8 switch3 :4;
      uint8 switch4 :4;
      uint8 switch5 :4;
      uint8 switch6 :4;
      uint8 switch7 :4;
      uint8 switch8 :4;
    } state;
} switchState_t;

// Switch states
#define SW_OFF           0x00
#define SW_ON            0x01
#define SW_NOT_IN_USE    0x02
#define SW_NOT_WORKING   0x03
#define SW_NOT_PRESENT   0x04
#define SW_UNKNOWN       0x05
typedef struct
{
  uint8 switch1;
  uint8 switch2;
  uint8 switch3;
  uint8 switch4;
  uint8 switch5;
  uint8 switch6;
  uint8 switch7;
  uint8 switch8;
} hwSwitchBoardState_t;

typedef struct
{
  switchBoardType_t sbType;
  switchState_t switchData;
} sBoard_t;

typedef struct
{
  uint8 flags;
} sInfoReq_t;

typedef struct
{
  switchBoardType_t sbType;
  hwSwitchBoardState_t currentState;
} sInfoRsp_t;

typedef struct
{
  sbMessageHdr_t hdr;
  union
  {
    sBoard_t boardData;
    sInfoReq_t infoReqData;
    sInfoRsp_t infoRspData;
  } data;
} sbMessage_t;

// Global Variables
extern hwSwitchBoardState_t SwitchBoard_Global_State;

// Fuctions
uint8 initSwitchBoardState(void);
uint8 setSwitchState(switchState_t *swState, uint8 swIndex, hwSwitchBoardState_t *hwState, uint8 *led_mask);
uint8 sentStateChangeRspMsg(sbMessage_t *pMsg, hwSwitchBoardState_t *pState, void *endPointDesc);
uint8 sentBoardInfoRspMsg(sbMessage_t *pMsg, void *endPointDesc);

#endif
