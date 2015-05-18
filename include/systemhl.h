#ifndef _SYSTEM_H
#define _SYSTEM_H
#define _DEBUG

#include "BS2State.h"
/* Defines */
#define HAVE_AHBPROT (*(vu32*)0xcd800064 == 0xFFFFFFFF ? 1 : 0)
#define BC 		0x0000000100000100ULL
#define MIOS 	0x0000000100000101ULL

#define STATE_CONSOLE_INIT   0x01
#define STATE_DVD_INIT       0x02
#define STATE_SD_INIT        0x04
#define STATE_NAND_INIT      0x08
#define STATE_VIDEO_INIT     0x10
#define STATE_AUDIO_INIT     0x20
#define STATE_PAD_INIT       0x40
#define STATE_WIILIGHT_INIT  0x80

#define STATE_DVD_DRIVE_RDY  0x01 <<16
/* Typedefs */
typedef struct {
	s32 ver;
	s32 rev;
} IOSInfo;

/* Global variables */
extern BS2State g_BS2State;

/*
* Instead of exposing the system state as a global variable,
* a helper function is used to check for a desired state.
* This ensures read-only status of system state outside SYSTEMHL calls.
*/
inline u32 SYSTEMHL_querySysState(u32 state);
inline void dbgprint(const char *text);

/*****************************************************
* The framebuffer functions are useless, and can be avoided for now..
*****************************************************/
//void* SYSTEMHL_getFramebuffer(void);
//GXRModeObj* SYSTEMHL_getVideoMode(void);
//void SYSTEMHL_setScreenData(void* fb, GXRModeObj* mode);

void SYSTEMHL_init(void);
void SYSTEMHL_ClearScreen(void);

s32 SYSTEMHL_readBF2State(void);
//s32 SYSTEMHL_writeBF2State(void);

void SYSTEMHL_waitForButtonAPress(void);

s32 SYSTEMHL_checkDVDViaDI(void);
const IOSInfo* SYSTEMHL_queryIOS(void);

s32 SYSTEMHL_dumpMemToSDFile(void);

void SYSTEMHL_bootGCDisc(void);

void SYSTEMHL_exitToSysMenu(void);
void SYSTEMHL_exitToLoader(void);
#endif //_SYSTEM_H

