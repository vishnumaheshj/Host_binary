#ifndef __SB_CLIENT_METHODS_H__
#define __SB_CLIENT_METHODS_H__

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/ipc.h>
#if 0
#include <sys/shm.h>
#endif
#include <sys/types.h>
#include <sys/msg.h>
#include <errno.h>


#include "rpc.h"
#include "mtSys.h"
#include "mtZdo.h"
#include "mtAf.h"
#include "mtParser.h"
#include "rpcTransport.h"
#include "dbgPrint.h"
#include "hostConsole.h"
#include "switchboard.h"

typedef struct
{
	uint8_t Index;
	uint8_t DeviceType;
	hwSwitchBoardState_t currentState;
	uint16_t NwkAddr;
	uint64_t IEEEAddr;
	uint8_t joinState;
} DeviceInfo_t;

typedef struct
{
	uint8_t EndPoint;
	uint8_t ActiveNow;
} AppInfo_t;

typedef struct
{
	DeviceInfo_t DevInfo;
	AppInfo_t AppInfo;
} NodeInfo_t;

NodeInfo_t nodeInfoList[64];
uint8_t joinedNodesCount = 0;

#define  NOT_READY  -1
#define  FILLED     1
#define  TAKEN      0

#if 0
//REMOVE_SHM
struct Memory {
	int  status;
	char data[256];
};
#else
struct msgq_buf {
    long mtype;
    char mtext[256];
};
#endif

int8_t initDone = 0;

#if 0
struct Memory  *ShmReadPTR,*ShmWritePTR; 
#else
int msgQWriteID, msgQReadID;
#endif

#if 1
int init_write_msgq()
{
        key_t          MsgQKEY; 
        int            MsgQID = -1;

        if ((MsgQKEY = ftok("/home", 'x')) == -1) {
                perror("ftok");
                return -1; // Need to add and handle proper error code.
        }
        if ((MsgQID = msgget(MsgQKEY, 0644)) == -1) {
                perror("msgget");
                return -1; // Need to add and handle proper error code.
        }

        return MsgQID;
}
int init_read_msgq()
{
        key_t          MsgQKEY;
        int            MsgQID = -1;

        if ((MsgQKEY = ftok("/home", 'y')) == -1) {
                perror("ftok");
                return -1; // Need to add and handle proper error code.
        }
        if ((MsgQID = msgget(MsgQKEY, 0644)) == -1) {
                perror("msgget");
                return -1; // Need to add and handle proper error code.
        }

        return MsgQID;
}
#endif
static int sbGetDataFromShmem(char *serverSpace)
{
#if 0
	char *data = ShmReadPTR->data;
	int dataSize = 0;
	if (data == NULL)
		return -1;
	dataSize = sizeof(sbMessage_t);
	memcpy(serverSpace, data, dataSize);
#else
	struct msgq_buf buf;
	buf.mtype = 0;
	int dataSize = 0;
	dataSize = sizeof(sbMessage_t);
	if (msgrcv(msgQReadID, &buf,dataSize, 0, 0) == -1) {
		    perror("msgrcv");
		    return -1;
	}
	memcpy(serverSpace, buf.mtext, dataSize);
#endif
	return dataSize;

}

static int sbSentDataToShmem(sbMessage_t *sbMsg)
{
	int dataSize;
#if 0
	if (ShmWritePTR == NULL)
			return -1;

	dataSize = sizeof(sbMessage_t);

	memset(ShmWritePTR->data, 0, 256);
	memcpy(ShmWritePTR->data, sbMsg, dataSize);
	ShmWritePTR->status = FILLED;
#else
	struct msgq_buf buf;
	buf.mtype = 1;	
	dataSize = sizeof(sbMessage_t);
	memcpy(buf.mtext, sbMsg, dataSize);
	if (msgsnd(msgQWriteID, &buf, dataSize, 0) == -1) {
		perror("msgsnd");
		return -1;
	}

#endif
	return dataSize;
}

static int sbSentDeviceReady(int ok)
{
    	int ret;
	sbMessage_t sbMsg;
	sbMsg.hdr.message_type = SB_DEVICE_READY_NTF;
	ret = sbSentDataToShmem(&sbMsg);
	return ret;
}

