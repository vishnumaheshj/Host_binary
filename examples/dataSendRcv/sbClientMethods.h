#ifndef __SB_CLIENT_METHODS_H__
#define __SB_CLIENT_METHODS_H__

#include "switchboard.h"

#define  NOT_READY  -1
#define  FILLED     0
#define  TAKEN      1

struct Memory {
	int  status;
	char data[50];
};

struct Memory  *ShmReadPTR,*ShmWritePTR;

static int sbGetDataFromShmem(char *serverCmd, char *data)
{
	if (data == NULL)
		return -1;

	sbMessage_t *sMsg = (sbMessage_t *)data;

	if (sMsg->hdr.message_type == SB_BOARD_INFO_REQ)
	{
		memcpy(serverCmd, data, SB_BOARD_INFO_REQ_LEN);
		return SB_BOARD_INFO_REQ_LEN;
	}
	else if (sMsg->hdr.message_type == SB_STATE_CHANGE_REQ)
	{
		memcpy(serverCmd, data, SB_STATE_CHANGE_REQ_LEN);
		return SB_STATE_CHANGE_REQ_LEN;
	}
	else
	{
		memcpy(serverCmd, data, 128);
		return 128;
	}
}

static int sbSentDataToShmem(char *data, struct Memory *ShmWritePTR)
{
	int dataSize;
	sbMessage_t *sMsg = (sbMessage_t *)data;

	if (ShmWritePTR == NULL)
			return -1;

	if (sMsg->hdr.message_type == SB_BOARD_INFO_RSP)
		dataSize = SB_BOARD_INFO_RSP_LEN;
	else if (sMsg->hdr.message_type == SB_STATE_CHANGE_RSP)
		dataSize = SB_STATE_CHANGE_RSP_LEN;
	else
		dataSize = 128;

	memset(ShmWritePTR->data, 0, 256);
	memcpy(ShmWritePTR->data, data, dataSize);
	ShmWritePTR->status = FILLED;
	return 0;
}
#endif
