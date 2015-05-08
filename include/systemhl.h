#ifndef _SYSTEM_H
#define _SYSTEM_H
#define _DEBUG

#define HAVE_AHBPROT (*(vu32*)0xcd800064 == 0xFFFFFFFF ? 1 : 0)
#define BC 		0x0000000100000100ULL
#define MIOS 	0x0000000100000101ULL

#define BUFFER_SIZE 0x8000

#include "BS2State.h"
/*
typedef struct {
	u32 checksum;
	u8 flags;
	u8 type;
	u8 discstate;
	u8 returnto;
	u32 unknown[6];
} StateFlags;
*/

typedef struct {
	s32 ver;
	s32 rev;
} IOSInfo;


/* Global variables */
extern StateFlags state;

void* SYSTEMHL_getFramebuffer(void);
GXRModeObj* SYSTEMHL_getVideoMode(void);
void SYSTEMHL_setScreenData(void* fb, GXRModeObj* mode);
void SYSTEMHL_init(void);
void SYSTEMHL_ClearScreen(void);

//s32 SYSTEMHL_readStateViaISFS(void);
//StateFlags* SYSTEMHL_getStateFlags(void);
s32 SYSTEMHL_readSMState(void);
s32 SYSTEMHL_writeSMState(void);

void SYSTEMHL_waitForButtonAPress(void);

s32 SYSTEMHL_checkDVDViaDI(void);
IOSInfo * SYSTEMHL_queryIOS(void);
s32 SYSTEMHL_initDisc(void);
const u8 *getDataBuf(void);

s32 SYSTEMHL_dumpMemToSDFile(void);

void SYSTEMHL_bootGCDisc(void);
#endif //_SYSTEM_H

