#include "Com_status.h"

Status state = IDLE;

char *state_arr[] = {"IDLE", "CONNECT", "LISTEN", "SPEECH"};
void Com_status_set_state(Status status)
{
    if (state != status) // 状态发生了切换
    {
        MY_LOGI("%s------>%s", state_arr[state], state_arr[status]);
        state = status;
    }
}