static int sbSentDeviceJoin(uint8 epStatus, uint8 devIndex, void *Data, uint8_t endPoint)
{
	if (epStatus == NS_EP_ACTIVE)
	{
		ActiveEpRspFormat_t *AERsp = (ActiveEpRspFormat_t *)Data;
		DataRequestFormat_t DataRequest;
		DataRequest.DstAddr     = AERsp->NwkAddr;
		DataRequest.DstEndpoint = endPoint;
		DataRequest.SrcEndpoint = 1;
		DataRequest.ClusterID   = 6;
		DataRequest.TransID     = 5;
		DataRequest.Options     = 0;
		DataRequest.Radius      = 0xEE;
		DataRequest.Len         = 1;

		(*(sbMessage_t *)(DataRequest.Data)).hdr.message_type = SB_DEVICE_TYPE_REQ;

		initDone = 0;
		afDataRequest(&DataRequest);
        printf("ZNP: dev type req, send to board\n");
		rpcWaitMqClientMsg(500);
		initDone = 1;
		return 0;
	}
	else if (epStatus == NS_BOARD_READY)
	{
		sbMessage_t sbMsg;
		sbMsg.hdr.message_type = SB_DEVICE_INFO_NTF;
		sbMsg.data.devInfo.joinState    = nodeInfoList[devIndex - 1].DevInfo.joinState;
		sbMsg.data.devInfo.sbType.type  = nodeInfoList[devIndex - 1].DevInfo.DeviceType;
		sbMsg.data.devInfo.devIndex     = nodeInfoList[devIndex - 1].DevInfo.Index;
		sbMsg.data.devInfo.ieeeAddr     = nodeInfoList[devIndex - 1].DevInfo.IEEEAddr;
		sbMsg.data.devInfo.epStatus     = nodeInfoList[devIndex - 1].AppInfo.ActiveNow;
		sbMsg.data.devInfo.currentState = nodeInfoList[devIndex - 1].DevInfo.currentState;
		sbSentDataToShmem(&sbMsg);
        printf("ZNP: info notification, send to python\n");
		return 0;
	}

	return 1;
}

static int processMsgFromZNP(IncomingMsgFormat_t *msg, sbMessage_t *sbMsg)
{
	hbMessage_t *Msg = (hbMessage_t *)(msg->Data);
	int j;
    printf("ZNP: new message\n");
	if (Msg->hdr.message_type == SB_DEVICE_TYPE_NTF)
	{
        printf("ZNP: device type notification\n");
		for (j = 0; j < joinedNodesCount; j++)
		{
			if (nodeInfoList[j].DevInfo.NwkAddr == msg->SrcAddr)
			{
                printf("ZNP: dtn: found device:%x\n", msg->SrcAddr);
				printf("Results from device type notofication\n");
				printf("device type:%d\n", Msg->data.infoRspData.sbType.type);
				printf("current state switch1:%d\n", Msg->data.infoRspData.currentState.switch1);
				printf("current state switch2:%d\n", Msg->data.infoRspData.currentState.switch2);
				printf("current state switch3:%d\n", Msg->data.infoRspData.currentState.switch3);
				printf("current state switch4:%d\n", Msg->data.infoRspData.currentState.switch4);
				printf("current state switch5:%d\n", Msg->data.infoRspData.currentState.switch5);
				printf("current state switch6:%d\n", Msg->data.infoRspData.currentState.switch6);
				printf("current state switch7:%d\n", Msg->data.infoRspData.currentState.switch7);
				printf("current state switch8:%d\n", Msg->data.infoRspData.currentState.switch8);
				nodeInfoList[j].DevInfo.DeviceType = Msg->data.infoRspData.sbType.type;
				nodeInfoList[j].DevInfo.currentState = Msg->data.infoRspData.currentState;
				nodeInfoList[j].AppInfo.ActiveNow = NS_BOARD_READY;
				sbSentDeviceJoin(NS_BOARD_READY, nodeInfoList[j].DevInfo.Index, Msg, nodeInfoList[j].AppInfo.EndPoint);
			}
		}
		return 0;
	}
	else if(Msg->hdr.message_type == SB_BOARD_INFO_RSP)
	{
		for (j = 0; j < joinedNodesCount; j++)
		{
			if (nodeInfoList[j].DevInfo.NwkAddr == msg->SrcAddr)
			{
				sbMsg->hdr.message_type = SB_BOARD_INFO_RSP;
				sbMsg->hdr.node_id = j + 1;
				sbMsg->data.infoRspData.sbType.type = Msg->data.infoRspData.sbType.type;
				sbMsg->data.infoRspData.currentState = Msg->data.infoRspData.currentState;
			}
		}
	}
	else if (Msg->hdr.message_type == SB_STATE_CHANGE_RSP)
	{
		for (j = 0; j < joinedNodesCount; j++)
		{
			if (nodeInfoList[j].DevInfo.NwkAddr == msg->SrcAddr)
			{
				sbMsg->hdr.message_type = SB_STATE_CHANGE_RSP;
				sbMsg->hdr.node_id = j + 1;
				sbMsg->data.boardData.sbType.type = Msg->data.boardData.sbType.type;
				sbMsg->data.boardData.switchData   = Msg->data.boardData.switchData;
			}
		}
	}
	else if (Msg->hdr.message_type == SB_DEVICE_READY_NTF)
	{
		for (j = 0; j < joinedNodesCount; j++)
		{
			if (nodeInfoList[j].DevInfo.NwkAddr == msg->SrcAddr)
			{
				sbMsg->hdr.node_id = j + 1;
			}
		}
		sbMsg->hdr.message_type = SB_DEVICE_READY_NTF;
	}

	return 1;
}
#endif
