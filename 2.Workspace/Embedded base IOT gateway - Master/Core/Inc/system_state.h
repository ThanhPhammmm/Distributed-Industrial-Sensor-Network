#ifndef INC_SYSTEM_STATE_H_
#define INC_SYSTEM_STATE_H_

#include <stdint.h>

typedef enum { SYS_IDLE = 0, SYS_STOP = 1, SYS_RUN = 2 } eSysState;

void        SysState_Init(void);
eSysState   SysState_Get(void);
void        SysState_Set(eSysState s);
const char *SysState_Name(eSysState s);

#endif /* INC_SYSTEM_STATE_H_ */
