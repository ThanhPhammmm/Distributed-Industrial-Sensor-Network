#include "system_state.h"
#include "FreeRTOS.h"
#include "task.h"

static volatile eSysState g_state = SYS_IDLE;

void SysState_Init(void){
	g_state = SYS_IDLE;
}

eSysState SysState_Get(void){
    eSysState s;
    taskENTER_CRITICAL();
    s = g_state;
    taskEXIT_CRITICAL();
    return s;
}

void SysState_Set(eSysState s){
    taskENTER_CRITICAL();
    g_state = s;
    taskEXIT_CRITICAL();
}

const char *SysState_Name(eSysState s){
    switch(s){
    case SYS_IDLE: return "IDLE";
    case SYS_STOP: return "STOP";
    case SYS_RUN: return "RUN ";
    default: return "????";
    }
}
