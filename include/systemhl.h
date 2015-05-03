#ifndef _SYSTEM_H
#define _SYSTEM_H
#define _DEBUG


#define HAVE_AHBPROT (*(vu32*)0xcd800064 == 0xFFFFFFFF ? 1 : 0)

#define BUFFER_SIZE 0x8000

// Source: http://www.wiibrew.org/wiki/Wii_Disc
#define MAGIC_WII 0x5D1C9EA3 //"\x5d\x1c\x9e\xa3" 
#define MAGIC_GC  0xC2339F3D //"\xc2\x33\x9f\x3d"

// Source: YAGCD, Chap. 4.2.1.4
#define INMEM_DVD_BI2 0x800000f4 //In memory location of disk header, when loaded by IPL

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


typedef struct {
//    u8 disc_id;
//    u8 game_code[2];
//    u8 region_code;
	u8 game_code_ext[4];
    u8 maker_code[2];
    u8 disc_nr;
    u8 disc_version;
    u8 audio_streaming;
    u8 streaming_buffer_size;
    u8 unused[14];
    u32 magic_wii;
    u32 magic_gc;
    u8 title[64];
    u8 disable_hashes;
    u8 disable_encryption;
	u8 padding[30];
} __attribute__((packed)) DISC_HEADER;

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
u32 SYSTEMHL_initDisc(void);
const u8 *getDataBuf(void);

s32 SYSTEMHL_dumpMemToSDFile(void);
#endif //_SYSTEM_H

