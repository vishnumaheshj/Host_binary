#ifndef __SB_CLIENT_METHODS_H__
#define __SB_CLIENT_METHODS_H__

#include "switchboard.h"

static int sbGetDataFromShmem(char *serverCmd, char *data)
{
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
		memcpy(serverCmd, data, 50);
		return 50;
	}
}

#endif
