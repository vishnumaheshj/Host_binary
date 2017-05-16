#ifndef __SB_CLIENT_METHODS_H__
#define __SB_CLIENT_METHODS_H__

#include "switchboard.h"

#define  NOT_READY  -1
#define  FILLED     1
#define  TAKEN      0

struct Memory {
	int  status;
	char data[256];
};

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

	// Make reading and writing separate threads on python side.
	//while(ShmWritePTR->status != TAKEN) continue;

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
    char buffer[128];
	sbMessage_t *Msg = (sbMessage_t *)buffer;
	Msg->hdr.message_type = SB_DEVICE_READY_NTF;
	Msg->data.hubInfo.status = ok? HUB_START_SUCCESS: HUB_START_UNKNOWN;
	ret = sbSentDataToShmem((char *)Msg);
	return ret;
}
#endif
