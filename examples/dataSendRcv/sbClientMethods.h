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
		memcpy(serverSpace, data, SB_BOARD_INFO_REQ_LEN);
		return SB_BOARD_INFO_REQ_LEN;
	}
	else if (sMsg->hdr.message_type == SB_STATE_CHANGE_REQ)
	{
		memcpy(serverSpace, data, SB_STATE_CHANGE_REQ_LEN);
		return SB_STATE_CHANGE_REQ_LEN;
	}
	else
	{
		memcpy(serverSpace, data, 128);
		return 128;
	}
}

static int sbSentDataToShmem(char *data)
{
	int dataSize;
	sbMessage_t *sMsg = (sbMessage_t *)data;

	if (ShmWritePTR == NULL)
			return -1;

	if (sMsg->hdr.message_type == SB_BOARD_INFO_RSP)
		dataSize = SB_BOARD_INFO_RSP_LEN;
	else if (sMsg->hdr.message_type == SB_STATE_CHANGE_RSP)
		dataSize = SB_STATE_CHANGE_RSP_LEN;
	else if (sMsg->hdr.message_type == SB_DEVICE_READY_NTF)
		dataSize = SB_DEVICE_READY_NTF_LEN;
	else
		dataSize = 128;

	memset(ShmWritePTR->data, 0, 256);
	memcpy(ShmWritePTR->data, data, dataSize);
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

static int sbSentDeviceJoin(uint8 joinState, uint8 devIndex, ActiveEpRspFormat_t *AERsp, uint8_t endPoint)
{
	if (joinState == NS_EP_ACTIVE)
	{
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
		rpcWaitMqClientMsg(500);
		initDone = 1;
	}

}
#endif
