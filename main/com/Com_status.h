#ifndef __COM_STATUS_H__
#define __COM_STATUS_H__

#include "Com_Debug.h"
typedef enum
{
    IDLE,
    CONNECT,
    LISTEN,
    SPEECH,
} Status;

extern Status state;
void Com_status_set_state(Status state);

#endif /* __COM_STATUS_H__ */