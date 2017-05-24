#ifndef __SB_CLIENT_METHODS_H__
#define __SB_CLIENT_METHODS_H__

#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include  <sys/ipc.h>
#include  <sys/shm.h>


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

struct Memory {
	int  status;
	char data[256];
};

uint8_t initDone = 0;

struct Memory  *ShmReadPTR,*ShmWritePTR;

static int sbGetDataFromShmem(char *serverSpace)
{
	char *data = ShmReadPTR->data;
	if (data == NULL)
		return -1;

	sbMessage_t *sMsg = (sbMessage_t *)data;

	if (sMsg->hdr.message_type == SB_BOARD_INFO_REQ)
	{
        printf("r:board info req\n");
		memcpy(serverSpace, data, SB_BOARD_INFO_REQ_LEN);
		return SB_BOARD_INFO_REQ_LEN;
	}
	else if (sMsg->hdr.message_type == SB_STATE_CHANGE_REQ)
	{
		printf("r:state change req\n");
		hbMessage_t hMsg;
		hMsg.hdr.message_type = SB_STATE_CHANGE_REQ;
		hMsg.data.boardData.sbType.type = sMsg->data.boardData.sbType.type;
		hMsg.data.boardData.switchData.state = sMsg->data.boardData.switchData.state;

		memcpy(serverSpace, &hMsg, SB_STATE_CHANGE_REQ_LEN);
		return SB_STATE_CHANGE_REQ_LEN;
	}
    else if (sMsg->hdr.message_type == SB_DEVICE_READY_REQ)
    {
        printf("r:device ready req\n");
        memcpy(serverSpace, data, SB_DEVICE_READY_REQ_LEN);
        return SB_DEVICE_READY_REQ_LEN;
    }
	else
	{
        printf("r:unknown message\n");
		memcpy(serverSpace, data, 128);
		return 128;
	}
}

static int sbSentDataToShmem(char *data)
{
	int dataSize;
	hbMessage_t *sMsg = (hbMessage_t *)data;
	sbMessage_t sbMsg;

	if (ShmWritePTR == NULL)
			return -1;

	if (sMsg->hdr.message_type == SB_BOARD_INFO_RSP)
	{
		sbMsg.hdr.message_type = SB_BOARD_INFO_RSP;
		sbMsg.data.infoRspData.sbType.type = sMsg->data.infoRspData.sbType.type;
		
		printf("Datas: %x %x %x %x %x %x %x %x\n", sMsg->data.infoRspData.currentState.switch1,
		sMsg->data.infoRspData.currentState.switch2, sMsg->data.infoRspData.currentState.switch3,
		sMsg->data.infoRspData.currentState.switch4, sMsg->data.infoRspData.currentState.switch5,
		sMsg->data.infoRspData.currentState.switch6, sMsg->data.infoRspData.currentState.switch7,
		sMsg->data.infoRspData.currentState.switch8);

		sbMsg.data.infoRspData.currentState = sMsg->data.infoRspData.currentState;
		dataSize = 40;
	}
	else if (sMsg->hdr.message_type == SB_STATE_CHANGE_RSP)
	{
		sbMsg.hdr.message_type = SB_STATE_CHANGE_RSP;
		sbMsg.data.boardData.sbType.type = sMsg->data.boardData.sbType.type;
		sbMsg.data.boardData.switchData   = sMsg->data.boardData.switchData;
		dataSize = 40;
	}
	else if (sMsg->hdr.message_type == SB_DEVICE_READY_NTF)
	{
		dataSize = SB_DEVICE_READY_NTF_LEN;
		sbMsg.hdr.message_type = SB_DEVICE_READY_NTF;
	}
	else if (sMsg->hdr.message_type == SB_DEVICE_INFO_NTF)
	{
		sbMsg = *(sbMessage_t *)data;
		dataSize = SB_DEVICE_INFO_NTF_LEN;
	}
	else
		dataSize = 128;

	memset(ShmWritePTR->data, 0, 256);
	memcpy(ShmWritePTR->data, &sbMsg, dataSize);
	ShmWritePTR->status = FILLED;
	return dataSize;
}

static int sbSentDeviceReady(int ok)
{
    int ret;
	uint8_t Msg;
	Msg = SB_DEVICE_READY_NTF;
	ret = sbSentDataToShmem((char *)&Msg);
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
		sbSentDataToShmem((char *)&sbMsg);
        printf("ZNP: info notification, send to python\n");
		return 0;
	}

	return 1;
}

static int processMsgFromZNP(IncomingMsgFormat_t *msg)
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

	return 1;
}
#endif
